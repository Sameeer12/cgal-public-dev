// Copyright (c) 2007  INRIA (France).
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
// Author(s)     : Laurent Saboret, Pierre Alliez


#ifndef CGAL_IMPLICIT_FCT_DELAUNAY_TRIANGULATION_H
#define CGAL_IMPLICIT_FCT_DELAUNAY_TRIANGULATION_H

#include <CGAL/license/Poisson_surface_reconstruction_3.h>

#include <CGAL/disable_warnings.h>

#include <CGAL/Point_with_normal_3.h>
#include <CGAL/Lightweight_vector_3.h>
#include <CGAL/property_map.h>
#include <CGAL/surface_reconstruction_points_assertions.h>

#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Triangulation_cell_base_with_info_3.h>

#include <CGAL/algorithm.h>
#include <CGAL/bounding_box.h>
#include <boost/random/random_number_generator.hpp>
#include <boost/random/linear_congruential.hpp>
#include <CGAL/property_map.h>

#include <vector>
#include <iterator>
#include <tuple>

#include"grad_fit.h"

namespace CGAL {

/// \internal
/// The Reconstruction_vertex_base_3 class is the default
/// vertex class of the Reconstruction_triangulation_3 class.
///
/// It provides the interface requested by the Poisson_reconstruction_function class:
/// - Each vertex stores a normal vector.
/// - A vertex is either an input point or a Steiner point added by Delaunay refinement.
/// - In order to solve a linear system over the triangulation, a vertex may be constrained
///   or not (i.e. may contribute to the right or left member of the linear system),
///   and has a unique index.
///
/// @param Gt   Geometric traits class / Point_3 is a typedef to Point_with_normal_3.
/// @param Cb   Vertex base class, model of TriangulationVertexBase_3.

template < typename Gt,
           typename Vb = Triangulation_vertex_base_3<Gt> >
class Reconstruction_vertex_base_3 : public Vb
{
// Public types
public:

  /// Geometric traits class / Point_3 is a typedef to Point_with_normal_3.
  typedef Gt Geom_traits;

  // Repeat Triangulation_vertex_base_3 public types
  /// \cond SKIP_IN_MANUAL
  typedef typename Vb::Cell_handle Cell_handle;
  template < typename TDS2 >
  struct Rebind_TDS
  {
    typedef typename Vb::template Rebind_TDS<TDS2>::Other   Vb2;
    typedef Reconstruction_vertex_base_3<Geom_traits, Vb2>  Other;
  };
  /// \endcond

  // Geometric types
  typedef typename Geom_traits::FT FT;
  typedef typename Geom_traits::Vector_3 Vector;           ///< typedef to Vector_3
  typedef typename Geom_traits::Point_3 Point;             ///< typedef to Point_with_normal_3
  typedef typename Geom_traits::Point_3 Point_with_normal; ///< typedef to Point_with_normal_3

// data members
private:

  // TODO: reduce memory footprint
  FT m_f; // value of the implicit function // float precise enough?
  bool m_constrained; // is vertex constrained? // combine constrained and type
  unsigned char m_type; // INPUT or STEINER
  unsigned int m_index; // index in matrix (to be stored outside)
  //Vector m_df;
  float m_sd;
	float m_ud;
	float m_sc;


// Public methods
public:

  Reconstruction_vertex_base_3()
    : Vb(), m_f(FT(0.0)), m_type(0), m_index(0)//, m_df(CGAL::NULL_VECTOR)
  {}

  Reconstruction_vertex_base_3(const Point_with_normal& p)
    : Vb(p), m_f(FT(0.0)), m_type(0), m_index(0)//, m_df(CGAL::NULL_VECTOR)
  {}

  Reconstruction_vertex_base_3(const Point_with_normal& p, Cell_handle c)
    : Vb(p,c), m_f(FT(0.0)), m_type(0), m_index(0)//, m_df(CGAL::NULL_VECTOR)
  {}

  Reconstruction_vertex_base_3(Cell_handle c)
    : Vb(c), m_f(FT(0.0)), m_type(0), m_index(0)//, m_df(CGAL::NULL_VECTOR)
  {}


