#include "incidence_mat.h"

using namespace Eigen;
using lf::mesh::Mesh;
using size_type = lf::base::size_type;

namespace IncidenceMatrices {

// @brief Create the mesh consisting of a triangle and quadrilateral
//        from the exercise sheet.
// @return Shared pointer to the hybrid2d mesh.
std::shared_ptr<Mesh> createDemoMesh() {
  // builder for a hybrid mesh in a world of dimension 2
  std::shared_ptr<lf::mesh::hybrid2d::MeshFactory> mesh_factory_ptr =
      std::make_shared<lf::mesh::hybrid2d::MeshFactory>(2);

  // Add points
  mesh_factory_ptr->AddPoint(Vector2d{0, 0});    // (0)
  mesh_factory_ptr->AddPoint(Vector2d{1, 0});    // (1)
  mesh_factory_ptr->AddPoint(Vector2d{1, 1});    // (2)
  mesh_factory_ptr->AddPoint(Vector2d{0, 1});    // (3)
  mesh_factory_ptr->AddPoint(Vector2d{0.5, 1});  // (4)

  // Add the triangle
  // First set the coordinates of its nodes:
  MatrixXd nodesOfTria(2, 3);
  nodesOfTria << 1, 1, 0.5, 0, 1, 1;
  mesh_factory_ptr->AddEntity(
      lf::base::RefEl::kTria(),                             // we want a triangle
      nonstd::span<const size_type>({1, 2, 4}),             // indices of the nodes
      std::make_unique<lf::geometry::TriaO1>(nodesOfTria)); // node coords

  // Add the quadrilateral
  MatrixXd nodesOfQuad(2, 4);
  nodesOfQuad << 0, 1, 0.5, 0, 0, 0, 1, 1;
  mesh_factory_ptr->AddEntity(
      lf::base::RefEl::kQuad(), nonstd::span<const size_type>({0, 1, 4, 3}),
      std::make_unique<lf::geometry::QuadO1>(nodesOfQuad));

  std::shared_ptr<Mesh> demoMesh_p = mesh_factory_ptr->Build();

  return demoMesh_p;
}

// @brief Compute the edge-vertex incidence matrix G for a given mesh
// @param mesh The input mesh of type lf::mesh::Mesh (or of derived type,
//             such as lf::mesh::hybrid2d::Mesh)
// @return The edge-vertex incidence matrix as Eigen::SparseMatrix<int>
/* SAM_LISTING_BEGIN_1 */
SparseMatrix<int> computeEdgeVertexIncidenceMatrix(const Mesh &mesh) {
  // Store edge-vertex incidence matrix here
  SparseMatrix<int, RowMajor> G;

#if SOLUTION
  // Mesh::NumEntities(unsigned codim) returns the number of elements
  // with given codimension. Codim(Edge) = 1, Codim(Node) = 2.
  const std::size_t numEdges = mesh.NumEntities(1),
                    numNodes = mesh.NumEntities(2);
  // Following the demo for the reserve()-initialising the sparse matrix given
  // in the exercise sheet. From (2.1a) we know that G has exactly 2 entries
  // per row.
  G = SparseMatrix<int, RowMajor>(numEdges, numNodes);
  G.reserve(VectorXi::Constant(numEdges, 2));

  // To compute G efficiently we iterate over all edges and check the index
  // of the nodes at its end. This is the efficient way to do the assembly,
  // introduced as "distribute scheme" in class. We cannot iterative over
  // vertices, because LehrFEM++ does not allow to visit the edges
  // adjacent to a vertex
  for (const lf::mesh::Entity *edge : mesh.Entities(1)) {
    // Get index of this edge
    size_type edgeIdx = mesh.Index(*edge);
    // Get the nodes and their indices.
    // Note, that seen from the edges the nodes have codim 1, not 2,
    // hence we call SubEntities(1). This is a relative codimension!
    auto nodes = edge->SubEntities(1);
    size_type firstNodeIdx = mesh.Index(*nodes[0]);
    size_type lastNodeIdx = mesh.Index(*nodes[1]);
    // Add the matrix entries according to the definition
    G.coeffRef(edgeIdx, firstNodeIdx) += 1;
    G.coeffRef(edgeIdx, lastNodeIdx) -= 1;
  }
#else
  //====================
  // Your code goes here
  //====================
#endif

  return G;
}
/* SAM_LISTING_END_1 */

// @brief Compute the cell-edge incidence matrix D for a given mesh
// @param mesh The input mesh of type lf::mesh::Mesh (or of derived type,
//             such as lf::mesh::hybrid2d::Mesh)
// @return The cell-edge incidence matrix as Eigen::SparseMatrix<int>
/* SAM_LISTING_BEGIN_2 */
SparseMatrix<int> computeCellEdgeIncidenceMatrix(const Mesh &mesh) {
  // Store cell-edge incidence matrix here
  SparseMatrix<int, RowMajor> D;

#if SOLUTION
  // Mesh::NumEntities(unsigned codim) returns the number of elements
  // with given codimension. Codim(Edge) = 0, Codim(Node) = 1.
  const std::size_t numCells = mesh.NumEntities(0),
                    numEdges = mesh.NumEntities(1);
  // Following the demo for the reserve()-initialising the sparse matrix given
  // in the exercise sheet. From (2.1a) we know that D has at most 4 entries
  // per row.
  D = SparseMatrix<int, RowMajor>(numCells, numEdges);
  D.reserve(VectorXi::Constant(numCells, 4));

  // To compute D efficiently we iterate over all cells and check the
  // orientations (+1 or -1, same as in the definition of the matrix D)
  // of its edges. For this we may use RelativeOrientations().
  for (const lf::mesh::Entity *cell : mesh.Entities(0)) {
    // Get cell index
    size_type cellIdx = mesh.Index(*cell);
    // Get edges and their orientations (these already the entries for D!)
    auto edges = cell->SubEntities(1);
    auto edgeOrientations = cell->RelativeOrientations();

    // Iterate over both and add to D
    auto edgeIt = edges.begin();
    auto orntIt = edgeOrientations.begin();
    for (; edgeIt != edges.end() && orntIt != edgeOrientations.end();
         ++edgeIt, ++orntIt) {
      // Get the edge index and add its orientation to D
      size_type edgeIdx = mesh.Index(**edgeIt);
      D.coeffRef(cellIdx, edgeIdx) += lf::mesh::to_sign(*orntIt);
    }
  }
#else
  //====================
  // Your code goes here
  //====================
#endif

  return D;
}
/* SAM_LISTING_END_2 */

// @brief For a given mesh test if the product of cell-edge and edge-vertex
//        incidence matrix is zero: D*G == 0?
// @param mesh The input mesh of type lf::mesh::Mesh (or of derived type,
//             such as lf::mesh::hybrid2d::Mesh)
// @return true, if the product is zero and false otherwise
/* SAM_LISTING_BEGIN_3 */
bool testZeroIncidenceMatrixProduct(const Mesh &mesh) {
  bool isZero = false;

#if SOLUTION
  SparseMatrix<int> G = computeEdgeVertexIncidenceMatrix(mesh),
                    D = computeCellEdgeIncidenceMatrix(mesh);

  SparseMatrix<int> O = D * G;
  // Possibility 1:
  // Not prone to roundoff errors, since O is an integer matrix!
  isZero = O.norm() == 0;
  // Possibility 2: But this doesn't use the fact that O is sparse.
  // isZero = MatrixXi(O).isZero(0);
#else
  //====================
  // Your code goes here
  //====================
#endif

  return isZero;
}
/* SAM_LISTING_END_3 */

}  // namespace IncidenceMatrices
