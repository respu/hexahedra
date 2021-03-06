//---------------------------------------------------------------------------
/// \file   client/opengl_vertex.hpp
/// \brief  Composition and binding of OpenGL vertex attributes
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
// Copyright 2011, 2012, nocte@hippie.nu
//---------------------------------------------------------------------------

#pragma once

#include <array>
#include <initializer_list>
#include <boost/fusion/algorithm.hpp>
#include <GL/glew.h>
#include <GL/gl.h>

namespace hexa {

// Map C++ types to OpenGL enumerations

template <class t>
struct gl_type { GLenum operator()() { return 0; } };

template <>
struct gl_type<uint8_t> { GLenum operator()() { return GL_UNSIGNED_BYTE; } };

template <>
struct gl_type<int8_t> { GLenum operator()() { return GL_BYTE; } };

template <>
struct gl_type<uint16_t> { GLenum operator()() { return GL_UNSIGNED_SHORT; } };

template <>
struct gl_type<int16_t> { GLenum operator()() { return GL_SHORT; } };

template <>
struct gl_type<float> { GLenum operator()() { return GL_FLOAT; } };


//---------------------------------------------------------------------------

// The following structs define the data that can be put in a vertex

#pragma pack(push, 1)

template <class t = float>
struct vtx_uv : public vector2<t>
{
    vtx_uv() { }
    vtx_uv(const vector2<t>& init) : vector2<t> (init) { }
    vtx_uv(std::initializer_list<t> init) : vector2<t> (init) { }
    vtx_uv(const t* init) : vector2<t> (init) { }

    static void bind(size_t i, size_t o, size_t stride)
    {
        glVertexAttribPointer(i, 2, gl_type<t>()(), GL_FALSE, stride, (GLvoid*)o);
    }

    static void bind_ogl2(size_t o, size_t stride)
    {
        glTexCoordPointer(2, gl_type<t>()(), stride, (GLvoid*)o);
    }

    static int client_state()
    {
        return GL_TEXTURE_COORD_ARRAY;
    }
};

template <class t = float>
struct vtx_xyz : public vector3<t>
{
    vtx_xyz() { }
    vtx_xyz(const vector3<t>& init) : vector3<t> (init) { }
    vtx_xyz(std::initializer_list<t> init) : vector3<t> (init) { }
    vtx_xyz(const t* init) : vector3<t> (init) { }

    static void bind(size_t i, size_t o, size_t stride)
    {
        glVertexAttribPointer(i, 3, gl_type<t>()(), GL_FALSE, stride, (GLvoid*)o);
    }

    static void bind_ogl2(size_t o, size_t stride)
    {
        glVertexPointer(3, gl_type<t>()(), stride, (GLvoid*)o);
    }

    static int client_state()
    {
        return GL_VERTEX_ARRAY;
    }
};

template <class t = float>
struct vtx_normal : public vector3<t>
{
    vtx_normal() { }
    vtx_normal(const vector3<t>& init) : vector3<t> (init) { }
    vtx_normal(std::initializer_list<t> init) : vector3<t> (init) { }
    vtx_normal(const t* init) : vector3<t> (init) { }

    static void bind(size_t i, size_t o, size_t stride)
    {
        glVertexAttribPointer(i, 3, gl_type<t>()(), GL_FALSE, stride, (GLvoid*)o);
    }

    static void bind_ogl2(size_t o, size_t stride)
    {
        glNormalPointer(gl_type<t>()(), stride, (GLvoid*)o);
    }

    static int client_state()
    {
        return GL_NORMAL_ARRAY;
    }
};


template <class t = uint8_t>
struct vtx_rgb : public vector3<t>
{
    vtx_rgb() { }
    vtx_rgb(const vector3<t>& init) : vector3<t> (init) { }
    vtx_rgb(const t* init) : vector3<t> (init) { }

    static void bind(size_t i, size_t o, size_t stride)
    {
        glVertexAttribPointer(i, 3, gl_type<t>()(), GL_TRUE, stride, (GLvoid*)o);
    }

    static void bind_ogl2(size_t o, size_t stride)
    {
        glColorPointer(3, gl_type<t>()(), stride, (GLvoid*)o);
    }

    static int client_state()
    {
        return GL_COLOR_ARRAY;
    }
};

template <class t = float>
struct vtx_scalar
{
    vtx_scalar() { }
    vtx_scalar(t init) : value (init) { }

    static void bind(size_t i, size_t o, size_t stride)
    {
        glVertexAttribIPointer(i, 1, gl_type<t>()(), stride, (GLvoid*)o);
    }

    static void bind_ogl2(size_t o, size_t stride)
    {
    }

    static int client_state()
    {
        return 0;
    }

    t value;
};

template <class t, size_t count>
struct vtx_array : public std::array<t, count>
{
    vtx_array() { }
    vtx_array(const std::array<t, count>& init) : std::array<t, count>(init) { }
    vtx_array(const t* init)
    {
        for (size_t i (0); i < count; ++i)
            (*this)[i] = init[i];
    }

