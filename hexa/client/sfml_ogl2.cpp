//---------------------------------------------------------------------------
// hexa/client/sfml_ogl2.cpp
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

#include "sfml_ogl2.hpp"

#include <list>
#include <stdexcept>
#include <sstream>
#include <random>
#include <unordered_map>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/locks.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

#include <hexa/basic_types.hpp>
#include <hexa/block_types.hpp>
#include <hexa/frustum.hpp>
#include <hexa/voxel_algorithm.hpp>
#include <hexa/voxel_range.hpp>

#include "game.hpp"
#include "scene.hpp"
#include "texture.hpp"
#include "occlusion_query.hpp"
#include "opengl.hpp"
#include "opengl_vertex.hpp"
#include "sfml_resource_manager.hpp"

using namespace boost;
using namespace boost::range;
using namespace boost::adaptors;
using namespace boost::numeric::ublas;

namespace fs = boost::filesystem;

namespace hexa {

namespace {

typedef vertex_1<vtx_xyz<int16_t> > occ_cube_vtx;

void opengl_check(int line)
{
    GLenum code (glGetError());
    if (code != GL_NO_ERROR)
        std::cerr << "opengl 2.x line " << line << " : " << gluErrorString(code) << std::endl;
}

} // anonymous namespace

class terrain_mesher_ogl2 : public terrain_mesher_i
{
public:
    terrain_mesher_ogl2 () : empty_(true)
    { }


