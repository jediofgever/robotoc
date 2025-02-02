#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "robotoc/robot/robot.hpp"
#include "robotoc/cost/time_varying_configuration_space_cost.hpp"
#include "robotoc/cost/cost_function_data.hpp"
#include "robotoc/ocp/split_solution.hpp"
#include "robotoc/ocp/split_kkt_residual.hpp"
#include "robotoc/ocp/split_kkt_matrix.hpp"

#include "robotoc/utils/derivative_checker.hpp"

#include "robot_factory.hpp"


namespace robotoc {

class TimeVaryingConfigurationRef final : public TimeVaryingConfigurationRefBase {
public:
  TimeVaryingConfigurationRef(const Eigen::VectorXd& q0_ref, 
                              const Eigen::VectorXd& v_ref,
                              const double t0, const double tf)
    : q0_ref_(q0_ref),
      v_ref_(v_ref),
      t0_(t0),
      tf_(tf) {
  }

  TimeVaryingConfigurationRef() {}

  ~TimeVaryingConfigurationRef() {}

  TimeVaryingConfigurationRef(const TimeVaryingConfigurationRef&) = default;

  TimeVaryingConfigurationRef& operator=( 
      const TimeVaryingConfigurationRef&) = default;

  TimeVaryingConfigurationRef(
      TimeVaryingConfigurationRef&&) noexcept = default;

  TimeVaryingConfigurationRef& operator=(
      TimeVaryingConfigurationRef&&) noexcept = default;

  void update_q_ref(const Robot& robot, const GridInfo& grid_info,
                    Eigen::VectorXd& q_ref) const override {
    robot.integrateConfiguration(q0_ref_, v_ref_, (grid_info.t-t0_), q_ref);
  }

  bool isActive(const GridInfo& grid_info) const override {
    if (t0_ <= grid_info.t && grid_info.t <= tf_)
      return true;
    else 
      return false;
  }

private:
  Eigen::VectorXd q0_ref_, v_ref_;
  double t0_, tf_;
};


class TimeVaryingConfigurationSpaceCostTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    grid_info = GridInfo::Random();
    grid_info0 = grid_info;
    grid_infof = grid_info;
    dt = grid_info0.dt;
    t = std::abs(Eigen::VectorXd::Random(1)[0]);
    t0 = t - std::abs(Eigen::VectorXd::Random(1)[0]);
    tf = t + std::abs(Eigen::VectorXd::Random(1)[0]);
    grid_info.t = t;
    grid_info0.t = t0 - dt;
    grid_infof.t = tf + dt;
  }

  virtual void TearDown() {
  }

  void testStageCost(Robot& robot) const;
  void testTerminalCost(Robot& robot) const;
  void testImpulseCost(Robot& robot) const;

  GridInfo grid_info, grid_info0, grid_infof;
  double dt, t, t0, tf;
};


void TimeVaryingConfigurationSpaceCostTest::testStageCost(Robot& robot) const {
  const int dimq = robot.dimq();
  const int dimv = robot.dimv();
  auto kkt_mat = SplitKKTMatrix::Random(robot);
  auto kkt_res = SplitKKTResidual::Random(robot);
  auto kkt_mat_ref = kkt_mat;
  auto kkt_res_ref = kkt_res;
  const Eigen::VectorXd q_weight = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd q0_ref = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(dimv); 
  auto ref = std::make_shared<TimeVaryingConfigurationRef>(q0_ref, v_ref, t0, tf);
  auto cost = std::make_shared<TimeVaryingConfigurationSpaceCost>(robot, ref);
  CostFunctionData data(robot);
  EXPECT_FALSE(cost->useKinematics());
  cost->set_q_weight(q_weight);

  const auto s = SplitSolution::Random(robot);
  const auto contact_status = robot.createContactStatus();
  EXPECT_DOUBLE_EQ(cost->evalStageCost(robot, contact_status, data, grid_info0, s), 0);
  EXPECT_DOUBLE_EQ(cost->evalStageCost(robot, contact_status, data, grid_infof, s), 0);
  cost->evalStageCostDerivatives(robot, contact_status, data, grid_info0, s, kkt_res);
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalStageCostDerivatives(robot, contact_status, data, grid_infof, s, kkt_res);
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalStageCostHessian(robot, contact_status, data, grid_info0, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
  cost->evalStageCostHessian(robot, contact_status, data, grid_infof, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));

  Eigen::VectorXd q_ref = Eigen::VectorXd::Zero(dimq);
  robot.integrateConfiguration(q0_ref, v_ref, (t-t0), q_ref);
  Eigen::VectorXd q_diff = Eigen::VectorXd::Zero(dimv); 
  robot.subtractConfiguration(s.q, q_ref, q_diff);
  const double cost_ref = 0.5 * dt * (q_weight.array()*q_diff.array()*q_diff.array()).sum();
  EXPECT_DOUBLE_EQ(cost->evalStageCost(robot, contact_status, data, grid_info, s), cost_ref);

  cost->evalStageCostDerivatives(robot, contact_status, data, grid_info, s, kkt_res);
  Eigen::MatrixXd Jq_diff = Eigen::MatrixXd::Zero(dimv, dimv);
  if (robot.hasFloatingBase()) {
    robot.dSubtractConfiguration_dqf(s.q, q_ref, Jq_diff);
    kkt_res_ref.lq() += dt * Jq_diff.transpose() * q_weight.asDiagonal() * q_diff;
  }
  else {
    kkt_res_ref.lq() += dt * q_weight.asDiagonal() * (s.q-q_ref);
  }
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalStageCostHessian(robot, contact_status, data, grid_info, s, kkt_mat);
  if (robot.hasFloatingBase()) {
    kkt_mat_ref.Qqq() += dt * Jq_diff.transpose() * q_weight.asDiagonal() * Jq_diff;
  }
  else {
    kkt_mat_ref.Qqq() += dt * q_weight.asDiagonal();
  }
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
  DerivativeChecker derivative_checker(robot);
  EXPECT_TRUE(derivative_checker.checkFirstOrderStageCostDerivatives(cost));
  if (!robot.hasFloatingBase()) {
    EXPECT_TRUE(derivative_checker.checkSecondOrderStageCostDerivatives(cost));
  }
}


