//---------------------------------------------------------------------------
// client/main.cpp
//
// This file is part of Hexahedra.
//
// Hexahedra is free software; you can redistribute it and/or modify it
// under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Copyright 2012, 2013, nocte@hippie.nu
//---------------------------------------------------------------------------

#include <cassert>
#include <iostream>
#include <signal.h>
#include <ctime>
#include <functional>
#include <GL/glew.h>
#include <GL/gl.h>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/locks.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/program_options/option.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/filesystem/operations.hpp>

// The Boost threadpool library is not included in the official distribution
// yet.  For now, it's distributed along with Hexahedra.
#include <boost/threadpool.hpp>

#include <enet/enet.h>

#include <hexa/config.hpp>
#include <hexa/drop_privileges.hpp>
#include <hexa/os.hpp>
#include <hexa/packet.hpp>
#include <hexa/persistence_sqlite.hpp>
#include <hexa/persistence_null.hpp>
#include <hexa/protocol.hpp>
#include <hexa/memory_cache.hpp>
#include <hexa/voxel_range.hpp>
#include <hexa/win32_minidump.hpp>

#include "sfml_ogl2.hpp"
#include "sfml_ogl3.hpp"
#include "game.hpp"
#include "main_game.hpp"
#include "main_menu.hpp"

namespace po = boost::program_options;
namespace fs = boost::filesystem;

using boost::bind;
using boost::ref;
using namespace hexa;

namespace hexa {

// Accessible by other modules
boost::threadpool::prio_pool pool (3);
po::variables_map global_settings;

}

int main (int argc, char* argv[])
{
    setup_minidump();
    std::setlocale(LC_ALL, "C.UTF-8");

    auto& vm (global_settings);
    po::options_description generic("Command line options");
    generic.add_options()
        ("version,v",   "print version string")
        ("help",        "show help message")
        ("newsingle",   "Immediately start a new single-player game")
        ("continue",    "Continue the last game")
        ("quickjoin",   "Join your favorite server")
        ;

    po::options_description config("Configuration");
    config.add_options()
        ("port", po::value<uint16_t>()->default_value(15556),
            "default port")
        ("hostname", po::value<std::string>()->default_value("localhost"),
            "server name")
        ("db", po::value<std::string>()->default_value("world.db"),
            "local database file name")
        ("uid", po::value<std::string>()->default_value("nobody"),
            "drop to this user id after initialising the client")
        ("chroot", po::value<std::string>()->default_value("/tmp"),
            "chroot to this path after initialising the client")
        ("ogl2",
            "Force the use of the OpenGL 2.0 backend")
        ("viewdist", po::value<unsigned int>()->default_value(20),
            "view distance in chunks")
        ("datadir", po::value<std::string>()->default_value(GAME_DATA_PATH),
            "where to find the game's assets")
        ("userdir", po::value<std::string>()->default_value(app_user_dir().string()),
            "user's game directory")
        ("vsync", po::value<bool>()->default_value(true),
            "use v-sync")
        ;


    po::options_description cmdline;
    cmdline.add(generic).add(config);

    try
    {
        po::store(po::parse_command_line(argc, argv, cmdline), vm);

        fs::path inifile (app_user_dir() / "hexahedra.ini");
        if (fs::exists(inifile))
            po::store(po::parse_config_file<char>(inifile.string().c_str(), config), vm);

        po::notify(vm);
    }
    catch (po::unknown_option& e)
    {
        std::cerr << "Unrecognized option: " << e.get_option_name() << std::endl;
        std::cerr << cmdline << std::endl;
        return EXIT_FAILURE;
    }
    catch (std::exception& e)
    {
        std::cerr << "Could not parse options: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (vm.count("help"))
    {
        std::cout << cmdline << std::endl;
        return EXIT_SUCCESS;
    }
    if (vm.count("version"))
    {
        std::cout << "hexahedra " << PROJECT_VERSION << std::endl;
        return EXIT_SUCCESS;
    }

    if (enet_initialize() != 0)
    {
        std::cerr << "Could not initialize ENet, exiting" << std::endl;
        return EXIT_FAILURE;
    }

    try
    {
        hexa::game game_states ("Hexahedra", 1200, 800);

        game_states.run(game_states.make_state<hexa::main_menu>());
        //game_states.run(game_states.make_state<hexa::main_game>("localhost", 15556, global_settings["viewdist"].as<unsigned int>()));
        //game_states.run(game_states.make_state<hexa::main_game>("hexahedra.net", 15556));

        std::cout << "Shut down..." << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cerr << "unknown exception caught"  << std::endl;
        return -1;
    }

    enet_deinitialize();

    return EXIT_SUCCESS;
}

