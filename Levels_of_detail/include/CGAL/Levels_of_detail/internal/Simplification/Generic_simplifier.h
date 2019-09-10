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

#ifndef CGAL_LEVELS_OF_DETAIL_INTERNAL_GENERIC_SIMPLIFIER_H
#define CGAL_LEVELS_OF_DETAIL_INTERNAL_GENERIC_SIMPLIFIER_H

// STL includes.
#include <memory>
#include <vector>
#include <utility>
#include <stdio.h>

// CGAL includes.
#include <CGAL/assertions.h>
#include <CGAL/barycenter.h>
#include <CGAL/property_map.h>

#include <CGAL/Random.h>
#include <CGAL/IO/Color.h>

#define CGAL_DO_NOT_USE_BOYKOV_KOLMOGOROV_MAXFLOW_SOFTWARE
#include <CGAL/internal/Surface_mesh_segmentation/Alpha_expansion_graph_cut.h>

// LOD includes.
#include <CGAL/Levels_of_detail/enum.h>

// Internal includes.
#include <CGAL/Levels_of_detail/internal/utils.h>
#include <CGAL/Levels_of_detail/internal/struct.h>

// Testing.
#include "../../../../../test/Levels_of_detail/include/Saver.h"

// Simplification.
#include <CGAL/Levels_of_detail/internal/Simplification/Alpha_shapes_filtering_2.h>

// Spatial search.
#include <CGAL/Levels_of_detail/internal/Spatial_search/K_neighbor_query.h>

// OpenCV.
#include "opencv2/opencv.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/core/utility.hpp"

#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

namespace CGAL {
namespace Levels_of_detail {
namespace internal {

  template<
  typename GeomTraits,
  typename PointMap3>
  class Generic_simplifier {

  public:
    using Traits = GeomTraits;
    using Point_map_3 = PointMap3;

    using FT = typename Traits::FT;
    using Line_2 = typename Traits::Line_2;
    using Point_2 = typename Traits::Point_2;
    using Point_3 = typename Traits::Point_3;
    using Vector_2 = typename Traits::Vector_2;
    using Vector_3 = typename Traits::Vector_3;
    using Plane_3 = typename Traits::Plane_3;
    using Segment_2 = typename Traits::Segment_2;

    using Points_2 = std::vector<Point_2>;
    using Points_3 = std::vector<Point_3>;
    using Indices = std::vector<std::size_t>;

    using Saver = Saver<Traits>;
    using Color = CGAL::Color;

    using Size_pair = std::pair<std::size_t, std::size_t>;

    using Identity_map_2 = 
    CGAL::Identity_property_map<Point_2>;

    using Alpha_shapes_filtering_2 = 
    internal::Alpha_shapes_filtering_2<Traits>;

    using Alpha_expansion = CGAL::internal::Alpha_expansion_graph_cut_boost;
    using Triangulation = internal::Triangulation<Traits>;
    using Location_type = typename Triangulation::Delaunay::Locate_type;

    struct Image_neighbors {

      Indices neighbors;
      void get_neighbors(Indices& neighbors_) const {
        neighbors_ = neighbors;
      }
    };

    using Neighbor_storage = std::vector<Image_neighbors>;

    struct Cluster_item {
      Cluster_item(
        const Point_3 _point, 
        const std::size_t _roof_idx) :
      input_point(_point),
      roof_idx(_roof_idx) { }
      
      Point_3 input_point;
      Point_3 final_point;
      std::size_t roof_idx;
    };

    using Cell_id = std::pair<long, long>;
    using Cell_data = std::vector<std::size_t>;
    using Grid = std::map<Cell_id, Cell_data>;

    struct Image_cell {
      
      Image_cell() : 
      roof_idx(std::size_t(-1)),
      zr(FT(255)), zg(FT(255)), zb(FT(255)),
      is_interior(false) { }

      std::size_t roof_idx;
      FT zr, zg, zb;
      bool is_interior;
    };

    struct Image {

      Image() : rows(0), cols(0) { }
      Image(const std::size_t _rows, const std::size_t _cols) : 
      rows(_rows), cols(_cols) { 
        resize(_rows, _cols);
      }

      void clear() {
        rows = 0;
        cols = 0;
        grid.clear();
      }

      void resize(const std::size_t _rows, const std::size_t _cols) {
        rows = _rows; cols = _cols;
        grid.resize(_rows);
        for (auto& pixels : grid)
          pixels.resize(_cols);
      }

      void create_pixel(
        const std::size_t i, const std::size_t j,
        const std::size_t roof_idx,
        const bool is_interior,
        const FT zr, const FT zg, const FT zb) {

        auto& pixel = grid[i][j];

        pixel.roof_idx = roof_idx;
        pixel.zr = zr;
        pixel.zg = zg;
        pixel.zb = zb;
        pixel.is_interior = is_interior;
      }

      std::size_t rows, cols;
      std::vector< std::vector<Image_cell> > grid;
    };

    struct Pixel {

      Pixel(
        const Point_2& p,
        const std::size_t _i, const std::size_t _j,
        const bool _is_interior) :
      point(Point_3(p.x(), p.y(), FT(0))),
      i(_i), j(_j),
      is_interior(_is_interior) 
      { }

      Point_3 point;
      std::size_t i;
      std::size_t j;
      bool is_interior;
    };

    struct Height_item {
      FT z;
      std::size_t label;
    };

    using Pair = std::pair<Point_2, Height_item>;
    using Pair_map = CGAL::First_of_pair_property_map<Pair>;
    using K_neighbor_query =
    internal::K_neighbor_query<Traits, std::vector<Pair>, Pair_map>;

    using OpenCVImage = cv::Mat;

    Generic_simplifier(
      const Indices& input_range,
      const Point_map_3 point_map_3,
      const FT grid_cell_width_2,
      const FT alpha_shape_size_2,
      const FT graph_cut_beta_2,
      const FT max_height_difference,
      const FT image_noise,
      const FT min_length) :
    m_input_range(input_range),
    m_point_map_3(point_map_3),
    m_grid_cell_width_2(grid_cell_width_2),
    m_alpha_shape_size_2(alpha_shape_size_2),
    m_beta(graph_cut_beta_2),
    m_max_height_difference(max_height_difference),
    m_image_noise(image_noise),
    m_min_length(min_length),
    m_val_min(+internal::max_value<FT>()),
    m_val_max(-internal::max_value<FT>()),
    m_num_labels(0),
    m_rows_min(+internal::max_value<long>()),
    m_rows_max(-internal::max_value<long>()),
    m_cols_min(+internal::max_value<long>()),
    m_cols_max(-internal::max_value<long>()),
    m_samples_per_face(20),
    m_k(FT(6))
    { }

    void add_exterior_points(const Indices& range) {
      m_num_labels += 1;
      m_height_map[1] = FT(0);
      for (const std::size_t idx : range) {
        const Point_3& p = get(m_point_map_3, idx);
        Point_2 q = Point_2(p.x(), p.y());
        m_cluster.push_back(Cluster_item(Point_3(q.x(), q.y(), FT(0)), 1));
      }
      save_cluster("/Users/monet/Documents/lod/logs/buildings/tmp/cluster-full");
    }

