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

#ifndef CGAL_BARYCENTRIC_WACHSPRESS_WEIGHTS_2_H
#define CGAL_BARYCENTRIC_WACHSPRESS_WEIGHTS_2_H

#include <CGAL/license/Barycentric_coordinates_2.h>

// Internal includes.
#include <CGAL/Barycentric_coordinates_2/internal/utils_2.h>
#include <CGAL/Barycentric_coordinates_2/barycentric_enum_2.h>

// [1] Reference: "M. S. Floater, K. Hormann, and G. Kos.
// A general construction of barycentric coordinates over convex polygons.
// Advances in Computational Mathematics, 24(1-4):311-331, 2006.".

namespace CGAL {
namespace Barycentric_coordinates {

  /*!
    \ingroup PkgBarycentricCoordinates2RefWeights

    \brief 2D Wachspress weights.

    This class implements 2D Wachspress weights ( \cite cgal:bc:fhk-gcbcocp-06,
    \cite cgal:bc:mlbd-gbcip-02, \cite cgal:bc:w-rfeb-75 ), which can be normalized
    to get the corresponding Wachspress coordinates.

    Wachspress coordinates are well-defined and non-negative in the closure
    of a strictly convex polygon.

    \tparam GeomTraits
    is a model of `BarycentricTraits_2`.

    \cgalModels `AnalyticWeights_2`
  */
  template<typename GeomTraits>
  class Wachspress_weights_2 {

  public:

    /// \name Types
    /// @{

    /// \cond SKIP_IN_MANUAL
    using GT = GeomTraits;
    using Area_2 = typename GeomTraits::Compute_area_2;
    /// \endcond

    /// Number type.
    typedef typename GeomTraits::FT FT;

    /// Point type.
    typedef typename GeomTraits::Point_2 Point_2;

    /// @}

    /// \name Initialization
    /// @{

    /*!
      \brief initializes all internal data structures.

      This class implements the behavior of Wachspress weights
      for 2D query points.

      \tparam Polygon
      is a model of `ConstRange`.

      \tparam VertexMap
      is a `ReadablePropertyMap` whose key type is `Polygon::value_type` and
      value type is `Point_2`. The default is `CGAL::Identity_property_map`.

      \param polygon
      An instance of `Polygon` with the vertices of a strictly convex polygon.

      \param policy
      One of the `CGAL::Barycentric_coordinates::Computation_policy`.
      The default is `CGAL::Barycentric_coordinates::Computation_policy::DEFAULT`.

      \param traits
      An instance of `GeomTraits`. The default initialization is provided.

      \param vertex_map
      An instance of `VertexMap` that maps a vertex from `polygon`
      to `Point_2`. The default is the identity property map.

      \pre `polygon.size() >= 3`
      \pre `polygon is simple`
      \pre `polygon is strictly convex`
    */
    template<
    typename Polygon,
    typename VertexMap = CGAL::Identity_property_map<typename GeomTraits::Point_2> >
    Wachspress_weights_2(
      const Polygon& polygon,
      const Computation_policy policy
      = Computation_policy::DEFAULT,
      const GeomTraits traits = GeomTraits(),
      const VertexMap vertex_map = VertexMap()) :
    m_computation_policy(policy),
    m_traits(traits),
    m_area_2(m_traits.compute_area_2_object()) {

      m_polygon.clear();
      m_polygon.reserve(polygon.size());
      for (const auto& item : polygon)
        m_polygon.push_back(get(vertex_map, item));
      CGAL_precondition(m_polygon.size() >= 3);

      A.resize(m_polygon.size());
      C.resize(m_polygon.size());
      w.resize(m_polygon.size());

      const internal::Polygon_type polygon_type =
        internal::polygon_type_2(m_polygon, m_traits);

      if (polygon_type != internal::Polygon_type::STRICTLY_CONVEX)
        m_is_strictly_convex_polygon = false;
      else
        m_is_strictly_convex_polygon = true;

      CGAL_precondition(
        CGAL::is_simple_2(m_polygon.begin(), m_polygon.end(), m_traits));
      CGAL_precondition(
        m_is_strictly_convex_polygon);
    }

