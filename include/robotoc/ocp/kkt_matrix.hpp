#ifndef ROBOTOC_KKT_MATRIX_HPP_
#define ROBOTOC_KKT_MATRIX_HPP_

#include <iostream>

#include "robotoc/hybrid/hybrid_container.hpp"
#include "robotoc/ocp/split_kkt_matrix.hpp"
#include "robotoc/impulse/impulse_split_kkt_matrix.hpp"
#include "robotoc/ocp/switching_constraint_jacobian.hpp"


namespace robotoc {

///
/// @typedef KKTMatrix 
/// @brief The KKT matrix of the (hybrid) optimal control problem. 
///
using KKTMatrix = hybrid_container<SplitKKTMatrix, ImpulseSplitKKTMatrix, 
                                   SwitchingConstraintJacobian>;

std::ostream& operator<<(std::ostream& os, const KKTMatrix& kkt_matrix);

} // namespace robotoc

#endif // ROBOTOC_KKT_MATRIX_HPP_ 