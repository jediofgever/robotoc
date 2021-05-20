#include "idocp/riccati/riccati_recursion.hpp"

#include <omp.h>
#include <stdexcept>
#include <cassert>

namespace idocp {

RiccatiRecursion::RiccatiRecursion(const Robot& robot, const int N, 
                                   const int max_num_impulse, 
                                   const int nthreads)
  : nthreads_(nthreads),
    N_(N),
    N_all_(N+1),
    factorizer_(robot),
    lqr_policy_(robot, N, max_num_impulse),
    max_primal_step_sizes_(Eigen::VectorXd::Zero(N+1+3*max_num_impulse)), 
    max_dual_step_sizes_(Eigen::VectorXd::Zero(N+1+3*max_num_impulse)) {
  try {
    if (N <= 0) {
      throw std::out_of_range("invalid value: N must be positive!");
    }
    if (max_num_impulse < 0) {
      throw std::out_of_range("invalid value: max_num_impulse must be non-negative!");
    }
    if (nthreads <= 0) {
      throw std::out_of_range("invalid value: nthreads must be positive!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
}


RiccatiRecursion::RiccatiRecursion()
  : nthreads_(0),
    N_(0),
    N_all_(0),
    factorizer_(),
    max_primal_step_sizes_(), 
    max_dual_step_sizes_() {
}


RiccatiRecursion::~RiccatiRecursion() {
}


void RiccatiRecursion::backwardRiccatiRecursion(
    const OCP& ocp, KKTMatrix& kkt_matrix, KKTResidual& kkt_residual, 
    RiccatiFactorization& factorization) {
  const OCPDiscretizer& ocp_discretizer = ocp.discrete();
  const int N = ocp_discretizer.N();
  factorization[N].Pqq = kkt_matrix[N].Qqq();
  factorization[N].Pvv = kkt_matrix[N].Qvv();
  factorization[N].sq = - kkt_residual[N].lq();
  factorization[N].sv = - kkt_residual[N].lv();
  for (int i=N-1; i>=0; --i) {
    if (ocp_discretizer.isTimeStageBeforeImpulse(i)) {
      const int impulse_index = ocp_discretizer.impulseIndexAfterTimeStage(i);
      factorizer_.backwardRiccatiRecursion(factorization[i+1], 
                                           ocp_discretizer.dt_aux(impulse_index), 
                                           kkt_matrix.aux[impulse_index], 
                                           kkt_residual.aux[impulse_index], 
                                           factorization.aux[impulse_index], 
                                           lqr_policy_.aux[impulse_index]);
      factorizer_.backwardRiccatiRecursion(factorization.aux[impulse_index], 
                                           kkt_matrix.impulse[impulse_index], 
                                           kkt_residual.impulse[impulse_index], 
                                           factorization.impulse[impulse_index]);
      factorizer_.backwardRiccatiRecursion(factorization.impulse[impulse_index], 
                                           ocp_discretizer.dt(i), 
                                           kkt_matrix[i], kkt_residual[i], 
                                           factorization[i], lqr_policy_[i]);
    }
    else if (ocp_discretizer.isTimeStageBeforeLift(i)) {
      const int lift_index = ocp_discretizer.liftIndexAfterTimeStage(i);
      if (ocp_discretizer.isTimeStageBeforeImpulse(i+1)) {
        const int impulse_index = ocp_discretizer.impulseIndexAfterTimeStage(i+1);
        factorizer_.backwardRiccatiRecursion(factorization[i+1], 
                                             ocp_discretizer.dt_lift(lift_index), 
                                             kkt_matrix.lift[lift_index], 
                                             kkt_residual.lift[lift_index], 
                                             kkt_matrix.switching[impulse_index], 
                                             kkt_residual.switching[impulse_index], 
                                             factorization.lift[lift_index], 
                                             factorization.switching[impulse_index], 
                                             lqr_policy_.lift[lift_index]);
      }
      else {
        factorizer_.backwardRiccatiRecursion(factorization[i+1], 
                                             ocp_discretizer.dt_lift(lift_index), 
                                             kkt_matrix.lift[lift_index], 
                                             kkt_residual.lift[lift_index], 
                                             factorization.lift[lift_index], 
                                             lqr_policy_.lift[lift_index]);
      }
      factorizer_.backwardRiccatiRecursion(factorization.lift[lift_index], 
                                           ocp_discretizer.dt(i), 
                                           kkt_matrix[i], kkt_residual[i], 
                                           factorization[i], lqr_policy_[i]);
    }
    else {
      if (ocp_discretizer.isTimeStageBeforeImpulse(i+1)) {
        const int impulse_index = ocp_discretizer.impulseIndexAfterTimeStage(i+1);
        factorizer_.backwardRiccatiRecursion(factorization[i+1], ocp_discretizer.dt(i), 
                                             kkt_matrix[i], kkt_residual[i], 
                                             kkt_matrix.switching[impulse_index], 
                                             kkt_residual.switching[impulse_index], 
                                             factorization[i], 
                                             factorization.switching[impulse_index], 
                                             lqr_policy_[i]);
      }
      else {
        factorizer_.backwardRiccatiRecursion(factorization[i+1], ocp_discretizer.dt(i), 
                                             kkt_matrix[i], kkt_residual[i], 
                                             factorization[i], lqr_policy_[i]);
      }
    }
  }
}


void RiccatiRecursion::computeInitialStateDirection(
    const aligned_vector<Robot>& robots, const Eigen::VectorXd& q, 
    const Eigen::VectorXd& v, const KKTMatrix& kkt_matrix, const Solution& s, 
    Direction& d) {
  assert(q.size() == robots[0].dimq());
  assert(v.size() == robots[0].dimv());
  if (robots[0].hasFloatingBase()) {
    robots[0].subtractConfiguration(q, s[0].q, d[0].dq());
    d[0].dq().template head<6>() 
        = - kkt_matrix[0].Fqq_prev_inv * d[0].dq().template head<6>();
    d[0].dv() = v - s[0].v;
  }
  else {
    d[0].dq() = q - s[0].q;
    d[0].dv() = v - s[0].v;
  }
}


void RiccatiRecursion::forwardRiccatiRecursion(
    const OCP& ocp, const KKTMatrix& kkt_matrix, 
    const KKTResidual& kkt_residual, Direction& d) const {
  const OCPDiscretizer& ocp_discretizer = ocp.discrete();
  const int N = ocp_discretizer.N();
  for (int i=0; i<N; ++i) {
    if (ocp_discretizer.isTimeStageBeforeImpulse(i)) {
      const int impulse_index = ocp_discretizer.impulseIndexAfterTimeStage(i);
      factorizer_.forwardRiccatiRecursion(kkt_matrix[i], kkt_residual[i],  
                                          ocp_discretizer.dt(i), lqr_policy_[i], 
                                          d[i], d.impulse[impulse_index]);
      factorizer_.forwardRiccatiRecursion(kkt_matrix.impulse[impulse_index], 
                                          kkt_residual.impulse[impulse_index],
                                          d.impulse[impulse_index], d.aux[impulse_index]);
      factorizer_.forwardRiccatiRecursion(kkt_matrix.aux[impulse_index], 
                                          kkt_residual.aux[impulse_index],
                                          ocp_discretizer.dt_aux(impulse_index), 
                                          lqr_policy_.aux[impulse_index], 
                                          d.aux[impulse_index], d[i+1]);
    }
    else if (ocp_discretizer.isTimeStageBeforeLift(i)) {
      const int lift_index = ocp_discretizer.liftIndexAfterTimeStage(i);
      factorizer_.forwardRiccatiRecursion(kkt_matrix[i], kkt_residual[i],  
                                          ocp_discretizer.dt(i), lqr_policy_[i], 
                                          d[i], d.lift[lift_index]);
      factorizer_.forwardRiccatiRecursion(kkt_matrix.lift[lift_index], 
                                          kkt_residual.lift[lift_index], 
                                          ocp_discretizer.dt_lift(lift_index), 
                                          lqr_policy_.lift[lift_index], 
                                          d.lift[lift_index], d[i+1]);
    }
    else {
      factorizer_.forwardRiccatiRecursion(kkt_matrix[i], kkt_residual[i],  
                                          ocp_discretizer.dt(i), 
                                          lqr_policy_[i], d[i], d[i+1]);
    }
  }
}


void RiccatiRecursion::computeDirection(
    OCP& ocp, aligned_vector<Robot>& robots, 
    const RiccatiFactorization& factorization, const Solution& s, 
    Direction& d) {
  assert(robots.size() == nthreads_);
  const int N = ocp.discrete().N();
  const int N_impulse = ocp.discrete().N_impulse();
  const int N_lift = ocp.discrete().N_lift();
  const int N_all = N + 1 + 2*N_impulse + N_lift;
  #pragma omp parallel for num_threads(nthreads_)
  for (int i=0; i<N_all; ++i) {
    if (i < N) {
      RiccatiFactorizer::computeCostateDirection(factorization[i], d[i]);
      ocp[i].computeCondensedPrimalDirection(robots[omp_get_thread_num()], 
                                             ocp.discrete().dt(i), s[i], d[i]);
      if (ocp.discrete().isTimeStageBeforeImpulse(i+1)) {
        const int impulse_index = ocp.discrete().impulseIndexAfterTimeStage(i+1);
        d[i].setImpulseStatusByDimension(s[i].dimi());
        RiccatiFactorizer::computeLagrangeMultiplierDirection(
            factorization.switching[impulse_index], d[i]);
      }
      max_primal_step_sizes_.coeffRef(i) = ocp[i].maxPrimalStepSize();
      max_dual_step_sizes_.coeffRef(i) = ocp[i].maxDualStepSize();
    }
    else if (i == N) {
      RiccatiFactorizer::computeCostateDirection(factorization[N], d[N]);
      max_primal_step_sizes_.coeffRef(N) = ocp.terminal.maxPrimalStepSize();
      max_dual_step_sizes_.coeffRef(N) = ocp.terminal.maxDualStepSize();
    }
    else if (i < N + 1 + N_impulse) {
      const int impulse_index  = i - (N+1);
      RiccatiFactorizer::computeCostateDirection(
          factorization.impulse[impulse_index], d.impulse[impulse_index]);
      ocp.impulse[impulse_index].computeCondensedPrimalDirection(
          robots[omp_get_thread_num()], s.impulse[impulse_index], 
          d.impulse[impulse_index]);
      max_primal_step_sizes_.coeffRef(i) 
          = ocp.impulse[impulse_index].maxPrimalStepSize();
      max_dual_step_sizes_.coeffRef(i) 
          = ocp.impulse[impulse_index].maxDualStepSize();
    }
    else if (i < N + 1 + 2*N_impulse) {
      const int impulse_index  = i - (N+1+N_impulse);
      RiccatiFactorizer::computeCostateDirection(
          factorization.aux[impulse_index], d.aux[impulse_index]);
      ocp.aux[impulse_index].computeCondensedPrimalDirection(
          robots[omp_get_thread_num()], ocp.discrete().dt_aux(impulse_index), 
          s.aux[impulse_index], d.aux[impulse_index]);
      max_primal_step_sizes_.coeffRef(i) 
          = ocp.aux[impulse_index].maxPrimalStepSize();
      max_dual_step_sizes_.coeffRef(i) 
          = ocp.aux[impulse_index].maxDualStepSize();
    }
    else {
      const int lift_index = i - (N+1+2*N_impulse);
      RiccatiFactorizer::computeCostateDirection(factorization.lift[lift_index], 
                                                 d.lift[lift_index]);
      ocp.lift[lift_index].computeCondensedPrimalDirection(
          robots[omp_get_thread_num()], ocp.discrete().dt_lift(lift_index), 
          s.lift[lift_index], d.lift[lift_index]);
      const int time_stage_after_lift 
          = ocp.discrete().timeStageAfterLift(lift_index);
      if (ocp.discrete().isTimeStageBeforeImpulse(time_stage_after_lift)) {
        const int impulse_index
            = ocp.discrete().impulseIndexAfterTimeStage(time_stage_after_lift);
        d.lift[lift_index].setImpulseStatusByDimension(s.lift[lift_index].dimi());
        RiccatiFactorizer::computeLagrangeMultiplierDirection(
            factorization.switching[impulse_index], d.lift[lift_index]);
      }
      max_primal_step_sizes_.coeffRef(i) 
          = ocp.lift[lift_index].maxPrimalStepSize();
      max_dual_step_sizes_.coeffRef(i) 
          = ocp.lift[lift_index].maxDualStepSize();
    }
  }
  N_all_ = N_all;
}


double RiccatiRecursion::maxPrimalStepSize() const {
  return max_primal_step_sizes_.head(N_all_).minCoeff();
}


double RiccatiRecursion::maxDualStepSize() const {
  return max_dual_step_sizes_.head(N_all_).minCoeff();
}


void RiccatiRecursion::getStateFeedbackGain(const int time_stage, 
                                            Eigen::MatrixXd& Kq, 
                                            Eigen::MatrixXd& Kv) const {
  assert(time_stage >= 0);
  assert(time_stage < N_);
  Kq = lqr_policy_[time_stage].Kq();
  Kv = lqr_policy_[time_stage].Kv();
}

} // namespace idocp