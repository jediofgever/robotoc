#include <string>
#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/robot/impulse_status.hpp"
#include "idocp/robot/contact_status.hpp"
#include "idocp/impulse/split_impulse_ocp.hpp"
#include "idocp/impulse/impulse_split_solution.hpp"
#include "idocp/impulse/impulse_split_direction.hpp"
#include "idocp/impulse/impulse_kkt_residual.hpp"
#include "idocp/impulse/impulse_kkt_matrix.hpp"
#include "idocp/impulse/impulse_dynamics_forward_euler.hpp"
#include "idocp/cost/impulse_cost_function.hpp"
#include "idocp/cost/cost_function_data.hpp"
#include "idocp/cost/joint_space_impulse_cost.hpp"
#include "idocp/cost/impulse_force_cost.hpp"
#include "idocp/constraints/impulse_constraints.hpp"
#include "idocp/constraints/impulse_normal_force.hpp"


namespace idocp {

class SplitImpulseOCPTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    fixed_base_urdf = "../urdf/iiwa14/iiwa14.urdf";
    floating_base_urdf = "../urdf/anymal/anymal.urdf";
  }

  virtual void TearDown() {
  }

  static std::shared_ptr<ImpulseCostFunction> createCost(const Robot& robot);

  static std::shared_ptr<ImpulseConstraints> createConstraints(const Robot& robot);

  static ImpulseSplitSolution generateFeasibleSolution(
      Robot& robot, const ImpulseStatus& impulse_status,
      const std::shared_ptr<ImpulseConstraints>& constraints);

  static void testLinearizeOCP(
      Robot& robot, const ImpulseStatus& impulse_status, 
      const std::shared_ptr<ImpulseCostFunction>& cost,
      const std::shared_ptr<ImpulseConstraints>& constraints);

  static void testComputeKKTResidual(
      Robot& robot, const ImpulseStatus& impulse_status, 
      const std::shared_ptr<ImpulseCostFunction>& cost,
      const std::shared_ptr<ImpulseConstraints>& constraints);

  static void testCostAndConstraintViolation(
      Robot& robot, const ImpulseStatus& impulse_status, 
      const std::shared_ptr<ImpulseCostFunction>& cost,
      const std::shared_ptr<ImpulseConstraints>& constraints);

  std::string fixed_base_urdf, floating_base_urdf;
};


std::shared_ptr<ImpulseCostFunction> SplitImpulseOCPTest::createCost(const Robot& robot) {
  auto joint_cost = std::make_shared<JointSpaceImpulseCost>(robot);
  const Eigen::VectorXd q_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  Eigen::VectorXd q_ref = Eigen::VectorXd::Random(robot.dimq());
  robot.normalizeConfiguration(q_ref);
  const Eigen::VectorXd v_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(robot.dimv());
  const Eigen::VectorXd dv_weight = Eigen::VectorXd::Random(robot.dimv()).array().abs();
  const Eigen::VectorXd dv_ref = Eigen::VectorXd::Random(robot.dimv());
  joint_cost->set_q_weight(q_weight);
  joint_cost->set_q_ref(q_ref);
  joint_cost->set_v_weight(v_weight);
  joint_cost->set_v_ref(v_ref);
  joint_cost->set_dv_weight(dv_weight);
  joint_cost->set_dv_ref(dv_ref);
  auto impulse_force_cost = std::make_shared<idocp::ImpulseForceCost>(robot);
  std::vector<Eigen::Vector3d> f_weight;
  for (int i=0; i<robot.max_point_contacts(); ++i) {
    f_weight.push_back(Eigen::Vector3d::Constant(0.001));
  }
  impulse_force_cost->set_f_weight(f_weight);
  auto cost = std::make_shared<ImpulseCostFunction>();
  cost->push_back(joint_cost);
  cost->push_back(impulse_force_cost);
  return cost;
}


std::shared_ptr<ImpulseConstraints> SplitImpulseOCPTest::createConstraints(const Robot& robot) {
  auto impulse_normal_force = std::make_shared<ImpulseNormalForce>(robot);
  auto constraints = std::make_shared<ImpulseConstraints>();
  constraints->push_back(impulse_normal_force);
  return constraints;
}


