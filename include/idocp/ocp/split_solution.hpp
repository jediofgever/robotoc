#ifndef IDOCP_SPLIT_SOLUTION_HPP_
#define IDOCP_SPLIT_SOLUTION_HPP_

#include <vector>

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"


namespace idocp {

///
/// @class SplitSolution
/// @brief Solution split into each time stage. 
///
class SplitSolution {
public:
  ///
  /// @brief Construct a split solution.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  ///
  SplitSolution(const Robot& robot);

  ///
  /// @brief Default constructor. Does not construct any datas. 
  ///
  SplitSolution();

  ///
  /// @brief Destructor. 
  ///
  ~SplitSolution();

  ///
  /// @brief Use default copy constructor. 
  ///
  SplitSolution(const SplitSolution&) = default;

  ///
  /// @brief Use default copy assign operator. 
  ///
  SplitSolution& operator=(const SplitSolution&) = default;

  ///
  /// @brief Use default move constructor. 
  ///
  SplitSolution(SplitSolution&&) noexcept = default;

  ///
  /// @brief Use default move assign operator. 
  ///
  SplitSolution& operator=(SplitSolution&&) noexcept = default;

  ///
  /// @brief Set contact status from robot model, i.e., set dimension of the 
  /// contacts and equality constraints.
  /// @param[in] robot Robot model. Must be initialized by URDF or XML.
  ///
  void setContactStatus(const Robot& robot);

  ///
  /// @brief Stack of Lagrange multiplier with respect to equality constraints 
  /// that is active at the current contact status.
  /// @return Reference to the stack of Lagrange multiplier with respect to 
  /// equality constraints.
  ///
  Eigen::VectorBlock<Eigen::VectorXd> mu_stack();

  ///
  /// @brief Stack of Lagrange multiplier with respect to equality constraint 
  /// that is active at the current contact status.
  /// @return Const reference to the stack of Lagrange multiplier with respect 
  /// to equality constraints.
  ///
  const Eigen::VectorBlock<const Eigen::VectorXd> mu_stack() const;


  ///
  /// @brief Lagrange multiplier with respect to floating base constraints.
  /// @return Reference to Lagrange multiplier with respect to floating base 
  /// constraints.
  ///
  Eigen::VectorBlock<Eigen::VectorXd> mu_floating_base();

  ///
  /// @brief Lagrange multiplier with respect to floating base constraints.
  /// @return Reference to Lagrange multiplier with respect to floating base 
  /// constraints.
  ///
  const Eigen::VectorBlock<const Eigen::VectorXd> mu_floating_base() const;

  ///
  /// @brief Stack of Lagrange multiplier with respect to active contact 
  /// constraints.
  /// @return Reference to the stack of Lagrange multiplier with respect to 
  /// active contact constraints.
  ///
  Eigen::VectorBlock<Eigen::VectorXd> mu_contacts();

  ///
  /// @brief Stack of Lagrange multiplier with respect to active contact 
  /// constraints.
  /// @return Const reference to the stack of Lagrange multiplier with respect 
  /// to active contact constraints.
  ///
  const Eigen::VectorBlock<const Eigen::VectorXd> mu_contacts() const;

  ///
  /// @brief Set the stack of the Lagrange multiplier with respect to active 
  /// equality constraint from mu_floating_base and mu_contacts.
  ///
  void set_mu_stack();

  ///
  /// @brief Set the Lagrange multiplier with respect to active contact 
  /// constraints from mu_stack.
  ///
  void set_mu_contact();

  ///
  /// @brief Stack of active contact forces.
  /// @return Reference to the stack of active contact forces.
  ///
  Eigen::VectorBlock<Eigen::VectorXd> f_stack();

  ///
  /// @brief Stack of active contact forces.
  /// @return Const reference to the stack of active contact forces.
  ///
  const Eigen::VectorBlock<const Eigen::VectorXd> f_stack() const;

  ///
  /// @brief Set the stack of contact forces from each contact forces.
  ///
  void set_f_stack();

  ///
  /// @brief Set the each contact forces from stack of contact forces.
  ///
  void set_f();

  ///
  /// @brief Returns the number of active contacts.
  /// @return Number of active contacts.
  ///
  int num_active_contacts() const;

  ///
  /// @brief Returns the dimension of equality constraint at the current 
  /// contact status.
  /// @return Dimension of equality constraint.
  ///
  int dimc() const;

  ///
  /// @brief Returns the dimension of the stack of contact forces at the current 
  /// contact status.
  /// @return Dimension of contact forces.
  ///
  int dimf() const;

  /// @brief Lagrange multiplier with respect to transition of q. 
  Eigen::VectorXd lmd;

  /// @brief Lagrange multiplier with respect to transition of v. 
  Eigen::VectorXd gmm;

  /// @brief Lagrange multiplier with respect to contact constraint. 
  std::vector<Eigen::Vector3d> mu_contact;

  /// @brief Generalized acceleration. 
  Eigen::VectorXd a;

  /// @brief Contact forces. 
  std::vector<Eigen::Vector3d> f;

  /// @brief Configuration. 
  Eigen::VectorXd q;

  /// @brief Generalized velocity. 
  Eigen::VectorXd v;

  /// @brief Control input torques. 
  Eigen::VectorXd u;

  /// @brief Lagrange multiplier with respect to inverse dynamics. 
  Eigen::VectorXd beta;

  ///
  /// @brief Generates split solution filled randomly.
  /// @return Split solution filled randomly.
  ///
  static SplitSolution Random(const Robot& robot);

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

private:
  /// @brief Stack of Lagrange multiplier with respect to equality constraints. 
  Eigen::VectorXd mu_stack_;

  /// @brief Stack of the contact forces. 
  Eigen::VectorXd f_stack_;

  /// @brief Dimension of passive joints. 
  bool has_floating_base_;

  /// @brief Dimension of passive joints. 
  int dim_passive_;

  /// @brief Dimension of passive joints. 
  std::vector<bool> is_each_contact_active_;

  /// @brief Dimension of contact forces at the current contact status. 
  int dimf_;

  /// @brief Dimension of equality constraints at the current contact status. 
  int dimc_;

};

} // namespace idocp 

#include "idocp/ocp/split_solution.hxx"

#endif // IDOCP_SPLIT_SOLUTION_HPP_