    void create_cluster() {

      m_cluster.clear();
      m_cluster.reserve(m_input_range.size());
      m_height_map.clear();

      for (const std::size_t idx : m_input_range) {
        const Point_3& point = get(m_point_map_3, idx);
        m_cluster.push_back(Cluster_item(point, 0));
        m_val_min = CGAL::min(point.z(), m_val_min);
        m_val_max = CGAL::max(point.z(), m_val_max);
      }
      m_height_map[0] = m_val_max;
      m_num_labels = 1;
      save_cluster("/Users/monet/Documents/lod/logs/buildings/tmp/cluster-origin");
    }

    void create_cluster_from_regions(
      const std::vector<Indices>& regions,
      const Indices& unclassified) {

      std::vector<Points_3> roofs;
      create_sampled_roofs(regions, roofs);

      std::size_t num_points = 0;
      for (const auto& roof : roofs)
        num_points += roof.size();

      m_cluster.clear();
      m_cluster.reserve(num_points);

      for (std::size_t i = 0; i < roofs.size(); ++i) {

        FT val_min = +internal::max_value<FT>();
        FT val_max = -internal::max_value<FT>();
        for (const auto& point : roofs[i]) {

          m_cluster.push_back(Cluster_item(point, i));
          val_min = CGAL::min(point.z(), val_min);
          val_max = CGAL::max(point.z(), val_max);
        }
        m_height_map[i] = val_max;
        m_val_min = CGAL::min(val_min, m_val_min);
        m_val_max = CGAL::max(val_max, m_val_max);
      }
      m_num_labels = roofs.size();
      save_cluster("/Users/monet/Documents/lod/logs/buildings/tmp/cluster-origin");

      for (const std::size_t idx : unclassified) {
        const auto& point = get(m_point_map_3, *(m_input_range.begin() + idx));
        m_cluster.push_back(Cluster_item(point, std::size_t(-1)));
      }
    }

    void transform_cluster() {

      std::vector<Point_2> points;
      points.reserve(m_cluster.size());

      for (const auto& item : m_cluster)
        points.push_back(internal::point_2_from_point_3(item.input_point));

      Vector_2 dir;
      internal::estimate_direction_2(points, dir);
      const Vector_2 y_dir = Vector_2(FT(0), FT(1));

			internal::compute_angle_2(dir, y_dir, m_angle_2d);
      internal::compute_barycenter_2(points, m_b);
      
      for (Point_2& p : points)
        internal::rotate_point_2(m_angle_2d, m_b, p);

      CGAL::Identity_property_map<Point_2> pmap;
      std::vector<Point_2> bbox;
      internal::bounding_box_2(points, pmap, bbox);

      m_tr = bbox[0];
      for (Point_2& p : points)
        internal::translate_point_2(m_tr, p);

      for (std::size_t i = 0; i < points.size(); ++i) {
        const Point_2& p = points[i];
        m_cluster[i].final_point = 
          Point_3(p.x(), p.y(), m_cluster[i].input_point.z());
      }
    }

    void create_grid() {
      CGAL_assertion(m_cluster.size() >= 3);

      Cell_id cell_id;
      m_grid.clear();

      for (std::size_t i = 0; i < m_cluster.size(); ++i) {
        const auto& item = m_cluster[i];
        
        const Point_3& point = item.final_point;
        get_cell_id(point, cell_id);
        m_grid[cell_id].push_back(i);
      }
      save_grid("/Users/monet/Documents/lod/logs/buildings/tmp/grid");
    }

    void create_image(
      const Triangulation& tri,
      const bool use_triangulation) {

      const std::size_t rowsdiff = std::size_t(m_rows_max - m_rows_min);
      const std::size_t colsdiff = std::size_t(m_cols_max - m_cols_min);
      const std::size_t rows = (rowsdiff + 3); // +2 = +1 (diff pixel) +2 (boundary pixels)
      const std::size_t cols = (colsdiff + 3); // +2 = +1 (diff pixel) +2 (boundary pixels)

      std::cout << "Resolution (original): " << cols << "x" << rows << std::endl;
      std::cout << "Cols: " << colsdiff << " Rows: " << rowsdiff << std::endl;
      std::cout << "Val min: " << m_val_min << " Val max: " << m_val_max << std::endl;

      m_image.clear();
      m_image.resize(rows, cols);

      initialize_image(m_image);
      save_image("/Users/monet/Documents/lod/logs/buildings/tmp/image-origin.jpg", m_image);
      create_label_map(m_image);

      inpaint_image_opencv(m_image);

      if (!use_triangulation)
        update_interior_pixels_after_paint_default(m_image);
      else
        update_interior_pixels_after_paint_tri(tri, m_image);

      save_image("/Users/monet/Documents/lod/logs/buildings/tmp/image-paints.jpg", m_image);
      // save_point_cloud("/Users/monet/Documents/lod/logs/buildings/tmp/point-cloud-paints", m_image);
      
      apply_graphcut(m_image);
      // update_interior_pixels_after_graphcut(m_image);

      save_image("/Users/monet/Documents/lod/logs/buildings/tmp/image-gcuted.jpg", m_image);
      // save_point_cloud("/Users/monet/Documents/lod/logs/buildings/tmp/point-cloud-gcuted", m_image);
    }

