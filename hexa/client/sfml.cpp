//---------------------------------------------------------------------------
// client/sfml.cpp
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
// Copyright 2012-2013, nocte@hippie.nu
//---------------------------------------------------------------------------

#include "sfml.hpp"

#include <chrono>
#include <list>
#include <stdexcept>
#include <sstream>
#include <random>
#include <unordered_map>

#include <boost/range/algorithm.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/thread/locks.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/program_options/variables_map.hpp>

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>

//#include <Gwen/Controls.h>

#include <hexa/basic_types.hpp>
#include <hexa/block_types.hpp>
#include <hexa/voxel_algorithm.hpp>
#include <hexa/voxel_range.hpp>

#include "event.hpp"
#include "game.hpp"
#include "hud.hpp"
#include "opengl.hpp"
#include "scene.hpp"
#include "texture.hpp"
#include "sky_shader.hpp"
#include "sfml_resource_manager.hpp"

#include "sfml_ogl3.hpp"

using namespace boost;
using namespace boost::range;
using namespace boost::adaptors;
using namespace boost::numeric::ublas;
using namespace boost::math::constants;

namespace po = boost::program_options;
namespace fs = boost::filesystem;

//---------------------------------------------------------------------------

namespace hexa {

extern po::variables_map global_settings;

static double tot_elapsed (0);

struct star
{
    star(double x, double y, float m) : pos (x, y), mag(m) { }

    yaw_pitch pos;
    float     mag;
};
struct bsc_hdr
{
    int32_t seq;
    int32_t first;
    int32_t nrs;
    int32_t stnum;
    int32_t proper_motion_flag;
    int32_t magnitudes_flag;
    int32_t bytes_per_star;
};
struct bsc_rec
{
    float   catnr;
    double  ascension;
    double  declination;
    char    spectral[2];
    uint16_t magnitude;
    float    prop_motion_asc;
    float    prop_motion_dec;
};
static std::vector<star> stars;


namespace {

std::array<vector, 4> make_quad (yaw_pitch pos, float radius, float dist = 1.0f)
{
    std::array<vector, 4> quad;

    quaternion<float> q;
    q.rotate(pos.x, vector(0, 1, 0));
    q.rotate(pos.y, vector(1, 0, 0));

    quad[0] = q * vector(-radius, -radius, dist);
    quad[1] = q * vector( radius, -radius, dist);
    quad[2] = q * vector( radius,  radius, dist);
    quad[3] = q * vector(-radius,  radius, dist);

    return quad;
}

void init_opengl()
{
	glewInit();
    std::cout << glGetString(GL_EXTENSIONS) << std::endl;

	if (GLEW_ARB_shading_language_100)
		std::cout << "Found: GLSL"<< std::endl;

	if (GLEW_ARB_shader_objects)
		std::cout << "Found: Shader objects"<< std::endl;

    if (GLEW_ARB_vertex_shader)
		std::cout << "Found: Vertex shader"<< std::endl;

	if (GLEW_ARB_fragment_shader)
		std::cout << "Found: Fragment shader"<< std::endl;

    GLfloat fogColor[4] = {0.56f, 0.67f, 1.0f, 1.0f};
    //GLfloat fogColor[4] = {0.8f, 0.3f, 0.5f, 1.0f};
    //GLfloat fogColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    glCheck(glClearDepth(1.0f));
    glCheck(glClearColor(fogColor[0], fogColor[1], fogColor[2], fogColor[3]));
}

void save_opengl_state()
{
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    /*
    glPushAttrib(GL_COLOR_BUFFER_BIT | GL_CURRENT_BIT |
                 GL_ENABLE_BIT | GL_TEXTURE_BIT  | GL_TRANSFORM_BIT |
                 GL_VIEWPORT_BIT);
    */
}

void restore_opengl_state()
{
    glPopAttrib();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void opengl_cube_face(float size, vector3<int> pos, direction_type d)
{
    glTranslatef(pos.x, pos.z, -pos.y);
    glBegin(GL_QUADS);

    float zero (0.f);

    switch (d)
    {
    case dir_north:
        glVertex3f(zero, zero, -size);
        glVertex3f(zero, size, -size);
        glVertex3f(size, size, -size);
        glVertex3f(size, zero, -size);
        break;

    case dir_south:
        glVertex3f(size, zero, -zero);
        glVertex3f(size, size, -zero);
        glVertex3f(zero, size, -zero);
        glVertex3f(zero, zero, -zero);
        break;

    case dir_east:
        glVertex3f(size, zero, -size);
        glVertex3f(size, size, -size);
        glVertex3f(size, size, -zero);
        glVertex3f(size, zero, -zero);
        break;

    case dir_west:
        glVertex3f(zero, zero, -zero);
        glVertex3f(zero, size, -zero);
        glVertex3f(zero, size, -size);
        glVertex3f(zero, zero, -size);
        break;

    case dir_up:
        glVertex3f(zero, size, -size);
        glVertex3f(zero, size, -zero);
        glVertex3f(size, size, -zero);
        glVertex3f(size, size, -size);
        break;

    case dir_down:
        glVertex3f(size, zero, -size);
        glVertex3f(size, zero, -zero);
        glVertex3f(zero, zero, -zero);
        glVertex3f(zero, zero, -size);
        break;
    }
    glEnd();
}

void opengl_cube(float size, vector3<int> pos)
{
    glTranslatef(pos.x, pos.z, -pos.y);

    glColor3f(1.0f, 1.0f, 0.0f);

    // Draw a cube
    glBegin(GL_QUADS);

    float zero (-0.001f);
    size += 0.001f;

    glVertex3f(size, zero, zero);
    glVertex3f(size, zero, size);
    glVertex3f(zero, zero, size);
    glVertex3f(zero, zero, zero);

    glVertex3f(zero, size, zero);
    glVertex3f(zero, size, size);
    glVertex3f(size, size, size);
    glVertex3f(size, size, zero);

    glVertex3f(zero, zero, zero);
    glVertex3f(zero, zero, size);
    glVertex3f(zero, size, size);
    glVertex3f(zero, size, zero);

    glVertex3f(size, size, zero);
    glVertex3f(size, size, size);
    glVertex3f(size, zero, size);
    glVertex3f(size, zero, zero);

    glVertex3f(size, size, zero);
    glVertex3f(size, zero, zero);
    glVertex3f(zero, zero, zero);
    glVertex3f(zero, size, zero);

    glVertex3f(zero, size, size);
    glVertex3f(zero, zero, size);
    glVertex3f(size, zero, size);
    glVertex3f(size, size, size);

    glEnd();
}

} // anonymous namespace


sfml::sfml(sf::RenderWindow& win)
    : width_  (win.getSize().x)
    , height_ (win.getSize().y)
    , app_    (win)

