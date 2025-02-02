#include "robotoc/impulse/impulse_state_equation.hpp"

#include <cassert>


namespace robotoc {

ImpulseStateEquation::ImpulseStateEquation(const Robot& robot)
  : Fqq_inv_(),
    Fqq_prev_inv_(),
    Fqq_tmp_(),
    Fq_tmp_(),
    se3_jac_inverse_(),
    has_floating_base_(robot.hasFloatingBase()) {
  if (robot.hasFloatingBase()) {
    Fqq_inv_.resize(6, 6);
    Fqq_inv_.setZero();
    Fqq_prev_inv_.resize(6, 6);
    Fqq_prev_inv_.setZero();
    Fqq_tmp_.resize(6, 6);
    Fqq_tmp_.setZero();
    Fq_tmp_.resize(6);
    Fq_tmp_.setZero();
  }
}


ImpulseStateEquation::ImpulseStateEquation()
  : Fqq_inv_(),
    Fqq_prev_inv_(),
    Fqq_tmp_(),
    Fq_tmp_(),
    se3_jac_inverse_(),
    has_floating_base_(false) {
}


ImpulseStateEquation::~ImpulseStateEquation() {
}


void ImpulseStateEquation::evalStateEquation(
    const Robot& robot, const ImpulseSplitSolution& s, 
    const Eigen::VectorXd& q_next, const Eigen::VectorXd& v_next, 
    ImpulseSplitKKTResidual& kkt_residual) {
  assert(q_next.size() == robot.dimq());
  assert(v_next.size() == robot.dimv());
  robot.subtractConfiguration(s.q, q_next, kkt_residual.Fq());
  kkt_residual.Fv() = s.v + s.dv - v_next;
}


void ImpulseStateEquation::linearizeStateEquation(
    const Robot& robot, const Eigen::VectorXd& q_prev, 
    const ImpulseSplitSolution& s, const SplitSolution& s_next, 
    ImpulseSplitKKTMatrix& kkt_matrix, ImpulseSplitKKTResidual& kkt_residual) {
  assert(q_prev.size() == robot.dimq());
  evalStateEquation(robot, s, s_next.q, s_next.v, kkt_residual);
  if (robot.hasFloatingBase()) {
    robot.dSubtractConfiguration_dqf(s.q, s_next.q, kkt_matrix.Fqq());
    robot.dSubtractConfiguration_dq0(q_prev, s.q, kkt_matrix.Fqq_prev);
    kkt_residual.lq().template head<6>().noalias() 
        += kkt_matrix.Fqq().template topLeftCorner<6, 6>().transpose() 
              * s_next.lmd.template head<6>();
    kkt_residual.lq().template head<6>().noalias() 
        += kkt_matrix.Fqq_prev.template topLeftCorner<6, 6>().transpose() 
              * s.lmd.template head<6>();
    kkt_residual.lq().tail(robot.dimv()-6).noalias() 
        += s_next.lmd.tail(robot.dimv()-6) - s.lmd.tail(robot.dimv()-6);
  }
  else {
    kkt_matrix.Fqq().diagonal().fill(1.);
    kkt_residual.lq().noalias() += s_next.lmd - s.lmd;
  }
  kkt_residual.lv().noalias() += s_next.gmm - s.gmm;
  kkt_residual.ldv.noalias() += s_next.gmm;
}


void ImpulseStateEquation::correctLinearizedStateEquation(
    const Robot& robot, const ImpulseSplitSolution& s, 
    const SplitSolution& s_next, ImpulseSplitKKTMatrix& kkt_matrix, 
    ImpulseSplitKKTResidual& kkt_residual) {
  if (has_floating_base_) {
    se3_jac_inverse_.compute(kkt_matrix.Fqq_prev, Fqq_prev_inv_);
    robot.dSubtractConfiguration_dq0(s.q, s_next.q, kkt_matrix.Fqq_prev);
    se3_jac_inverse_.compute(kkt_matrix.Fqq_prev, Fqq_inv_);
    Fqq_tmp_ = kkt_matrix.Fqq().template topLeftCorner<6, 6>();
    Fq_tmp_  = kkt_residual.Fq().template head<6>();
    kkt_matrix.Fqq().template topLeftCorner<6, 6>().noalias() = - Fqq_inv_ * Fqq_tmp_;
    kkt_residual.Fq().template head<6>().noalias() = - Fqq_inv_ * Fq_tmp_;
  }
}

} // namespace robotoc 