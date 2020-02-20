/*!
\ingroup PkgShape_regularizationConcepts
\cgalConcept

A concept that describes the set of methods used by the `CGAL::Regularization::Shape_regularization` 
to apply regularization.

\cgalHasModel 
- `CGAL::Regularization::Angle_regularization_2`, 
- `CGAL::Regularization::Ordinate_regularization_2`
*/
class RegularizationType {

public:

  /*!  
  returns the max bound on an item value that is regularized.

  `CGAL::Regularization::Shape_regularization` calls this function for each item 
  with the index `i` that participates in the regularization process.
  */
  typename GeomTraits::FT bound(const std::size_t i) const { 

  }

  /*!  
    returns an objective function value between two items, which are direct neighbors.

    `CGAL::Regularization::Shape_regularization` calls this function for each neighbor pair 
    `i <-> j` that participates in the regularization process.
  */
  typename GeomTraits::FT target_value(const std::size_t i, const std::size_t j) {

  }

  /*!
    applies the results from the QP solver to the initial items.

    `CGAL::Regularization::Shape_regularization` calls this function one time, after
    the QP problem has been solved during the regularization process.
  */
  void update(
    const std::vector<GeomTraits::FT>& result) {
    
  }
};