    //, GwenRenderer(app_)
    //, Skin(&GwenRenderer)
{
    /*
    Skin.Init( "DefaultSkin.png" );
    Skin.SetDefaultFont( L"default.ttf", 15 );
    pCanvas = new Gwen::Controls::Canvas( &Skin );
    pCanvas->SetSize( win.getSize().x, win.getSize().y );
    GwenInput.Initialize(pCanvas);

    auto btn = new Gwen::Controls::Button(pCanvas);
    btn->SetText(L"Testje test");
    btn->SetPos(20, 200);

    auto tf = new Gwen::Controls::TextBox(pCanvas);
    tf->SetSize(200, 20);
    tf->SetPos(20, 240);
    tf->SetText("Lawl");
    tf->SetKeyboardInputEnabled(true);
    tf->Focus();
*/
    world_offset_ = world_center;
    auto settings (app_.getSettings());

    app_.setVerticalSyncEnabled(global_settings["vsync"].as<bool>());
    init_opengl();

    ui_img_ = images("ui");
    if (ui_img_ == nullptr)
        throw std::runtime_error("No UI atlas found");

    ui_font_ = fonts("default");
    if (ui_font_ == nullptr)
        throw std::runtime_error("No font found");

    for (int y (0); y < 8; ++y)
    {
        for (int x (0); x < 16; ++x)
        {
            sf::Sprite& elem (ui_elem_[x + y * 16]);
            elem.setTexture(*ui_img_);
			elem.setTextureRect(sf::IntRect(x * 16, y * 16, 16, 16));
        }
    }
    for (int y (0); y < 4; ++y)
    {
        for (int x (0); x < 8; ++x)
        {
            sf::Sprite& elem (ui_elem_[128 + x + y * 8]);
            elem.setTexture(*ui_img_);
			elem.setTextureRect(sf::IntRect(x * 32, 128 + y * 32, 32, 32));
        }
    }

    ui_elem_[0].setOrigin(8, 8);

    std::vector<char> buf (2000000);
    fs::path datadir (global_settings["datadir"].as<std::string>());
    std::ifstream bsc ((datadir / "bsc5").string(), std::ios::binary);
    if (bsc)
    {
        bsc.read(&buf[0], 1800000);
        const char* c (&buf[28]);
        for (int i (0); i < 9090; ++i)
        {
            std::vector<char> twist1 (c + 4, c + 12), twist2(c+12, c + 20), twist3(c + 22, c + 24);
            std::reverse(twist1.begin(), twist1.end());
            std::reverse(twist2.begin(), twist2.end());
            std::reverse(twist3.begin(), twist3.end());
            double* x ((double*)&twist1[0]);
            double* y ((double*)&twist2[0]);
            uint16_t* z ((uint16_t*)&twist3[0]);

            if (*z < 400)
                stars.emplace_back(*x, *y, 400 - *z);

            c += 32;
        }
    }
    else
    {
        double m (pi<double>() / 10000.);
        for (int i = 0 ; i < 2000 ; ++i)
            stars.emplace_back((rand() % 10000) * m , (rand() % 10000) * m * 2., 30 + rand() % 300);
    }
}

sfml::~sfml()
{
}

void sfml::draw_chunk_face (const chunk_coordinates& p, direction_type d)
{
    //glLoadIdentity();
    //gluLookAt(camx, camy, camz, lax, lay, laz, up.x, up.y, up.z);

    vector3<int> offset (p * chunk_size - world_offset_);
    opengl_cube_face(16.0f, offset, d);
}

void sfml::draw_chunk_cube (const chunk_coordinates& p)
{
    //glLoadIdentity();
    //gluLookAt(camx, camy, camz, lax, lay, laz, up.x, up.y, up.z);

    vector3<int> offset (p * chunk_size - world_offset_);
    opengl_cube(16.0f, offset);
}

void sfml::prepare(const player& plr)
{
    const float eye_height (1.7f);

    vector c (plr.rel_world_position(world_offset_));
    c.z += eye_height;

    float bob (0.0f), rock (0.0f);
    if (!plr.is_airborne)
    {
        bob  = std::abs(std::sin(tot_elapsed * 10.)) * length(plr.velocity) * 0.016;
        rock = std::sin(tot_elapsed * 10.) * length(plr.velocity) * 0.0012;
    }

    camera_ = camera(vector(0, 0, 0), plr.head_angle(), rock,
                     1.22173048f, (float)width_ / (float)height_, 0.1f, 64000.f);

    vector target (from_spherical(plr.head_angle()) * 1000.f);

    glCheck(glDisable(GL_CULL_FACE));
    glCheck(glDisable(GL_LIGHTING));
    glCheck(glDisable(GL_DEPTH_TEST));
    glCheck(glDisable(GL_ALPHA_TEST));
    glCheck(glDisable(GL_TEXTURE_2D));
    glCheck(glDisable(GL_BLEND));
    glCheck(glDisable(GL_COLOR_MATERIAL));
    glCheck(glEnable(GL_NORMALIZE));
    glCheck(glDisable(GL_FOG));

    glCheck(glDisableClientState(GL_VERTEX_ARRAY));
    glCheck(glDisableClientState(GL_COLOR_ARRAY));
    glCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));

    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glCheck(glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT));
    glCheck(glDepthMask(GL_FALSE));

    static float hack (0.5f);
    hack += 0.0001f * 2.0f * pi<float>();

    {
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_.projection_matrix().as_ptr());

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(camera_.model_view_matrix().as_ptr());
    }


    skydome lulz;
    std::vector<color> colors;

    skylight pretty (yaw_pitch(0, hack), 1.9f);

    for(auto p : lulz.vertices())
    {
        color t (pretty(p));
        t.Y() *= 2.5f;
        t.x() -= 0.33f;
        t.y() -= 0.38f;
        t.x() *= 1.6f;
        t.y() *= 1.6f;
        t.x() += 0.33f;
        t.y() += 0.40f;
        colors.push_back(xyY_to_srgb(t));
    }

    color amb (pretty(normalize(vector(1, 1, 0.4))));

    amb.Y() *= 2.5f;
    amb.x() -= 0.33f;
    amb.y() -= 0.38f;
    amb.x() *= 1.6f;
    amb.y() *= 1.6f;
    amb.x() += 0.33f;
    amb.y() += 0.40f;

    horizon_color_ = xyY_to_srgb(amb);

    glBegin(GL_TRIANGLES);
    for (auto& tri : lulz.triangles())
    {
        for (int i (0); i < 3; ++i)
        {
            unsigned int a (tri[i]);
            gl::color(colors[a]);
            gl::vertex(lulz.vertices()[a]);
        }
    }
    glCheck(glEnd());

    {
    glRotatef(hack * 57.2957795, -1, 0, 0);
    float sun_size(5);

    gl::enable({ GL_TEXTURE_2D, GL_BLEND });
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    textures("star")->bind();

    double x (clamp(-3.0 * std::cos(hack), 0.0, 1.0));

    glBegin(GL_QUADS);
    glColor4f(1,1,1,x);

    for (auto& s : stars)
    {
        auto test (make_quad(s.pos, 0.0023 + s.mag * 0.00002));

        glTexCoord2f(0, 0);
        glVertex3f(test[0][0], test[0][1], test[0][2]);
        glTexCoord2f(1, 0);
        glVertex3f(test[1][0], test[1][1], test[1][2]);
        glTexCoord2f(1, 1);
        glVertex3f(test[2][0], test[2][1], test[2][2]);
        glTexCoord2f(0, 1);
        glVertex3f(test[3][0], test[3][1], test[3][2]);
    }
    glCheck(glEnd());


    textures("moon")->bind();
    glBegin(GL_QUADS);
        gl::color(color::white());

        glTexCoord2f(0, 0);
        glVertex3f(-sun_size, -sun_size, -100);

        glTexCoord2f(1, 0);
        glVertex3f(sun_size, -sun_size, -100);

        glTexCoord2f(1, 1);
        glVertex3f(sun_size, sun_size, -100);

        glTexCoord2f(0, 1);
        glVertex3f(-sun_size, sun_size, -100);
    glCheck(glEnd());


    textures("sun")->bind();
    typedef vertex_2<vtx_xyz<>, vtx_uv<>> sunvtx;

    std::vector<sunvtx> sunarr {
        { { -sun_size, -sun_size, 100 } , { 0, 0 } },
        { { -sun_size,  sun_size, 100 } , { 0, 1 } },
        { {  sun_size,  sun_size, 100 } , { 1, 1 } },
        { {  sun_size, -sun_size, 100 } , { 1, 0 } } };

    gl::vbo sunvbo (sunarr);

    sunvbo.bind();

    glCheck(glEnableClientState(GL_VERTEX_ARRAY));
    glCheck(glEnableClientState(GL_TEXTURE_COORD_ARRAY));
    bind_attributes_ogl2<sunvtx>();

    glColor4f(1,1,1,1);
    sunvbo.draw();
    sunvbo.unbind();
    glCheck(glDisableClientState(GL_VERTEX_ARRAY));
    glCheck(glDisableClientState(GL_TEXTURE_COORD_ARRAY));

    }

    texture::unbind();

    camera_.move_to((c + vector(0, 0, bob)) * 16.f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(camera_.projection_matrix().as_ptr());

    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(camera_.model_view_matrix().as_ptr());

    glCheck(glEnable(GL_CULL_FACE));
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthMask(GL_TRUE));
}

