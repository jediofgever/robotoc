#include <string>

#include <gtest/gtest.h>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/split_parnmpc.hpp"
#include "idocp/ocp/kkt_residual.hpp"
#include "idocp/ocp/kkt_matrix.hpp"
#include "idocp/cost/cost_function.hpp"
#include "idocp/cost/cost_function_data.hpp"
#include "idocp/cost/joint_space_cost.hpp"
#include "idocp/cost/contact_cost.hpp"
#include "idocp/constraints/constraints.hpp"
#include "idocp/constraints/joint_position_lower_limit.hpp"
#include "idocp/constraints/joint_position_upper_limit.hpp"
#include "idocp/constraints/joint_velocity_lower_limit.hpp"
#include "idocp/constraints/joint_velocity_upper_limit.hpp"


namespace idocp {

class FloatingBaseSplitParNMPCTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    urdf = "../urdf/anymal/anymal.urdf";
    std::vector<int> contact_frames = {14, 24, 34, 44};
    const double baum_a = std::abs(Eigen::VectorXd::Random(1)[0]);
    const double baum_b = std::abs(Eigen::VectorXd::Random(1)[0]);
    robot = Robot(urdf, contact_frames, baum_a, baum_b);
    std::random_device rnd;
    for (const auto frame : contact_frames) {
      contact_status.push_back(rnd()%2==0);
    }
    robot.setContactStatus(contact_status);
    s = SplitSolution::Random(robot);
    s_next = SplitSolution::Random(robot);
    s_tmp = SplitSolution::Random(robot);
    s_old = SplitSolution::Random(robot);
    s_new = SplitSolution::Random(robot);
    d = SplitDirection::Random(robot);
    d_prev = SplitDirection::Random(robot);
    dtau = std::abs(Eigen::VectorXd::Random(1)[0]);
    t = std::abs(Eigen::VectorXd::Random(1)[0]);
    q_prev = Eigen::VectorXd::Random(robot.dimq());
    robot.normalizeConfiguration(q_prev);
    v_prev = Eigen::VectorXd::Random(robot.dimv());
    dq_prev = Eigen::VectorXd::Random(robot.dimv());
    dv_prev = Eigen::VectorXd::Random(robot.dimv());
    robot.normalizeConfiguration(s_next.q);
    std::shared_ptr<JointSpaceCost> joint_cost = std::make_shared<JointSpaceCost>(robot);
    std::shared_ptr<ContactCost> contact_cost = std::make_shared<ContactCost>(robot);
    const Eigen::VectorXd q_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    Eigen::VectorXd q_ref = Eigen::VectorXd::Random(robot.dimq());
    robot.normalizeConfiguration(q_ref);
    const Eigen::VectorXd v_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(robot.dimv());
    const Eigen::VectorXd a_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    const Eigen::VectorXd a_ref = Eigen::VectorXd::Random(robot.dimv());
    const Eigen::VectorXd u_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    const Eigen::VectorXd u_ref = Eigen::VectorXd::Random(robot.dimv());
    const Eigen::VectorXd qf_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    const Eigen::VectorXd vf_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
    std::vector<Eigen::Vector3d> f_weight, f_ref;
    for (int i=0; i<robot.max_point_contacts(); ++i) {
      f_weight.push_back(Eigen::Vector3d::Random());
      f_ref.push_back(Eigen::Vector3d::Random());
    }
    joint_cost->set_q_weight(q_weight);
    joint_cost->set_q_ref(q_ref);
    joint_cost->set_v_weight(v_weight);
    joint_cost->set_v_ref(v_ref);
    joint_cost->set_a_weight(a_weight);
    joint_cost->set_a_ref(a_ref);
    joint_cost->set_u_weight(u_weight);
    joint_cost->set_u_ref(u_ref);
    joint_cost->set_qf_weight(qf_weight);
    joint_cost->set_vf_weight(vf_weight);
    contact_cost->set_f_weight(f_weight);
    contact_cost->set_f_ref(f_ref);
    cost = std::make_shared<CostFunction>();
    cost->push_back(joint_cost);
    cost->push_back(contact_cost);
    cost_data = CostFunctionData(robot);
    constraints = std::make_shared<Constraints>();
    auto joint_lower_limit = std::make_shared<JointPositionLowerLimit>(robot);
    auto joint_upper_limit = std::make_shared<JointPositionUpperLimit>(robot);
    auto velocity_lower_limit = std::make_shared<JointVelocityLowerLimit>(robot);
    auto velocity_upper_limit = std::make_shared<JointVelocityUpperLimit>(robot);
    constraints->push_back(joint_upper_limit); 
    constraints->push_back(joint_lower_limit);
    constraints->push_back(velocity_lower_limit); 
    constraints->push_back(velocity_upper_limit);
    constraints_data = constraints->createConstraintsData(robot);
    kkt_matrix = KKTMatrix(robot);
    kkt_residual = KKTResidual(robot);
    state_equation = StateEquation(robot);
    robot_dynamics = RobotDynamics(robot);
  }

  virtual void TearDown() {
  }

  double dtau, t;
  std::string urdf;
  Robot robot;
  std::vector<bool> contact_status;
  std::shared_ptr<CostFunction> cost;
  CostFunctionData cost_data;
  std::shared_ptr<Constraints> constraints;
  ConstraintsData constraints_data;
  SplitSolution s, s_next, s_tmp, s_old, s_new;
  SplitDirection d, d_prev;
  KKTMatrix kkt_matrix;
  KKTResidual kkt_residual;
  StateEquation state_equation;
  RobotDynamics robot_dynamics;
  Eigen::VectorXd q_prev, v_prev, dq_prev, dv_prev;
};


