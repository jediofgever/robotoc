#include "robotoc/cost/time_varying_com_cost.hpp"


namespace robotoc {

TimeVaryingCoMCost::TimeVaryingCoMCost(
    const Robot& robot, 
    const std::shared_ptr<TimeVaryingCoMRefBase>& com_ref) 
  : CostFunctionComponentBase(),
    com_ref_(com_ref),
    com_weight_(Eigen::Vector3d::Zero()),
    comf_weight_(Eigen::Vector3d::Zero()),
    comi_weight_(Eigen::Vector3d::Zero()) {
}


TimeVaryingCoMCost::TimeVaryingCoMCost()
  : CostFunctionComponentBase(),
    com_ref_(),
    com_weight_(),
    comf_weight_(),
    comi_weight_() {
}


TimeVaryingCoMCost::~TimeVaryingCoMCost() {
}


void TimeVaryingCoMCost::set_com_ref(
    const std::shared_ptr<TimeVaryingCoMRefBase>& com_ref) {
  com_ref_ = com_ref;
}


void TimeVaryingCoMCost::set_com_weight(const Eigen::Vector3d& com_weight) {
  com_weight_ = com_weight;
}


void TimeVaryingCoMCost::set_comf_weight(const Eigen::Vector3d& comf_weight) {
  comf_weight_ = comf_weight;
}


void TimeVaryingCoMCost::set_comi_weight(const Eigen::Vector3d& comi_weight) {
  comi_weight_ = comi_weight;
}


bool TimeVaryingCoMCost::useKinematics() const {
  return true;
}


double TimeVaryingCoMCost::evalStageCost(Robot& robot, 
                                         const ContactStatus& contact_status, 
                                         CostFunctionData& data, 
                                         const GridInfo& grid_info, 
                                         const SplitSolution& s) const {
  if (com_ref_->isActive(grid_info)) {
    double l = 0;
    com_ref_->update_com_ref(grid_info, data.x3d_ref);
    data.diff_3d = robot.CoM() - data.x3d_ref;
    l += (com_weight_.array()*data.diff_3d.array()*data.diff_3d.array()).sum();
    return 0.5 * grid_info.dt * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingCoMCost::evalStageCostDerivatives(
    Robot& robot, const ContactStatus& contact_status, CostFunctionData& data, 
    const GridInfo& grid_info, const SplitSolution& s, 
    SplitKKTResidual& kkt_residual) const {
  if (com_ref_->isActive(grid_info)) {
    data.J_3d.setZero();
    robot.getCoMJacobian(data.J_3d);
    kkt_residual.lq().noalias() 
        += grid_info.dt * data.J_3d.transpose() * com_weight_.asDiagonal() * data.diff_3d;
  }
}


void TimeVaryingCoMCost::evalStageCostHessian(
    Robot& robot, const ContactStatus& contact_status, CostFunctionData& data, 
    const GridInfo& grid_info, const SplitSolution& s, 
    SplitKKTMatrix& kkt_matrix) const {
  if (com_ref_->isActive(grid_info)) {
    kkt_matrix.Qqq().noalias()
        += grid_info.dt * data.J_3d.transpose() * com_weight_.asDiagonal() * data.J_3d;
  }
}


double TimeVaryingCoMCost::evalTerminalCost(Robot& robot, 
                                            CostFunctionData& data, 
                                            const GridInfo& grid_info, 
                                            const SplitSolution& s) const {
  if (com_ref_->isActive(grid_info)) {
    double l = 0;
    com_ref_->update_com_ref(grid_info, data.x3d_ref);
    data.diff_3d = robot.CoM() - data.x3d_ref;
    l += (comf_weight_.array()*data.diff_3d.array()*data.diff_3d.array()).sum();
    return 0.5 * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingCoMCost::evalTerminalCostDerivatives(
    Robot& robot, CostFunctionData& data, const GridInfo& grid_info, 
    const SplitSolution& s, SplitKKTResidual& kkt_residual) const {
  if (com_ref_->isActive(grid_info)) {
    data.J_3d.setZero();
    robot.getCoMJacobian(data.J_3d);
    kkt_residual.lq().noalias() 
        += data.J_3d.transpose() * comf_weight_.asDiagonal() * data.diff_3d;
  }
}


void TimeVaryingCoMCost::evalTerminalCostHessian(
    Robot& robot, CostFunctionData& data, const GridInfo& grid_info, 
    const SplitSolution& s, SplitKKTMatrix& kkt_matrix) const {
  if (com_ref_->isActive(grid_info)) {
    kkt_matrix.Qqq().noalias()
        += data.J_3d.transpose() * comf_weight_.asDiagonal() * data.J_3d;
  }
}


double TimeVaryingCoMCost::evalImpulseCost(
    Robot& robot, const ImpulseStatus& impulse_status, CostFunctionData& data, 
    const GridInfo& grid_info, const ImpulseSplitSolution& s) const {
  if (com_ref_->isActive(grid_info)) {
    double l = 0;
    com_ref_->update_com_ref(grid_info, data.x3d_ref);
    data.diff_3d = robot.CoM() - data.x3d_ref;
    l += (comi_weight_.array()*data.diff_3d.array()*data.diff_3d.array()).sum();
    return 0.5 * l;
  }
  else {
    return 0;
  }
}


void TimeVaryingCoMCost::evalImpulseCostDerivatives(
    Robot& robot, const ImpulseStatus& impulse_status, CostFunctionData& data, 
    const GridInfo& grid_info, const ImpulseSplitSolution& s, 
    ImpulseSplitKKTResidual& kkt_residual) const {
  if (com_ref_->isActive(grid_info)) {
    data.J_3d.setZero();
    robot.getCoMJacobian(data.J_3d);
    kkt_residual.lq().noalias() 
        += data.J_3d.transpose() * comi_weight_.asDiagonal() * data.diff_3d;
  }
}


void TimeVaryingCoMCost::evalImpulseCostHessian(
    Robot& robot, const ImpulseStatus& impulse_status, CostFunctionData& data, 
    const GridInfo& grid_info, const ImpulseSplitSolution& s, 
    ImpulseSplitKKTMatrix& kkt_matrix) const {
  if (com_ref_->isActive(grid_info)) {
    kkt_matrix.Qqq().noalias()
        += data.J_3d.transpose() * comi_weight_.asDiagonal() * data.J_3d;
  }
}

} // namespace robotoc