  /// Gets/sets the value of the implicit function.
  /// Default value is 0.0.
  FT  f() const { return m_f; }
  FT& f()       { return m_f; }

  //Gets/sets the gradient at the vertex
  //Vector  df() const { return m_df; }
  //Vector& df()       { return m_df; }

  /// Gets/sets the type = INPUT or STEINER.
  unsigned char  type() const { return m_type; }
  unsigned char& type()       { return m_type; }

  /// Gets/sets the index in matrix.
  unsigned int  index() const { return m_index; }
  unsigned int& index()       { return m_index; }

  /// Gets/sets normal vector.
  /// Default value is null vector.
  const Vector& normal() const { return this->point().normal(); }
  Vector&       normal()       { return this->point().normal(); }

  const float sd() const { return m_sd; }
	float& sd() { return m_sd; }

	const float ud() const { return m_ud; }
	float& ud() { return m_ud; }

	const float sc() const { return m_sc; }
	float& sc() { return m_sc; }

// Private methods
private:

    /// Copy constructor and operator =() are not implemented.
    Reconstruction_vertex_base_3(const Reconstruction_vertex_base_3& toCopy);
    Reconstruction_vertex_base_3& operator =(const Reconstruction_vertex_base_3& toCopy);

}; // end of Reconstruction_vertex_base_3


template < typename Info, typename Gt,
typename Cb = CGAL::Triangulation_cell_base_with_info_3<Info,Gt> >
class Reconstruction_cell_base_3 : public Cb{
public:
	typedef typename Gt::FT FT;
	typedef typename Cb::Cell_handle Cell_handle;
	typedef typename Cb::Vertex_handle Vertex_handle;
	typedef typename Cb::Point Point;
	typedef typename Gt::Vector_3 Vector;
	typedef typename Gt::Triangle_3 Triangle;
	typedef typename Gt::Tetrahedron_3 Tetrahedron;
	typedef typename Gt::Plane_3 Plane;
	typedef typename Gt::Oriented_side_3 Oriented_side;


public:
	template < typename TDS2 >
	struct Rebind_TDS {
		typedef typename Cb::template Rebind_TDS<TDS2>::Other Cb2;
		typedef Reconstruction_cell_base_3<Info, Gt, Cb2> Other;
	};

	Reconstruction_cell_base_3()
		: Cb() {
	}

 	Reconstruction_cell_base_3(Vertex_handle v0, Vertex_handle v1, Vertex_handle v2, Vertex_handle v3)
	 : Cb(v0, v1, v2, v3){
	 }

 	Reconstruction_cell_base_3(Vertex_handle v0, Vertex_handle v1, Vertex_handle v2, Vertex_handle v3, Cell_handle n0, Cell_handle n1,
	 	 Cell_handle n2, Cell_handle n3): Cb(v0, v1, v2, v3, n0, n1, n2, n3){
	 }

  const Vector df() const
  {
   	 return compute_df(0);
 	}


  FT compute_volume() const{
    const Point& pa = this->vertex(0)->point();
    const Point& pb = this->vertex(1)->point();
    const Point& pc = this->vertex(2)->point();
    const Point& pd = this->vertex(3)->point();
    Tetrahedron tet(pa, pb, pc, pd);
    return CGAL::abs(tet.volume()); // abs of signed volume
  }

	Vector unnormalized_ingoing_normal(const int index) const
	{
		const Point& p1 = this->vertex((index+1)%4)->point();
		const Point& p2 = this->vertex((index+2)%4)->point();
		const Point& p3 = this->vertex((index+3)%4)->point();
		Vector cross = CGAL::cross_product(p2 - p1, p3 - p1);

		if(index%2 == 0)
			return -0.5 * cross; // magnitude of area of triangle is 0.5 the magnitude of cross product of two sides
		else
			return 0.5 * cross;
	}


