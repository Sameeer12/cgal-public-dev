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

#ifndef CGAL_BARYCENTRIC_UTILS_2_H
#define CGAL_BARYCENTRIC_UTILS_2_H

#include <CGAL/license/Barycentric_coordinates_2.h>

// STL includes.
#include <set>
#include <map>
#include <list>
#include <string>
#include <memory>
#include <vector>
#include <utility>
#include <iterator>
#include <tuple>

// Boost headers.
#include <boost/mpl/has_xxx.hpp>
#include <boost/optional/optional.hpp>

// CGAL includes.
#include <CGAL/barycenter.h>
#include <CGAL/assertions.h>
#include <CGAL/utils.h>
#include <CGAL/number_utils.h>
#include <CGAL/property_map.h>
#include <CGAL/Polygon_2_algorithms.h>

// Internal includes.
#include <CGAL/Barycentric_coordinates_2/barycentric_enum_2.h>

namespace CGAL {
namespace Barycentric_coordinates {
namespace internal {

  enum class Query_point_location {

    /// Query point is located at the vertex of the polygon.
    ON_VERTEX = 0,

    /// Query point is located on the edge of the polygon.
    ON_EDGE = 1,

    /// Query point is located in the polygon's interior.
    ON_BOUNDED_SIDE = 2,

    /// Query point is located in the polygon's exterior.
    ON_UNBOUNDED_SIDE = 3,

    /// Location is unspecified. Leads to all coordinates being set to zero.
    UNSPECIFIED = 4
  };

  template<typename GeomTraits>
  class Default_sqrt {

  private:
    using Traits = GeomTraits;
    using FT = typename Traits::FT;

  public:
    FT operator()(const FT value) const {
      return static_cast<FT>(
        CGAL::sqrt(CGAL::to_double(CGAL::abs(value))));
    }
  };

  BOOST_MPL_HAS_XXX_TRAIT_NAMED_DEF(Has_nested_type_Sqrt, Sqrt, false)

  // Case: do_not_use_default = false.
  template<typename GeomTraits,
  bool do_not_use_default = Has_nested_type_Sqrt<GeomTraits>::value>
  class Get_sqrt {

  public:
    using Traits = GeomTraits;
    using Sqrt = Default_sqrt<Traits>;

    static Sqrt sqrt_object(const Traits& ) {
      return Sqrt();
    }
  };

  // Case: do_not_use_default = true.
  template<typename GeomTraits>
  class Get_sqrt<GeomTraits, true> {

  public:
    using Traits = GeomTraits;
    using Sqrt = typename Traits::Sqrt;

    static Sqrt sqrt_object(const Traits& traits) {
      return traits.sqrt_object();
    }
  };

  template<typename FT>
  void normalize(std::vector<FT>& values) {

    FT sum = FT(0);
    for (const auto& value : values)
      sum += value;

    if (sum == FT(0))
      return;

    CGAL_assertion(sum != FT(0));
    const FT inv_sum = FT(1) / sum;

    for (auto& value : values)
      value *= inv_sum;
  }

  template<typename OutputIterator>
  void get_default(const std::size_t size, OutputIterator output) {
    for (std::size_t i = 0; i < size; ++i)
      *(output++) = 0;
  }

  template<
  typename OutputIterator,
  typename GeomTraits>
  OutputIterator linear_coordinates_2(
    const typename GeomTraits::Point_2& source,
    const typename GeomTraits::Point_2& target,
    const typename GeomTraits::Point_2& query,
    OutputIterator coordinates,
    const GeomTraits traits) {

    CGAL_precondition(source != target);
    if (source == target)
      return coordinates;

    // Number type.
    using FT = typename GeomTraits::FT;

    // Functions.
    const auto scalar_product_2   = traits.compute_scalar_product_2_object();
    const auto squared_distance_2 = traits.compute_squared_distance_2_object();

    // Project point on the segment.
    const FT opposite_scalar_product =
    scalar_product_2(query - target, source - target);

    // Compute coordinates.
    const FT b0 = opposite_scalar_product / squared_distance_2(source, target);
    const FT b1 = FT(1) - b0;

    // Return coordinates.
    *(coordinates++) = b0;
    *(coordinates++) = b1;

    return coordinates;
  }

