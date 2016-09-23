/*
Copyright_License {

  Top Hat Soaring Glide Computer - http://www.tophatsoaring.org/
  Copyright (C) 2000-2016 The Top Hat Soaring Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_VARIO_INFO_HPP
#define XCSOAR_VARIO_INFO_HPP

#include "Math/fixed.hpp"
#include "LiftDatabase.hpp"

#include <type_traits>

/** Derived vario data */
struct VarioInfo
{
  fixed sink_rate;

  /** Average vertical speed based on 30s */
  fixed average;
  /** Average vertical speed of the airmass based on 30s */
  fixed netto_average;

  /** Instant glide ratio over ground */
  fixed gr;
  /** Glide ratio over ground while in Cruise mode */
  fixed cruise_gr;

  /**
   * Average glide ratio over ground.  Zero means the value is not available.
   */
  fixed average_gr;

  /** Instant lift/drag ratio */
  fixed ld_vario;

  /**
   * The lift of each ten degrees while circling.
   * Index 1 equals 5 to 15 degrees.
   */
  LiftDatabase lift_database;

  void Clear();
};

static_assert(std::is_trivial<VarioInfo>::value, "type is not trivial");

#endif