	Vector compute_df(const int ref) const
	{
			FT fref = this->vertex(ref)->f();

			Vector df = CGAL::NULL_VECTOR;
			double volume = this->compute_volume();
			for(int i = 1; i <= 3; i++)
			{
        //face opposite each of i
				const int other = (ref + i) % 4;
				FT fother = this->vertex(other)->f();
				const Vector normal = this->unnormalized_ingoing_normal(other) / (3.0 * volume);
				df = df + (fother - fref) * normal;

      }
			return df;
	}
};

/// \internal
/// Helper class:
/// Reconstruction_triangulation_default_geom_traits_3
/// changes in a geometric traits class the Point_3 type to
/// Point_with_normal_3<BaseGt>.
///
/// @param BaseGt   Geometric traits class.
template <class BaseGt>
struct Reconstruction_triangulation_default_geom_traits_3 : public BaseGt
{
  typedef Point_with_normal_3<BaseGt> Point_3;
};


/// \internal
/// The Reconstruction_triangulation_3 class
/// provides the interface requested by the Poisson_reconstruction_function class:
/// - Each vertex stores a normal vector.
/// - A vertex is either an input point or a Steiner point added by Delaunay refinement.
/// - In order to solve a linear system over the triangulation, a vertex may be constrained
///   or not (i.e. may contribute to the right or left member of the linear system),
///   and has a unique index.
/// The vertex class must derive from Reconstruction_vertex_base_3.
///
/// @param BaseGt   Geometric traits class.
/// @param Gt       Geometric traits class / Point_3 is a typedef to Point_with_normal_3<BaseGt>.
/// @param Tds      Model of TriangulationDataStructure_3. The vertex class
///                 must derive from Reconstruction_vertex_base_3.

template <class BaseGt,
          class Gt = Reconstruction_triangulation_default_geom_traits_3<BaseGt>,
          class Tds_ = Triangulation_data_structure_3<Reconstruction_vertex_base_3<Gt>, Reconstruction_cell_base_3<int,Gt> > >
class Reconstruction_triangulation_3 : public Delaunay_triangulation_3<Gt,Tds_>
{
// Private types
private:

  // Base class
  typedef Delaunay_triangulation_3<Gt,Tds_>  Base;

  // Auxiliary class to build an iterator over input points.
  class Is_steiner_point
  {
  public:
      typedef typename Base::Finite_vertices_iterator Finite_vertices_iterator;

      bool operator()(const Finite_vertices_iterator& v) const
      {
        return (v->type() == Reconstruction_triangulation_3::STEINER);
      }
  };

// Public types
public:

  /// Geometric traits class / Point_3 is a typedef to Point_with_normal_3<BaseGt>.
  typedef Gt  Geom_traits;

  // Repeat base class' types
  /// \cond SKIP_IN_MANUAL
  typedef Tds_ Triangulation_data_structure;
  typedef typename Base::Segment      Segment;
  typedef typename Base::Triangle     Triangle;
  typedef typename Base::Tetrahedron  Tetrahedron;
  typedef typename Base::Line         Line;
  typedef typename Base::Ray          Ray;
  typedef typename Base::Object       Object;
  typedef typename Base::Cell_handle   Cell_handle;
  typedef typename Base::Vertex_handle Vertex_handle;
  typedef typename Base::Cell   Cell;
  typedef typename Base::Vertex Vertex;
  typedef typename Base::Facet  Facet;


  typedef typename Base::Edge   Edge;
  typedef typename Base::Cell_circulator  Cell_circulator;
  typedef typename Base::Facet_circulator Facet_circulator;
  typedef typename Base::Cell_iterator    Cell_iterator;
  typedef typename Base::Facet_iterator   Facet_iterator;
  typedef typename Base::Edge_iterator    Edge_iterator;
  typedef typename Base::Vertex_iterator  Vertex_iterator;
  typedef typename Base::Point_iterator Point_iterator;
  typedef typename Base::Finite_vertices_iterator Finite_vertices_iterator;
  typedef typename Base::Finite_cells_iterator    Finite_cells_iterator;
  typedef typename Base::Finite_facets_iterator   Finite_facets_iterator;
  typedef typename Base::Finite_edges_iterator    Finite_edges_iterator;
  typedef typename Base::All_cells_iterator       All_cells_iterator;
  typedef typename Base::All_vertices_iterator       All_vertices_iterator;
  typedef typename Base::Locate_type Locate_type;
  /// \endcond