void sfml::draw_bar(float x, float y, int index, int width, double ratio)
{
    for (int i (0); i < width; ++i)
    {
        int idx (index);

        ui_elem_[idx].setPosition(x, y);
        app_.draw(ui_elem_[idx]);
        x += ui_elem_[idx].getTextureRect().width;
    }
}

void sfml::draw_ui(double elapsed, const hud& h)
{
    static int step (0);
    static sf::Text info;
    static double acc_elapsed (0);

    acc_elapsed += elapsed;
    tot_elapsed += elapsed;
    ++step;

    glCheck(glEnable(GL_CULL_FACE));
    glCheck(glDisable(GL_DEPTH_TEST));

    glCheck(glMatrixMode(GL_PROJECTION));
    glCheck(glLoadIdentity());
    glCheck(gluOrtho2D(0, width_, height_, 0));
    glCheck(glMatrixMode(GL_MODELVIEW));
    glCheck(glLoadIdentity());

    app_.pushGLStates();
    app_.resetGLStates();
    app_.draw(ui_elem_[0]);

//    pCanvas->RenderCanvas();

    auto& msgs (h.console_messages());
    if (!msgs.empty())
    {
        float height (20 * msgs.size());
        sf::RectangleShape bg ({ 500.f, height });
        bg.setPosition(5, height_ - (50 + height));
        bg.setFillColor(sf::Color(0, 0, 0, 100));
        app_.draw(bg);

        float y (height_ - (50 + height));
        for (auto& line : msgs)
        {
            std::u32string wide;
            sf::Utf8::toUtf32(line.msg.begin(), line.msg.end(), std::back_inserter(wide));
            sf::String l ((const unsigned int*)&wide[0]);

            sf::Text txt (l, *ui_font_, 16);
            txt.setPosition(10, y);
            app_.draw(txt);
            y += 20;
        }
    }

    if (h.hotbar_needs_update)
        draw_hotbar(h);

    sf::Sprite hb (hotbar_.getTexture());
    hb.setPosition((width_ - hb.getGlobalBounds().width) * 0.5,
                    height_ - hb.getGlobalBounds().height - 1);
    app_.draw(hb);

    if (acc_elapsed > 0.2)
    {
        info.setFont(*ui_font_);
        info.setPosition(10, 10);
        info.setCharacterSize(14);

        //world_coordinates p (plr.position());
        //world_coordinates r (p - world_center);

        GLint total_mem_kb = 0;
        glGetIntegerv(0x9048, &total_mem_kb);

        GLint cur_avail_mem_kb = 0;
        glGetIntegerv(0x9049, &cur_avail_mem_kb);

        /*
        info.setString((boost::format("%i FPS | %i ; %i ; %i | %i of %i MB")
            % int((double)step / acc_elapsed + 0.5f)
            % (int)r.x % (int)r.y % (int)r.z
            % int((total_mem_kb - cur_avail_mem_kb) / 1024)
            % int(total_mem_kb / 1024)
            ).str());
*/
        info.setString((boost::format("%i FPS")
            % int((double)step / acc_elapsed + 0.5f)
            ).str());

        step = 0;
        acc_elapsed = 0;
    }

    app_.draw(info);
    app_.popGLStates();
}

