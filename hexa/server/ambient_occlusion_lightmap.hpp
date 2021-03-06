//---------------------------------------------------------------------------
/// \file   server/ambient_occlusion_lightmap.hpp
/// \brief  Generate a lightmap based on an ambient occlusion model.
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

#include <array>
#include <vector>
#include <hexa/basic_types.hpp>
#include <hexa/ray_bundle.hpp>
#include <hexa/storage_i.hpp>
#include "lightmap_generator_i.hpp"

namespace hexa {

template <class> class neighborhood;

class ambient_occlusion_lightmap : public lightmap_generator_i
{
    typedef std::array<ray_bundle, 6>  rays;
    std::vector<rays> detail_levels_;

public:
    ambient_occlusion_lightmap(storage_i& cache,
                               const boost::property_tree::ptree& config);

    virtual ~ambient_occlusion_lightmap();

    virtual lightmap& generate(const chunk_coordinates& pos,
                               const surface& s,
                               lightmap& chunk,
                               unsigned int phase) const;

    unsigned int phases() const { return 3; }

private:
    rays  precalc (float length, unsigned int count) const;

    float recurse(const ray_bundle& r, float ray_power,
                  const world_coordinates& blk,
                  neighborhood<chunk_ptr>& nbh,
                  bool first = true) const;
};

} // namespace hexa