    void create_contours() {

      const std::size_t pixels_per_cell = get_pixels_per_cell(m_image);

      OpenCVImage mask(
        m_image.rows * pixels_per_cell, 
        m_image.cols * pixels_per_cell, 
        CV_8UC1, cv::Scalar(255, 255, 255));

      for (std::size_t i = 0; i < m_image.rows; ++i)
        for (std::size_t j = 0; j < m_image.cols; ++j)
          if (!m_image.grid[i][j].is_interior)
            create_pixel(i, j, pixels_per_cell, 0, mask);

      save_opencv_image("/Users/monet/Documents/lod/logs/buildings/tmp/cv-mask.jpg", mask);

      std::vector< std::vector<cv::Point> > cnt_before, cnt_after;
      std::vector<cv::Vec4i> hierarchy;
      cv::findContours(
        mask, cnt_before, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

      cnt_after.resize(cnt_before.size());
      for (std::size_t k = 0; k < cnt_before.size(); k++)
        cv::approxPolyDP(
          OpenCVImage(
            cnt_before[k]), cnt_after[k], CGAL::to_double(m_image_noise), true);
      std::cout << "Num contours: " << cnt_after.size() << std::endl;

      OpenCVImage cnt(
        m_image.rows * pixels_per_cell, 
        m_image.cols * pixels_per_cell, 
        CV_8UC3, cv::Scalar(255, 255, 255));

      const cv::Scalar color = cv::Scalar(255, 0, 0);
      cv::drawContours(cnt, cnt_after, -1, color, 3);
      save_opencv_image("/Users/monet/Documents/lod/logs/buildings/tmp/cv-contours.jpg", cnt);

      std::vector<Segment_2> segments;
      
      /*
      std::vector< std::pair<std::vector<Point_2>, FT> > points;
      std::pair<std::vector<Point_2>, FT> dependent; */

      m_contours.clear();
      /* m_contour_points.clear(); */

      const Point_2 tr = Point_2(-m_tr.x(), -m_tr.y());
      for (std::size_t k = 0; k < cnt_after.size(); ++k) {
        const auto& contour = cnt_after[k];
        /* const auto& original = cnt_before[k]; */

        segments.clear();
        /* points.clear(); */

        /* FT max_error = -FT(1); */
        for (std::size_t i = 0; i < contour.size(); ++i) {
          const std::size_t ip = (i + 1) % contour.size();
          const auto& p1 = contour[i];
          const auto& p2 = contour[ip];
          
          const auto x1 = p1.x;
          const auto y1 = p1.y;
          const auto x2 = p2.x;
          const auto y2 = p2.y;

          const int si = int(y1) / pixels_per_cell;
          const int sj = int(x1) / pixels_per_cell;

          const int ti = int(y2) / pixels_per_cell;
          const int tj = int(x2) / pixels_per_cell;

          Point_2 s = get_point_from_id(si, sj);
          Point_2 t = get_point_from_id(ti, tj);

          internal::translate_point_2(tr, s);
          internal::translate_point_2(tr, t);

          internal::rotate_point_2(-m_angle_2d, m_b, s);
          internal::rotate_point_2(-m_angle_2d, m_b, t);

          segments.push_back(Segment_2(s, t));
          /* create_contour_points(p1, p2, original, dependent); */
          /* max_error = CGAL::max(max_error, dependent.second); */
          /* points.push_back(dependent); */
        }
        
        /*
        for (auto& pair : points)
          pair.second = max_error; */

        if (segments.size() >= 4) {
          m_contours.push_back(segments);
          /* m_contour_points.push_back(points); */
        }
      }

      m_approximate_boundaries_2.clear();
      for (auto& contour : m_contours) {
        for (auto& segment : contour) {
          /* if (rectify_segment(segment)) */
          m_approximate_boundaries_2.push_back(segment);
        }
      }
    }

    void create_contour_points(
      const cv::Point& start, const cv::Point& end,
      const std::vector<cv::Point>& original,
      std::pair< std::vector<Point_2>, FT>& result) {

      const Point_2 tr = Point_2(-m_tr.x(), -m_tr.y());
      const std::size_t pixels_per_cell = get_pixels_per_cell(m_image);

      const Point_2 a = Point_2(int(start.x), int(start.y));
      const Point_2 b = Point_2(int(end.x), int(end.y));
      const Line_2 line = Line_2(a, b);

      result.first.clear(); FT max_error = -FT(1);
      for (const cv::Point& cvp : original) {

        const Point_2 c = Point_2(int(cvp.x), int(cvp.y));
        const Point_2 proj = line.projection(c);
        const FT length = internal::distance(c, proj);
        
        if (std::floor(length) <= m_image_noise) {

          const int pi = int(c.y()) / pixels_per_cell;
          const int pj = int(c.x()) / pixels_per_cell;
          Point_2 p = get_point_from_id(pi, pj);
          internal::translate_point_2(tr, p);
          internal::rotate_point_2(-m_angle_2d, m_b, p);
          
          result.first.push_back(p);

          const int qi = int(proj.y()) / pixels_per_cell;
          const int qj = int(proj.x()) / pixels_per_cell;
          Point_2 q = get_point_from_id(qi, qj);
          internal::translate_point_2(tr, q);
          internal::rotate_point_2(-m_angle_2d, m_b, q);

          const FT error = internal::distance(p, q);
          max_error = CGAL::max(max_error, error);
        }
      }
      result.second = max_error;
    }

    void rectify_contour(
      std::vector<Segment_2>& contour) {
      for (auto& segment : contour)
        rectify_segment(segment);
    }

    bool rectify_segment(Segment_2& segment) {

      bool success = false;
      if (internal::distance(segment.source(), segment.target()) > m_min_length)
        success = true;

      const FT b1 = FT(1) / FT(10);
      const FT b2 = FT(9) / FT(10);

      const Point_2& s = segment.source();
      const Point_2& t = segment.target();

      const FT sx = b1 * s.x() + b2 * t.x();
      const FT sy = b1 * s.y() + b2 * t.y();

      const FT tx = b2 * s.x() + b1 * t.x();
      const FT ty = b2 * s.y() + b1 * t.y();

      segment = Segment_2(Point_2(sx, sy), Point_2(tx, ty));
      return success;
    }

    std::size_t get_pixels_per_cell(const Image& image) {
      const std::size_t num_rows = image.rows;
      const std::size_t num_cols = image.cols;

      const std::size_t resolution = 1000;
      const std::size_t rows_coef = std::ceil(resolution / num_rows);
      const std::size_t cols_coef = std::ceil(resolution / num_cols);
      const std::size_t pixels_per_cell = CGAL::max(rows_coef, cols_coef);

      return pixels_per_cell;
    }

    void create_pixel(
      const std::size_t i, const std::size_t j, 
      const std::size_t pixels_per_cell,
      const unsigned char color,
      OpenCVImage& image) {

      const std::size_t il = i * pixels_per_cell;
      const std::size_t jl = j * pixels_per_cell;
      for (std::size_t ii = il; ii < il + pixels_per_cell; ++ii)
        for (std::size_t jj = jl; jj < jl + pixels_per_cell; ++jj)
          image.at<unsigned char>(ii, jj) = color;
    }

    void create_pixel(
      const std::size_t i, const std::size_t j, 
      const std::size_t pixels_per_cell,
      const uchar zr, const uchar zg, const uchar zb, 
      OpenCVImage& image) {

      const std::size_t il = i * pixels_per_cell;
      const std::size_t jl = j * pixels_per_cell;
      for (std::size_t ii = il; ii < il + pixels_per_cell; ++ii) {
        for (std::size_t jj = jl; jj < jl + pixels_per_cell; ++jj) {
          cv::Vec3b& bgr = image.at<cv::Vec3b>(ii, jj);
          bgr[0] = zb;
          bgr[1] = zg;
          bgr[2] = zr;
        }
      }
    }

    void get_approximate_boundaries_2(
      std::vector<Segment_2>& approximate_boundaries_2) {
      approximate_boundaries_2 = m_approximate_boundaries_2;
    }

    void get_contours(
      std::vector< std::vector<Segment_2> >& contours) {
      contours = m_contours;
    }

    void get_contour_points(
      std::vector< std::vector< std::pair<std::vector<Point_2>, FT> > >& contour_points) {
      contour_points = m_contour_points;
    }

    void get_outer_boundary_points_2(
      Points_2& boundary_points_2) {

      boundary_points_2.clear();
      m_boundary_map.clear();

      const Point_2 tr = Point_2(-m_tr.x(), -m_tr.y());
      std::vector<std::size_t> ni, nj; std::size_t pt_idx = 0;
      for (long i = 1; i < m_image.rows - 1; ++i) {
        for (long j = 1; j < m_image.cols - 1; ++j) {
          get_grid_neighbors_4(i, j, ni, nj);

          for (std::size_t k = 0; k < 4; ++k) {
            const long ii = ni[k];
            const long jj = nj[k];

            if (is_outer_boundary_pixel(i, j, ii, jj)) {

              Point_2 p = get_point_from_id(i, j);
              Point_2 q = get_point_from_id(ii, jj);
              
              internal::translate_point_2(tr, p);
              internal::translate_point_2(tr, q);

              internal::rotate_point_2(-m_angle_2d, m_b, p);
              internal::rotate_point_2(-m_angle_2d, m_b, q);

              const Point_2 res = internal::middle_point_2(p, q);
              boundary_points_2.push_back(res);
              m_boundary_map[res] = pt_idx; ++pt_idx;
            }
          }
        }
      }
    }

    void create_neighbor_storage(
      const bool outer) {

      m_neighbor_storage.clear();
      Image_neighbors imn;
      std::vector<std::size_t> ni, nj;

      for (long i = 1; i < m_image.rows - 1; ++i) {
        for (long j = 1; j < m_image.cols - 1; ++j) {
          get_grid_neighbors_4(i, j, ni, nj);

          for (std::size_t k = 0; k < 4; ++k) {
            const long ii = ni[k];
            const long jj = nj[k];

            if (outer) {
              if (is_outer_boundary_pixel(i, j, ii, jj)) {
                create_imn(i, j, ii, jj, imn);
                m_neighbor_storage.push_back(imn);
              }
            } else {
              if (is_inner_boundary_pixel(i, j, ii, jj)) {
                create_imn(i, j, ii, jj, imn);
                m_neighbor_storage.push_back(imn);
              }
            }
          }
        }
      }
    }

    void create_imn(
      const std::size_t i1, const std::size_t j1,
      const std::size_t i2, const std::size_t j2,
      Image_neighbors& imn) {
      imn.neighbors.clear();
      
      add_imn_neighbor(i1, j1 + 1, i2, j2 + 1, imn);
      add_imn_neighbor(i1, j1 - 1, i2, j2 - 1, imn);
      add_imn_neighbor(i1 + 1, j1, i2 + 1, j2, imn);
      add_imn_neighbor(i1 - 1, j1, i2 - 1, j2, imn);
      add_imn_neighbor(i1 + 1, j1 + 1, i2 + 1, j2 + 1, imn);
      add_imn_neighbor(i1 - 1, j1 + 1, i2 - 1, j2 + 1, imn);
      add_imn_neighbor(i1 + 1, j1 - 1, i2 + 1, j2 - 1, imn);
      add_imn_neighbor(i1 - 1, j1 - 1, i2 - 1, j2 - 1, imn);

      add_imn_neighbor(i1, j1, i2 + 1, j2 - 1, imn);
      add_imn_neighbor(i1, j1, i2 - 1, j2 - 1, imn);
      add_imn_neighbor(i1, j1, i2 + 1, j2 + 1, imn);
      add_imn_neighbor(i1, j1, i2 - 1, j2 + 1, imn);

      add_imn_neighbor(i1 + 1, j1 + 1, i2, j2, imn);
      add_imn_neighbor(i1 + 1, j1 - 1, i2, j2, imn);
      add_imn_neighbor(i1 - 1, j1 + 1, i2, j2, imn);
      add_imn_neighbor(i1 - 1, j1 - 1, i2, j2, imn);  
    }

    void add_imn_neighbor(
      const std::size_t i1, const std::size_t j1,
      const std::size_t i2, const std::size_t j2,
      Image_neighbors& imn) {

      const Point_2 tr = Point_2(-m_tr.x(), -m_tr.y());

      Point_2 p = get_point_from_id(i1, j1);
      Point_2 q = get_point_from_id(i2, j2);
      
      internal::translate_point_2(tr, p);
      internal::translate_point_2(tr, q);

      internal::rotate_point_2(-m_angle_2d, m_b, p);
      internal::rotate_point_2(-m_angle_2d, m_b, q);

      const Point_2 res = internal::middle_point_2(p, q);
      if (m_boundary_map.find(res) != m_boundary_map.end())
        imn.neighbors.push_back(m_boundary_map.at(res));
    }

    const Neighbor_storage& get_neighbor_storage() const {
      return m_neighbor_storage;
    }

    void get_points_for_visibility_2(
      std::vector< std::pair<Point_2, bool> >& points) {
      
      std::vector<Pixel> point_cloud;
      create_point_cloud(m_image, point_cloud);
      add_extra_levels(2, point_cloud);
      
      points.clear();
      points.reserve(point_cloud.size());
      const Point_2 tr = Point_2(-m_tr.x(), -m_tr.y());

      for (const auto& pixel : point_cloud) {
        Point_2 p = Point_2(pixel.point.x(), pixel.point.y());

        internal::translate_point_2(tr, p);
        internal::rotate_point_2(-m_angle_2d, m_b, p);

        points.push_back(std::make_pair(p, pixel.is_interior));
      }

      save_regular_points(
        points, "/Users/monet/Documents/lod/logs/buildings/tmp/visibility_points_2");
    }

    void get_points_for_visibility_3(
      const Triangulation& tri,
      const Indices& cluster,
      const std::vector<Indices>& roof_regions,
      std::vector<Point_3>& points,
      std::vector<Indices>& updated_regions) {

      std::size_t num_points = 0;
      for (const auto& roof_region : roof_regions)
        num_points += roof_region.size();
      
      updated_regions.clear();
      updated_regions.resize(m_num_labels);

      Height_item item;
      std::vector<Pair> pairs;
      pairs.reserve(num_points);

      for (std::size_t i = 0; i < roof_regions.size(); ++i) {
        const auto& roof_region = roof_regions[i];

        for (const std::size_t idx : roof_region) {
          const auto& p = get(m_point_map_3, *(cluster.begin() + idx));
          item.z = p.z();
          item.label = i;
          pairs.push_back(
            std::make_pair(
              Point_2(p.x(), p.y()), item));
        }
      }

      Pair_map pmap;
      K_neighbor_query neighbor_query(pairs, m_k, pmap);

      std::vector<Pixel> point_cloud;
      create_point_cloud(m_image, point_cloud);

      points.clear();
      const Point_2 tr = Point_2(-m_tr.x(), -m_tr.y());

      std::size_t pt_idx = 0;
      for (const auto& pixel : point_cloud) {
        if (!pixel.is_interior) continue;

        Point_2 p = Point_2(pixel.point.x(), pixel.point.y());

        internal::translate_point_2(tr, p);
        internal::rotate_point_2(-m_angle_2d, m_b, p);

        Location_type type; int stub;
        const auto fh = tri.delaunay.locate(p, type, stub);
        if (
          type == Triangulation::Delaunay::FACE &&
          !tri.delaunay.is_infinite(fh) &&
          fh->info().tagged) {

          std::size_t region_idx = std::size_t(-1);
          const FT height = get_height(p, pairs, neighbor_query, region_idx);
          points.push_back(Point_3(p.x(), p.y(), height)); ++pt_idx;
          updated_regions[region_idx].push_back(pt_idx);
        }
      }

      m_saver.export_points(
        points, 
        Color(0, 0, 0), 
        "/Users/monet/Documents/lod/logs/buildings/tmp/visibility_points_3");
    }

    void get_inner_boundary_points_2(
      Points_2& boundary_points_2) {

      boundary_points_2.clear();
      m_boundary_map.clear();

      const Point_2 tr = Point_2(-m_tr.x(), -m_tr.y());
      std::vector<std::size_t> ni, nj; std::size_t pt_idx = 0;
      for (long i = 1; i < m_image.rows - 1; ++i) {
        for (long j = 1; j < m_image.cols - 1; ++j) {
          get_grid_neighbors_4(i, j, ni, nj);

          for (std::size_t k = 0; k < 4; ++k) {
            const long ii = ni[k];
            const long jj = nj[k];

            if (is_inner_boundary_pixel(i, j, ii, jj)) {

              Point_2 p = get_point_from_id(i, j);
              Point_2 q = get_point_from_id(ii, jj);
              
              internal::translate_point_2(tr, p);
              internal::translate_point_2(tr, q);

              internal::rotate_point_2(-m_angle_2d, m_b, p);
              internal::rotate_point_2(-m_angle_2d, m_b, q);

              const Point_2 res = internal::middle_point_2(p, q);
              boundary_points_2.push_back(res);
              m_boundary_map[res] = pt_idx; ++pt_idx;
            }
          }
        }
      }

      std::vector<Point_3> points;
      points.reserve(boundary_points_2.size());
      for (const auto& p : boundary_points_2)
        points.push_back(Point_3(p.x(), p.y(), FT(0)));
      m_saver.export_points(
        points, 
        Color(0, 0, 0), 
        "/Users/monet/Documents/lod/logs/buildings/tmp/inner_points");
    }

  private:
    const Indices& m_input_range;
    const Point_map_3 m_point_map_3;

    const FT m_grid_cell_width_2;
    const FT m_alpha_shape_size_2;
    const FT m_beta;
    const FT m_max_height_difference;
    const FT m_image_noise;
    const FT m_min_length;

    // Cluster.
    std::vector<Cluster_item> m_cluster;
    FT m_val_min, m_val_max;
    std::size_t m_num_labels;
    
    // Transform.
    Point_2 m_b, m_tr;
    FT m_angle_2d;
    
    // Grid.
    Grid m_grid;
    long m_rows_min, m_rows_max;
    long m_cols_min, m_cols_max;
    
    // Image.
    Image m_image;
    std::map<std::size_t, FT> m_height_map;
    std::map<std::size_t, Point_3> m_label_map;
    std::map<Point_3, std::size_t> m_inv_label_map;
    std::map<std::size_t, Plane_3> m_plane_map;

    const std::size_t m_samples_per_face;
    const FT m_k;

    Saver m_saver;
    Neighbor_storage m_neighbor_storage;
    std::map<Point_2, std::size_t> m_boundary_map;
    std::vector<Segment_2> m_approximate_boundaries_2;
    std::vector< std::vector<Segment_2> > m_contours;
    std::vector< std::vector< std::pair<std::vector<Point_2>, FT> > > m_contour_points;

    void add_extra_levels(
      const int levels,
      std::vector<Pixel>& point_cloud) {
        
      for (int i = 0; i < m_image.rows; ++i) {
        for (int j = -levels; j < 0; ++j) {
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, false));
        }
        for (int j = m_image.cols; j < m_image.cols + levels; ++j) {
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, false));
        }
      }

