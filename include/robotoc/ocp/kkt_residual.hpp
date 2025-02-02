#ifndef ROBOTOC_KKT_RESIDUAL_HPP_
#define ROBOTOC_KKT_RESIDUAL_HPP_

#include <iostream>

#include "robotoc/hybrid/hybrid_container.hpp"
#include "robotoc/ocp/split_kkt_residual.hpp"
#include "robotoc/impulse/impulse_split_kkt_residual.hpp"
#include "robotoc/ocp/switching_constraint_residual.hpp"


namespace robotoc {

///
/// @typedef KKTResidual 
/// @brief The KKT residual of the (hybrid) optimal control problem. 
///
using KKTResidual = hybrid_container<SplitKKTResidual, ImpulseSplitKKTResidual, 
                                     SwitchingConstraintResidual>;

std::ostream& operator<<(std::ostream& os, const KKTResidual& kkt_residual);

} // namespace robotoc

#endif // ROBOTOC_KKT_RESIDUAL_HPP_ 