void TimeVaryingConfigurationSpaceCostTest::testTerminalCost(Robot& robot) const {
  const int dimq = robot.dimq();
  const int dimv = robot.dimv();
  auto kkt_mat = SplitKKTMatrix::Random(robot);
  auto kkt_res = SplitKKTResidual::Random(robot);
  auto kkt_mat_ref = kkt_mat;
  auto kkt_res_ref = kkt_res;
  const Eigen::VectorXd qf_weight = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd q0_ref = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(dimv); 
  auto ref = std::make_shared<TimeVaryingConfigurationRef>(q0_ref, v_ref, t0, tf);
  auto cost = std::make_shared<TimeVaryingConfigurationSpaceCost>(robot, ref);
  CostFunctionData data(robot);
  EXPECT_FALSE(cost->useKinematics());
  cost->set_qf_weight(qf_weight);

  const auto s = SplitSolution::Random(robot);
  EXPECT_DOUBLE_EQ(cost->evalTerminalCost(robot, data, grid_info0, s), 0);
  EXPECT_DOUBLE_EQ(cost->evalTerminalCost(robot, data, grid_infof, s), 0);
  cost->evalTerminalCostDerivatives(robot, data, grid_info0, s, kkt_res);
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalTerminalCostDerivatives(robot, data, grid_infof, s, kkt_res);
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalTerminalCostHessian(robot, data, grid_info0, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
  cost->evalTerminalCostHessian(robot, data, grid_infof, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));

  Eigen::VectorXd q_ref = Eigen::VectorXd::Zero(dimq);
  robot.integrateConfiguration(q0_ref, v_ref, (t-t0), q_ref);
  Eigen::VectorXd q_diff = Eigen::VectorXd::Zero(dimv); 
  robot.subtractConfiguration(s.q, q_ref, q_diff);
  const double cost_ref = 0.5 * (qf_weight.array()*q_diff.array()*q_diff.array()).sum();
  EXPECT_DOUBLE_EQ(cost->evalTerminalCost(robot, data, grid_info, s), cost_ref);

  cost->evalTerminalCostDerivatives(robot, data, grid_info, s, kkt_res);
  Eigen::MatrixXd Jq_diff = Eigen::MatrixXd::Zero(dimv, dimv);
  if (robot.hasFloatingBase()) {
    robot.dSubtractConfiguration_dqf(s.q, q_ref, Jq_diff);
    kkt_res_ref.lq() += Jq_diff.transpose() * qf_weight.asDiagonal() * q_diff;
  }
  else {
    kkt_res_ref.lq() += qf_weight.asDiagonal() * (s.q-q_ref);
  }
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalTerminalCostHessian(robot, data, grid_info, s, kkt_mat);
  if (robot.hasFloatingBase()) {
    kkt_mat_ref.Qqq() += Jq_diff.transpose() * qf_weight.asDiagonal() * Jq_diff;
  }
  else {
    kkt_mat_ref.Qqq() += qf_weight.asDiagonal();
  }
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
  DerivativeChecker derivative_checker(robot);
  EXPECT_TRUE(derivative_checker.checkFirstOrderTerminalCostDerivatives(cost));
  if (!robot.hasFloatingBase()) {
    EXPECT_TRUE(derivative_checker.checkSecondOrderTerminalCostDerivatives(cost));
  }
}


