//---------------------------------------------------------------------------
/// \file   server/radiosity_lightmap.hpp
/// \brief  Add radiosity as a postprocessing step.
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

class radiosity_lightmap : public lightmap_generator_i
{
public:
    radiosity_lightmap(storage_i& cache,
                       const boost::property_tree::ptree& conf);

    virtual ~radiosity_lightmap();

    virtual lightmap& generate(const chunk_coordinates& pos,
                               const surface& s,
                               lightmap& chunk,
                               unsigned int phase = 0) const;

    unsigned int phases() const { return 3; }

private:
};

} // namespace hexa

