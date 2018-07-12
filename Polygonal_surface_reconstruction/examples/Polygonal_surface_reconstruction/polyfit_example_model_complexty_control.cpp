
#define HAS_SCIP

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/IO/read_ply_points.h>
#include <CGAL/IO/Writer_OFF.h>
#include <CGAL/property_map.h>
#include <CGAL/Surface_mesh.h>
#include <CGAL/Polygonal_surface_reconstruction.h>

#ifdef HAS_SCIP
#include <CGAL/SCIP_mixed_integer_program_traits.h>
#else
#include <CGAL/GLPK_mixed_integer_program_traits.h>
#endif

#include <CGAL/Timer.h>

#include <fstream>


typedef CGAL::Exact_predicates_inexact_constructions_kernel		Kernel;
typedef Kernel::Point_3											Point;
typedef Kernel::Vector_3										Vector;
typedef	CGAL::Polygonal_surface_reconstruction<Kernel>			Polygonal_surface_reconstruction;
typedef CGAL::Surface_mesh<Point>								Surface_mesh;

#ifdef HAS_SCIP
typedef CGAL::SCIP_mixed_integer_program_traits<double>			MIP_Solver;
#else
typedef CGAL::GLPK_mixed_integer_program_traits<double>			MIP_Solver;
#endif

// Point with normal, and plane index
typedef boost::tuple<Point, Vector, int>						PNI;
typedef CGAL::Nth_of_tuple_property_map<0, PNI>					Point_map;
typedef CGAL::Nth_of_tuple_property_map<1, PNI>					Normal_map;
typedef CGAL::Nth_of_tuple_property_map<2, PNI>					Plane_index_map;

/*
* The following example shows how to control the model complexity by
* increasing the influence of the model complexity term.
* In this example, the intermediate results from plane extraction and 
* candidate generation are cached and reused.
*/

int main()
{
    const std::string& input_file("data/building.ply");
	std::ifstream input_stream(input_file.c_str());

	std::vector<PNI> points; // store points

	std::cout << "Loading point cloud: " << input_file << "...";
	CGAL::Timer t;
	t.start();

	if (!input_stream ||
		!CGAL::read_ply_points_with_properties(
			input_stream,
			std::back_inserter(points),
			CGAL::make_ply_point_reader(Point_map()),
			CGAL::make_ply_normal_reader(Normal_map()),
			std::make_pair(Plane_index_map(), CGAL::PLY_property<int>("segment_index"))))
	{
		std::cerr << "Error: cannot read file " << input_file << std::endl;
		return EXIT_FAILURE;
	}
	else
		std::cout << " Done. " << points.size() << " points. Time: " << t.time() << " sec." << std::endl;

	//////////////////////////////////////////////////////////////////////////

	std::cout << "Generating candidate faces...";
	t.reset();

	Polygonal_surface_reconstruction algo(
		points,
		Point_map(),
		Normal_map(),
		Plane_index_map()
	);

	std::cout << " Done. Time: " << t.time() << " sec." << std::endl;

	//////////////////////////////////////////////////////////////////////////

	// reconstruction with complexity control
	
	// model 1: more detail
	Surface_mesh model;

	std::cout << "Reconstructing with complexity 0.2...";
	t.reset();
	if (!algo.reconstruct<MIP_Solver>(model, 0.43, 0.27, 0.2)) {
		std::cerr << " Failed: " << algo.error_message() << std::endl;
		return EXIT_FAILURE;
	}
	else {
       const std::string& output_file = "data/building_result_complexity-0.2.off";
       std::ofstream output_stream(output_file.c_str());
       if (output_stream && CGAL::write_off(output_stream, model))
			std::cout << " Done. Saved to " << output_file << ". Time: " << t.time() << " sec." << std::endl;
		else {
           std::cerr << " Failed saving file." << std::endl;
           return EXIT_FAILURE;
       }
	}

	// model 2: a little less detail
	std::cout << "Reconstructing with complexity 0.6...";
	t.reset();
	if (!algo.reconstruct<MIP_Solver>(model, 0.43, 0.27, 0.6)) {
		std::cerr << " Failed: " << algo.error_message() << std::endl;
		return EXIT_FAILURE;
	}
	else {
       const std::string& output_file = "data/building_result_complexity-0.6.off";
       std::ofstream output_stream(output_file.c_str());
       if (output_stream && CGAL::write_off(output_stream, model))
			std::cout << " Done. Saved to " << output_file << ". Time: " << t.time() << " sec." << std::endl;
		else {
           std::cerr << " Failed saving file." << std::endl;
           return EXIT_FAILURE;
       }
	}

	// model 3: more less detail
	std::cout << "Reconstructing with complexity 1.5...";
	t.reset();
	if (!algo.reconstruct<MIP_Solver>(model, 0.43, 0.27, 1.5)) {
		std::cerr << " Failed: " << algo.error_message() << std::endl;
		return EXIT_FAILURE;
	}
	else {
		const std::string& output_file = "data/building_result_complexity-1.5.off";
		std::ofstream output_stream(output_file.c_str());
		if (output_stream && CGAL::write_off(output_stream, model))
			std::cout << " Done. Saved to " << output_file << ". Time: " << t.time() << " sec." << std::endl;
		else {
			std::cerr << " Failed saving file." << std::endl;
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}
