#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "robotoc/robot/robot.hpp"
#include "robotoc/robot/contact_status.hpp"
#include "robotoc/robot/impulse_status.hpp"
#include "robotoc/ocp/split_solution.hpp"
#include "robotoc/ocp/split_direction.hpp"
#include "robotoc/ocp/split_kkt_matrix.hpp"
#include "robotoc/ocp/split_kkt_residual.hpp"
#include "robotoc/constraints/constraints.hpp"
#include "robotoc/constraints/constraints_data.hpp"
#include "robotoc/constraints/joint_position_lower_limit.hpp"
#include "robotoc/constraints/joint_position_upper_limit.hpp"
#include "robotoc/constraints/joint_velocity_lower_limit.hpp"
#include "robotoc/constraints/joint_velocity_upper_limit.hpp"
#include "robotoc/constraints/joint_torques_lower_limit.hpp"
#include "robotoc/constraints/joint_torques_upper_limit.hpp"
#include "robotoc/constraints/joint_acceleration_lower_limit.hpp"
#include "robotoc/constraints/joint_acceleration_upper_limit.hpp"
#include "robotoc/constraints/friction_cone.hpp"
#include "robotoc/constraints/pdipm.hpp"

#include "robot_factory.hpp"

namespace robotoc {

class ConstraintsTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    barrier = 1.0e-03;
    mu = 0.7;
  }

  virtual void TearDown() {
  }

  std::shared_ptr<Constraints> createConstraints(const Robot& robot, 
                                                 const double barrier=1.0e-03,
                                                 const double fraction_to_boundary_rule=0.995) const;

  void timeStage0(Robot& robot, const ContactStatus& contact_status) const;
  void timeStage1(Robot& robot, const ContactStatus& contact_status) const;
  void timeStage2(Robot& robot, const ContactStatus& contact_status) const;

  double barrier, mu;
};


std::shared_ptr<Constraints> ConstraintsTest::createConstraints(const Robot& robot, 
                                                                const double barrier, 
                                                                const double fraction_to_boundary_rule) const {
  auto joint_position_lower = std::make_shared<robotoc::JointPositionLowerLimit>(robot);
  auto joint_position_upper = std::make_shared<robotoc::JointPositionUpperLimit>(robot);
  auto joint_velocity_lower = std::make_shared<robotoc::JointVelocityLowerLimit>(robot);
  auto joint_velocity_upper = std::make_shared<robotoc::JointVelocityUpperLimit>(robot);
  auto joint_torques_lower = std::make_shared<robotoc::JointTorquesLowerLimit>(robot);
  auto joint_torques_upper = std::make_shared<robotoc::JointTorquesUpperLimit>(robot);
  auto joint_accel_lower = std::make_shared<robotoc::JointAccelerationLowerLimit>(robot, Eigen::VectorXd::Constant(robot.dimv(), -10));
  auto joint_accel_upper = std::make_shared<robotoc::JointAccelerationUpperLimit>(robot, Eigen::VectorXd::Constant(robot.dimv(), 10));
  auto friction_cone = std::make_shared<robotoc::FrictionCone>(robot, 0.7);
  auto constraints = std::make_shared<Constraints>(barrier, fraction_to_boundary_rule);
  constraints->push_back(joint_position_lower);
  constraints->push_back(joint_position_upper);
  constraints->push_back(joint_velocity_lower);
  constraints->push_back(joint_velocity_upper);
  constraints->push_back(joint_torques_lower);
  constraints->push_back(joint_torques_upper);
  constraints->push_back(joint_accel_lower);
  constraints->push_back(joint_accel_upper);
  constraints->push_back(friction_cone);
  return constraints;  
}


