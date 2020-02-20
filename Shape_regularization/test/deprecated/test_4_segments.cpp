// STL includes.
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>

// CGAL includes.
#include <CGAL/property_map.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Simple_cartesian.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Shape_regularization.h>


template<class Traits>
bool test_shape_regularization_segments_2() { 
  using Segment_2 = typename Traits::Segment_2;
  using Point_2 = typename Traits::Point_2;
  using FT = typename Traits::FT;

  using Input_range = std::vector<Segment_2>;
  using Segment_map = CGAL::Identity_property_map<Segment_2>;

  using Neighbor_query = CGAL::Regularization::Delaunay_neighbor_query_2<Traits, Input_range, Segment_map>;
  using Regularization_type_angles = CGAL::Regularization::Angle_regularization_2<Traits, Input_range, Segment_map>;
  using Regularization_type_ordinates = CGAL::Regularization::Ordinate_regularization_2<Traits, Input_range, Segment_map>;

  using Shape_regularization_angles = CGAL::Regularization::Shape_regularization
    <Traits, Input_range, Neighbor_query, Regularization_type_angles>;
  using Shape_regularization_ordinates = CGAL::Regularization::Shape_regularization
    <Traits, Input_range, Neighbor_query, Regularization_type_ordinates>;

  const Point_2 a = Point_2(0.2, 1.0);
  const Point_2 b = Point_2(0.2, 0.2);

  const Point_2 c = Point_2(0.2364941756489, 1.0451701975863);
  const Point_2 d = Point_2(0.2410972444872, 1.5975384581787);

  const Point_2 e = Point_2(0.6,1.6);
  const Point_2 f = Point_2(0.6392626989976, 1.4134157046479);

  const Point_2 g = Point_2(0.2, 2.0);
  const Point_2 h = Point_2(0.595533545034, 2.0463376699101);

  Input_range input_range;
  input_range.push_back(Segment_2(a, b));
  input_range.push_back(Segment_2(c, d));
  input_range.push_back(Segment_2(e, f));
  input_range.push_back(Segment_2(g, h));

  assert(input_range.size() == 4);
  if(input_range.size() != 4) {
    return false;
  }

  Neighbor_query neighbor_query(input_range);
  std::vector<std::size_t> vec;
  vec.resize(input_range.size());
  std::iota(vec.begin(), vec.end(), 0);
  neighbor_query.add_group(vec);
  
  const FT bound_angles = FT(5);
  Regularization_type_angles regularization_type_angles(input_range, bound_angles);
  regularization_type_angles.add_group(vec);

  Shape_regularization_angles shape_regularization_angles(
    input_range, neighbor_query, regularization_type_angles);

  shape_regularization_angles.regularize();

  std::vector <std::vector <std::size_t>> parallel_groups;
  regularization_type_angles.parallel_groups(std::back_inserter(parallel_groups));

  assert(input_range.size() == 4);
  if(input_range.size() != 4) {;
    return false;
  }
  assert(parallel_groups.size() == 3);
  if(parallel_groups.size() != 3) {
    return false;
  }
  
  const std::size_t modified_seg_ang = regularization_type_angles.number_of_modified_segments();
  assert(modified_seg_ang == 4);
  if(modified_seg_ang != 4) {
    return false;
  }

  std::vector<FT> reference_values;
  reference_values.reserve(4);
  reference_values.push_back(FT(1.6));
  reference_values.push_back(FT(3.12030));
  reference_values.push_back(FT(4.25267));
  reference_values.push_back(FT(4.84187));

  for (std::size_t i = 0; i < input_range.size(); ++i) {
    const Segment_2 & segment = input_range[i];
    const FT point1 = segment.source().x() + segment.source().y();
    const FT point2 = segment.target().x() + segment.target().y();
    FT both = point1 + point2;
    both = floor(CGAL::to_double(both) * 100000.0) / 100000.0;
    assert(both == reference_values[i]);
    if (both != reference_values[i]) {
      return false;
    }
  }

  // Regularization for ordinates:

  const FT bound_ordinates = FT(1) / FT(10);
  Regularization_type_ordinates regularization_type_ordinates(input_range, bound_ordinates);

  neighbor_query.clear();
  for(const auto & group : parallel_groups) {
    neighbor_query.add_group(group);
    regularization_type_ordinates.add_group(group);
  }

  Shape_regularization_ordinates shape_regularization_ordinates(
    input_range, neighbor_query, regularization_type_ordinates);
  shape_regularization_ordinates.regularize();

  assert(input_range.size() == 4);
  if(input_range.size() != 4) {
    return false;
  }

  const std::size_t modified_seg_ord = regularization_type_ordinates.number_of_modified_segments();
  assert(modified_seg_ord == 2);
  if(modified_seg_ord != 2) {
    return false;
  }

  reference_values.clear();
  reference_values.reserve(4);
  reference_values.push_back(FT(1.52780));
  reference_values.push_back(FT(2.90371));
  reference_values.push_back(FT(4.25267));
  reference_values.push_back(FT(4.84187));

  for (std::size_t i = 0; i < input_range.size(); ++i) {
    const Segment_2 & segment = input_range[i];
    const FT point1 = segment.source().x() + segment.source().y();
    const FT point2 = segment.target().x() + segment.target().y();
    FT both = point1 + point2;
    both = floor(CGAL::to_double(both) * 100000.0) / 100000.0;
    assert(both == reference_values[i]);
    if (both != reference_values[i]) {
      return false;
    }
  }

  return true;
}

int main() {
  bool cartesian_double_test_success = true;
  if (!test_shape_regularization_segments_2< CGAL::Simple_cartesian<double> >()) 
    cartesian_double_test_success = false;
      
  std::cout << "cartesian_double_test_success: " << cartesian_double_test_success << std::endl;
  assert(cartesian_double_test_success);

  // ------>

  bool exact_inexact_test_success = true;
  if (!test_shape_regularization_segments_2<CGAL::Exact_predicates_inexact_constructions_kernel>()) 
    exact_inexact_test_success = false;
    
  std::cout << "exact_inexact_test_success: " << exact_inexact_test_success << std::endl;
  assert(exact_inexact_test_success);

  // ------>

  bool exact_exact_test_success = true;
  if (!test_shape_regularization_segments_2<CGAL::Exact_predicates_exact_constructions_kernel>()) 
    exact_exact_test_success = false;
    
  std::cout << "exact_exact_test_success: " << exact_exact_test_success << std::endl;
  assert(exact_exact_test_success);

  const bool success = cartesian_double_test_success && exact_inexact_test_success && exact_exact_test_success;
  return (success) ? EXIT_SUCCESS : EXIT_FAILURE;
}