  // Geometric types
  typedef typename Geom_traits::FT FT;
  typedef typename Geom_traits::Vector_3 Vector; ///< typedef to Vector_3<BaseGt>
  typedef typename Geom_traits::Point_3 Point;  ///< typedef to Point_with_normal_3<BaseGt>
  typedef typename Geom_traits::Point_3 Point_with_normal; ///< Point_with_normal_3<BaseGt>
  typedef typename Geom_traits::Sphere_3 Sphere;
  typedef typename Geom_traits::Iso_cuboid_3 Iso_cuboid;
  typedef typename std::pair<Triangle,Vector> Face;
  /// Point type
  enum Point_type {
    INPUT=0,    ///< Input point.
    STEINER=1   ///< Steiner point created by Delaunay refinement.
  };

  /// Iterator over input vertices.
  typedef Filter_iterator<Finite_vertices_iterator, Is_steiner_point>
                                                    Input_vertices_iterator;

  /// Iterator over input points.
  typedef Iterator_project<Input_vertices_iterator,
                           Project_point<Vertex> >  Input_point_iterator;

  mutable Sphere sphere;
  std::vector<Point_with_normal> points;
  std::size_t fraction;
  std::list<double> fractions;
  Vertex_handle constrained_vertex;
  //boost::associative_property_map< std::map<Vertex_handle, Vector> > m_grad_pmap;


public:

  /// Default constructor.
  Reconstruction_triangulation_3()
  {}

  ~Reconstruction_triangulation_3()
  {}

  // Default copy constructor and operator =() are fine.

  // Repeat base class' public methods used below
  /// \cond SKIP_IN_MANUAL
  using Base::points_begin;
  using Base::points_end;
  using Base::number_of_vertices;
  using Base::finite_vertices_begin;
  using Base::finite_vertices_end;
  using Base::all_vertices_begin;
  using Base::all_vertices_end;

  using Base::geom_traits;

  std::list<Face> m_contour;
  /// \endcond

  /// Gets first iterator over input vertices.
  Input_vertices_iterator input_vertices_begin() const
  {
      return Input_vertices_iterator(finite_vertices_end(), Is_steiner_point(),
                                     finite_vertices_begin());
  }
  /// Gets past-the-end iterator over input vertices.
  Input_vertices_iterator input_vertices_end() const
  {
      return Input_vertices_iterator(finite_vertices_end(), Is_steiner_point());
  }

  /// Gets iterator over the first input point.
  Input_point_iterator input_points_begin() const
  {
      return Input_point_iterator(input_vertices_begin());
  }
  /// Gets past-the-end iterator over the input points.
  Input_point_iterator input_points_end() const
  {
      return Input_point_iterator(input_vertices_end());
  }

  /// Gets the bounding sphere of input points.


  Sphere bounding_sphere() const
  {
    return sphere;
  }

  void initialize_bounding_sphere() const
  {
    Iso_cuboid ic = bounding_box(points.begin(), points.end());
    Point center = midpoint((ic.min)(), (ic.max)());
    sphere = Sphere(center, squared_distance(center, (ic.max)()));
  }

  /// Insert point in the triangulation.
  /// Default type is INPUT.
  template <typename Visitor>
  Vertex_handle insert(const Point_with_normal& p,
                       Point_type type,// = INPUT,
                       Cell_handle start,// = Cell_handle(),
                       Visitor visitor)
  {

    if(type == INPUT){
      visitor.before_insertion();
    }
    if(this->dimension() < 3){
      Vertex_handle v = Base::insert(p, start);
      v->type() = static_cast<unsigned char>(type);
      return v;
    }
    typename Base::Locate_type lt;
    int li, lj;
    Cell_handle ch = Base::locate(p, lt, li, lj, start);

    Vertex_handle v = Base::insert(p, lt, ch, li, lj);
    v->type() = static_cast<unsigned char>(type);
    return v;

  }

  /// Insert the [first, beyond) range of points in the triangulation using a spatial sort.
  /// Default type is INPUT.
  ///
  /// @commentheading Template Parameters:
  /// @param InputIterator iterator over input points.
  /// @param PointPMap is a model of `ReadablePropertyMap` with a value_type = Point_3.
  ///        It can be omitted if InputIterator value_type is convertible to Point_3.
  /// @param NormalPMap is a model of `ReadablePropertyMap` with a value_type = Vector_3.
  ///
  /// @return the number of inserted points.

