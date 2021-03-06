//---------------------------------------------------------------------------
// hexa/client/clock.cpp
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
// Copyright 2013, nocte@hippie.nu
//---------------------------------------------------------------------------

#include "clock.hpp"

#include <chrono>

using namespace std::chrono;

namespace hexa {
namespace clock {

namespace {

steady_clock::time_point start_;

} // anonymous namespace

void init()
{
    start_ = steady_clock::now();
}

uint64_t now()
{
    return duration_cast<milliseconds>(steady_clock::now() - start_).count();
}

gameclock_t game_time ()
{
    return static_cast<gameclock_t>(now() / 100);
}

clientclock_t client_time(uint64_t client_offset)
{
    return static_cast<clientclock_t>(now() - client_offset);
}

}} // namespace hexa::clock