void TimeVaryingConfigurationSpaceCostTest::testImpulseCost(Robot& robot) const {
  const int dimq = robot.dimq();
  const int dimv = robot.dimv();
  auto kkt_mat = ImpulseSplitKKTMatrix::Random(robot);
  auto kkt_res = ImpulseSplitKKTResidual::Random(robot);
  auto kkt_mat_ref = kkt_mat;
  auto kkt_res_ref = kkt_res;
  const Eigen::VectorXd qi_weight = Eigen::VectorXd::Random(dimv);
  const Eigen::VectorXd q0_ref = robot.generateFeasibleConfiguration();
  const Eigen::VectorXd v_ref = Eigen::VectorXd::Random(dimv); 
  auto ref = std::make_shared<TimeVaryingConfigurationRef>(q0_ref, v_ref, t0, tf);
  auto cost = std::make_shared<TimeVaryingConfigurationSpaceCost>(robot, ref);
  CostFunctionData data(robot);
  EXPECT_FALSE(cost->useKinematics());
  cost->set_qi_weight(qi_weight);

  const auto s = ImpulseSplitSolution::Random(robot);
  const auto impulse_status = robot.createImpulseStatus();
  EXPECT_DOUBLE_EQ(cost->evalImpulseCost(robot, impulse_status, data, grid_info0, s), 0);
  EXPECT_DOUBLE_EQ(cost->evalImpulseCost(robot, impulse_status, data, grid_infof, s), 0);
  cost->evalImpulseCostDerivatives(robot, impulse_status, data, grid_info0, s, kkt_res);
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalImpulseCostDerivatives(robot, impulse_status, data, grid_infof, s, kkt_res);
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalImpulseCostHessian(robot, impulse_status, data, grid_info0, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
  cost->evalImpulseCostHessian(robot, impulse_status, data, grid_infof, s, kkt_mat);
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));

  Eigen::VectorXd q_ref = Eigen::VectorXd::Zero(dimq);
  robot.integrateConfiguration(q0_ref, v_ref, (t-t0), q_ref);
  Eigen::VectorXd q_diff = Eigen::VectorXd::Zero(dimv); 
  robot.subtractConfiguration(s.q, q_ref, q_diff);
  const double cost_ref = 0.5 * (qi_weight.array()*q_diff.array()*q_diff.array()).sum();
  EXPECT_DOUBLE_EQ(cost->evalImpulseCost(robot, impulse_status, data, grid_info, s), cost_ref);

  cost->evalImpulseCostDerivatives(robot, impulse_status, data, grid_info, s, kkt_res);
  Eigen::MatrixXd Jq_diff = Eigen::MatrixXd::Zero(dimv, dimv);
  if (robot.hasFloatingBase()) {
    robot.dSubtractConfiguration_dqf(s.q, q_ref, Jq_diff);
    kkt_res_ref.lq() += Jq_diff.transpose() * qi_weight.asDiagonal() * q_diff;
  }
  else {
    kkt_res_ref.lq() += qi_weight.asDiagonal() * (s.q-q_ref);
  }
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  cost->evalImpulseCostHessian(robot, impulse_status, data, grid_info, s, kkt_mat);
  if (robot.hasFloatingBase()) {
    kkt_mat_ref.Qqq() += Jq_diff.transpose() * qi_weight.asDiagonal() * Jq_diff;
  }
  else {
    kkt_mat_ref.Qqq() += qi_weight.asDiagonal();
  }
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
  DerivativeChecker derivative_checker(robot);
  EXPECT_TRUE(derivative_checker.checkFirstOrderImpulseCostDerivatives(cost));
  if (!robot.hasFloatingBase()) {
    EXPECT_TRUE(derivative_checker.checkSecondOrderImpulseCostDerivatives(cost));
  }
}


TEST_F(TimeVaryingConfigurationSpaceCostTest, fixedBase) {
  auto robot = testhelper::CreateRobotManipulator(dt);
  testStageCost(robot);
  testTerminalCost(robot);
  testImpulseCost(robot);
}


TEST_F(TimeVaryingConfigurationSpaceCostTest, floatingBase) {
  auto robot = testhelper::CreateQuadrupedalRobot(dt);
  testStageCost(robot);
  testTerminalCost(robot);
  testImpulseCost(robot);
}

} // namespace robotoc


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}