  // This variant requires all parameters.
  template <typename InputIterator,
            typename PointPMap,
            typename NormalPMap,
            typename Visitor
  >
  int insert(
    InputIterator first,  ///< iterator over the first input point.
    InputIterator beyond, ///< past-the-end iterator over the input points.
    PointPMap point_pmap, ///< property map: `value_type of InputIterator` -> `Point_3` (the position of an input point).
    NormalPMap normal_pmap, ///< property map: `value_type of InputIterator` -> `Vector_3` (the *oriented* normal of an input point).
    Visitor visitor)
  {
    if(! points.empty()){
      std::cerr << "WARNING: not all points inserted yet" << std::endl;
    }
    // Convert input points to Point_with_normal_3
    //std::vector<Point_with_normal> points;
    for (InputIterator it = first; it != beyond; ++it)
    {
      Point_with_normal pwn(get(point_pmap,*it), get(normal_pmap,*it));
      points.push_back(pwn);
    }
    std::size_t n = points.size();

    initialize_bounding_sphere();

    typedef std::iterator_traits<InputIterator> Iterator_traits;
    typedef typename Iterator_traits::difference_type Diff_t;
    boost::rand48 random;
    boost::random_number_generator<boost::rand48, Diff_t> rng(random);
    CGAL::cpp98::random_shuffle (points.begin(), points.end(), rng);
    fraction = 0;

    fractions.clear();
    fractions.push_back(1.0);

    double m = static_cast<double>(n);

    while(m > 500){
      m /= 2;
      fractions.push_front(m/n);
    }

    insert_fraction(visitor);
    return 0;
  }

  template <typename Visitor>
  bool insert_fraction(Visitor visitor)
  {
    if(fractions.empty()){
      points.clear();
      return false;
    }
    double frac = fractions.front();
    fractions.pop_front();
    std::size_t more = (std::size_t)(points.size() * frac) - fraction;
    if((fraction+more) > points.size()){
      more = points.size() - fraction;
    }
    Cell_handle hint;
    spatial_sort (points.begin()+fraction, points.begin()+fraction+more, geom_traits());
    for (typename std::vector<Point_with_normal>::const_iterator p = points.begin()+fraction;
         p != points.begin()+fraction+more; ++p)
    {
      Vertex_handle v = insert(*p, INPUT, hint, visitor);
      hint = v->cell();
    }
    fraction += more;
    return true;
  }

  /// \cond SKIP_IN_MANUAL
  // This variant creates a default point property map = Identity_property_map.
  template <typename InputIterator,
            typename NormalPMap,
            typename Visitor
  >
  int insert(
    InputIterator first,  ///< iterator over the first input point.
    InputIterator beyond, ///< past-the-end iterator over the input points.
    NormalPMap normal_pmap, ///< property map: `value_type of InputIterator` -> `Vector_3` (the *oriented* normal of an input point).
    Visitor visitor)
  {
    return insert(
      first,beyond,
      make_identity_property_map(
      typename std::iterator_traits<InputIterator>::value_type()),
      normal_pmap,
      visitor);
  }
  /// \endcond

  /// Delaunay refinement callback:
  /// insert STEINER point in the triangulation.
  template <class CellIt>
  Vertex_handle
  insert_in_hole(const Point_with_normal& p, CellIt cell_begin, CellIt cell_end,
	         Cell_handle begin, int i,
                 Point_type type = STEINER)
  {
      Vertex_handle v = Base::insert_in_hole(p, cell_begin, cell_end, begin, i);
      v->type() = static_cast<unsigned char>(type);
      return v;
  }

  /// Index unconstrained vertices following the order of Finite_vertices_iterator.
  /// @return the number of unconstrained vertices.
  unsigned int index_unconstrained_vertices()
  {
    unsigned int index = 0;
    for (Finite_vertices_iterator v = finite_vertices_begin(),
         e = finite_vertices_end();
         v!= e;
         ++v)
    {
      if(! is_constrained(v))
        v->index() = index++;
    }
    return index;
  }