TEST_F(FloatingBaseSplitParNMPCTest, isFeasible) {
  SplitParNMPC parnmpc(robot, cost, constraints);
  std::cout << constraints_data.size() << std::endl;
  EXPECT_EQ(parnmpc.isFeasible(robot, s), 
            constraints->isFeasible(robot, constraints_data, s));
}


TEST_F(FloatingBaseSplitParNMPCTest, KKTErrorNormOnlyStateEquation) {
  auto empty_cost = std::make_shared<CostFunction>();
  auto empty_constraints = std::make_shared<Constraints>();
  robot.setContactStatus(std::vector<bool>({false, false, false, false}));
  kkt_residual.setContactStatus(robot);
  kkt_matrix.setContactStatus(robot);
  s.setContactStatus(robot);
  SplitParNMPC parnmpc(robot, empty_cost, empty_constraints);
  robot.RNEA(s.q, s.v, s.a, s.u);
  s.beta.setZero();
  s.mu_stack().setZero();
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  const double kkt_error = parnmpc.computeSquaredKKTErrorNorm(robot, t, dtau, q_prev, 
                                                       v_prev, s, s_next);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  Eigen::MatrixXd Jdiffminus = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd Jdiffplus = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.subtractConfiguration(q_prev, s.q, qdiff);
  robot.dSubtractdConfigurationMinus(q_prev, s.q, Jdiffminus);
  robot.dSubtractdConfigurationPlus(s.q, s_next.q, Jdiffplus);
  kkt_residual.Fq() = qdiff + dtau * s.v;
  kkt_residual.Fv() = v_prev - s.v + dtau * s.a;
  kkt_residual.lq() = Jdiffplus.transpose() * s_next.lmd 
                      + Jdiffminus.transpose() * s.lmd;
  kkt_residual.lv() = s_next.gmm - s.gmm + dtau * s.lmd;
  kkt_residual.la() = dtau * s.gmm;
  double kkt_error_ref = kkt_residual.Fq().squaredNorm()
                         + kkt_residual.Fv().squaredNorm()
                         + kkt_residual.lq().squaredNorm()
                         + kkt_residual.lv().squaredNorm()
                         + kkt_residual.la().squaredNorm()
                         + dtau*dtau*s.u.head(6).squaredNorm();
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
  auto pair = parnmpc.costAndViolation(robot, t, dtau, s);
  EXPECT_DOUBLE_EQ(pair.first, 0);
}