    vtx_array(std::initializer_list<t> init)
    {
        assert(init.size() == count);
        std::copy(init.begin(), init.end(), this->begin());
    }

    static void bind(size_t i, size_t o, size_t stride)
    {
        glVertexAttribIPointer(i, count, gl_type<t>()(), stride, (GLvoid*)o);
    }

    static void bind_ogl2(size_t o, size_t stride)
    {
    }

    static int client_state()
    {
        return 0;
    }
};

template <class t, size_t count>
struct vtx_normalized_array : public std::array<t, count>
{
    vtx_normalized_array() { }
    vtx_normalized_array(const std::array<t, count>& init) : std::array<t, count>(init) { }
    vtx_normalized_array(const t* init)
    {
        for (size_t i (0); i < count; ++i)
            (*this)[i] = init[i];
    }

    vtx_normalized_array(std::initializer_list<t> init)
    {
        assert(init.size() == count);
        std::copy(init.begin(), init.end(), this->begin());
    }

    static void bind(size_t i, size_t o, size_t stride)
    {
        glVertexAttribPointer(i, count, gl_type<t>()(), true, stride, (GLvoid*)o);
    }

    static void bind_ogl2(size_t o, size_t stride)
    {
    }

    static int client_state()
    {
        return 0;
    }
};

template <size_t count>
struct vtx_padding : public std::array<char, count>
{
    static void bind(size_t i, size_t o, size_t stride)
    {
    }

    static void bind_ogl2(size_t o, size_t stride)
    {
    }

    static int client_state()
    {
        return 0;
    }
};

//---------------------------------------------------------------------------

namespace ogl
{

    /*

template <typename Head>
class vertex : public Head
{
public:
    vertex (const Head& init)
        : Head(init)
    { }

    static void bind_ogl2()
    {
        _bind_ogl2(0, sizeof(Head));
    }

    static void bind_ogl3()
    {
        _bind_ogl3(0, 0, sizeof(Head));
    }

    static void enable_client_states()
    {
        Head::enable_client_state();
    }

    static void disable_client_states()
    {
        Head::disable_client_state();
    }

protected:
    static void _bind_ogl2(size_t offset, size_t size)
    {
        Head::bind_ogl2(offset, size);
    }

    static void _bind_ogl3(size_t count, size_t offset, size_t size)
    {
        Head::bind_ogl3(count, offset, size);
    }
};

template <typename Head, typename... Tail>
class vertex : public Head, public vertex<Tail...>
{
public:
    vertex (const Head& init, const Tail&... rest)
        : Head(init)
        , vertex<Tail...>(rest)
    { }

    static void bind_ogl2()
    {
        _bind_ogl2(0, sizeof(vertex<Head, Tail...>));
    }

    static void bind_ogl3()
    {
        _bind_ogl3(0, 0, sizeof(vertex<Head, Tail...>));
    }

    static void enable_client_states()
    {
        Head::enable_client_state();
        vertex<Tail...>::enable_client_states();
    }

    static void disable_client_states()
    {
        Head::disable_client_state();
        vertex<Tail...>::disable_client_states();
    }

protected:
    static void _bind_ogl2(size_t offset, size_t size)
    {
        Head::bind_ogl2(offset, size);
        vertex<Tail...>::_bind_ogl2(offset + sizeof(Head), size);
    }

    static void _bind_ogl3(size_t count, size_t offset, size_t size)
    {
        Head::bind_ogl3(count, offset, size);
        vertex<Tail...>::_bind_ogl2(count + 1, offset + sizeof(Head), size);
    }
};

*/

}

/** OpenGL vertex type.
 *  A vertex can hold a position, color, texture coordinate, or general
 *  scalar values, in any order and combination. */
template <class elem1>
class vertex_1
{
public:
    enum { element_count = 1 };

    typedef elem1 value_type_1;
    typedef elem1 value_type_2;
    typedef elem1 value_type_3;
    typedef elem1 value_type_4;
    typedef elem1 value_type_5;

public:
    vertex_1 () {}

    vertex_1 (elem1 i1)
        : e1 (i1) { }

public:
    elem1   e1;
};

template <class elem1, class elem2>
class vertex_2
{
public:
    enum { element_count = 2 };

    typedef elem1 value_type_1;
    typedef elem2 value_type_2;
    typedef elem2 value_type_3;
    typedef elem2 value_type_4;
    typedef elem2 value_type_5;

public:
    vertex_2 () { }

    vertex_2 (elem1 i1, elem2 i2 = elem2())
        : e1 (i1), e2 (i2) { }

public:
    elem1   e1;
    elem2   e2;
};

template <class elem1, class elem2, class elem3>
class vertex_3
{
public:
    enum { element_count = 3 };