  /// Is vertex constrained, i.e.
  /// does it contribute to the right or left member of the linear system?

  bool is_constrained(Vertex_handle v) const
  {
    return v == constrained_vertex;
  }

  void constrain(Vertex_handle v)
  {
    constrained_vertex = v;
  }

  Vector compute_df(Vertex_handle v)
	{
		// get incident cells
		std::vector<Cell_handle> cells;
		this->incident_cells(v, std::back_inserter(cells));

		FT sum_volumes = 0.0;
		Vector sum_vec = CGAL::NULL_VECTOR;

		typename std::vector<Cell_handle>::iterator it;
		for(it = cells.begin(); it != cells.end(); it++)
		{
			Cell_handle c = *it;
			if(this->is_infinite(c))
				continue;

			// use cell (get gradient df and compute cell volume)
			const FT volume = c->compute_volume();
			sum_vec = sum_vec + volume * c->df();
			sum_volumes += volume;
		}

		assert(sum_volumes != 0);
    Vector vec;
		if (sum_volumes != 0.0)
			vec = sum_vec / sum_volumes;
		else
			vec = CGAL::NULL_VECTOR;

    return vec;
	}

  template <typename InputRange, typename GradientMap>
  void compute_grad_per_vertex(InputRange* vertex_gradients, GradientMap gradient_map)
  {
    for(auto it = this->finite_vertices_begin(); it != this->finite_vertices_end(); it++)
    {

      Vector df = compute_df(it);
      auto it1 = std::find_if(vertex_gradients->begin(), vertex_gradients->end(),
      [it](const std::pair<Vertex_handle, Vector>& element){ return (element.first)->point() == it->point();} );
      std::cout << get(gradient_map, *it1) << " ";
      put(gradient_map, *it1, df);
      std::cout << get(gradient_map, *it1) << std::endl;

    }

  }


  //grad using bounded sphere averaging:

  void compute_grad_bounding_sphere(std::map<Vertex_handle, Vector> &grad_of_vertex)
  {
    for(auto it = this->finite_vertices_begin(); it != this->finite_vertices_end(); it++)
    {
      FT sq_radius = 10000.0;
      std::vector<Edge> edges;
      this->incident_edges(it, std::back_inserter(edges));
      for(auto e = edges.begin(); e != edges.end(); e++)
      {
        Cell_handle c = e->first;
        int index1 = e->second;
        int index2 = e->third;
        Vector v(c->vertex(index1)->point(), c->vertex(index2)->point());
        if(v.squared_length() < sq_radius)
          sq_radius = v.squared_length();
      }

      //create the bounding sphere around the vertex
      Sphere bounding_sphere(it->point(), sq_radius);
      std::vector<Cell_handle> cells;
      this->incident_cells(it, std::back_inserter(cells));
      FT volumes = 0.0;
      Vector grad(0.0, 0.0, 0.0);
      Point center = it->point();
      for(auto c = cells.begin(); c != cells.end(); c++)
      {
        std::vector<Point> points;
        for(int i = 0; i < 4; i++)
        {
          Point p = (*c)->vertex(i)->point();
          if(! (p == center))
          {
            if(bounding_sphere.has_on_unbounded_side(p))
            {
              Vector v(center, p);
              Point temp = center + v * std::sqrt(sq_radius)/std::sqrt(v*v);
              points.push_back(temp);
            }
            else points.push_back(p);
          }
        }
        Tetrahedron tet(center, points[0], points[1], points[2]);
        FT volume = CGAL::abs(tet.volume());
        grad += volume * (*c)->df();
        volumes += volume;
      }
      grad /= volumes;

      //it->df() = grad;

      //CGAL::First_of_pair_property_map<std::pair<Vertex_handle,
      //    Vector> >::put(vertex_df, it, grad);

      grad_of_vertex.insert(std::make_pair(it, grad));
    }

    boost::associative_property_map< std::map<Vertex_handle, Vector> > grad_pmap(grad_of_vertex);
    this->set_grad_pmap(grad_pmap);

  }

//marching tetrahedra code:
  double value_level_set(Vertex_handle v)
	{
		return v->f();
	}

