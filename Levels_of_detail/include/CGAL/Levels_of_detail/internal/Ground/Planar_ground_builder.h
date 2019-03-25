// Copyright (c) 2019 INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is a part of CGAL (www.cgal.org).
// You can redistribute it and/or modify it under the terms of the GNU
// General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// SPDX-License-Identifier: GPL-3.0+
//
//
// Author(s)     : Dmitry Anisimov, Simon Giraudot, Pierre Alliez, Florent Lafarge, and Andreas Fabri
//

#ifndef CGAL_LEVELS_OF_DETAIL_INTERNAL_PLANAR_GROUND_BUILDER_H
#define CGAL_LEVELS_OF_DETAIL_INTERNAL_PLANAR_GROUND_BUILDER_H

#include <CGAL/license/Levels_of_detail.h>

// STL includes.
#include <vector>

// CGAL includes.
#include <CGAL/barycenter.h>

// LOD includes.
#include <CGAL/Levels_of_detail/enum.h>

// Internal includes.
#include <CGAL/Levels_of_detail/internal/utils.h>

namespace CGAL {
namespace Levels_of_detail {
namespace internal {

  template<typename GroundBase>
  class Planar_ground_builder {

  public:
    using Ground_base = GroundBase;

    using Traits = typename Ground_base::Traits;
    using Triangulation = typename Ground_base::Triangulation::Delaunay;
    using FT = typename Traits::FT;
    using Point_2 = typename Traits::Point_2;
    using Plane_3 = typename Traits::Plane_3;
    using Face_handle = typename Triangulation::Face_handle;
    using Vertex_handle = typename Triangulation::Vertex_handle;

    Planar_ground_builder(Ground_base& ground_base) :
    m_ground_base(ground_base)
    { }

    void initialize() {

      std::vector<Vertex_handle> vhs; vhs.reserve(4);
      Triangulation& tri = m_ground_base.triangulation.delaunay;
      const Plane_3& plane = m_ground_base.plane;
      const auto& bbox = m_ground_base.bbox;
      tri.clear();

      // Add bounding box vertices.
      for (const Point_2& p : bbox) {
        const Vertex_handle vh = tri.insert(p);
        vh->info().z = internal::position_on_plane_3(p, plane).z();
        vhs.push_back(vh);
      }

      // Add bounding box edges as constraints.
      for (std::size_t i = 0; i < vhs.size(); ++i) {
        const std::size_t ip = (i + 1) % vhs.size();
        if (vhs[i] != vhs[ip])
          tri.insert_constraint(vhs[i], vhs[ip]);
      }
    }

    template<typename Urban_object>
    void add_object_footprint(
      const Urban_object& object,
      const Reconstruction_type type) { 
      
      switch (type) {
        case Reconstruction_type::LOD0: {
          add_urban_object_boundaries(object.boundaries0());
          return; }
        case Reconstruction_type::LOD1: {
          add_urban_object_boundaries(object.boundaries1());
          return; }
        case Reconstruction_type::LOD2: {
          add_urban_object_boundaries(object.boundaries2());
          return; }
        default: {
          return;
        }
      }
    }

    template<typename Urban_object>
    void tag_faces(const Urban_object& object) {
      Triangulation& tri = m_ground_base.triangulation.delaunay;

      // Tag all faces that belong to this object. 
      for (auto fh = tri.finite_faces_begin();
      fh != tri.finite_faces_end(); ++fh) {
        if (is_valid(fh, object.base.triangulation.delaunay)) {

          fh->info().urban_tag = object.urban_tag;
          fh->info().object_index = object.index;
          fh->info().tagged = true;
        }
      }
    }

    void finilize() { 
      Triangulation& tri = m_ground_base.triangulation.delaunay;
      for (auto fh = tri.finite_faces_begin(); 
      fh != tri.finite_faces_end(); ++fh)
        set_ground_heights(fh);
    }

  protected:
    Ground_base& m_ground_base;

    template<typename FH>
    bool is_valid(
      const FH& query,
      const Triangulation& base) const {
      
      if (query->info().tagged)
        return false;

      const Point_2 b = CGAL::barycenter(
        query->vertex(0)->point(), FT(1),
        query->vertex(1)->point(), FT(1),
        query->vertex(2)->point(), FT(1));
      const Face_handle fh = base.locate(b);
      return !base.is_infinite(fh);
    }

    template<typename FH>
    void set_ground_heights(FH& fh) const {
      for (std::size_t k = 0; k < 3; ++k) {
        const Vertex_handle& vh = fh->vertex(k); 
        const std::size_t idx = fh->index(vh);
        CGAL_assertion(idx >= 0 && idx < 3);
        fh->info().z[idx] = vh->info().z;
      }
    }

    template<typename Boundary>
    void add_urban_object_boundaries(
      const std::vector<Boundary>& boundaries) {

      Triangulation& tri = m_ground_base.triangulation.delaunay;
      const Plane_3& plane = m_ground_base.plane;

      // Add object boundaries as constraints.
      for (const auto& boundary : boundaries) {
        const auto& segment = boundary.segment;

        const Point_2& s = segment.source();
        const Point_2& t = segment.target();

        const Vertex_handle svh = tri.insert(s);
        const Vertex_handle tvh = tri.insert(t);

        svh->info().z = internal::position_on_plane_3(s, plane).z();
        tvh->info().z = internal::position_on_plane_3(t, plane).z();
        if (svh != tvh)
          tri.insert_constraint(svh, tvh);
      }
    }
  };

} // internal
} // Levels_of_detail
} // CGAL

#endif // CGAL_LEVELS_OF_DETAIL_INTERNAL_PLANAR_GROUND_BUILDER_H