    /// @}

    /// \name Access
    /// @{

    /*!
      \brief computes 2D Wachspress weights.

      This function fills `weights` with 2D Wachspress weights computed at the `query`
      point with respect to the vertices of the input polygon. If `query` belongs to
      the polygon boundary, the returned weights are normalized.

      \tparam OutputIterator
      is an output iterator whose value type is `FT`.

      \param query
      A query point.

      \param weights
      An output iterator that stores the computed weights.

      \return an output iterator.
    */
    template<typename OutputIterator>
    OutputIterator operator()(
      const Point_2& query,
      OutputIterator weights) {

      const bool normalize = false;
      return compute(normalize, query, weights);
    }

    /*!
      \brief computes 2D Wachspress coordinates.

      This function fills `coordinates` with 2D Wachspress coordinates computed at the `query`
      point with respect to the vertices of the input polygon.

      \tparam OutputIterator
      is an output iterator whose value type is `FT`.

      \param query
      A query point.

      \param coordinates
      An output iterator that stores the computed coordinates.

      \return an output iterator.
    */
    template<typename OutputIterator>
    OutputIterator coordinates(
      const Point_2& query,
      OutputIterator coordinates) {

      const bool normalize = true;
      return compute(normalize, query, coordinates);
    }

    /// @}

  private:

    // Fields.
    std::vector<FT> A;
    std::vector<FT> C;
    std::vector<FT> w;

    const Computation_policy m_computation_policy;
    const GeomTraits m_traits;

    const Area_2 m_area_2;

    std::vector<Point_2> m_polygon;
    bool m_is_strictly_convex_polygon;

    // Functions.
    template<typename OutputIterator>
    OutputIterator compute(
      const bool normalize,
      const Point_2& query,
      OutputIterator weights) {

      switch (m_computation_policy) {

        case Computation_policy::PRECISE_COMPUTATION: {
          return max_precision_weights(normalize, query, weights);
        }

        case Computation_policy::PRECISE_COMPUTATION_WITH_EDGE_CASES: {
          const auto edge_case = verify(query, weights);
          if (edge_case == internal::Edge_case::BOUNDARY)
            return weights;
          if (edge_case == internal::Edge_case::UNBOUNDED)
            std::cerr << std::endl <<
            "query does not belong to the polygon" << std::endl;
          return max_precision_weights(normalize, query, weights);
        }

        case Computation_policy::FAST_COMPUTATION: {
          return max_speed_weights(normalize, query, weights);
        }

        case Computation_policy::FAST_COMPUTATION_WITH_EDGE_CASES: {
          const auto edge_case = verify(query, weights);
          if (edge_case == internal::Edge_case::BOUNDARY)
            return weights;
          if (edge_case == internal::Edge_case::UNBOUNDED)
            std::cerr << std::endl <<
            "query does not belong to the polygon" << std::endl;
          return max_speed_weights(normalize, query, weights);
        }

        default: {
          internal::get_default(m_polygon.size(), weights);
          return weights;
        }
      }
      return weights;
    }

    template<typename OutputIterator>
    internal::Edge_case verify(
      const Point_2& query,
      OutputIterator weights) const {

      const auto result = internal::locate_wrt_polygon_2(
        m_polygon, query, m_traits);

      if (!result)
        return internal::Edge_case::UNBOUNDED;

      const internal::Query_point_location location = (*result).first;
      const std::size_t index = (*result).second;

      if (location == internal::Query_point_location::ON_UNBOUNDED_SIDE)
        return internal::Edge_case::UNBOUNDED;

      if (
        location == internal::Query_point_location::ON_VERTEX ||
        location == internal::Query_point_location::ON_EDGE ) {
        internal::boundary_coordinates_2(
          m_polygon, query, location, index, weights, m_traits);
        return internal::Edge_case::BOUNDARY;
      }

      return internal::Edge_case::INTERIOR;
    }

