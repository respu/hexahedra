//---------------------------------------------------------------------------
/// \file   area_data.hpp
/// \brief  A flat, 16x16 part of the world's map.
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
// Copyright 2012, nocte@hippie.nu
//---------------------------------------------------------------------------

#pragma once

#include <cassert>
#include <memory>
#include <vector>
#include <utility>

#include "basic_types.hpp"
#include "serialize.hpp"

namespace hexa {

/** A flat, 16x16 part of the world's map.
 *  It is used to store things like a detailed height map, or different
 *  kinds of biome info such as temperature or humidity. */
class area_data : boost::noncopyable
{
    typedef std::vector<int16_t>  array_t;
    array_t buf_;

public:
    typedef array_t::value_type        value_type;
    typedef array_t::iterator          iterator;
    typedef array_t::const_iterator    const_iterator;

public:
    area_data() { buf_.resize(chunk_area); }

    area_data(area_data&& m) : buf_ (std::move(m.buf_)) { }

    /** Fill the area with zero. */
    void clear() { std::fill(begin(), end(), 0); }

    /** Copy the contents to another area. */
    void copy(const area_data& other) { buf_ = other.buf_; }

    iterator       begin()       { return buf_.begin(); }
    const_iterator begin() const { return buf_.begin(); }
    iterator       end()         { return buf_.end();   }
    const_iterator end() const   { return buf_.end();   }

    value_type& operator[](map_index pos)
        { return operator()(pos.x, pos.y); }

    value_type  operator[](map_index pos) const
        { return operator()(pos.x, pos.y); }

    value_type& operator()(uint8_t x, uint8_t y)
    {
        assert (x < chunk_size);
        assert (y < chunk_size);
        return buf_[x + y * chunk_size];
    }

    value_type  operator()(uint8_t x, uint8_t y) const
    {
        assert (x < chunk_size);
        assert (y < chunk_size);
        return buf_[x + y * chunk_size];
    }

    value_type  get(int x, int y)
        { return operator()(x,y); }

    void        set(int x, int y, int value)
        { operator()(x,y) = value; }

    /** Dummy resize function.
     *  This function only exists to make this class play nice with other
     *  algorithms that expect the 'resize' function to exist. */
    void resize(size_t dummy)
        { assert (dummy == chunk_area); }

    /** Return the number of elements (usually 256). */
    size_t size() const
        { return chunk_area; }

    template <class archiver>
    archiver& serialize(archiver& ar)
        { return ar.raw_data(buf_, chunk_area); }
};

/** Reference-counted pointer to area_data. */
typedef std::shared_ptr<area_data>    area_ptr;

} // namespace hexa