	bool level_set(Cell_handle c,
		const FT value,
		const int i1,
		const int i2,
		Point& p)
	{
		const Point& p1 = c->vertex(i1)->point();
		const Point& p2 = c->vertex(i2)->point();
		double v1 = value_level_set(c->vertex(i1));
		double v2 = value_level_set(c->vertex(i2));

		// std::cout << v1 << " " << value << " " << v2 << std::endl;

		if(v1 <= value && v2 >= value)
		{
			double ratio = (value - v1) / (v2 - v1);
			p = p1 + ratio * (p2-p1);
			return true;
		}
		else if(v2 <= value && v1 >= value)
		{
			double ratio = (value - v2) / (v1 - v2);
			p = p2 + ratio * (p1-p2);
			return true;
		}
		return false;
	}

  bool extract_level_set_points(Cell_handle cell,
    const double value,
    std::list<Point>& points)
  {
    Point point;
    if(level_set(cell,value,0,1,point)) points.push_back(point);
    if(level_set(cell,value,0,2,point)) points.push_back(point);
    if(level_set(cell,value,0,3,point)) points.push_back(point);
    if(level_set(cell,value,1,2,point)) points.push_back(point);
    if(level_set(cell,value,1,3,point)) points.push_back(point);
    if(level_set(cell,value,2,3,point)) points.push_back(point);
    return points.size() != 0;
  }


  unsigned int marching_tets(const FT value)
	{
		unsigned int nb_tri = 0;
		Finite_cells_iterator c;
		for(c = this->finite_cells_begin();
			c != this->finite_cells_end();
			c++)
			nb_tri += contour(c,value);
		return nb_tri;
	}

	unsigned int contour(Cell_handle cell,
		const FT value)
	{
		typename std::list<Point> points;
		if(!extract_level_set_points(cell,value,points))
			return 0;

		// only 3 or 4
		if(points.size() == 3)
		{
			typename std::list<Point>::iterator it = points.begin();
			const Point& a = (*it); it++;
			const Point& b = (*it); it++;
			const Point& c = (*it);

			Triangle triangle = Triangle(a,b,c);
			Vector n = CGAL::cross_product((b-a), (c-a));
			n = n / std::sqrt(n*n);

			m_contour.push_back(Face(triangle, n));
			return 1;
		}
		else if(points.size() == 4)
		{
			typename std::list<Point>::iterator it = points.begin();
			typename std::vector<Point> p(4);
			for(int i=0;i<4;i++)
			{
				p[i] = (*it);
				it++;
			}
			// compute normal
			Vector u = p[1] - p[0];
			Vector v = p[2] - p[0];
			Vector n = CGAL::cross_product(u,v);
			n = n / std::sqrt(n*n);

			Point cen = CGAL::centroid(p[0],p[1],p[3]);
			//if(cen.x() > 0.0)
		  //{
				m_contour.push_back(Face(Triangle(p[0],p[1],p[3]),n));
				m_contour.push_back(Face(Triangle(p[0],p[3],p[2]),n));
		//	}

			return 2;
		}
		return 0;
	}

  void marching_tets_to_off(std::string filename){
    std::ofstream outfile(filename);
    outfile << "OFF" << std::endl;
    outfile << 3 * m_contour.size() << " " << m_contour.size() << " 0" << std::endl;
    for(auto it = m_contour.begin(); it != m_contour.end(); it++){
      outfile << (it->first).vertex(0) << std::endl;
      outfile << (it->first).vertex(1) << std::endl;
      outfile << (it->first).vertex(2) << std::endl;
    }
    int i = 0;
    for(auto it = m_contour.begin(); it != m_contour.end(); it++, i+=3){
      outfile << "3 " << i << " " << i + 1 << " " << i + 2 << std::endl;
    }
    outfile.close();
  }