    typedef elem1 value_type_1;
    typedef elem2 value_type_2;
    typedef elem3 value_type_3;
    typedef elem3 value_type_4;
    typedef elem3 value_type_5;

public:
    vertex_3 () { }

    vertex_3 (elem1 i1, elem2 i2, elem3 i3 = elem3())
        : e1 (i1), e2 (i2), e3 (i3) { }

public:
    elem1   e1;
    elem2   e2;
    elem3   e3;
};

template <class elem1, class elem2, class elem3, class elem4>
class vertex_4
{
public:
    enum { element_count = 4 };

    typedef elem1 value_type_1;
    typedef elem2 value_type_2;
    typedef elem3 value_type_3;
    typedef elem4 value_type_4;
    typedef elem4 value_type_5;

public:
    vertex_4 () { }

    vertex_4 (elem1 i1, elem2 i2, elem3 i3, elem4 i4 = elem4())
        : e1 (i1), e2 (i2), e3 (i3), e4 (i4) { }

public:
    elem1   e1;
    elem2   e2;
    elem3   e3;
    elem4   e4;
};

template <class elem1, class elem2, class elem3, class elem4, class elem5>
class vertex_5
{
public:
    enum { element_count = 5 };

    typedef elem1 value_type_1;
    typedef elem2 value_type_2;
    typedef elem3 value_type_3;
    typedef elem4 value_type_4;
    typedef elem5 value_type_5;

public:
    vertex_5 () { }

    vertex_5 (elem1 i1, elem2 i2, elem3 i3, elem4 i4, elem5 i5 = elem5())
        : e1 (i1), e2 (i2), e3 (i3), e4 (i4), e5(i5) { }

public:
    elem1   e1;
    elem2   e2;
    elem3   e3;
    elem4   e4;
    elem5   e5;
};

#pragma pack(pop)

//---------------------------------------------------------------------------

template <class vertex_t>
void bind_attributes()
{
    size_t offset (0), size (sizeof(vertex_t));
    vertex_t::value_type_1::bind(0, offset, size);

    if (vertex_t::element_count > 1)
    {
        offset += sizeof(typename vertex_t::value_type_1);
        vertex_t::value_type_2::bind(1, offset, size);
    }

    if (vertex_t::element_count > 2)
    {
        offset += sizeof(typename vertex_t::value_type_2);
        vertex_t::value_type_3::bind(2, offset, size);
    }

    if (vertex_t::element_count > 3)
    {
        offset += sizeof(typename vertex_t::value_type_3);
        vertex_t::value_type_4::bind(3, offset, size);
    }

    if (vertex_t::element_count > 4)
    {
        offset += sizeof(typename vertex_t::value_type_4);
        vertex_t::value_type_5::bind(4, offset, size);
    }
}

template <class vertex_t>
void bind_attributes_ogl2()
{
    size_t offset (0), size (sizeof(vertex_t));
    vertex_t::value_type_1::bind_ogl2(offset, size);

    if (vertex_t::element_count > 1)
    {
        offset += sizeof(typename vertex_t::value_type_1);
        vertex_t::value_type_2::bind_ogl2(offset, size);
    }

    if (vertex_t::element_count > 2)
    {
        offset += sizeof(typename vertex_t::value_type_2);
        vertex_t::value_type_3::bind_ogl2(offset, size);
    }

    if (vertex_t::element_count > 3)
    {
        offset += sizeof(typename vertex_t::value_type_3);
        vertex_t::value_type_4::bind_ogl2(offset, size);
    }

    if (vertex_t::element_count > 4)
    {
        offset += sizeof(typename vertex_t::value_type_4);
        vertex_t::value_type_5::bind_ogl2(offset, size);
    }
}

template <typename vertex_t>
void enable_vertex_attributes()
{
    for (int i {0}; i < vertex_t::element_count; ++i)
        glEnableVertexAttribArray(i);
}

template <typename vertex_t>
void disable_vertex_attributes()
{
    for (int i {0}; i < vertex_t::element_count; ++i)
        glDisableVertexAttribArray(i);
}

template <typename vertex_t>
std::vector<int> client_states()
{
    std::vector<int> result;

    if (vertex_t::value_type_1::client_state())
        result.push_back(vertex_t::value_type_1::client_state());

    if (vertex_t::element_count > 1 && vertex_t::value_type_2::client_state())
        result.push_back(vertex_t::value_type_2::client_state());

    if (vertex_t::element_count > 2 && vertex_t::value_type_3::client_state())
        result.push_back(vertex_t::value_type_3::client_state());

    if (vertex_t::element_count > 3 && vertex_t::value_type_4::client_state())
        result.push_back(vertex_t::value_type_4::client_state());

    if (vertex_t::element_count > 4 && vertex_t::value_type_5::client_state())
        result.push_back(vertex_t::value_type_5::client_state());

    return result;
}

template <typename vertex_t>
void enable_client_states()
{
    for (auto i : client_states<vertex_t>())
        glEnableClientState(i);
}


} // namespace hexa