  template<
  typename OutputIterator,
  typename GeomTraits>
  OutputIterator planar_coordinates_2(
    const typename GeomTraits::Point_2& p0,
    const typename GeomTraits::Point_2& p1,
    const typename GeomTraits::Point_2& p2,
    const typename GeomTraits::Point_2& query,
    OutputIterator coordinates,
    const GeomTraits traits) {

    // Number type.
    using FT = typename GeomTraits::FT;

    // Functions.
    const auto area_2 = traits.compute_area_2_object();
    const FT total_area = area_2(p0, p1, p2);

    CGAL_precondition(total_area != FT(0));
    if (total_area == FT(0))
      return coordinates;

    // Compute some related sub-areas.
    const FT A1 = area_2(p1, p2, query);
    const FT A2 = area_2(p2, p0, query);

    // Compute the inverted total area of the triangle.
    const FT inverted_total_area = FT(1) / total_area;

    // Compute coordinates.
    const FT b0 = A1 * inverted_total_area;
    const FT b1 = A2 * inverted_total_area;
    const FT b2 = FT(1) - b0 - b1;

    // Return coordinates.
    *(coordinates++) = b0;
    *(coordinates++) = b1;
    *(coordinates++) = b2;

    return coordinates;
  }

  template<typename GeomTraits>
  boost::optional< std::pair<Query_point_location, std::size_t> >
  get_edge_index(
    const std::vector<typename GeomTraits::Point_2>& polygon,
    const typename GeomTraits::Point_2& query,
    const GeomTraits traits) {

    const auto collinear_2 = traits.collinear_2_object();
    const auto collinear_are_ordered_along_line_2 =
      traits.collinear_are_ordered_along_line_2_object();
    CGAL_precondition(polygon.size() >= 3);

    const std::size_t n = polygon.size();
    for (std::size_t i = 0; i < n; ++i) {
      if (polygon[i] == query)
        return std::make_pair(Query_point_location::ON_VERTEX, i);

      const std::size_t ip = (i + 1) % n;
      if (
        collinear_2(
          polygon[i], polygon[ip], query) &&
        collinear_are_ordered_along_line_2(
          polygon[i], query, polygon[ip]))
        return std::make_pair(Query_point_location::ON_EDGE, i);
    }

    /*
    using FT = typename GeomTraits::FT;
    using Vector_2 = typename GeomTraits::Vector_2;
    using Segment_2 = typename GeomTraits::Segment_2;

    const FT tol = FT(1) / FT(100000);
    for (std::size_t i = 0; i < n; ++i) {
      const Segment_2 segment = Segment_2(query, polygon[i]);
      const FT r = segment.squared_length();
      if (CGAL::abs(r) < tol)
        return std::make_pair(Query_point_location::ON_VERTEX, i);
    }

    for (std::size_t i = 0; i < n; ++i) {
      const std::size_t ip = (i + 1) % n;

      const Vector_2 s1 = Vector_2(query, polygon[i]);
      const Vector_2 s2 = Vector_2(query, polygon[ip]);

      const FT A = CGAL::determinant(s1, s2) / FT(2);
      const FT D = CGAL::scalar_product(s1, s2);

      if (CGAL::abs(A) < tol && D < FT(0))
        return std::make_pair(Query_point_location::ON_EDGE, i);
    } */

    return boost::none;
  }

  template<typename GeomTraits>
  boost::optional< std::pair<Query_point_location, std::size_t> >
  locate_wrt_polygon_2(
    const std::vector<typename GeomTraits::Point_2>& polygon,
    const typename GeomTraits::Point_2& query,
    const GeomTraits traits) {

    const auto type = CGAL::bounded_side_2(
      polygon.begin(), polygon.end(), query, traits);

    // Locate point with respect to different polygon locations.
    switch (type) {
      case CGAL::ON_BOUNDED_SIDE:
        return std::make_pair(Query_point_location::ON_BOUNDED_SIDE, std::size_t(-1));
      case CGAL::ON_UNBOUNDED_SIDE:
        return std::make_pair(Query_point_location::ON_UNBOUNDED_SIDE, std::size_t(-1));
      case CGAL::ON_BOUNDARY:
        return get_edge_index(polygon, query, traits);
      default:
        return std::make_pair(Query_point_location::UNSPECIFIED, std::size_t(-1));
    }
    return boost::none;
  }

  enum class Polygon_type {

    // Concave polygon = non-convex polygon.
    CONCAVE = 0,

    // This is a convex polygon with collinear vertices.
    WEAKLY_CONVEX = 1,

    // This is a convex polygon without collinear vertices.
    STRICTLY_CONVEX = 2
  };

