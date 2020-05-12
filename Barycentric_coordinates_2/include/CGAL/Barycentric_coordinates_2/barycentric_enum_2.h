// Copyright (c) 2019 INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0+
//
//
// Author(s)     : Dmitry Anisimov, David Bommes, Kai Hormann, Pierre Alliez
//

#ifndef CGAL_BARYCENTRIC_ENUM_2_H
#define CGAL_BARYCENTRIC_ENUM_2_H

#include <CGAL/license/Barycentric_coordinates_2.h>

namespace CGAL {

/*!
  \ingroup PkgBarycentricCoordinates2Ref

  The namespace `Barycentric_coordinates` contains implementations of all
  generalized barycentric coordinates: 2D, 3D, related enumerations, etc.
*/
namespace Barycentric_coordinates {

/// \name Computation Policies
/// @{

/*!
  `Computation_policy` provides a way to choose an asymptotic time complexity
  of the algorithm.
*/
enum class Computation_policy {

  /*!
    Computation is very precise but has typically a quadratic time complexity
    with respect to the number of the polygon vertices. In addition,
    each query point is controlled for different edge cases, which slows down
    the computation. This is the default strategy.
  */
  PRECISE_COMPUTATION_WITH_EDGE_CASES = 0,

  /*!
    Computation is very precise but has typically a quadratic time complexity
    with respect to the number of the polygon vertices.
  */
  PRECISE_COMPUTATION = 1,

  /*!
    Computation has typically a linear time complexity with respect to the
    number of the polygon vertices, but may be less precise. In addition,
    each query point is controlled for different edge cases, which slows down
    the computation.
  */
  FAST_COMPUTATION_WITH_EDGE_CASES = 2,

  /*!
    Computation has typically a linear time complexity with respect to the
    number of the polygon vertices, but may be less precise.
  */
  FAST_COMPUTATION = 3,

  /*!
    The default value is `PRECISE_COMPUTATION_WITH_EDGE_CASES`.
  */
  DEFAULT = PRECISE_COMPUTATION_WITH_EDGE_CASES
};

/// @}

} // namespace Barycentric_coordinates
} // namespace CGAL

#endif // CGAL_BARYCENTRIC_ENUM_2_H
