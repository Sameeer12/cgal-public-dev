#include <CGAL/Projection_traits_xy_3.h>
#include <CGAL/interpolation_functions.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Barycentric_coordinates_2.h>

// Typedefs.
using Kernel     = CGAL::Exact_predicates_inexact_constructions_kernel;
using Projection = CGAL::Projection_traits_xy_3<Kernel>;

using FT    = typename Projection::FT;
using Point = typename Projection::Point_2;

using Domain     = CGAL::Barycentric_coordinates::Delaunay_domain_2<Projection>;
using Mean_value = CGAL::Barycentric_coordinates::Mean_value_weights_2<Projection>;

using Vertex_function_value = std::map<Point, FT, typename Projection::Less_xy_2>;
using Function_value_access = CGAL::Data_access<Vertex_function_value>;
using Point_with_coordinate = std::pair<Point, FT>;

int main() {

  // Construct a polygon that bounds a three-dimensional terrain.
  // Note that z-coordinate of each vertex represents the height function.
  // Projection in 2D is performed automatically by the Projection traits class.
  const std::vector<Point> polygon = {
    Point(0.03, 0.05, 0.000), Point(0.07, 0.04, 10.00), Point(0.10, 0.04, 20.00),
    Point(0.14, 0.04, 30.00), Point(0.17, 0.07, 40.00), Point(0.19, 0.09, 50.00),
    Point(0.22, 0.11, 60.00), Point(0.25, 0.11, 70.00), Point(0.27, 0.10, 80.00),
    Point(0.30, 0.07, 90.00), Point(0.31, 0.04, 100.0), Point(0.34, 0.03, 110.0),
    Point(0.37, 0.02, 120.0), Point(0.40, 0.03, 130.0), Point(0.42, 0.04, 140.0),
    Point(0.44, 0.07, 150.0), Point(0.45, 0.10, 160.0), Point(0.46, 0.13, 170.0),
    Point(0.46, 0.19, 180.0), Point(0.47, 0.26, 190.0), Point(0.47, 0.31, 200.0),
    Point(0.47, 0.35, 210.0), Point(0.45, 0.37, 220.0), Point(0.41, 0.38, 230.0),
    Point(0.38, 0.37, 240.0), Point(0.35, 0.36, 250.0), Point(0.32, 0.35, 260.0),
    Point(0.30, 0.37, 270.0), Point(0.28, 0.39, 280.0), Point(0.25, 0.40, 290.0),
    Point(0.23, 0.39, 300.0), Point(0.21, 0.37, 310.0), Point(0.21, 0.34, 320.0),
    Point(0.23, 0.32, 330.0), Point(0.24, 0.29, 340.0), Point(0.27, 0.24, 350.0),
    Point(0.29, 0.21, 360.0), Point(0.29, 0.18, 370.0), Point(0.26, 0.16, 380.0),
    Point(0.24, 0.17, 390.0), Point(0.23, 0.19, 400.0), Point(0.24, 0.22, 410.0),
    Point(0.24, 0.25, 420.0), Point(0.21, 0.26, 430.0), Point(0.17, 0.26, 440.0),
    Point(0.12, 0.24, 450.0), Point(0.07, 0.20, 460.0), Point(0.03, 0.15, 470.0),
    Point(0.01, 0.10, 480.0), Point(0.02, 0.07, 490.0)
  };

  // Instantiate a Delaunay domain.
  std::list<Point> list_of_seeds;
  list_of_seeds.push_back(Point(0.1, 0.1, 0.0));

  Domain domain(polygon);
  domain.create(0.05, list_of_seeds);

  // Associate each polygon vertex with the corresponding function value.
  Vertex_function_value vertex_function_value;
  for(const auto& vertex : polygon)
    vertex_function_value.insert(
      std::make_pair(vertex, vertex.z()));

  std::vector<Point_with_coordinate> boundary;
  boundary.resize(polygon.size());

  // Use it to store all generated interior points with the interpolated data.
  std::vector<Point> queries;
  queries.reserve(domain.number_of_vertices());

  // Instantiate the class with the mean value weights.
  Mean_value mean_value(polygon);

  // Compute mean value coordinates and use them to interpolate data
  // from the polygon boundary to its interior.
  std::vector<FT> coordinates;
  coordinates.reserve(polygon.size());
  for (std::size_t i = 0; i < domain.number_of_vertices(); ++i) {
    const auto& query = domain.vertex(i);

    coordinates.clear();
    mean_value.coordinates(query, std::back_inserter(coordinates));
    for (std::size_t i = 0; i < polygon.size(); ++i)
      boundary[i] = std::make_pair(polygon[i], coordinates[i]);

    const FT f = CGAL::linear_interpolation(
      boundary.begin(), boundary.end(), FT(1),
      Function_value_access(vertex_function_value));
    queries.push_back(Point(query.x(), query.y(), f));
  }

  // Output interpolated heights.
  std::cout << std::endl <<
    "interpolated heights (all queries): "
  << std::endl << std::endl;
  for (const auto& query : queries)
    std::cout << query.z() << std::endl;
  std::cout << std::endl;

  return EXIT_SUCCESS;
}