  template<typename GeomTraits>
  Polygon_type
  polygon_type_2(
    const std::vector<typename GeomTraits::Point_2>& polygon,
    const GeomTraits traits) {

    using Point_2 = typename GeomTraits::Point_2;
    const auto collinear_2 = traits.collinear_2_object();
    CGAL_precondition(polygon.size() >= 3);

    // First, test the polygon on convexity.
    if (CGAL::is_convex_2(polygon.begin(), polygon.end(), traits)) {

      // Test all the consequent triplets of the polygon's vertices on collinearity.
      // In case we find at least one, return WEAKLY_CONVEX polygon.
      const std::size_t n = polygon.size();
      for (std::size_t i = 0; i < n; ++i) {

        const std::size_t im = (i + n - 1) % n;
        const std::size_t ip = (i + 1) % n;

        if (collinear_2(polygon[im], polygon[i], polygon[ip]))
          return Polygon_type::WEAKLY_CONVEX;
      }
      // Otherwise, return STRICTLY_CONVEX polygon.
      return Polygon_type::STRICTLY_CONVEX;
    }
    // Otherwise, return CONCAVE polygon.
    return Polygon_type::CONCAVE;
  }

  template<
  typename OutputIterator,
  typename GeomTraits>
  std::pair<OutputIterator, bool> coordinates_on_last_edge_2(
    const std::vector<typename GeomTraits::Point_2>& polygon,
    const typename GeomTraits::Point_2& query,
    OutputIterator coordinates,
    const GeomTraits traits) {

    using FT = typename GeomTraits::FT;
    const std::size_t n = polygon.size();

    std::vector<FT> b;
    b.reserve(2);

    const auto& source = polygon[n - 1];
    const auto& target = polygon[0];

    linear_coordinates_2(
      source, target, query, std::back_inserter(b), traits);
    *(coordinates++) = b[1];
    for (std::size_t i = 1; i < n - 1; ++i)
      *(coordinates++) = FT(0);
    *(coordinates++) = b[0];

    return std::make_pair(coordinates, true);
  }

  template<
  typename OutputIterator,
  typename GeomTraits>
  std::pair<OutputIterator, bool> boundary_coordinates_2(
    const std::vector<typename GeomTraits::Point_2>& polygon,
    const typename GeomTraits::Point_2& query,
    const Query_point_location location,
    const std::size_t index,
    OutputIterator coordinates,
    const GeomTraits traits) {

    using FT = typename GeomTraits::FT;
    const std::size_t n = polygon.size();

    // Compute coordinates with respect to the query point location.
    switch (location) {

      case Query_point_location::ON_VERTEX: {
        CGAL_precondition(index >= 0 && index < n);

        for (std::size_t i = 0; i < n; ++i)
          if (i == index)
            *(coordinates++) = FT(1);
          else
            *(coordinates++) = FT(0);
        return std::make_pair(coordinates, true);
      }

      case Query_point_location::ON_EDGE: {
        CGAL_precondition(index >= 0 && index < n);

        if (index == n - 1)
          return coordinates_on_last_edge_2(
            polygon, query, coordinates, traits);

        const std::size_t indexp = (index + 1) % n;

        const auto& source = polygon[index];
        const auto& target = polygon[indexp];

        for (std::size_t i = 0; i < n; ++i)
          if (i == index) {
            linear_coordinates_2(source, target, query, coordinates, traits); ++i;
          } else {
            *(coordinates++) = FT(0);
          }
        return std::make_pair(coordinates, true);
      }

      default: {
        internal::get_default(n, coordinates);
        return std::make_pair(coordinates, false);
      }
    }
    return std::make_pair(coordinates, false);
  }

  enum class Edge_case {

    UNBOUNDED = 0, // point is on the unbounded side of the polygon
    BOUNDARY  = 1, // point is on the boundary of the polygon
    INTERIOR  = 2  // point is in the interior of the polygon
  };

  template<typename GeomTraits>
  typename GeomTraits::FT
  cotangent_2(
    const typename GeomTraits::Vector_2& v1,
    const typename GeomTraits::Vector_2& v2,
    const GeomTraits traits) {

    using FT = typename GeomTraits::FT;
    const auto scalar_product_2 = traits.compute_scalar_product_2_object();
    const auto cross_product_2  = traits.compute_determinant_2_object();

    const FT dot = scalar_product_2(v1, v2);
    const FT det = cross_product_2(v1, v2);
    return dot / CGAL::abs(det);
  }

} // namespace internal
} // namespace Barycentric_coordinates
} // namespace CGAL

#endif // CGAL_BARYCENTRIC_UTILS_2_H