    void add_face(chunk_index p, direction_type side,
                  uint16_t texture, light l)
    {
        static const int8_t offsets[6][4][3] =
            { { {1, 0, 1}, {1, 0, 0}, {1, 1, 0}, {1, 1, 1} },
              { {0, 1, 1}, {0, 1, 0}, {0, 0, 0}, {0, 0, 1} },
              { {1, 1, 1}, {1, 1, 0}, {0, 1, 0}, {0, 1, 1} },
              { {0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {1, 0, 1} },
              { {0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1} },
              { {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0} } };

        const int8_t(*o)[3] = offsets[side];


        empty_ = false;

        float u0 ((float(texture % 16)) / 16.f + 1.f/64.f);
        float u1 (u0 + 1.f/32.f);

        float v0 ((float(texture / 16)) / 16.f + 1.f/64.f);
        float v1 (v0 + 1.f/32.f);

        vector temp;
        temp  = vector(0.56f, 0.67f, 1.0f) * 0.6f * (float(l.ambient)  / 15.f);
        temp += vector(1.00f, 1.00f, 0.7f) * 0.6f * (float(l.sunlight) / 15.f);

        const float gamma (0.8f);
        vector3<uint8_t> rgb (std::min(255.f, 255 * std::pow(temp[0], gamma)), 
                              std::min(255.f, 255 * std::pow(temp[1], gamma)), 
                              std::min(255.f, 255 * std::pow(temp[2], gamma))); 

        data_.emplace_back(ogl2_terrain_vertex
            (vector3<float>(p.x+o[0][0], p.y+o[0][1], p.z+o[0][2]) * 16.f,
             vector2<float>(u0, v0), rgb));

        data_.emplace_back(ogl2_terrain_vertex
            (vector3<float>(p.x+o[1][0], p.y+o[1][1], p.z+o[1][2]) * 16.f,
             vector2<float>(u0, v1), rgb));

        data_.emplace_back(ogl2_terrain_vertex
            (vector3<float>(p.x+o[2][0], p.y+o[2][1], p.z+o[2][2]) * 16.f,
             vector2<float>(u1, v1), rgb));

        data_.emplace_back(ogl2_terrain_vertex
            (vector3<float>(p.x+o[3][0], p.y+o[3][1], p.z+o[3][2]) * 16.f,
             vector2<float>(u1, v0), rgb));
    }

    void add_custom_block (chunk_index voxel,
                           const custom_block& model,
                           const std::vector<light>& intensities)
    {
    }

    bool empty() const { return empty_; }

    gl::vbo make_buffer() const
    {
        return gl::make_vbo(data_);
    }

public:
    bool empty_;
    std::vector<ogl2_terrain_vertex> data_;
};

//---------------------------------------------------------------------------

sfml_ogl2::sfml_ogl2(sf::RenderWindow& app)
    : sfml(app)
    , textures_ready_(false)
{  
    static const int16_t cube[24*3] = {
        0  , 0  , 0   ,   256, 0  , 0   ,   256, 256, 0   ,   0  , 256, 0   ,
        256, 0  , 0   ,   256, 0  , 256 ,   256, 256, 256 ,   256, 256, 0   ,
        0  , 256, 256 ,   256, 256, 256 ,   256, 0  , 256 ,   0  , 0  , 256 ,
        0  , 256, 0   ,   0  , 256, 256 ,   0  , 0  , 256 ,   0  , 0  , 0   ,
        0  , 0  , 0   ,   256, 0  , 0   ,   256, 0  , 256 ,   0  , 0  , 256 ,
        0  , 256, 256 ,   256, 256, 256 ,   256, 256, 0   ,   0  , 256, 0    };

    occlusion_block_ = gl::vbo(cube, 24, sizeof(occ_cube_vtx));
}

sfml_ogl2::~sfml_ogl2()
{
}

std::unique_ptr<terrain_mesher_i>
sfml_ogl2::make_terrain_mesher()
{
    return std::unique_ptr<terrain_mesher_i>(new terrain_mesher_ogl2);
}

void sfml_ogl2::load_textures(const std::vector<std::string>& name_list)
{
    textures_ready_ = false;

    sf::Image atlas;
    temp_img_.create(512, 512, sf::Color::Transparent);
    fs::path unknown_file (resource_file(res_block_texture, "unknown"));

    unsigned int x (0), y (0);

    for (auto& name : name_list)
    {
        fs::path file (resource_file(res_block_texture, name));

        sf::Image tile;
        if (fs::is_regular_file(file))
        {
            tile.loadFromFile(file.string());
        }
        else
        {
            std::string clip (name.begin(), find_last(name, ".").begin());

            fs::path clipfile (resource_file(res_block_texture, clip));
            if (fs::is_regular_file(clipfile))
                tile.loadFromFile(clipfile.string());
            else
                tile.loadFromFile(unknown_file.string());
        }

        // Add an 8-pixel edge around every texture to prevent seaming
        // when mipmapping is turned on.
        temp_img_.copy(tile, x, y, sf::IntRect(8, 8, 8, 8), true);
        temp_img_.copy(tile, x + 8, y, sf::IntRect(0, 8, 16, 8), true);
        temp_img_.copy(tile, x + 24, y, sf::IntRect(0, 8, 8, 8), true);

        temp_img_.copy(tile, x, y + 8 , sf::IntRect(8, 0, 8, 16), true);
        temp_img_.copy(tile, x + 8, y + 8, sf::IntRect(0, 0, 16, 16), true);
        temp_img_.copy(tile, x + 24, y + 8 , sf::IntRect(0, 0, 8, 16), true);

        temp_img_.copy(tile, x, y + 24, sf::IntRect(8, 0, 8, 8), true);
        temp_img_.copy(tile, x + 8, y + 24, sf::IntRect(0, 0, 16, 8), true);
        temp_img_.copy(tile, x + 24, y + 24, sf::IntRect(0, 0, 8, 8), true);

        x += 32;
        if (x >= 512)
        {
            x = 0;
            y += 32;
        }
    }

    //temp_img_.saveToFile("/tmp/test.png");
    textures_ready_ = true;
}

void sfml_ogl2::sky_color(const color& c)
{
    glClearColor(c.r(), c.g(), c.b(), 1.0);
    opengl_check(__LINE__);
}

void sfml_ogl2::sun_color(const color&)
{
}

void sfml_ogl2::ambient_color(const color&)
{
}

void sfml_ogl2::prepare(const player& plr)
{
    if (textures_ready_)
    {
        texture_atlas_.load(temp_img_, texture::transparent);
        textures_ready_ = false;
    }

    sky_color(color(0.56, 0.67, 1.0));
    sfml::prepare(plr);
    move_player(plr.chunk_position());
    boost::range::for_each(process_vbo_queue(), [&](chunk_coordinates c)
    {
        on_new_vbo(c);
    });

}

void sfml_ogl2::opaque_pass()
{
    static const float fog_color[4] = { 0.56, 0.67, 1.0, 1.0 };

    if (!texture_atlas_)
        return;

    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE, GL_EXP2);
    glFogfv(GL_FOG_COLOR, fog_color);
    glFogf(GL_FOG_DENSITY, 2.2f / (float)(view_dist_ * chunk_size));
    glHint(GL_FOG_HINT, GL_NICEST);
    
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    glCheck(glLoadMatrixf(camera_.model_view_matrix().as_ptr()));
    frustum clip (camera_.mvp_matrix());
    const float sphere_diam (16.f * 13.86f);

    glEnable(GL_TEXTURE_2D);
    texture_atlas_.bind();
    bind_attributes_ogl2<ogl2_terrain_vertex>();