void sfml::waiting_screen() const
{

}

void sfml::display()
{
    app_.display();
}

void sfml::resize(unsigned int w, unsigned int h)
{
    width_ = w;
    height_ = h;

    auto current_view (app_.getView());
    current_view.setSize(w, h);
    current_view.setCenter(w * 0.5, h * 0.5);
    app_.setView(current_view);
    glViewport(0, 0, w, h);

    ui_elem_[0].setPosition(w * 0.5, h * 0.5);
}

void sfml::process(const event &ev)
{
}

void sfml::process(const sf::Event &ev)
{
    //GwenInput.ProcessMessage(*const_cast<sf::Event*>(&ev));
}

void sfml::draw_hotbar(const hud& h)
{
    static const light prefab[6] = { 7, 7, 11, 11, 15, 15 };

    std::cout << "draw hotbar " << h.hotbar.size() << std::endl;

    if (h.hotbar.empty())
        return;

    ui_elem_manager::resource slot       (ui_elem("slot"));
    ui_elem_manager::resource slot_left  (ui_elem("slot-left"));
    ui_elem_manager::resource slot_right (ui_elem("slot-right"));
    ui_elem_manager::resource slot_sep   (ui_elem("slot-sep"));
    ui_elem_manager::resource slot_actv  (ui_elem("slot-active"));

    if (slot == nullptr || slot_actv == nullptr)
        return;

    size_t size_left  (slot_left  ? (*slot_left).getGlobalBounds().width : 0),
           size_slot  ((*slot).getGlobalBounds().width),
           size_actv  ((*slot_actv).getGlobalBounds().width),
           size_sep   (slot_sep   ? (*slot_sep).getGlobalBounds().width  : 0),
           size_right (slot_right ? (*slot_right).getGlobalBounds().width: 0);

    size_t slot_height ((*slot).getGlobalBounds().height),
           actv_height ((*slot_actv).getGlobalBounds().height);

    size_t total (h.hotbar.size() * size_slot + size_left + size_right);
    if (h.hotbar.size() >= 2)
        total += (h.hotbar.size() - 1) * size_sep;

    size_t edge (size_actv > size_slot ? (size_actv - size_slot) * 0.5 : 0);
    size_t edge_l (0), edge_r (0);

    if (edge > size_left)
        edge_l = edge - size_left;

    if (edge > size_right)
        edge_r = edge - size_right;

    total += edge_l + edge_r;
    size_t total_height (std::max(slot_height, actv_height));

    if (hotbar_.getSize().x != total || hotbar_.getSize().y != total_height)
        hotbar_.create(total, total_height);

    hotbar_.clear(sf::Color(0,0,0,0));

    size_t pen_x (edge_l), pen_y (0);
    if (actv_height > slot_height)
        pen_y = (actv_height - slot_height) * 0.5;

    if (slot_left)
    {
        slot_left->setPosition(pen_x, pen_y);
        hotbar_.draw(*slot_left);
        pen_x += size_left;
    }

    for (size_t cnt (0); cnt < h.hotbar.size(); ++cnt)
    {
        slot->setPosition(pen_x, pen_y);
        hotbar_.draw(*slot);
        pen_x += size_slot;

        if (slot_sep && cnt + 1 < h.hotbar.size())
        {
            slot_sep->setPosition(pen_x, pen_y);
            hotbar_.draw(*slot_sep);
            pen_x += size_sep;
        }
    }

    if (slot_right)
    {
        slot_right->setPosition(pen_x, pen_y);
        hotbar_.draw(*slot_right);
        pen_x += size_right;
    }

    if (h.active_slot >= 0 && h.active_slot < h.hotbar.size())
    {
        slot_actv->setPosition(edge_l + size_left + h.active_slot * (size_slot + size_sep) + size_slot * 0.5 - size_actv * 0.5, 0);
        hotbar_.draw(*slot_actv);
    }

    pen_x = edge_l + size_left;
    for (auto& curr_slot : h.hotbar)
    {
        switch (curr_slot.type)
        {
        case hotbar_slot::material:
            {
            auto     temp (make_terrain_mesher());
            uint16_t mat_idx (find_material(curr_slot.name));
            const    material& m (material_prop[mat_idx]);
            chunk_index c (0,0,0);

            float scale (1.25f);
            if (m.model.empty())
            {
                for (int i (0); i < 6; ++i)
                    (*temp).add_face(c, direction_type(i), m.textures[i], prefab[i]);
            }
            else
            {
                scale = 1.5f;
                (*temp).add_custom_block(c, m.model, std::vector<light>(prefab, prefab + 6));
            }

            gl::vbo mesh ((*temp).make_buffer());

            glPushMatrix();
            glLoadIdentity();

            glTranslatef(pen_x + size_slot * 0.5f, pen_y + slot_height * 0.5f, 0.f);
            glScalef(scale, scale, 0.0f);
            glRotatef( 30, 1, 0, 0);
            glRotatef( 30, 0, 1, 0);
            glTranslatef(-8.0f, -8.0f, -8.0f);
            draw(mesh);
            glPopMatrix();

            }
            break;

        case hotbar_slot::item:
            {
            auto icon (sprites(curr_slot.name));
            if (icon)
            {
                (*icon).setPosition(pen_x + size_slot * 0.5 - 16,
                                    pen_y + slot_height * 0.5 - 16);
                hotbar_.draw(*icon);
            }

            }
        }
        pen_x += size_slot + size_sep;
    }

    hotbar_.display();
}

} // namespace hexa

