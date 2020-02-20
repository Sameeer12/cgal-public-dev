// Copyright (c) 2019 GeometryFactory Sarl (France).
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
// Author(s)     : Jean-Philippe Bauchet, Florent Lafarge, Gennadii Sytov, Dmitry Anisimov
//

#ifndef CGAL_SHAPE_REGULARIZATION_PARALLEL_GROUPS_2
#define CGAL_SHAPE_REGULARIZATION_PARALLEL_GROUPS_2

// #include <CGAL/license/Shape_regularization.h>

#include <map>
#include <vector>
#include <CGAL/Shape_regularization/internal/Segment_data_2.h>

namespace CGAL {
namespace Regularization {

  /*!
    \ingroup PkgShape_regularization_utilities

    \brief Groups segments that have a similar angle value into groups of parallel segments

    \tparam GeomTraits 
    must be a model of `Kernel`.

    \tparam InputRange 
    must be a model of `ConstRange` whose iterator type is `RandomAccessIterator`.

    \tparam SegmentMap 
    must be an `LvaluePropertyMap` whose key type is the value type of the input 
    range and value type is `Kernel::Segment_2`.

  */
  template<
    typename GeomTraits,
    typename InputRange,
    typename SegmentMap>
  struct Parallel_groups_2 {

  public:

    /// \name Types
    /// @{

    /// \cond SKIP_IN_MANUAL
    using Traits = GeomTraits;
    using Input_range = InputRange;
    using Segment_map = SegmentMap;
    /// \endcond

    /// Number type.
    typedef typename GeomTraits::FT FT;

    /// \cond SKIP_IN_MANUAL
    using Segment = typename GeomTraits::Segment_2;
    using Segment_data = typename internal::Segment_data_2<Traits>;
    /// \endcond

    /// @}

    /// \name Initialization
    /// @{
    /*!
      \brief initializes all internal data structures and sets up the tolerance value.

      \param input_range 
      an instance of `InputRange` with 2D segments.

      \param tolerance
      a tolerance value for angles. `tolerance`\f$^{-1}\f$ is the angle bound value.

      \param segment_map
      an instance of `SegmentMap` that maps an item from `input_range` 
      to `Kernel::Segment_2`

      \pre `input_range.size() > 0`
      \pre `tolerance > 0`

    */
    Parallel_groups_2 (
      const InputRange& input_range, 
      const FT tolerance = FT(1000000),
      const SegmentMap segment_map = SegmentMap()) :
    m_input_range(input_range),
    m_segment_map(segment_map),
    m_tolerance(CGAL::abs(tolerance)) {

      CGAL_precondition(m_input_range.size() > 0);
      CGAL_precondition(m_tolerance > FT(0));

      build_segment_data();
      make_parallel_groups();
    }
    /// @}

    // \name Access
    /// @{ 

    /*!
      \brief returns groups of indices of parallel segments.

      \param groups
      Must be a type of OutputIterator
    */
    template<typename OutputIterator>
    OutputIterator parallel_groups(OutputIterator groups) {
      CGAL_precondition(m_parallel_groups_angle_map.size() > 0);

      for(const auto & mi : m_parallel_groups_angle_map) {
        const std::vector <std::size_t> & group = mi.second;
        *(groups++) = group;
      }
      return groups;
    }
     /// @}

  private:
    const Input_range& m_input_range;
    const Segment_map  m_segment_map;
    std::vector<Segment_data> m_segments;
    const FT m_tolerance;
    std::map <FT, std::vector<std::size_t>> m_parallel_groups_angle_map;

    void build_segment_data() {
      for (std::size_t i = 0; i < m_input_range.size(); ++i) {
        const Segment& seg = get(m_segment_map, *(m_input_range.begin() + i));
        const Segment_data seg_data(seg, i);
        m_segments.push_back(seg_data);
      }
      CGAL_postcondition(m_segments.size() > 0);
    }

    void make_parallel_groups() {
      for (const auto & seg : m_segments) {
        const FT angle = static_cast<FT> (floor(CGAL::to_double(seg.m_orientation * m_tolerance))) / m_tolerance;
        const std::size_t seg_index = seg.m_index;
        m_parallel_groups_angle_map[angle].push_back(seg_index);
      }
    }

  };

} // namespace Regularization
} // namespace CGAL

#endif // CGAL_SHAPE_REGULARIZATION_PARALLEL_GROUPS_2