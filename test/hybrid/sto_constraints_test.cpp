#include <memory>

#include <gtest/gtest.h>
#include "Eigen/Core"

#include "robotoc/hybrid/contact_sequence.hpp"
#include "robotoc/hybrid/sto_constraints.hpp"

#include "robot_factory.hpp"
#include "contact_sequence_factory.hpp"


namespace robotoc {

class STOConstraintsTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    srand((unsigned int) time(0));
    N = 20;
    max_num_impulse = 5;
    T = 1.0;
    t = 0.1; 
    min_dt  = std::abs(Eigen::VectorXd::Random(1)[0]);

    auto robot = testhelper::CreateQuadrupedalRobot();
    kkt_matrix = KKTMatrix(robot, N, max_num_impulse);
    kkt_residual = KKTResidual(robot, N, max_num_impulse);
    d = Direction(robot, N, max_num_impulse);

    const double dt = T / N;
    contact_sequence 
        = testhelper::CreateContactSequenceSharedPtr(robot, N, max_num_impulse, 0, 3*dt);

    discretization = TimeDiscretization(T, N, 2*max_num_impulse);
    discretization.discretize(contact_sequence, t);
  }

  virtual void TearDown() {
  }

  int N, max_num_impulse;
  double T, t, min_dt;
  KKTMatrix kkt_matrix;
  KKTResidual kkt_residual;
  Direction d;
  std::shared_ptr<ContactSequence> contact_sequence;
  TimeDiscretization discretization;
};


TEST_F(STOConstraintsTest, testParam) {
  const int max_num_switches = 2 * max_num_impulse;
  auto constraints = STOConstraints(max_num_switches, min_dt);
  EXPECT_DOUBLE_EQ(constraints.barrier(), 1.0e-03);
  EXPECT_DOUBLE_EQ(constraints.fractionToBoundaryRule(), 0.995);
  const double barrier = 0.8;
  const double fraction_to_boundary_rule = 0.5;
  constraints.setBarrier(barrier);
  constraints.setFractionToBoundaryRule(fraction_to_boundary_rule);
  EXPECT_DOUBLE_EQ(constraints.barrier(), barrier);
  EXPECT_DOUBLE_EQ(constraints.fractionToBoundaryRule(), fraction_to_boundary_rule);
}


TEST_F(STOConstraintsTest, test) {
  const int max_num_switches = 2 * max_num_impulse;
  auto constraints = STOConstraints(max_num_switches, min_dt);
  constraints.setSlack(discretization);
  constraints.evalConstraint(discretization);
  constraints.linearizeConstraints(discretization, kkt_residual);
  constraints.condenseSlackAndDual(discretization, kkt_matrix, kkt_residual);
  constraints.expandSlackAndDual(discretization, d);
  const double primal_step_size = constraints.maxPrimalStepSize();
  const double dual_step_size = constraints.maxDualStepSize();
  EXPECT_TRUE(0 < primal_step_size && primal_step_size <= 1.0);
  EXPECT_TRUE(0 < dual_step_size && dual_step_size <= 1.0);
  constraints.updateSlack(primal_step_size);
  constraints.updateDual(dual_step_size);
  const auto& min_dwell_times = constraints.minimumDwellTimes();
  EXPECT_EQ(min_dwell_times.size(), 2*max_num_impulse+1);
  for (const double e : min_dwell_times) {
    EXPECT_DOUBLE_EQ(e, min_dt);
  }
}

} // namespace robotoc


int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