      for (int j = 0; j < m_image.cols; ++j) {
        for (int i = -levels; i < 0; ++i) {
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, false));
        }
        for (int i = m_image.rows; i < m_image.rows + levels; ++i) {
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, false));
        }
      }

      for (int i = -levels; i < 0; ++i) {
        for (int j = -levels; j < 0; ++j) {
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, false));
        }
      }

      for (int i = -levels; i < 0; ++i) {
        for (int j = m_image.cols; j < m_image.cols + levels; ++j) {
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, false));
        }
      }

      for (int i = m_image.rows; i < m_image.rows + levels; ++i) {
        for (int j = -levels; j < 0; ++j) {
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, false));
        }
      }

      for (int i = m_image.rows; i < m_image.rows + levels; ++i) {
        for (int j = m_image.cols; j < m_image.cols + levels; ++j) {
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, false));
        }
      }
    }

    void create_sampled_roofs(
      const std::vector<Indices>& regions,
      std::vector<Points_3>& roofs) {

      roofs.clear();
      roofs.reserve(regions.size());

      Points_3 roof; Plane_3 plane;
      for (std::size_t i = 0; i < regions.size(); ++i) {
        const auto& region = regions[i];
        roof.clear();
  
        internal::plane_from_points_3(
        m_input_range, m_point_map_3, region, plane);
        internal::project_on_plane_3(
        m_input_range, m_point_map_3, region, plane, roof);
        sample_roof_region(plane, roof);
        roofs.push_back(roof);
        m_plane_map[i] = plane;
      }
    }

    void sample_roof_region(
      const Plane_3& plane, Points_3& roof) {

      Point_3 b;
      internal::compute_barycenter_3(roof, b);

      Points_2 points;
      points.reserve(roof.size());
      for (const auto& p : roof) {
        const Point_2 q = internal::to_2d(p, b, plane);
        points.push_back(q);
      }
      apply_filtering(points);

      roof.clear();
      for (const auto& p : points) {
        const Point_3 q = internal::to_3d(p, b, plane);
        roof.push_back(q);
      }
    }

    void apply_filtering(std::vector<Point_2>& points) {

      const std::size_t nump = points.size();
      Alpha_shapes_filtering_2 filtering(m_alpha_shape_size_2);
      const FT sampling_2 = m_alpha_shape_size_2 / FT(2);

      Identity_map_2 identity_map_2;
      filtering.add_points(points, identity_map_2);
      points.clear(); 
      filtering.get_samples(sampling_2, m_samples_per_face, points);
    }

    FT get_height(
      const Point_2& p,
      const std::vector<Pair>& pairs,
      K_neighbor_query& neighbor_query,
      std::size_t& region_idx) {
      
      Indices neighbors;
      neighbor_query(p, neighbors);

      std::vector<FT> sums(m_num_labels, FT(0));
      for (const std::size_t idx : neighbors) {
        const auto& item = pairs[idx].second;
        const std::size_t label = item.label;
        sums[label] += FT(1);
      }
      
      std::size_t final_label = std::size_t(-1); 
      FT max_sum = -FT(1);
      for (std::size_t i = 0; i < sums.size(); ++i) {
        if (sums[i] > max_sum) {
          final_label = i;
          max_sum = sums[i];
        }
      }

      const Plane_3& plane = m_plane_map.at(final_label);
      const Point_3 pos = internal::position_on_plane_3(p, plane);
      region_idx = final_label;

      return pos.z();
    }

    void get_cell_id(
      const Point_3& point, 
      Cell_id& cell_id) {

      const long id_x = get_id_value(point.x());
      const long id_y = get_id_value(point.y());
      
      cell_id = std::make_pair(id_x, id_y);

      m_cols_min = CGAL::min(id_x, m_cols_min);
      m_rows_min = CGAL::min(id_y, m_rows_min);

      m_cols_max = CGAL::max(id_x, m_cols_max);
      m_rows_max = CGAL::max(id_y, m_rows_max);
    }

    long get_id_value(const FT value) {

      CGAL_precondition(m_grid_cell_width_2 > FT(0));
      const long id = static_cast<long>(
        CGAL::to_double(value / m_grid_cell_width_2));
      if (value >= FT(0)) return id;
      return id - 1;
    }

    void initialize_image(
      Image& image) {

      std::size_t numcells = 0;
      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {

          const long id_x = get_id_x(j-1);
          const long id_y = get_id_y(i-1);
          
          const Cell_id cell_id = std::make_pair(id_x, id_y);
          if (m_grid.find(cell_id) != m_grid.end()) {
            ++numcells;

            const auto& indices = m_grid.at(cell_id);
            initialize_pixel(i, j, indices, image);
          }
        }
      }
      std::cout << "Num cells: " << m_grid.size() << " : " << numcells << std::endl;
    }

    long get_id_x(const int j) {
      return m_cols_min + long(j);
    }

    long get_id_y(const int i) {
      return m_rows_max - long(i);
    }

    void initialize_pixel(
      const std::size_t i, const std::size_t j,
      const Cell_data& indices,
      Image& image) {

      std::size_t roof_idx = std::size_t(-1);
      FT zr = FT(255), zg = FT(255), zb = FT(255);
      const bool success = get_pixel_data(indices, roof_idx, zr, zg, zb);
      if (success) 
        image.create_pixel(i, j, roof_idx, true, zr, zg, zb);
    }

    bool get_pixel_data(
      const Cell_data& indices,
      std::size_t& roof_idx,
      FT& zr, FT& zg, FT& zb) {

      std::vector<int> tmp(m_num_labels, 0);
      for (const std::size_t idx : indices)
        if (m_cluster[idx].roof_idx != std::size_t(-1))
          tmp[m_cluster[idx].roof_idx] += 1;

      std::size_t final_idx = std::size_t(-1); int max_num = -1;
      for (std::size_t i = 0; i < tmp.size(); ++i) {
        if (tmp[i] > max_num) {
          final_idx = i;
          max_num = tmp[i];
        }
      }

      roof_idx = final_idx;
      Random rand(roof_idx);
      zr = static_cast<FT>(64 + rand.get_int(0, 192));
      zg = static_cast<FT>(64 + rand.get_int(0, 192));
      zb = static_cast<FT>(64 + rand.get_int(0, 192));

      std::size_t num_vals = 0;
      for (const int val : tmp)
        if (val == 0) num_vals += 1;
      if (num_vals == tmp.size())
        return false;
      return true;
    }

    void create_label_map(
      const Image& image) {

      m_label_map.clear();
      m_inv_label_map.clear();
      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {
          const auto& cell = image.grid[i][j];
          if (cell.roof_idx != std::size_t(-1)) {
            
            const Point_3 color = Point_3(cell.zr, cell.zg, cell.zb);
            m_label_map[cell.roof_idx] = color;
            m_inv_label_map[color] = cell.roof_idx;
          }
        }
      }
    }

    void inpaint_image_opencv(
      Image& image) {
      
      OpenCVImage input(
        image.rows, 
        image.cols, 
        CV_8UC3, cv::Scalar(255, 255, 255));

      OpenCVImage mask(
        image.rows, 
        image.cols, 
        CV_8U, cv::Scalar(0, 0, 0));

      for (std::size_t i = 0; i < image.rows; ++i) {
        for (std::size_t j = 0; j < image.cols; ++j) {
          const uchar zr = saturate_z(image.grid[i][j].zr);
          const uchar zg = saturate_z(image.grid[i][j].zg);
          const uchar zb = saturate_z(image.grid[i][j].zb);
          
          cv::Vec3b& bgr = input.at<cv::Vec3b>(i, j);
          bgr[0] = zb;
          bgr[1] = zg;
          bgr[2] = zr;

          if (!image.grid[i][j].is_interior) {
            unsigned char& val = mask.at<unsigned char>(i, j);
            val = static_cast<unsigned char>(255);
          }
        }
      }

      OpenCVImage inpainted;
      inpaint(input, mask, inpainted, 0, cv::INPAINT_TELEA);

      Image colored(image.rows, image.cols);
      for (std::size_t i = 1; i < colored.rows - 1; ++i) {
        for (std::size_t j = 1; j < colored.cols - 1; ++j) {
          const cv::Vec3b& bgr = inpainted.at<cv::Vec3b>(i, j);
          
          const std::size_t roof_idx = image.grid[i][j].roof_idx;
          const bool is_interior = image.grid[i][j].is_interior;

          const FT zr = FT(static_cast<std::size_t>(bgr[2]));
          const FT zg = FT(static_cast<std::size_t>(bgr[1]));
          const FT zb = FT(static_cast<std::size_t>(bgr[0]));

          colored.create_pixel(i, j, roof_idx, is_interior, zr, zg, zb);
        }
      }
      image = colored;
    }

    void update_interior_pixels_after_paint_default(Image& image) {

      std::vector<Pixel> point_cloud;
      create_point_cloud(image, point_cloud);

      Points_3 points;
      create_input_points(points);

      CGAL::Identity_property_map<Point_3> pmap;
      Alpha_shapes_filtering_2 filtering(m_alpha_shape_size_2);
      filtering.add_points(points, pmap);
      filtering.set_interior_labels(point_cloud);

      for (const auto& pixel : point_cloud) {
        if (pixel.is_interior) {
          image.grid[pixel.i][pixel.j].is_interior = true;
        } else {
          image.grid[pixel.i][pixel.j].is_interior = false;
          image.grid[pixel.i][pixel.j].zr = FT(255);
          image.grid[pixel.i][pixel.j].zg = FT(255);
          image.grid[pixel.i][pixel.j].zb = FT(255);
          image.grid[pixel.i][pixel.j].roof_idx = std::size_t(-1);
        }
      }
    }

    void update_interior_pixels_after_paint_tri(
      const Triangulation& tri,
      Image& image) {

      std::vector<Pixel> point_cloud;
      create_point_cloud(image, point_cloud);

      const Point_2 tr = Point_2(-m_tr.x(), -m_tr.y());
      for (const auto& pixel : point_cloud) {
        
        Location_type type; int stub;
        Point_2 p = Point_2(pixel.point.x(), pixel.point.y());
        internal::translate_point_2(tr, p);
        internal::rotate_point_2(-m_angle_2d, m_b, p);
        const auto fh = tri.delaunay.locate(p, type, stub);

        if (
          !tri.delaunay.is_infinite(fh) &&
          fh->info().tagged) {

          image.grid[pixel.i][pixel.j].is_interior = true;
        } else {
          image.grid[pixel.i][pixel.j].is_interior = false;
          image.grid[pixel.i][pixel.j].zr = FT(255);
          image.grid[pixel.i][pixel.j].zg = FT(255);
          image.grid[pixel.i][pixel.j].zb = FT(255);
          image.grid[pixel.i][pixel.j].roof_idx = std::size_t(-1);
        }
      }
    }

    void update_interior_pixels_after_graphcut(Image& image) {
      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {
          auto& cell = image.grid[i][j];
          const std::size_t label = get_label(cell.zr, cell.zg, cell.zb);
          if (label == m_num_labels - 1) {
            cell.is_interior = false;
            cell.zr = FT(255);
            cell.zg = FT(255);
            cell.zb = FT(255);
            cell.roof_idx = std::size_t(-1);
          } else {
            cell.is_interior = true;
          }
        }
      }
    }

    void create_input_points(Points_3& points) {
      
      points.clear();
      points.reserve(m_input_range.size());
      for (const std::size_t idx : m_input_range) {
        const Point_3& p = get(m_point_map_3, idx);

        Point_2 q = Point_2(p.x(), p.y());
        internal::rotate_point_2(m_angle_2d, m_b, q);
        internal::translate_point_2(m_tr, q);
        points.push_back(Point_3(q.x(), q.y(), FT(0)));
      }
    }

    void create_point_cloud(
      const Image& image,
      std::vector<Pixel>& point_cloud) {

      point_cloud.clear();
      for (std::size_t i = 0; i < image.rows; ++i) {
        for (std::size_t j = 0; j < image.cols; ++j) {
          const auto& cell = image.grid[i][j];
          
          const bool is_interior = cell.is_interior;
          const Point_2 p = get_point_from_id(i, j);
          point_cloud.push_back(Pixel(p, i, j, is_interior));
        }
      }
    }

    Point_2 get_point_from_id(
      const int i, const int j) {

      const long id_x = get_id_x(j);
      const long id_y = get_id_y(i);

      const FT x = get_coordinate(id_x);
      const FT y = get_coordinate(id_y);

      return Point_2(x, y);
    }

    const FT get_coordinate(long id) {

      CGAL_precondition(m_grid_cell_width_2 > FT(0));
      if (id < 0) id = id + 1;
      const FT half = m_grid_cell_width_2 / FT(2);
      const FT value = static_cast<FT>(id) * m_grid_cell_width_2 + half;
      return value;
    }

    void apply_graphcut(
      Image& image) {

      std::map<Size_pair, std::size_t> idx_map;
      set_idx_map(image, idx_map);

      std::vector<std::size_t> labels;
      set_initial_labels(image, idx_map, labels);
      apply_new_labels(idx_map, labels, image);

      save_image("/Users/monet/Documents/lod/logs/buildings/tmp/image-labels.jpg", image);

      std::vector<Size_pair> edges;
      std::vector<double> edge_weights;
      set_graphcut_edges(image, idx_map, edges, edge_weights);
      
      std::vector< std::vector<double> > cost_matrix;
      set_cost_matrix(image, idx_map, cost_matrix);

      compute_graphcut(edges, edge_weights, cost_matrix, labels);
      apply_new_labels(idx_map, labels, image);
    }

    void set_idx_map(
      const Image& image,
      std::map<Size_pair, std::size_t>& idx_map) {

      idx_map.clear();
      std::size_t pixel_idx = 0;
      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {

          idx_map[std::make_pair(i, j)] = pixel_idx;
          ++pixel_idx;
        }
      }
    }

    void set_initial_labels(
      const Image& image,
      const std::map<Size_pair, std::size_t>& idx_map,
      std::vector<std::size_t>& labels) {

      labels.clear();
      labels.resize(idx_map.size());
      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {

          const std::size_t label = get_label(
            image.grid[i][j].zr,
            image.grid[i][j].zg,
            image.grid[i][j].zb);
          const std::size_t pixel_idx = idx_map.at(std::make_pair(i, j));
          labels[pixel_idx] = label;
        }
      }
    }

    std::size_t get_label(
      const FT zr, const FT zg, const FT zb) {

      if (zr == FT(255) && zg == FT(255) && zb == FT(255))
        return m_num_labels;

      const Point_3 key = Point_3(zr, zg, zb);
      if (m_inv_label_map.find(key) != m_inv_label_map.end())
        return m_inv_label_map.at(key);

      FT d_min = FT(1000000000000); std::size_t label = std::size_t(-1);
      for (const auto& pair: m_label_map) {
        const FT zr_diff = zr - pair.second.x();
        const FT zg_diff = zg - pair.second.y();
        const FT zb_diff = zb - pair.second.z();

        const double r = CGAL::to_double(zr_diff * zr_diff);
        const double g = CGAL::to_double(zg_diff * zg_diff);
        const double b = CGAL::to_double(zb_diff * zb_diff);
        
        const FT d = static_cast<FT>(CGAL::sqrt(r + g + b));
        if (d < d_min) {
          d_min = d; label = pair.first;
        }
      }
      return label;
    }

    void apply_new_labels(
      const std::map<Size_pair, std::size_t>& idx_map,
      const std::vector<std::size_t>& labels,
      Image& image) {

      Image labeled(image.rows, image.cols);
      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {
          const std::size_t pixel_idx = idx_map.at(std::make_pair(i, j));
          
          Point_3 color; 
          bool is_interior = image.grid[i][j].is_interior;

          if (labels[pixel_idx] == m_num_labels) {
            color = Point_3(FT(255), FT(255), FT(255));
            is_interior = false;
          } else {
            color = m_label_map.at(labels[pixel_idx]);
            is_interior = true;
          }
          
          const std::size_t roof_idx = image.grid[i][j].roof_idx;
          labeled.create_pixel(i, j, roof_idx, is_interior, 
            color.x(), color.y(), color.z());
        }
      }
      image = labeled;
    }

    void set_graphcut_edges(
      const Image& image,
      const std::map<Size_pair, std::size_t>& idx_map,
      std::vector<Size_pair>& edges,
      std::vector<double>& edge_weights) {

      edges.clear();
      edge_weights.clear();
      std::vector<std::size_t> ni, nj;
      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {
          get_grid_neighbors_4(i, j, ni, nj);
          const std::size_t idxi = idx_map.at(std::make_pair(i, j));

          for (std::size_t k = 0; k < 4; ++k) { 
            const Size_pair pair = std::make_pair(ni[k], nj[k]);
            if (idx_map.find(pair) != idx_map.end()) {

              const std::size_t idxj = idx_map.at(pair);
              edges.push_back(std::make_pair(idxi, idxj));
              
              const double edge_weight = create_edge_weight(
                i, j, ni[k], nj[k], image);
              edge_weights.push_back(edge_weight);

            }
          }
        }
      }
    }

    void get_grid_neighbors_4(
      const std::size_t i, const std::size_t j,
      std::vector<std::size_t>& ni, 
      std::vector<std::size_t>& nj) {

      ni.clear(); nj.clear();
      ni.resize(4); nj.resize(4);

      CGAL_assertion(i > 0 && j > 0);

      ni[0] = i - 1; nj[0] = j;
      ni[1] = i;     nj[1] = j + 1;
      ni[2] = i + 1; nj[2] = j;
      ni[3] = i;     nj[3] = j - 1;
    }

    double create_edge_weight(
      const std::size_t i1, const std::size_t j1,
      const std::size_t i2, const std::size_t j2,
      const Image& image) {

      double edge_weight = 1.0;
      return CGAL::to_double(m_beta) * edge_weight;
    }

    void set_cost_matrix(
      const Image& image,
      const std::map<Size_pair, std::size_t>& idx_map,
      std::vector< std::vector<double> >& cost_matrix) {

      CGAL_assertion(idx_map.size() > 0);

      cost_matrix.clear();
      cost_matrix.resize(m_num_labels + 1);
      for (std::size_t i = 0; i < m_num_labels + 1; ++i)
        cost_matrix[i].resize(idx_map.size());

      std::vector<double> probabilities;
      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {
          const std::size_t pixel_idx = idx_map.at(std::make_pair(i, j));

          create_probabilities(i, j, image, probabilities);
          for (std::size_t k = 0; k < m_num_labels + 1; ++k)
            cost_matrix[k][pixel_idx] = 
              get_cost(image, i, j, probabilities[k]);
        }
      }
      // save_cost_matrix(image, idx_map, cost_matrix);
    }

    void create_probabilities(
      const std::size_t i, const std::size_t j,
      const Image& image,
      std::vector<double>& probabilities) {

      probabilities.clear();
      probabilities.resize(m_num_labels + 1, 0.0);
      std::vector<std::size_t> nums(m_num_labels + 1, 0);

      std::vector<std::size_t> ni, nj;
      get_grid_neighbors_8(i, j, ni, nj);

      for (std::size_t k = 0; k < 8; ++k) {
        const std::size_t ii = ni[k];
        const std::size_t jj = nj[k];

        const auto& cell = image.grid[ii][jj];
        const std::size_t label = get_label(cell.zr, cell.zg, cell.zb);
        probabilities[label] += FT(1);
        nums[label] += 1;
      }

      double sum = 0.0;
      for (std::size_t k = 0; k < m_num_labels + 1; ++k) {
        if (nums[k] == 0) continue;
        probabilities[k] /= static_cast<double>(nums[k]);
        sum += probabilities[k];
      }

      if (sum == 0.0)
        return;

      CGAL_assertion(sum > 0.0); double final_sum = 0.0;
      for (std::size_t k = 0; k < m_num_labels + 1; ++k) {
        probabilities[k] /= sum;
        final_sum += probabilities[k];
      }
      CGAL_assertion(CGAL::abs(1.0 - final_sum) < 0.00001);
    }

    void get_grid_neighbors_8(
      const std::size_t i, const std::size_t j,
      std::vector<std::size_t>& ni, 
      std::vector<std::size_t>& nj) {

      ni.clear(); nj.clear();
      ni.resize(8); nj.resize(8);

      CGAL_assertion(i > 0 && j > 0);

      ni[0] = i - 1; nj[0] = j - 1;
      ni[1] = i - 1; nj[1] = j;
      ni[2] = i - 1; nj[2] = j + 1;
      ni[3] = i;     nj[3] = j + 1;
      ni[4] = i + 1; nj[4] = j + 1;
      ni[5] = i + 1; nj[5] = j;
      ni[6] = i + 1; nj[6] = j - 1;
      ni[7] = i;     nj[7] = j - 1;
    }

    double get_cost(
      const Image& image,
      const std::size_t i, const std::size_t j,
      const double prob) {
      
      const double weight = get_weight(i, j, image);
      return (1.0 - prob) * weight;
    }

    double get_weight(
      const std::size_t i, const std::size_t j,
      const Image& image) {

      return 1.0;
    }

    void compute_graphcut(
      const std::vector<Size_pair>& edges,
      const std::vector<double>& edge_weights,
      const std::vector< std::vector<double> >& cost_matrix,
      std::vector<std::size_t>& labels) {

      std::cout << "Initial labels (size " << 
      labels.size() << ")" << std::endl;

      Alpha_expansion graphcut;
      graphcut(edges, edge_weights, cost_matrix, labels);

      std::cout << "Final labels (size " << 
      labels.size() << ")" << std::endl;
    }

    bool is_outer_boundary_pixel(
      const long i1, const long j1,
      const long i2, const long j2) {

      const auto& cell1 = m_image.grid[i1][j1];
      const auto& cell2 = m_image.grid[i2][j2];

      return ( cell1.is_interior && !cell2.is_interior );
    }

    bool is_inner_boundary_pixel(
      const long i1, const long j1,
      const long i2, const long j2) {

      const auto& cell1 = m_image.grid[i1][j1];
      const auto& cell2 = m_image.grid[i2][j2];

      if (!cell1.is_interior || !cell2.is_interior) return false;

      const std::size_t label1 = get_label(cell1.zr, cell1.zg, cell1.zb);
      const std::size_t label2 = get_label(cell2.zr, cell2.zg, cell2.zb);

      if(label1 == label2) return false;

      const FT h1 = get_pixel_height(label1);
      const FT h2 = get_pixel_height(label2);

      return ( CGAL::abs(h1 - h2) > m_max_height_difference );
    }

    FT get_pixel_height(const std::size_t label) {
      CGAL_assertion(m_height_map.find(label) != m_height_map.end());
      return m_height_map.at(label);
    }

    void save_cluster(const std::string name) {
      
      std::vector<Point_3> points;
      points.reserve(m_cluster.size());
      for (const auto& item: m_cluster)
        points.push_back(item.input_point);
      const Color color(0, 0, 0);
      m_saver.export_points(points, color, name);
    }

    void save_grid(const std::string name) {

      std::vector<Point_3> tmp;
      std::vector< std::vector<Point_3> > points;
      points.reserve(m_grid.size());

      for (const auto& pair : m_grid) {
        tmp.clear();
        for (const std::size_t idx : pair.second)
          tmp.push_back(m_cluster[idx].final_point);
        points.push_back(tmp);
      }
      m_saver.clear();
      m_saver.export_points(points, name);
    }

    void save_image(
      const std::string name,
      const Image& image) {
      
      const std::size_t pixels_per_cell = get_pixels_per_cell(image);
      OpenCVImage cvimage(
        image.rows * pixels_per_cell, 
        image.cols * pixels_per_cell, 
        CV_8UC3, cv::Scalar(255, 255, 255));

      for (std::size_t i = 0; i < image.rows; ++i) {
        for (std::size_t j = 0; j < image.cols; ++j) {
          
          const uchar zr = saturate_z(image.grid[i][j].zr);
          const uchar zg = saturate_z(image.grid[i][j].zg);
          const uchar zb = saturate_z(image.grid[i][j].zb);
          create_pixel(i, j, pixels_per_cell, zr, zg, zb, cvimage);
        }
      }
      save_opencv_image(name, cvimage);
    }

    uchar saturate_z(const FT val) {
      const float z = static_cast<float>(val);
      return cv::saturate_cast<uchar>(z);
    }

    void save_opencv_image(
      const std::string& name,
      const OpenCVImage& image) {
      imwrite(name, image);
    }

    void save_point_cloud(
      const std::string name,
      const Image& image) {

      std::vector<Pixel> point_cloud;
      create_point_cloud(image, point_cloud);

      std::vector<Point_3> points;
      points.reserve(point_cloud.size());
      for (const auto& pixel : point_cloud) {
        if (!pixel.is_interior) continue;
        points.push_back(pixel.point);
      }
        
      const Color color(0, 0, 0);
      m_saver.export_points(points, color, name);
    }

    void save_cost_matrix(
      const Image& image,
      const std::map<Size_pair, std::size_t>& idx_map,
      const std::vector< std::vector<double> >& cost_matrix) {

      std::vector<Image> images(m_num_labels + 1);
      for (std::size_t k = 0; k < m_num_labels + 1; ++k)
        images[k].resize(image.rows, image.cols);

      for (std::size_t i = 1; i < image.rows - 1; ++i) {
        for (std::size_t j = 1; j < image.cols - 1; ++j) {
          
          const std::size_t pixel_idx = idx_map.at(std::make_pair(i, j));
          for (std::size_t k = 0; k < m_num_labels + 1; ++k) {
            const double prob = get_probability(cost_matrix[k][pixel_idx]);

            // Default color.
            images[k].grid[i][j].zr = FT(125);
            images[k].grid[i][j].zg = FT(0);
            images[k].grid[i][j].zb = FT(0);

            const FT z = FT(prob) * FT(255);
            images[k].create_pixel(i, j, 0, true, z, z, z);
          }
        }
      }

      for (std::size_t k = 0; k < m_num_labels + 1; ++k) {
        const std::string name = "/Users/monet/Documents/lod/logs/buildings/tmp/" 
        + std::to_string(k) + "-image-probs.jpg";
        save_image(name, images[k]);
      }
    }

    double get_probability(const double cost) {
      return 1.0 - cost;
    }

    void save_regular_points(
      const std::vector< std::pair<Point_2, bool> >& input,
      const std::string name) {

      std::vector<Point_3> points;
      points.reserve(input.size());
      for (const auto& p : input)
        points.push_back(Point_3(p.first.x(), p.first.y(), FT(0)));
        
      const Color color(0, 0, 0);
      m_saver.export_points(points, color, name);
    }
  };

} // internal
} // Levels_of_detail
} // CGAL

#endif // CGAL_LEVELS_OF_DETAIL_INTERNAL_GENERIC_SIMPLIFIER_H
