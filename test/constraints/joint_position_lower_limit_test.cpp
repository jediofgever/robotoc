#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/ocp/split_solution.hpp"
#include "idocp/ocp/split_direction.hpp"
#include "idocp/ocp/split_kkt_matrix.hpp"
#include "idocp/ocp/split_kkt_residual.hpp"
#include "idocp/constraints/joint_position_lower_limit.hpp"
#include "idocp/constraints/pdipm.hpp"

#include "robot_factory.hpp"

namespace idocp {

class JointPositionLowerLimitTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    barrier = 1.0e-04;
    dt = std::abs(Eigen::VectorXd::Random(1)[0]);
  }

  virtual void TearDown() {
  }

  void testKinematics(Robot& robot) const;
  void testIsFeasible(Robot& robot) const;
  void testSetSlack(Robot& robot) const;
  void testComputePrimalAndDualResidual(Robot& robot) const;
  void testComputePrimalResidualDerivatives(Robot& robot) const;
  void testCondenseSlackAndDual(Robot& robot) const;
  void testExpandSlackAndDual(Robot& robot) const;

  double barrier, dt;
};


void JointPositionLowerLimitTest::testKinematics(Robot& robot) const {
  JointPositionLowerLimit limit(robot); 
  EXPECT_FALSE(limit.useKinematics());
  EXPECT_TRUE(limit.kinematicsLevel() == KinematicsLevel::PositionLevel);
}


void JointPositionLowerLimitTest::testIsFeasible(Robot& robot) const {
  JointPositionLowerLimit limit(robot); 
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  EXPECT_EQ(limit.dimc(), robot.dimv()-robot.dim_passive());
  SplitSolution s(robot);
  EXPECT_TRUE(limit.isFeasible(robot, data, s));
  s.q = 2*robot.lowerJointPositionLimit();
  EXPECT_FALSE(limit.isFeasible(robot, data, s));
}


void JointPositionLowerLimitTest::testSetSlack(Robot& robot) const {
  JointPositionLowerLimit limit(robot);
  ConstraintComponentData data(limit.dimc(), limit.barrier()), data_ref(limit.dimc(), limit.barrier());
  const int dimc = limit.dimc();
  const auto s = SplitSolution::Random(robot);
  const Eigen::VectorXd qmin = robot.lowerJointPositionLimit();
  limit.setSlack(robot, data, s);
  data_ref.slack = -qmin + s.q.tail(dimc);
  EXPECT_TRUE(data.isApprox(data_ref));
}


void JointPositionLowerLimitTest::testComputePrimalAndDualResidual(Robot& robot) const {
  JointPositionLowerLimit limit(robot); 
  const int dimc = limit.dimc();
  const auto s = SplitSolution::Random(robot);
  const Eigen::VectorXd qmin = robot.lowerJointPositionLimit();
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  data.slack.setRandom();
  data.dual.setRandom();
  auto data_ref = data;
  limit.computePrimalAndDualResidual(robot, data, s);
  data_ref.residual = - s.q.tail(dimc) + qmin + data_ref.slack;
  pdipm::ComputeComplementarySlackness(barrier, data_ref);
  EXPECT_TRUE(data.isApprox(data_ref));
}


void JointPositionLowerLimitTest::testComputePrimalResidualDerivatives(Robot& robot) const {
  JointPositionLowerLimit limit(robot);
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  const int dimc = limit.dimc();
  const auto s = SplitSolution::Random(robot);
  limit.setSlack(robot, data, s);
  auto data_ref = data;
  auto kkt_res = SplitKKTResidual::Random(robot);
  auto kkt_res_ref = kkt_res;
  limit.computePrimalResidualDerivatives(robot, data, dt, s, kkt_res);
  kkt_res_ref.lq().tail(dimc) -= dt * data_ref.dual;
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
}


void JointPositionLowerLimitTest::testCondenseSlackAndDual(Robot& robot) const {
  JointPositionLowerLimit limit(robot);
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  const int dimc = limit.dimc();
  const auto s = SplitSolution::Random(robot);
  const Eigen::VectorXd qmin = robot.lowerJointPositionLimit();
  limit.setSlack(robot, data, s);
  auto data_ref = data;
  auto kkt_mat = SplitKKTMatrix::Random(robot);
  auto kkt_res = SplitKKTResidual::Random(robot);
  auto kkt_mat_ref = kkt_mat;
  auto kkt_res_ref = kkt_res;
  limit.condenseSlackAndDual(robot, data, dt, s, kkt_mat, kkt_res);
  kkt_res_ref.lq().tail(dimc).array() 
      -= dt * (data_ref.dual.array()*data_ref.residual.array()-data_ref.cmpl.array()) 
               / data_ref.slack.array();
  kkt_mat_ref.Qqq().diagonal().tail(dimc).array() 
      += dt * data_ref.dual.array() / data_ref.slack.array();
  EXPECT_TRUE(kkt_res.isApprox(kkt_res_ref));
  EXPECT_TRUE(kkt_mat.isApprox(kkt_mat_ref));
}


void JointPositionLowerLimitTest::testExpandSlackAndDual(Robot& robot) const {
  JointPositionLowerLimit limit(robot);
  ConstraintComponentData data(limit.dimc(), limit.barrier());
  const int dimc = limit.dimc();
  const auto s = SplitSolution::Random(robot);
  const Eigen::VectorXd qmin = robot.lowerJointPositionLimit();
  limit.setSlack(robot, data, s);
  data.residual.setRandom();
  data.cmpl.setRandom();
  auto data_ref = data;
  const auto d = SplitDirection::Random(robot);
  limit.expandSlackAndDual(data, s, d);
  data_ref.dslack = d.dq().tail(dimc) - data_ref.residual;
  pdipm::ComputeDualDirection(data_ref);
  EXPECT_TRUE(data.isApprox(data_ref));
}


TEST_F(JointPositionLowerLimitTest, fixedBase) {
  auto robot = testhelper::CreateFixedBaseRobot(dt);
  testKinematics(robot);
  testIsFeasible(robot);
  testSetSlack(robot);
  testComputePrimalAndDualResidual(robot);
  testComputePrimalResidualDerivatives(robot);
  testCondenseSlackAndDual(robot);
  testExpandSlackAndDual(robot);
}


TEST_F(JointPositionLowerLimitTest, floatingBase) {
  auto robot = testhelper::CreateFloatingBaseRobot(dt);
  testKinematics(robot);
  testIsFeasible(robot);
  testSetSlack(robot);
  testComputePrimalAndDualResidual(robot);
  testComputePrimalResidualDerivatives(robot);
  testCondenseSlackAndDual(robot);
  testExpandSlackAndDual(robot);
}

} // namespace idocp


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}