TEST_F(FloatingBaseSplitParNMPCTest, KKTErrorNormStateEquationAndInverseDynamics) {
  auto empty_cost = std::make_shared<CostFunction>();
  auto empty_constraints = std::make_shared<Constraints>();
  robot.setContactStatus(std::vector<bool>({false, false, false, false}));
  kkt_residual.setContactStatus(robot);
  kkt_matrix.setContactStatus(robot);
  s.setContactStatus(robot);
  SplitParNMPC parnmpc(robot, empty_cost, empty_constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  s.mu_stack().setZero();
  const double kkt_error = parnmpc.computeSquaredKKTErrorNorm(robot, t, dtau, q_prev, 
                                                       v_prev, s, s_next);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  Eigen::MatrixXd Jdiffminus = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd Jdiffplus = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.subtractConfiguration(q_prev, s.q, qdiff);
  robot.dSubtractdConfigurationMinus(q_prev, s.q, Jdiffminus);
  robot.dSubtractdConfigurationPlus(s.q, s_next.q, Jdiffplus);
  kkt_residual.Fq() = qdiff + dtau * s.v;
  kkt_residual.Fv() = v_prev - s.v + dtau * s.a;
  kkt_residual.lq() = Jdiffplus.transpose() * s_next.lmd 
                      + Jdiffminus.transpose() * s.lmd;
  kkt_residual.lv() = s_next.gmm - s.gmm + dtau * s.lmd;
  kkt_residual.la() = dtau * s.gmm;
  Eigen::VectorXd u_res = Eigen::VectorXd::Zero(robot.dimv());
  Eigen::MatrixXd du_dq = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd du_dv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd du_da = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.setContactForces(s.f);
  robot.RNEA(s.q, s.v, s.a, kkt_residual.u_res);
  kkt_residual.u_res -= s.u;
  robot.RNEADerivatives(s.q, s.v, s.a, du_dq, du_dv, du_da);
  robot.updateKinematics(s.q, s.v, s.a);
  kkt_residual.lq() += dtau * du_dq.transpose() * s.beta;
  kkt_residual.lv() += dtau * du_dv.transpose() * s.beta;
  kkt_residual.la() += dtau * du_da.transpose() * s.beta;
  kkt_residual.lu -= dtau * s.beta;
  double kkt_error_ref = kkt_residual.Fq().squaredNorm()
                         + kkt_residual.Fv().squaredNorm()
                         + kkt_residual.lq().squaredNorm()
                         + kkt_residual.lv().squaredNorm()
                         + kkt_residual.la().squaredNorm()
                         + kkt_residual.lu.squaredNorm()
                         + dtau*dtau*kkt_residual.u_res.squaredNorm()
                         + dtau*dtau*s.u.head(6).squaredNorm();
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
  auto pair = parnmpc.costAndViolation(robot, t, dtau, s);
  EXPECT_DOUBLE_EQ(pair.first, 0);
}


TEST_F(FloatingBaseSplitParNMPCTest, KKTErrorNormStateEquationAndRobotDynamics) {
  auto empty_cost = std::make_shared<CostFunction>();
  auto empty_constraints = std::make_shared<Constraints>();
  robot.setContactStatus(contact_status);
  kkt_residual.setContactStatus(robot);
  kkt_matrix.setContactStatus(robot);
  s.setContactStatus(robot);
  SplitParNMPC parnmpc(robot, empty_cost, empty_constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  const double kkt_error = parnmpc.computeSquaredKKTErrorNorm(robot, t, dtau, q_prev, 
                                                       v_prev, s, s_next);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  Eigen::MatrixXd Jdiffminus = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd Jdiffplus = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot.subtractConfiguration(q_prev, s.q, qdiff);
  robot.dSubtractdConfigurationMinus(q_prev, s.q, Jdiffminus);
  robot.dSubtractdConfigurationPlus(s.q, s_next.q, Jdiffplus);
  kkt_residual.Fq() = qdiff + dtau * s.v;
  kkt_residual.Fv() = v_prev - s.v + dtau * s.a;
  kkt_residual.lq() = Jdiffplus.transpose() * s_next.lmd 
                      + Jdiffminus.transpose() * s.lmd;
  kkt_residual.lv() = s_next.gmm - s.gmm + dtau * s.lmd;
  kkt_residual.la() = dtau * s.gmm;
  Eigen::VectorXd u_res = Eigen::VectorXd::Zero(robot.dimv());
  Eigen::MatrixXd du_dq = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd du_dv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd du_da = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd du_df = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimf());
  robot.setContactForces(s.f);
  robot.RNEA(s.q, s.v, s.a, kkt_residual.u_res);
  kkt_residual.u_res -= s.u;
  robot.RNEADerivatives(s.q, s.v, s.a, du_dq, du_dv, du_da);
  robot.updateKinematics(s.q, s.v, s.a);
  robot.dRNEAPartialdFext(du_df);
  kkt_residual.lq() += dtau * du_dq.transpose() * s.beta;
  kkt_residual.lv() += dtau * du_dv.transpose() * s.beta;
  kkt_residual.la() += dtau * du_da.transpose() * s.beta;
  kkt_residual.lf() += dtau * du_df.transpose() * s.beta;
  kkt_residual.lu -= dtau * s.beta;
  robot.computeBaumgarteResidual(dtau, kkt_residual.C().tail(robot.dimf()));
  robot.computeBaumgarteDerivatives(dtau, kkt_matrix.Cq().bottomRows(robot.dimf()), 
                                    kkt_matrix.Cv().bottomRows(robot.dimf()), 
                                    kkt_matrix.Ca().bottomRows(robot.dimf()));
  kkt_residual.lq() += kkt_matrix.Cq().transpose() * s.mu_stack();
  kkt_residual.lv() += kkt_matrix.Cv().transpose() * s.mu_stack();
  kkt_residual.la() += kkt_matrix.Ca().transpose() * s.mu_stack();
  kkt_residual.lu.head(6) += dtau * s.mu_stack().head(6);
  kkt_residual.C().head(6) = dtau * s.u.head(6);
  double kkt_error_ref = kkt_residual.Fq().squaredNorm()
                         + kkt_residual.Fv().squaredNorm()
                         + kkt_residual.lq().squaredNorm()
                         + kkt_residual.lv().squaredNorm()
                         + kkt_residual.la().squaredNorm()
                         + kkt_residual.lf().squaredNorm()
                         + kkt_residual.lu.squaredNorm()
                         + dtau*dtau*kkt_residual.u_res.squaredNorm()
                         + kkt_residual.C().squaredNorm();
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
  auto pair = parnmpc.costAndViolation(robot, t, dtau, s);
  EXPECT_DOUBLE_EQ(pair.first, 0);
  const double violation_ref = kkt_residual.Fx().lpNorm<1>() 
                                + dtau * kkt_residual.u_res.lpNorm<1>()
                                + kkt_residual.C().lpNorm<1>();
  EXPECT_DOUBLE_EQ(pair.second, violation_ref);
}


TEST_F(FloatingBaseSplitParNMPCTest, KKTErrorNormEmptyCost) {
  auto empty_cost = std::make_shared<CostFunction>();
  SplitParNMPC parnmpc(robot, empty_cost, constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  const double kkt_error = parnmpc.computeSquaredKKTErrorNorm(robot, t, dtau, q_prev, 
                                                       v_prev, s, s_next);
  kkt_residual.setContactStatus(robot);
  kkt_matrix.setContactStatus(robot);
  s.setContactStatus(robot);
  constraints->augmentDualResidual(robot, constraints_data, dtau, kkt_residual);
  constraints->augmentDualResidual(robot, constraints_data, dtau, kkt_residual.lu);
  state_equation.linearizeBackwardEuler(robot, dtau, q_prev, v_prev, s, 
                                        s_next.lmd, s_next.gmm, s_next.q, 
                                        kkt_matrix, kkt_residual);
  robot_dynamics.augmentRobotDynamics(robot, dtau, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = kkt_residual.squaredKKTErrorNorm(dtau);
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


TEST_F(FloatingBaseSplitParNMPCTest, KKTErrorNormEmptyConstraints) {
  auto empty_constraints = std::make_shared<Constraints>();
  SplitParNMPC parnmpc(robot, cost, empty_constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  const double kkt_error = parnmpc.computeSquaredKKTErrorNorm(robot, t, dtau, q_prev, 
                                                       v_prev, s, s_next);
  kkt_residual.setContactStatus(robot);
  kkt_matrix.setContactStatus(robot);
  s.setContactStatus(robot);
  cost->computeStageCostDerivatives(robot, cost_data, t, dtau, s, kkt_residual);
  cost->lu(robot, cost_data, t, dtau, s.u, kkt_residual.lu);
  state_equation.linearizeBackwardEuler(robot, dtau, q_prev, v_prev, s, 
                                        s_next.lmd, s_next.gmm, s_next.q, 
                                        kkt_matrix, kkt_residual);
  robot_dynamics.augmentRobotDynamics(robot, dtau, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = kkt_residual.squaredKKTErrorNorm(dtau);
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


TEST_F(FloatingBaseSplitParNMPCTest, KKTErrorNorm) {
  SplitParNMPC parnmpc(robot, cost, constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  const double kkt_error = parnmpc.computeSquaredKKTErrorNorm(robot, t, dtau, q_prev, 
                                                       v_prev, s, s_next);
  kkt_residual.setContactStatus(robot);
  kkt_matrix.setContactStatus(robot);
  s.setContactStatus(robot);
  robot.updateKinematics(s.q, s.v, s.a);
  cost->computeStageCostDerivatives(robot, cost_data, t, dtau, s, kkt_residual);
  cost->lu(robot, cost_data, t, dtau, s.u, kkt_residual.lu);
  constraints->augmentDualResidual(robot, constraints_data, dtau, kkt_residual);
  constraints->augmentDualResidual(robot, constraints_data, dtau, kkt_residual.lu);
  state_equation.linearizeBackwardEuler(robot, dtau, q_prev, v_prev, s, 
                                        s_next.lmd, s_next.gmm, s_next.q, 
                                        kkt_matrix, kkt_residual);
  robot_dynamics.augmentRobotDynamics(robot, dtau, s, kkt_matrix, kkt_residual);
  double kkt_error_ref = kkt_residual.squaredKKTErrorNorm(dtau);
  kkt_error_ref += constraints->squaredKKTErrorNorm(robot, constraints_data, dtau, s);
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
}


TEST_F(FloatingBaseSplitParNMPCTest, costAndViolation) {
  SplitParNMPC parnmpc(robot, cost, constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  const double kkt_error = parnmpc.computeSquaredKKTErrorNorm(robot, t, dtau, q_prev, 
                                                       v_prev, s, s_next);
  const auto pair = parnmpc.costAndViolation(robot, t, dtau, s); 
  const double cost_ref 
      = cost->l(robot, cost_data, t, dtau, s) 
          + constraints->costSlackBarrier(constraints_data);
  EXPECT_DOUBLE_EQ(pair.first, cost_ref);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  robot.subtractConfiguration(q_prev, s.q, qdiff);
  kkt_residual.Fq() = qdiff + dtau * s.v;
  kkt_residual.Fv() = v_prev - s.v + dtau * s.a;
  robot.setContactForces(s.f);
  robot.RNEA(s.q, s.v, s.a, kkt_residual.u_res);
  kkt_residual.u_res -= s.u;
  robot.updateKinematics(s.q, s.v, s.a);
  robot.computeBaumgarteResidual(dtau, kkt_residual.C());
  const double violation_ref 
      = kkt_residual.Fq().lpNorm<1>() + kkt_residual.Fv().lpNorm<1>() 
          + dtau * kkt_residual.u_res.lpNorm<1>() 
          + kkt_residual.C().head(robot.dimf()).lpNorm<1>()
          + dtau * s.u.head(6).lpNorm<1>()
          + constraints->residualL1Nrom(robot, constraints_data, dtau, s);
  EXPECT_DOUBLE_EQ(pair.second, violation_ref);
}


TEST_F(FloatingBaseSplitParNMPCTest, costAndViolationWithStepSizeInitial) {
  SplitParNMPC parnmpc(robot, cost, constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  const double step_size = 0.3;
  const auto pair = parnmpc.costAndViolation(robot, step_size, t, dtau, q_prev, 
                                             v_prev, s, d, s_new); 
  robot.integrateConfiguration(s.q, d.dq(), step_size, s_new.q);
  s_new.v = s.v + step_size * d.dv();
  s_new.a = s.a + step_size * d.da();
  s_new.f_stack() = s.f_stack() + step_size * d.df();
  s_new.u = s.u + step_size * d.du;
  const double cost_ref 
      = cost->l(robot, cost_data, t, dtau, s_new) 
          + constraints->costSlackBarrier(constraints_data, step_size);
  EXPECT_DOUBLE_EQ(pair.first, cost_ref);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  robot.subtractConfiguration(q_prev, s_new.q, qdiff);
  kkt_residual.Fq() = qdiff + dtau * s_new.v;
  kkt_residual.Fv() = v_prev - s_new.v + dtau * s_new.a;
  robot.setContactForces(s_new.f);
  robot.RNEA(s_new.q, s_new.v, s_new.a, kkt_residual.u_res);
  kkt_residual.u_res -= s_new.u;
  robot.updateKinematics(s_new.q, s_new.v, s_new.a);
  robot.computeBaumgarteResidual(dtau, kkt_residual.C());
  const double violation_ref 
      = kkt_residual.Fq().lpNorm<1>() + kkt_residual.Fv().lpNorm<1>() 
          + dtau * kkt_residual.u_res.lpNorm<1>() 
          + kkt_residual.C().head(robot.dimf()).lpNorm<1>()
          + dtau * s_new.u.head(6).lpNorm<1>()
          + constraints->residualL1Nrom(robot, constraints_data, dtau, s_new);
  EXPECT_DOUBLE_EQ(pair.second, violation_ref);
}


TEST_F(FloatingBaseSplitParNMPCTest, costAndViolationWithStepSize) {
  SplitParNMPC parnmpc(robot, cost, constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  const double step_size = 0.3;
  const auto pair = parnmpc.costAndViolation(robot, step_size, t, dtau, s_old, 
                                             d_prev, s, d, s_new); 
  robot.integrateConfiguration(s.q, d.dq(), step_size, s_new.q);
  s_new.v = s.v + step_size * d.dv();
  s_new.a = s.a + step_size * d.da();
  s_new.f_stack() = s.f_stack() + step_size * d.df();
  s_new.u = s.u + step_size * d.du;
  const double cost_ref 
      = cost->l(robot, cost_data, t, dtau, s_new) 
          + constraints->costSlackBarrier(constraints_data, step_size);
  EXPECT_DOUBLE_EQ(pair.first, cost_ref);
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  robot.subtractConfiguration(s_old.q, s_new.q, qdiff);
  kkt_residual.Fq() = qdiff + dtau * s_new.v + step_size * d_prev.dq();
  kkt_residual.Fv() = s_old.v - s_new.v + dtau * s_new.a + step_size * d_prev.dv();
  robot.setContactForces(s_new.f);
  robot.RNEA(s_new.q, s_new.v, s_new.a, kkt_residual.u_res);
  kkt_residual.u_res -= s_new.u;
  robot.updateKinematics(s_new.q, s_new.v, s_new.a);
  robot.computeBaumgarteResidual(dtau, kkt_residual.C());
  const double violation_ref 
      = kkt_residual.Fq().lpNorm<1>() + kkt_residual.Fv().lpNorm<1>() 
          + dtau * kkt_residual.u_res.lpNorm<1>() 
          + kkt_residual.C().head(robot.dimf()).lpNorm<1>()
          + dtau * s_new.u.head(6).lpNorm<1>()
          + constraints->residualL1Nrom(robot, constraints_data, dtau, s_new);
  EXPECT_DOUBLE_EQ(pair.second, violation_ref);
}


TEST_F(FloatingBaseSplitParNMPCTest, coarseUpdate) {
  s.setContactStatus(robot);
  SplitParNMPC parnmpc(robot, cost, constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  constraints->setSlackAndDual(robot, constraints_data, dtau, s);
  Eigen::MatrixXd aux_mat_seed = Eigen::MatrixXd::Random(2*robot.dimv(), 
                                                         2*robot.dimv());
  const Eigen::MatrixXd aux_mat_next = aux_mat_seed.transpose() * aux_mat_seed;
  SplitSolution s_new_coarse(robot);
  s_new_coarse.setContactStatus(robot);
  parnmpc.coarseUpdate(robot, t, dtau, q_prev, v_prev, s, s_next, 
                       aux_mat_next, d, s_new_coarse);
  // coarse update ref
  kkt_residual.lu.setZero();
  kkt_matrix.Quu.setZero();
  cost->lu(robot, cost_data, t, dtau, s.u, kkt_residual.lu);
  constraints->augmentDualResidual(robot, constraints_data, dtau, 
                                    kkt_residual.lu);
  cost->luu(robot, cost_data, t, dtau, s.u, kkt_matrix.Quu);
  constraints->condenseSlackAndDual(robot, constraints_data, dtau, s.u, 
                                     kkt_matrix.Quu, kkt_residual.lu);
  robot_dynamics.condenseRobotDynamics(robot, dtau, s, kkt_matrix, kkt_residual);
  cost->computeStageCostDerivatives(robot, cost_data, t, dtau, s, 
                                     kkt_residual);
  constraints->augmentDualResidual(robot, constraints_data, dtau, 
                                   kkt_residual);
  state_equation.linearizeBackwardEuler(robot, dtau, q_prev, v_prev, s, 
                                        s_next.lmd, s_next.gmm, s_next.q, 
                                        kkt_matrix, kkt_residual);
  cost->computeStageCostHessian(robot, cost_data, t, dtau, s, kkt_matrix);
  constraints->condenseSlackAndDual(robot, constraints_data, dtau, s, 
                                     kkt_matrix, kkt_residual);
  kkt_matrix.Qxx().noalias() += aux_mat_next;
  kkt_matrix.symmetrize(); 
  const int dimKKT = kkt_residual.dimKKT();
  Eigen::MatrixXd kkt_matrix_inverse = Eigen::MatrixXd::Zero(dimKKT, dimKKT);
  kkt_matrix.invert(dtau, kkt_matrix_inverse.topLeftCorner(dimKKT, dimKKT));
  SplitDirection d_ref(robot);
  d_ref.split_direction() = kkt_matrix_inverse.topLeftCorner(dimKKT, dimKKT)
                              * kkt_residual.KKT_residual();
  SplitSolution s_new_coarse_ref(robot);
  s_new_coarse_ref.setContactStatus(robot);
  s_new_coarse_ref.lmd = s.lmd - d_ref.dlmd();
  s_new_coarse_ref.gmm = s.gmm - d_ref.dgmm();
  s_new_coarse_ref.mu_stack() = s.mu_stack() - d_ref.dmu();
  s_new_coarse_ref.a = s.a - d_ref.da();
  s_new_coarse_ref.f_stack() = s.f_stack() - d_ref.df();
  robot.integrateConfiguration(s.q, d_ref.dq(), -1, s_new_coarse_ref.q);
  s_new_coarse_ref.v = s.v - d_ref.dv();
  EXPECT_TRUE(s_new_coarse.lmd.isApprox(s_new_coarse_ref.lmd));
  EXPECT_TRUE(s_new_coarse.gmm.isApprox(s_new_coarse_ref.gmm));
  EXPECT_TRUE(s_new_coarse.mu_stack().isApprox(s_new_coarse_ref.mu_stack()));
  EXPECT_TRUE(s_new_coarse.a.isApprox(s_new_coarse_ref.a));
  EXPECT_TRUE(s_new_coarse.f_stack().isApprox(s_new_coarse_ref.f_stack()));
  EXPECT_TRUE(s_new_coarse.q.isApprox(s_new_coarse_ref.q));
  EXPECT_TRUE(s_new_coarse.v.isApprox(s_new_coarse_ref.v));
  const int dimx = 2*robot.dimv();
  Eigen::MatrixXd aux_mat = Eigen::MatrixXd::Zero(dimx, dimx);
  parnmpc.getAuxiliaryMatrix(aux_mat);
  Eigen::MatrixXd aux_mat_ref = - kkt_matrix_inverse.topLeftCorner(dimx, dimx);
  EXPECT_TRUE(aux_mat.isApprox(aux_mat_ref));
  parnmpc.backwardCorrectionSerial(robot, s_old, s_new, s_new_coarse);
  Eigen::VectorXd x_res = Eigen::VectorXd::Zero(dimx);
  x_res.head(robot.dimv()) = s_new.lmd - s_old.lmd;
  x_res.tail(robot.dimv()) = s_new.gmm - s_old.gmm;
  Eigen::VectorXd dx = kkt_matrix_inverse.topRightCorner(dimx, dimx) * x_res;
  s_new_coarse_ref.lmd -= dx.head(robot.dimv());
  s_new_coarse_ref.gmm -= dx.tail(robot.dimv());
  EXPECT_TRUE(s_new_coarse.lmd.isApprox(s_new_coarse_ref.lmd));
  EXPECT_TRUE(s_new_coarse.gmm.isApprox(s_new_coarse_ref.gmm));
  parnmpc.backwardCorrectionParallel(robot, d, s_new_coarse);
  d_ref.split_direction().tail(dimKKT-dimx)
      = kkt_matrix_inverse.bottomRightCorner(dimKKT-dimx, dimx) * x_res;
  s_new_coarse_ref.mu_stack().noalias() -= d_ref.dmu();
  s_new_coarse_ref.a.noalias() -= d_ref.da();
  s_new_coarse_ref.f_stack().noalias() -= d_ref.df();
  robot.integrateConfiguration(d_ref.dq(), -1, s_new_coarse_ref.q);
  s_new_coarse_ref.v.noalias() -= d_ref.dv();
  EXPECT_TRUE(s_new_coarse.mu_stack().isApprox(s_new_coarse_ref.mu_stack()));
  EXPECT_TRUE(s_new_coarse.a.isApprox(s_new_coarse_ref.a));
  EXPECT_TRUE(s_new_coarse.f_stack().isApprox(s_new_coarse_ref.f_stack()));
  EXPECT_TRUE(s_new_coarse.q.isApprox(s_new_coarse_ref.q));
  EXPECT_TRUE(s_new_coarse.v.isApprox(s_new_coarse_ref.v));
  parnmpc.forwardCorrectionSerial(robot, s_old, s_new, s_new_coarse);
  robot.subtractConfiguration(s_new.q, s_old.q, x_res.head(robot.dimv()));
  x_res.tail(robot.dimv()) = s_new.v - s_old.v;
  dx = kkt_matrix_inverse.bottomLeftCorner(dimx, dimx) * x_res;
  robot.integrateConfiguration(dx.head(robot.dimv()), -1, s_new_coarse_ref.q);
  s_new_coarse_ref.v -= dx.tail(robot.dimv());
  EXPECT_TRUE(s_new_coarse.q.isApprox(s_new_coarse_ref.q));
  EXPECT_TRUE(s_new_coarse.v.isApprox(s_new_coarse_ref.v));
  parnmpc.forwardCorrectionParallel(robot, d, s_new_coarse);
  d_ref.split_direction().head(dimKKT-dimx) = kkt_matrix_inverse.topLeftCorner(dimKKT-dimx, dimx) * x_res;
  s_new_coarse_ref.lmd -= d_ref.dlmd();
  s_new_coarse_ref.gmm -= d_ref.dgmm();
  s_new_coarse_ref.mu_stack() -= d_ref.dmu();
  s_new_coarse_ref.a -= d_ref.da();
  s_new_coarse_ref.f_stack() -= d_ref.df();
  EXPECT_TRUE(s_new_coarse.lmd.isApprox(s_new_coarse_ref.lmd));
  EXPECT_TRUE(s_new_coarse.gmm.isApprox(s_new_coarse_ref.gmm));
  EXPECT_TRUE(s_new_coarse.mu_stack().isApprox(s_new_coarse_ref.mu_stack()));
  EXPECT_TRUE(s_new_coarse.a.isApprox(s_new_coarse_ref.a));
  EXPECT_TRUE(s_new_coarse.f_stack().isApprox(s_new_coarse_ref.f_stack()));
  EXPECT_TRUE(s_new_coarse.q.isApprox(s_new_coarse_ref.q));
  EXPECT_TRUE(s_new_coarse.v.isApprox(s_new_coarse_ref.v));

  double condensed_KKT_ref = kkt_residual.KKT_residual().squaredNorm();
  condensed_KKT_ref += constraints->squaredKKTErrorNorm(robot, constraints_data, dtau, s);
  EXPECT_DOUBLE_EQ(condensed_KKT_ref, parnmpc.condensedSquaredKKTErrorNorm(robot, t, dtau, s));

  Eigen::MatrixXd Kuq = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd Kuv = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  parnmpc.getStateFeedbackGain(Kuq, Kuv);
  const int dimv = robot.dimv();
  const int dimf = robot.dimf();
  const int dimc = robot.dim_passive()+robot.dimf();
  const Eigen::MatrixXd da_dq = kkt_matrix_inverse.block(dimx+dimc, dimx+dimc+dimv+dimf, dimv, dimv);
  const Eigen::MatrixXd da_dv = kkt_matrix_inverse.block(dimx+dimc, dimx+dimc+dimv+dimf+dimv, dimv, dimv);
  const Eigen::MatrixXd df_dq = kkt_matrix_inverse.block(dimx+dimc+dimv, dimx+dimc+dimv+dimf, dimf, dimv);
  const Eigen::MatrixXd df_dv = kkt_matrix_inverse.block(dimx+dimc+dimv, dimx+dimc+dimv+dimf+dimv, dimf, dimv);
  Eigen::MatrixXd Kuq_ref = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  Eigen::MatrixXd Kuv_ref = Eigen::MatrixXd::Zero(robot.dimv(), robot.dimv());
  robot_dynamics.getStateFeedbackGain(da_dq, da_dv, df_dq, df_dv, Kuq_ref, Kuv_ref);
  EXPECT_TRUE(Kuq.isApprox(Kuq_ref));
  EXPECT_TRUE(Kuv.isApprox(Kuv_ref));
}


TEST_F(FloatingBaseSplitParNMPCTest, computePrimalDualDirection) {
  SplitParNMPC parnmpc(robot, cost, constraints);
  parnmpc.initConstraints(robot, 2, dtau, s);
  parnmpc.computePrimalAndDualDirection(robot, dtau, s, s_new, d);
  EXPECT_TRUE(d.dlmd().isApprox(s_new.lmd-s.lmd));
  EXPECT_TRUE(d.dgmm().isApprox(s_new.gmm-s.gmm));
  EXPECT_TRUE(d.dmu().isApprox(s_new.mu_stack()-s.mu_stack()));
  EXPECT_TRUE(d.da().isApprox(s_new.a-s.a));
  EXPECT_TRUE(d.df().isApprox(s_new.f_stack()-s.f_stack()));
  Eigen::VectorXd qdiff = Eigen::VectorXd::Zero(robot.dimv());
  robot.subtractConfiguration(s_new.q, s.q, qdiff);
  EXPECT_TRUE(d.dq().isApprox(qdiff));
  EXPECT_TRUE(d.dv().isApprox(s_new.v-s.v));
}


} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}