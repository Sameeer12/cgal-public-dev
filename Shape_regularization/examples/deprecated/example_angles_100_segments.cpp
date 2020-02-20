// STL includes.
#include <vector>
#include <string>
#include <iostream>
#include <fstream>

// CGAL includes.
#include <CGAL/property_map.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Shape_regularization.h>
#include <CGAL/Timer.h>

// for random generator
#include <algorithm>
#include <CGAL/point_generators_2.h>
#include <CGAL/function_objects.h>
#include <CGAL/Join_input_iterator.h>
#include <CGAL/Counting_iterator.h>

#include "saver_segments_2.h"

// Typedefs.
typedef CGAL::Exact_predicates_inexact_constructions_kernel Traits;

using Segment_2 = typename Traits::Segment_2;
using Point_2 = typename Traits::Point_2;
using FT = typename Traits::FT;

using Input_range = std::vector<Segment_2>;
using Segment_map = CGAL::Identity_property_map<Segment_2>;

using Neighbor_query = CGAL::Regularization::Delaunay_neighbor_query_2<Traits, Input_range, Segment_map>;
using Regularization_type_angles = CGAL::Regularization::Angle_regularization_2<Traits, Input_range, Segment_map>;
using Shape_regularization_angles = CGAL::Regularization::Shape_regularization
  <Traits, Input_range, Neighbor_query, Regularization_type_angles>;

using Saver = CGAL::Regularization::Saver_segments_2<Traits>;
typedef CGAL::Points_on_segment_2<Point_2>              PG;
typedef CGAL::Creator_uniform_2< Point_2, Segment_2>    Creator;
typedef CGAL::Join_input_iterator_2< PG, PG, Creator>   Segm_iterator;
typedef CGAL::Counting_iterator<Segm_iterator, Segment_2>  Count_iterator;


int main(int argc, char *argv[]) {

  std::string path;
  if(argc > 1) {
    path = argv[1];
  }

  CGAL::Timer timer;

  Input_range input_range;
  input_range.reserve(100);

  // A horizontal like fan.
  PG p1( Point_2(-250, -50), Point_2(-250, 50),50);   // Point generator.
  PG p2( Point_2( 250,-250), Point_2( 250,250),50);
  Segm_iterator  t1( p1, p2);                     // Segment generator.
  Count_iterator t1_begin( t1);                   // Finite range.
  Count_iterator t1_end( t1, 50);
  std::copy( t1_begin, t1_end, std::back_inserter(input_range));
  // A vertical like fan.
  PG p3( Point_2( -50,-250), Point_2(  50,-250),50);
  PG p4( Point_2(-250, 250), Point_2( 250, 250),50);
  Segm_iterator  t2( p3, p4);
  Count_iterator t2_begin( t2);
  Count_iterator t2_end( t2, 50);
  std::copy( t2_begin, t2_end, std::back_inserter(input_range));


  std::cout.precision(15);

  std::cout << std::endl;
  std::cout << "BEFORE:" << std::endl;
  for (const auto& segment : input_range)
    std::cout << segment << std::endl;
  std::cout << std::endl;

  Saver saver;
  std::string full_path = path + "example_angles_100_segments_before";
  saver.save_segments(input_range, full_path);

  Neighbor_query neighbor_query(input_range);
  std::vector<std::size_t> vec;
  vec.resize(input_range.size());
  std::iota(vec.begin(), vec.end(), 0);
  neighbor_query.add_group(vec);

  const FT bound_angles = FT(40);
  Regularization_type_angles regularization_type_angles(input_range, bound_angles);
  regularization_type_angles.add_group(vec);

  Shape_regularization_angles shape_regularization_angles(
    input_range, neighbor_query, regularization_type_angles);

  timer.start();
  shape_regularization_angles.regularize();
  timer.stop();

  std::cout << "Number of modified segments angles: " << regularization_type_angles.number_of_modified_segments()
            << "; Time = " << timer.time() << " sec." << std::endl;

  std::vector <std::vector <std::size_t>> parallel_groups;
  regularization_type_angles.parallel_groups(std::back_inserter(parallel_groups));

  std::cout << "parallel_groups.size() = " << parallel_groups.size() << std::endl;

  
  std::cout << "AFTER:" << std::endl;
  for (const auto& segment : input_range)
    std::cout << segment << std::endl;
  std::cout << std::endl;
  full_path = path + "example_angles_100_segments_after";
  saver.save_segments(input_range, full_path); 

  return EXIT_SUCCESS;
}
