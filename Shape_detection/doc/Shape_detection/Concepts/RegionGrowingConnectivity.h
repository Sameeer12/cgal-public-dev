/*!
\ingroup PkgShapeDetectionConcepts
\cgalConcept

A concept that describes the set of types and methods required by the class `CGAL::Shape_detection::Region_growing`.

\cgalHasModel `CGAL::Shape_detection::Fuzzy_sphere_connectivity_on_points`, `CGAL::Shape_detection::Nearest_neighbor_connectivity_on_points`, and `CGAL::Shape_detection::Connectivity_on_face_graph`
*/

class RegionGrowingConnectivity {

public:
    
    /// Find all items, which are connected to an item with the index `query_index`, and push their indices into `neighbors`, where `Item_index` is any signed integer and `Neighbors` is a random access container.
    void get_neighbors(Item_index query_index, Neighbors &neighbors) const {
        
    }
};