void ConstraintsTest::timeStage0(Robot& robot, 
                                 const ContactStatus& contact_status) const {
  const int time_stage = 0;
  auto constraints = createConstraints(robot);
  auto data = constraints->createConstraintsData(robot, time_stage);
  EXPECT_TRUE(data.position_level_data.empty());
  EXPECT_TRUE(data.velocity_level_data.empty());
  EXPECT_FALSE(data.acceleration_level_data.empty());
  EXPECT_EQ(data.acceleration_level_data.size(), 5);
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  const SplitDirection d = SplitDirection::Random(robot, contact_status);
  SplitKKTMatrix kkt_matrix(robot);
  SplitKKTResidual kkt_residual(robot);
  kkt_matrix.setContactStatus(contact_status);
  kkt_residual.setContactStatus(contact_status);
  constraints->setSlackAndDual(robot, contact_status, data, s);
  constraints->linearizeConstraints(robot, contact_status, data, s, kkt_residual);
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_TRUE(kkt_residual.lv().isZero());
  EXPECT_FALSE(kkt_residual.la.isZero());
  EXPECT_FALSE(kkt_residual.lu.isZero());
  if (contact_status.hasActiveContacts()) {
    EXPECT_FALSE(kkt_residual.lf().isZero());
  }
  constraints->condenseSlackAndDual(contact_status, data, kkt_matrix, kkt_residual);
  EXPECT_TRUE(kkt_matrix.Qqq().isZero());
  EXPECT_TRUE(kkt_matrix.Qvv().isZero());
  EXPECT_FALSE(kkt_matrix.Qaa.isZero());
  EXPECT_FALSE(kkt_matrix.Quu.isZero());
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_TRUE(kkt_residual.lv().isZero());
  EXPECT_FALSE(kkt_residual.la.isZero());
  EXPECT_FALSE(kkt_residual.lu.isZero());
  if (contact_status.hasActiveContacts()) {
    EXPECT_FALSE(kkt_residual.lf().isZero());
    EXPECT_FALSE(kkt_matrix.Qff().isZero());
  }
}


void ConstraintsTest::timeStage1(Robot& robot, 
                                 const ContactStatus& contact_status) const {
  const int time_stage = 1;
  auto constraints = createConstraints(robot);
  auto data = constraints->createConstraintsData(robot, time_stage);
  EXPECT_TRUE(data.position_level_data.empty());
  EXPECT_FALSE(data.velocity_level_data.empty());
  EXPECT_FALSE(data.acceleration_level_data.empty());
  EXPECT_EQ(data.velocity_level_data.size(), 2);
  EXPECT_EQ(data.acceleration_level_data.size(), 5);
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  const SplitDirection d = SplitDirection::Random(robot, contact_status);
  SplitKKTMatrix kkt_matrix(robot);
  SplitKKTResidual kkt_residual(robot);
  kkt_matrix.setContactStatus(contact_status);
  kkt_residual.setContactStatus(contact_status);
  constraints->setSlackAndDual(robot, contact_status, data, s);
  constraints->linearizeConstraints(robot, contact_status, data, s, kkt_residual);
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_FALSE(kkt_residual.lv().isZero());
  EXPECT_FALSE(kkt_residual.la.isZero());
  EXPECT_FALSE(kkt_residual.lu.isZero());
  if (contact_status.hasActiveContacts()) {
    EXPECT_FALSE(kkt_residual.lf().isZero());
  }
  constraints->condenseSlackAndDual(contact_status, data, kkt_matrix, kkt_residual);
  EXPECT_TRUE(kkt_matrix.Qqq().isZero());
  EXPECT_FALSE(kkt_matrix.Qvv().isZero());
  EXPECT_FALSE(kkt_matrix.Qaa.isZero());
  EXPECT_FALSE(kkt_matrix.Quu.isZero());
  EXPECT_TRUE(kkt_residual.lq().isZero());
  EXPECT_FALSE(kkt_residual.lv().isZero());
  EXPECT_FALSE(kkt_residual.la.isZero());
  EXPECT_FALSE(kkt_residual.lu.isZero());
  if (contact_status.hasActiveContacts()) {
    EXPECT_FALSE(kkt_residual.lf().isZero());
    EXPECT_FALSE(kkt_matrix.Qff().isZero());
  }
}


