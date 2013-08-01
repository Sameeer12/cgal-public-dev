namespace CGAL {
/*!
\ingroup PkgVisibility_2Classes

\brief This class is a model of the concept `Visibility_2` offering visibility queries within a polygon that may have hole(s).


\details The algorithm it applies to obtain visibility is without preprocessing. It relies on the algorithm of T. Asano \cite ta-aeafvpprh-85 based on angular plane sweep, with a time complexity of \f$O (n \log n)\f$ in the number of vertices.

\tparam Arrangement_2 is the type of input polygonal environment and output visibility polygon.

\tparam RegularizationTag indicates whether the output should be regularized. It can be
specified by one of the following: ::Tag_true or ::Tag_false.


\cgalModels `Visibility_2` 

\sa `::Visibility_2`
\sa `CGAL::Simple_visibility_2<Arrangement_2, RegularizationTag>`
\sa `CGAL::Preprocessed_rotational_sweep_visibility_2<Arrangement_2, RegularizationTag>`
\sa `CGAL::Triangular_expansion_visibility_2<Arrangement_2, RegularizationTag>`

*/
template <typename Arrangement_2, typename RegularizationTag>
class Rotational_sweep_visibility_2 {
public:

/// \name Types 
/// @{
   
 /*!
  The type of input arrangement.
  */
  typedef Arrangement_2  Input_arrangement_2;

   /*!
    The type of output arrangement.
    */
  typedef Arrangement_2 Output_arrangement_2;

 /*! 
   The Point_2 type which is used for queries. 
 */ 
  typedef Input_arrangement_2::Point_2 Point_2; 

  /*!
   Face_handle type of input arrangement.
   */
  typedef Input_arrangement_2::Face_handle Face_handle;

  /*!
   Halfedge_handle type of input arrangement.
   */
  typedef Input_arrangement_2::Halfedge_handle Halfedge_handle;

/// @}

/// \name Tags 
/// @{
  /*! 
    Tag identifying whether the regularized visibility area is computed. 
  */
  typedef RegularizationTag Regularization_tag;
  
  /*! 
    Tag identifying that the class supports general polygons (i.e.\ with holes). 
  */
  typedef ::Tag_true Supports_general_polygon_tag; 

  /*! 
    Tag identifying that the class supports general simple polygons. 
  */
  typedef ::Tag_true Supports_simple_polygon_tag; 
/// @}

/// \name Constructors 
/// @{

/*!
Default constructor creates an empty 'Rotational_sweep_visibility_2' object, that is not
attached to any arrangement yet.
*/
Rotational_sweep_visibility_2();

/*! 
Constructs a `Rotational_sweep_visibility_2` object from a given `Input_arrangement_2` and attaches it to `arr`.
*/ 
Rotational_sweep_visibility_2(const Input_arrangement_2& arr); 

/// @}

/// \name functions 
/// @{

/*!
Returns whether an arrangement is attached to the visibility object
*/
  bool is_attached();

/*!
Attaches the given arrangement to the visibility object.
In case the object is already attached to another arrangement, 
the visibility object gets detached before being attached to 'arr'.
*/
  void attach(const Input_arrangement_2 &arr);
  
/*!
Detaches the arrangement from the visibility object it is currently attached to
*/
  void detach();

/*!
Access to the attached arrangement
*/
  const Input_arrangement_2& arr();

/*! 
Computes the visibility region for the given query point `q` in the
face `f` of the arrangement that is attached to the visibility object. 
The visibility region of `q` will be stored in `out_arr`.
\param q is the query point from which the visibility region is computed
\param f is the face of the arrangement in which the visibility region is computed
\param out_arr is the output arrangement 
\pre `f` is a face of  `this->arr()`, defined as a regular polygon 
\pre `q` is in the interior or on the boundary of the given face `f`
\return the face handle to the face in `out_arr` that represents the visibility region
*/ 
  Face_handle visibility_region(const Point_2& q, const Face_handle& f, Output_arrangement_2& out_arr);

  /*!
  Computes the visibility region for the given query point `q` that is on `halfedge`.If `q` is an interior point of `halfedge`, the computed visibility region is restricted to the halfplane indicated by `halfedge`. If `q` is an endpoint of `halfedge`, the visibility region is restricted by `halfedge` and its next.
  The visibility region of `q` will be stored in `out_arr`.
  \param q is the query point from which the visibility region is computed
  \param halfedge the halfedge on which `q` is located
  \param out_arr is the output arrangement
  \pre `halfedge` is a halfedge of  `this->arr()`
  \pre `q` is on `halfedge`
  \pre `q` equals to `halfedge->target()->point()` if `q` is an endpoint of `halfedge`
  \return the face handle to the face in `out_arr` that represents the visibility region
  */
    Face_handle visibility_region(const Point_2& q, const Halfedge_handle& halfedge, Output_arrangement_2& out_arr);

/// @}

}; /* end Visibility_2 */
}