  template <typename InputRange, typename GradientMap>
  void output_grads_to_off(std::string filename, InputRange* input_range, GradientMap gradient_map) // output the gradients (only directions) to an off file for visualisation
	{	int i = 0;

    std::ofstream outfile("reduced_triangulation.off");

    outfile << "OFF" << std::endl
			           << 3*(this->number_of_facets()) << " "
			           << this->number_of_facets() << " 0" << std::endl;

    for(auto it = this->finite_facets_begin();
        it != this->finite_facets_end(); it++)
    {
      outfile << it->first->vertex((this->vertex_triple_index(it->second, 0)))->point() << std::endl;
      outfile << it->first->vertex((this->vertex_triple_index(it->second, 1)))->point() << std::endl;
      outfile << it->first->vertex((this->vertex_triple_index(it->second, 1)))->point() << std::endl;
    }
    i = 0;
    for(auto it = this->finite_facets_begin();
        it != this->finite_facets_end(); it++, i++)
    {
      outfile <<"3 " << 3*i << " " << 3*i + 1 << " " << 3*i + 2 << std::endl;
    }

		std::ofstream ofile(filename);

		ofile << "OFF" << std:: endl
			           << 7*(this->number_of_vertices()) << " "
			           << 2*(this->number_of_vertices()) << " 0" << std::endl;

		i = 0;

		for(auto it = this->finite_vertices_begin(); it != this->finite_vertices_end(); it++, i++)
		{
      //find the smallest circumsphere
      std::vector<Cell_handle> cells;
  		this->incident_cells(it, std::back_inserter(cells));
      FT scale = 10000.0;
      for(auto c = cells.begin(); c != cells.end(); c++)
      {
        Tetrahedron t((*c)->vertex(0)->point(), (*c)->vertex(1)->point(), (*c)->vertex(2)->point(), (*c)->vertex(3)->point());
        FT radius = std::sqrt(CGAL::squared_distance (CGAL::circumcenter(t), (*c)->vertex(0)->point()));
        if(scale > 2.0 * radius) scale = 2.0 * radius;
      }

			ofile << it->point()[0] - scale/20.0 << " " << it->point()[1] << " " << it->point()[2] << std::endl
				<< it->point()[0] + scale/20.0 << " " << it->point()[1] << " " << it->point()[2] << std::endl;
			Vector grad;// = get(gradient_map, it);//it->df(); CHECK
      grad = grad/std::sqrt(grad * grad);
			ofile << it->point()[0] + grad[0]*scale + scale/20.0 << " " << it->point()[1] + grad[1]*scale << " " << it->point()[2] + grad[2]*scale << std::endl
				    << it->point()[0] + grad[0]*scale - scale/20.0 << " " << it->point()[1] + grad[1]*scale << " " << it->point()[2] + grad[2]*scale<< std::endl;

			ofile << it->point()[0] + grad[0]*scale + scale/10.0 << " " << it->point()[1] + grad[1]*scale << " " << it->point()[2] + grad[2]*scale<< std::endl;
			ofile << it->point()[0] + grad[0]*scale - scale/10.0 << " " << it->point()[1] + grad[1]*scale << " " << it->point()[2] + grad[2]*scale<< std::endl;
			ofile << it->point()[0] + grad[0]*scale + grad[0]*scale/5.0 << " " << it->point()[1] + grad[1]*scale + grad[1]*scale/5.0 << " " << it->point()[2] + grad[2]*scale + grad[2]*scale/5.0<< std::endl;

    }

		i = 0;

		for(auto it = this->finite_vertices_begin(); it != this->finite_vertices_end(); it++, i++)
		{
			ofile << "4 " << 7*i << " " << 7*i + 1 << " " << 7*i + 2 << " " << 7*i + 3 << std::endl;
			ofile << "3 " << 7*i + 4 << " " << 7*i + 5 << " " << 7*i + 6 << std::endl;
		}

		ofile.close();
	}

/*
  void set_grad_pmap(boost::associative_property_map< std::map<Vertex_handle, Vector> > grad_pmap)
  {
    m_grad_pmap = grad_pmap;
  }

  boost::associative_property_map< std::map<Vertex_handle, Vector> > grad_pmap()
  {
    return m_grad_pmap;
  }
  */
}; // end of Reconstruction_triangulation_3

} //namespace CGAL

#include <CGAL/enable_warnings.h>

#endif // CGAL_IMPLICIT_FCT_DELAUNAY_TRIANGULATION_H