    template<typename OutputIterator>
    OutputIterator max_precision_weights(
      const bool normalize,
      const Point_2& query,
      OutputIterator weights) {

      // Get the number of vertices in the polygon.
      const std::size_t n = m_polygon.size();

      // Compute areas A following the area notation from [1].
      // Split the loop to make this computation faster.
      A[0] = m_area_2(m_polygon[0], m_polygon[1], query);
      for (std::size_t i = 1; i < n-1; ++i)
        A[i] = m_area_2(m_polygon[i], m_polygon[i+1], query);
      A[n-1] = m_area_2(m_polygon[n-1], m_polygon[0], query);

      // Initialize weights with areas C following the area notation from [1].
      // Then we multiply them by areas A as in the formula (5) from [1].
      // We also split the loop.
      w[0] = m_area_2(m_polygon[n-1], m_polygon[0], m_polygon[1]);
      for(std::size_t j = 1; j < n-1; ++j)
        w[0] *= A[j];

      for(std::size_t i = 1; i < n-1; ++i) {
        w[i] = m_area_2(m_polygon[i-1], m_polygon[i], m_polygon[i+1]);

        for (std::size_t j = 0; j < i-1; ++j)
          w[i] *= A[j];
        for (std::size_t j = i+1; j < n; ++j)
          w[i] *= A[j];
      }

      w[n-1] = m_area_2(m_polygon[n-2], m_polygon[n-1], m_polygon[0]);
      for (std::size_t j = 0; j < n-2; ++j)
        w[n-1] *= A[j];

      // Normalize if necessary.
      if (normalize)
        internal::normalize(w);

      // Return weights.
      for (std::size_t i = 0; i < n; ++i)
        *(weights++) = w[i];

      return weights;
    }

    template<typename OutputIterator>
    OutputIterator max_speed_weights(
      const bool normalize,
      const Point_2& query,
      OutputIterator weights) {

      // Get the number of vertices in the polygon.
      const std::size_t n = m_polygon.size();

      // Compute areas A and C following the area notation from [1].
      // Split the loop to make this computation faster.
      A[0] = m_area_2(m_polygon[0], m_polygon[1], query);
      C[0] = m_area_2(m_polygon[n-1], m_polygon[0], m_polygon[1]);

      for (std::size_t i = 1; i < n-1; ++i) {
        A[i] = m_area_2(m_polygon[i], m_polygon[i+1], query);
        C[i] = m_area_2(m_polygon[i-1], m_polygon[i], m_polygon[i+1]);
      }

      A[n-1] = m_area_2(m_polygon[n-1], m_polygon[0], query);
      C[n-1] = m_area_2(m_polygon[n-2], m_polygon[n-1], m_polygon[0]);

      // Compute unnormalized weights following the formula (28) from [1].
      CGAL_assertion(A[n-1] != FT(0) && A[0] != FT(0));
      w[0] = C[0] / (A[n-1] * A[0]);

      for (std::size_t i = 1; i < n-1; ++i) {
        CGAL_assertion(A[i-1] != FT(0) && A[i] != FT(0));
        w[i] = C[i] / (A[i-1] * A[i]);
      }

      CGAL_assertion(A[n-2] != FT(0) && A[n-1] != FT(0));
      w[n-1] = C[n-1] / (A[n-2] * A[n-1]);

      // Normalize if necessary.
      if (normalize)
        internal::normalize(w);

      // Return weights.
      for (std::size_t i = 0; i < n; ++i)
        *(weights++) = w[i];

      // Return weights.
      return weights;
    }
  };

} // namespace Barycentric_coordinates
} // namespace CGAL

#endif // CGAL_BARYCENTRIC_WACHSPRESS_WEIGHTS_2_H
