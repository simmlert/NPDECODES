#ifndef OUTPUTIMPEDANCE_HPP
#define OUTPUTIMPEDANCE_HPP

/** @file
 * @brief NPDE OutputImpedanceBVP
 * @author Erick Schulz
 * @date 12/07/2019
 * @copyright Developed at ETH Zurich
 */

#include <iostream>
// Eigen includes
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Sparse>
// Lehrfem++ includes
#include <lf/assemble/assemble.h>
#include <lf/geometry/geometry.h>
#include <lf/io/io.h>
#include <lf/mesh/hybrid2d/hybrid2d.h>
#include <lf/mesh/utils/utils.h>
#include <lf/uscalfe/uscalfe.h>

namespace OutputImpedanceBVP {

// Simplification of lenghty type names
using coord_t = Eigen::Vector2d;

// Library functions
Eigen::VectorXd solveImpedanceBVP(
    const std::shared_ptr<lf::uscalfe::FeSpaceLagrangeO1<double>> &fe_space_p,
    Eigen::Vector2d);

double computeBoundaryOutputFunctional(
    const Eigen::VectorXd,
    const std::shared_ptr<lf::uscalfe::FeSpaceLagrangeO1<double>> &fe_space_p,
    Eigen::Vector2d);

template <typename FUNCTOR_U>
Eigen::VectorXd interpolateData(
    std::shared_ptr<lf::uscalfe::UniformScalarFESpace<double>> fe_space_p,
    FUNCTOR_U &&u) {

  // Generate Lehrfem++ mesh functions out of the functors
  auto mf_u = lf::mesh::utils::MeshFunctionGlobal(
      [&u](coord_t x) -> double { return u(x); });

  Eigen::VectorXd dof_vector_u = lf::uscalfe::NodalProjection(*fe_space_p, mf_u);

  return dof_vector_u;
};

} // namespace OutputImpedanceBVP

#endif