    for (const auto& l : opaque_vbos)
    {
        for (const auto& v : l)
        {
            assert(v.second.id() != 0);
            assert(v.second.vertex_count() != 0);

            vector3<float> offset (vector3<int>(v.first * chunk_size - world_offset_));
            offset *= 16.f;

            if (v.second.id() && clip.is_inside(vector3<float>(offset.x + 128, offset.y + 128, offset.z + 128), sphere_diam))
            {
                glCheck(glTranslatef(offset.x, offset.y, offset.z));
                v.second.bind();
                bind_attributes_ogl2<ogl2_terrain_vertex>();
                v.second.draw();
                glCheck(glTranslatef(-offset.x, -offset.y, -offset.z));
            }
        }
    }
}

void sfml_ogl2::transparent_pass()
{
    if (!texture_atlas_)
        return;

    glCheck(glLoadMatrixf(camera_.model_view_matrix().as_ptr()));
    frustum clip (camera_.mvp_matrix());
    
    glEnable(GL_TEXTURE_2D);
    texture_atlas_.bind();

    const float sphere_diam (16.f * 13.86f);

    for (const auto& l : transparent_vbos | reversed)
    {
        for (const auto& v : l)
        {
            assert(v.second.id() != 0);
            assert(v.second.vertex_count() != 0);

            vector3<float> offset (vector3<int>(v.first * chunk_size - world_offset_));
            offset *= 16.f;

            if (v.second.id() && clip.is_inside(vector3<float>(offset.x + 128.f, offset.y + 128.f, offset.z + 128.f), sphere_diam))
            {
                glCheck(glTranslatef(offset.x, offset.y, offset.z));
                v.second.bind();
                bind_attributes_ogl2<ogl2_terrain_vertex>();
                v.second.draw();
                glCheck(glTranslatef(-offset.x, -offset.y, -offset.z));
            }
        }
    }

    gl::vbo::unbind();
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_FOG);
    opengl_check(__LINE__);
}

void sfml_ogl2::handle_occlusion_queries()
{
    if (!texture_atlas_)
        return;

    glCheck(glLoadMatrixf(camera_.model_view_matrix().as_ptr()));
    glCheck(glDisable(GL_TEXTURE_2D));

    frustum clip (camera_.mvp_matrix());

    const float sphere_diam (16.f * 13.86f);

    glCheck(glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE));
    glCheck(glDisable(GL_CULL_FACE));
    enable_vertex_attributes<occ_cube_vtx>();

    for (oqs_t& l : occlusion_queries)
    {
        for (sfml_ogl2::oqs_t::value_type& v : l)
        {
            vector3<float> offset (vector3<int>(v.first * chunk_size - world_offset_));
            offset *= 16.f;

            if (clip.is_inside(vector3<float>(offset.x + 128, offset.y + 128, offset.z + 128), sphere_diam))
            {
                occlusion_query& qry (v.second);
                glTranslatef(offset.x, offset.y, offset.z);

                if (qry.state() == occlusion_query::busy)
                {
                    occlusion_block_.bind();
                    bind_attributes<occ_cube_vtx>();
                    glColor3f(1.f, 0.f, 0.f);
                    occlusion_block_.draw();
                }
                else if (qry.state() == occlusion_query::visible)
                {
                    occlusion_block_.bind();
                    bind_attributes<occ_cube_vtx>();
                    glColor3f(0.f, 1.f, 0.f);
                    occlusion_block_.draw();
                }
                else if (qry.state() == occlusion_query::air)
                {
                    occlusion_block_.bind();
                    bind_attributes<occ_cube_vtx>();
                    glColor3f(0.5f, 0.5f, 0.5f);
                    occlusion_block_.draw();
                }
                else
                {
                    qry.begin_query();
                    occlusion_block_.bind();
                    bind_attributes<occ_cube_vtx>();
                    glColor3f(1.f, 1.f, 1.f);
                    occlusion_block_.draw();
                    qry.end_query();
                }
                glTranslatef(-offset.x, -offset.y, -offset.z);
            }
        }
    }

    gl::vbo::unbind();
    disable_vertex_attributes<occ_cube_vtx>();
    glCheck(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));
    glCheck(glEnable(GL_CULL_FACE));
    glCheck(glEnable(GL_TEXTURE_2D));
}

void sfml_ogl2::draw(const gl::vbo& v) const
{
    if (!texture_atlas_)
        return;

    glCheck(glEnable(GL_TEXTURE_2D));
    glCheck(glEnable(GL_CULL_FACE));

    texture_atlas_.bind();
    v.bind();
    bind_attributes<ogl2_terrain_vertex>();
    v.draw();
    v.unbind();

    texture_atlas_.unbind();
}

void sfml_ogl2::draw_model(const wfpos& p, uint16_t m) const
{
    (void)p; (void)m;
}

} // namespace hexa