ImpulseSplitSolution SplitImpulseOCPTest::generateFeasibleSolution(
    Robot& robot, const ImpulseStatus& impulse_status,
    const std::shared_ptr<ImpulseConstraints>& constraints) {
  auto data = constraints->createConstraintsData(robot);
  ImpulseSplitSolution s = ImpulseSplitSolution::Random(robot, impulse_status);
  while (!constraints->isFeasible(robot, data, s)) {
    s = ImpulseSplitSolution::Random(robot, impulse_status);
  }
  return s;
}


void SplitImpulseOCPTest::testLinearizeOCP(
    Robot& robot, const ImpulseStatus& impulse_status, 
    const std::shared_ptr<ImpulseCostFunction>& cost,
    const std::shared_ptr<ImpulseConstraints>& constraints) {
  const SplitSolution s_prev = SplitSolution::Random(robot);
  const ImpulseSplitSolution s = generateFeasibleSolution(robot, impulse_status, constraints);
  const SplitSolution s_next = SplitSolution::Random(robot);
  SplitImpulseOCP ocp(robot, cost, constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  ocp.initConstraints(robot, s);
  ImpulseKKTMatrix kkt_matrix(robot);
  ImpulseKKTResidual kkt_residual(robot);
  ocp.linearizeOCP(robot, impulse_status, t, s_prev.q, s, s_next, kkt_matrix, kkt_residual);
  ImpulseKKTMatrix kkt_matrix_ref(robot);
  kkt_matrix_ref.setImpulseStatus(impulse_status);
  ImpulseKKTResidual kkt_residual_ref(robot);
  kkt_residual_ref.setImpulseStatus(impulse_status);
  auto cost_data = cost->createCostFunctionData(robot);
  auto constraints_data = constraints->createConstraintsData(robot);
  constraints->setSlackAndDual(robot, constraints_data, s);
  const Eigen::VectorXd v_after_impulse = s.v + s.dv;
  robot.updateKinematics(s.q, v_after_impulse);
  cost->computeStageCostDerivatives(robot, cost_data, t, s, kkt_residual_ref);
  cost->computeStageCostHessian(robot, cost_data, t, s, kkt_matrix_ref);
  constraints->augmentDualResidual(robot, constraints_data, s, kkt_residual_ref);
  constraints->condenseSlackAndDual(robot, constraints_data, s, kkt_matrix_ref, kkt_residual_ref);
  stateequation::LinearizeImpulseForwardEuler(robot, s_prev.q, s, s_next, kkt_matrix_ref, kkt_residual_ref);
  ImpulseDynamicsForwardEuler id(robot);
  robot.updateKinematics(s.q, v_after_impulse);
  id.linearizeImpulseDynamics(robot, impulse_status, s, kkt_matrix_ref, kkt_residual_ref);
  id.condenseImpulseDynamics(robot, impulse_status, kkt_matrix_ref, kkt_residual_ref);
  EXPECT_TRUE(kkt_matrix.isApprox(kkt_matrix_ref));
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
  ImpulseSplitDirection d = ImpulseSplitDirection::Random(robot, impulse_status);
  auto d_ref = d;
  const SplitDirection d_next = SplitDirection::Random(robot);
  ocp.computeCondensedPrimalDirection(robot, s, d);
  id.computeCondensedPrimalDirection(robot, d_ref);
  constraints->computeSlackAndDualDirection(robot, constraints_data, s, d_ref);
  EXPECT_TRUE(d.isApprox(d_ref));
  EXPECT_DOUBLE_EQ(ocp.maxPrimalStepSize(), constraints->maxSlackStepSize(constraints_data));
  EXPECT_DOUBLE_EQ(ocp.maxDualStepSize(), constraints->maxDualStepSize(constraints_data));
  ocp.computeCondensedDualDirection(robot, kkt_matrix, kkt_residual, d_next, d);
  id.computeCondensedDualDirection(robot, kkt_matrix, kkt_residual, d_next.dgmm(), d_ref);
  EXPECT_TRUE(d.isApprox(d_ref));
  const double step_size = std::abs(Eigen::VectorXd::Random(1)[0]);
  auto s_updated = s;
  auto s_updated_ref = s;
  ocp.updatePrimal(robot, step_size, d, s_updated);
  s_updated_ref.integrate(robot, step_size, d);
  constraints->updateSlack(constraints_data, step_size);
  EXPECT_TRUE(s_updated.isApprox(s_updated_ref));
}


void SplitImpulseOCPTest::testComputeKKTResidual(
    Robot& robot, const ImpulseStatus& impulse_status, 
    const std::shared_ptr<ImpulseCostFunction>& cost,
    const std::shared_ptr<ImpulseConstraints>& constraints) {
  const SplitSolution s_prev = SplitSolution::Random(robot);
  const ImpulseSplitSolution s = generateFeasibleSolution(robot, impulse_status, constraints);
  const SplitSolution s_next = SplitSolution::Random(robot);
  SplitImpulseOCP ocp(robot, cost, constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  ocp.initConstraints(robot, s);
  ImpulseKKTMatrix kkt_matrix(robot);
  ImpulseKKTResidual kkt_residual(robot);
  ocp.computeKKTResidual(robot, impulse_status, t, s_prev.q, s, s_next, kkt_matrix, kkt_residual);
  const double kkt_error = ocp.squaredNormKKTResidual(kkt_residual);
  ImpulseKKTMatrix kkt_matrix_ref(robot);
  kkt_matrix_ref.setImpulseStatus(impulse_status);
  ImpulseKKTResidual kkt_residual_ref(robot);
  kkt_residual_ref.setImpulseStatus(impulse_status);
  auto cost_data = cost->createCostFunctionData(robot);
  auto constraints_data = constraints->createConstraintsData(robot);
  constraints->setSlackAndDual(robot, constraints_data, s);
  const Eigen::VectorXd v_after_impulse = s.v + s.dv;
  robot.updateKinematics(s.q, v_after_impulse);
  cost->computeStageCostDerivatives(robot, cost_data, t, s, kkt_residual_ref);
  constraints->augmentDualResidual(robot, constraints_data, s, kkt_residual_ref);
  stateequation::LinearizeImpulseForwardEuler(robot, s_prev.q, s, s_next, kkt_matrix_ref, kkt_residual_ref);
  ImpulseDynamicsForwardEuler id(robot);
  robot.updateKinematics(s.q, v_after_impulse);
  id.linearizeImpulseDynamics(robot, impulse_status, s, kkt_matrix_ref, kkt_residual_ref);
  const double kkt_error_ref = kkt_residual_ref.Fx().squaredNorm()
                                + kkt_residual_ref.lx().squaredNorm()
                                + kkt_residual_ref.ldv.squaredNorm()
                                + kkt_residual_ref.lf().squaredNorm()
                                + id.squaredNormImpulseDynamicsResidual(kkt_residual_ref)
                                + constraints->squaredNormPrimalAndDualResidual(constraints_data);
  EXPECT_DOUBLE_EQ(kkt_error, kkt_error_ref);
  EXPECT_TRUE(kkt_matrix.isApprox(kkt_matrix_ref));
  EXPECT_TRUE(kkt_residual.isApprox(kkt_residual_ref));
}


void SplitImpulseOCPTest::testCostAndConstraintViolation(
    Robot& robot, const ImpulseStatus& impulse_status, 
    const std::shared_ptr<ImpulseCostFunction>& cost,
    const std::shared_ptr<ImpulseConstraints>& constraints) {
  const SplitSolution s_prev = SplitSolution::Random(robot);
  const ImpulseSplitSolution s = generateFeasibleSolution(robot, impulse_status, constraints);
  const ImpulseSplitDirection d = ImpulseSplitDirection::Random(robot, impulse_status);
  SplitImpulseOCP ocp(robot, cost, constraints);
  const double t = std::abs(Eigen::VectorXd::Random(1)[0]);
  const double step_size = 0.3;
  ocp.initConstraints(robot, s);
  const double stage_cost = ocp.stageCost(robot, t, s, step_size);
  ImpulseKKTResidual kkt_residual(robot);
  const double constraint_violation = ocp.constraintViolation(robot, impulse_status, t, s, s_prev.q, s_prev.v, kkt_residual);
  ImpulseKKTMatrix kkt_matrix_ref(robot);
  kkt_matrix_ref.setImpulseStatus(impulse_status);
  ImpulseKKTResidual kkt_residual_ref(robot);
  kkt_residual_ref.setImpulseStatus(impulse_status);
  auto cost_data = cost->createCostFunctionData(robot);
  auto constraints_data = constraints->createConstraintsData(robot);
  constraints->setSlackAndDual(robot, constraints_data, s);
  const Eigen::VectorXd v_after_impulse = s.v + s.dv;
  robot.updateKinematics(s.q, v_after_impulse);
  double stage_cost_ref = 0;
  stage_cost_ref += cost->l(robot, cost_data, t, s);
  stage_cost_ref += constraints->costSlackBarrier(constraints_data, step_size);
  EXPECT_DOUBLE_EQ(stage_cost, stage_cost_ref);
  constraints->computePrimalAndDualResidual(robot, constraints_data, s);
  stateequation::ComputeImpulseForwardEulerResidual(robot, s, s_prev.q, 
                                                    s_prev.v, kkt_residual_ref);
  ImpulseDynamicsForwardEuler id(robot);
  id.computeImpulseDynamicsResidual(robot, impulse_status, s, kkt_residual_ref);
  double constraint_violation_ref = 0;
  constraint_violation_ref += constraints->l1NormPrimalResidual(constraints_data);
  constraint_violation_ref += stateequation::L1NormStateEuqationResidual(kkt_residual_ref);
  constraint_violation_ref += id.l1NormImpulseDynamicsResidual(kkt_residual_ref);
  EXPECT_DOUBLE_EQ(constraint_violation, constraint_violation_ref);
}


TEST_F(SplitImpulseOCPTest, fixedBase) {
  std::vector<int> contact_frames = {18};
  ImpulseStatus impulse_status(contact_frames.size());
  for (int i=0; i<contact_frames.size(); ++i) {
    impulse_status.setContactPoint(i, Eigen::Vector3d::Random());
  }
  Robot robot(fixed_base_urdf, contact_frames);
  impulse_status.setImpulseStatus({false});
  const auto cost = createCost(robot);
  const auto constraints = createConstraints(robot);
  testLinearizeOCP(robot, impulse_status, cost, constraints);
  testComputeKKTResidual(robot, impulse_status, cost, constraints);
  testCostAndConstraintViolation(robot, impulse_status, cost, constraints);
  impulse_status.setImpulseStatus({true});
  testLinearizeOCP(robot, impulse_status, cost, constraints);
  testComputeKKTResidual(robot, impulse_status, cost, constraints);
  testCostAndConstraintViolation(robot, impulse_status, cost, constraints);
}


TEST_F(SplitImpulseOCPTest, floatingBase) {
  std::vector<int> contact_frames = {14, 24, 34, 44};
  ImpulseStatus impulse_status(contact_frames.size());
  for (int i=0; i<contact_frames.size(); ++i) {
    impulse_status.setContactPoint(i, Eigen::Vector3d::Random());
  }
  Robot robot(floating_base_urdf, contact_frames);
  impulse_status.setImpulseStatus({false, false, false, false});
  const auto cost = createCost(robot);
  const auto constraints = createConstraints(robot);
  testLinearizeOCP(robot, impulse_status, cost, constraints);
  testComputeKKTResidual(robot, impulse_status, cost, constraints);
  testCostAndConstraintViolation(robot, impulse_status, cost, constraints);
  std::random_device rnd;
  std::vector<bool> is_contact_active;
  for (const auto frame : contact_frames) {
    is_contact_active.push_back(rnd()%2==0);
  }
  if (!impulse_status.hasActiveImpulse()) {
    impulse_status.activateImpulse(0);
  }
  impulse_status.setImpulseStatus(is_contact_active);
  testLinearizeOCP(robot, impulse_status, cost, constraints);
  testComputeKKTResidual(robot, impulse_status, cost, constraints);
  testCostAndConstraintViolation(robot, impulse_status, cost, constraints);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}