void ConstraintsTest::timeStage2(Robot& robot, 
                                 const ContactStatus& contact_status) const {
  const int time_stage = 2;
  auto constraints = createConstraints(robot);
  auto data = constraints->createConstraintsData(robot, time_stage);
  EXPECT_FALSE(data.position_level_data.empty());
  EXPECT_FALSE(data.velocity_level_data.empty());
  EXPECT_FALSE(data.acceleration_level_data.empty());
  EXPECT_EQ(data.position_level_data.size(), 2);
  EXPECT_EQ(data.velocity_level_data.size(), 2);
  EXPECT_EQ(data.acceleration_level_data.size(), 5);
  const SplitSolution s = SplitSolution::Random(robot, contact_status);
  const SplitDirection d = SplitDirection::Random(robot, contact_status);
  SplitKKTMatrix kkt_matrix(robot);
  SplitKKTResidual kkt_residual(robot);
  kkt_matrix.setContactStatus(contact_status);
  kkt_residual.setContactStatus(contact_status);
  constraints->setSlackAndDual(robot, contact_status, data, s);
  constraints->linearizeConstraints(robot, contact_status, data, s, kkt_residual);
  EXPECT_FALSE(kkt_residual.lq().isZero());
  EXPECT_FALSE(kkt_residual.lv().isZero());
  EXPECT_FALSE(kkt_residual.la.isZero());
  EXPECT_FALSE(kkt_residual.lu.isZero());
  if (contact_status.hasActiveContacts()) {
    EXPECT_FALSE(kkt_residual.lf().isZero());
  }
  constraints->condenseSlackAndDual(contact_status, data, kkt_matrix, kkt_residual);
  EXPECT_FALSE(kkt_matrix.Qqq().isZero());
  EXPECT_FALSE(kkt_matrix.Qvv().isZero());
  EXPECT_FALSE(kkt_matrix.Qaa.isZero());
  EXPECT_FALSE(kkt_matrix.Quu.isZero());
  EXPECT_FALSE(kkt_residual.lq().isZero());
  EXPECT_FALSE(kkt_residual.lv().isZero());
  EXPECT_FALSE(kkt_residual.la.isZero());
  EXPECT_FALSE(kkt_residual.lu.isZero());
  if (contact_status.hasActiveContacts()) {
    EXPECT_FALSE(kkt_residual.lf().isZero());
    EXPECT_FALSE(kkt_matrix.Qff().isZero());
  }
}


TEST_F(ConstraintsTest, fixedBase) {
  auto robot = testhelper::CreateRobotManipulator(0.001);
  auto contact_status = robot.createContactStatus();
  timeStage0(robot, contact_status);
  timeStage1(robot, contact_status);
  timeStage2(robot, contact_status);
  contact_status.activateContact(0);
  timeStage0(robot, contact_status);
  timeStage1(robot, contact_status);
  timeStage2(robot, contact_status);
}


TEST_F(ConstraintsTest, floatingBase) {
  auto robot = testhelper::CreateQuadrupedalRobot(0.001);
  auto contact_status = robot.createContactStatus();
  timeStage0(robot, contact_status);
  timeStage1(robot, contact_status);
  timeStage2(robot, contact_status);
  contact_status.setRandom();
  timeStage0(robot, contact_status);
  timeStage1(robot, contact_status);
  timeStage2(robot, contact_status);
}


TEST_F(ConstraintsTest, testParams) {
  auto robot = testhelper::CreateQuadrupedalRobot(0.001);
  auto friction_cone = std::make_shared<robotoc::FrictionCone>(robot, 0.7);
  auto constraints = std::make_shared<Constraints>(0.1, 0.5);
  constraints->push_back(friction_cone);
  EXPECT_DOUBLE_EQ(friction_cone->barrier(), 0.1);
  EXPECT_DOUBLE_EQ(friction_cone->fractionToBoundaryRule(), 0.5);
  constraints->setBarrier(0.2);
  EXPECT_DOUBLE_EQ(friction_cone->barrier(), 0.2);
  constraints->setFractionToBoundaryRule(0.8);
  EXPECT_DOUBLE_EQ(friction_cone->fractionToBoundaryRule(), 0.8);
}

} // namespace robotoc


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}