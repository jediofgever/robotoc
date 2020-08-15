#ifndef IDOCP_JOINT_ACCELERATION_LOWER_LIMIT_HPP_
#define IDOCP_JOINT_ACCELERATION_LOWER_LIMIT_HPP_

#include "Eigen/Core"

#include "idocp/robot/robot.hpp"
#include "idocp/constraints/constraint_component_base.hpp"
#include "idocp/constraints/constraint_component_data.hpp"


namespace idocp {

class JointAccelerationLowerLimit final : public ConstraintComponentBase {
public:
  JointAccelerationLowerLimit(const Robot& robot, const Eigen::VectorXd& amin,
                              const double barrier=1.0e-08,
                              const double fraction_to_boundary_rate=0.995);

  JointAccelerationLowerLimit();

  ~JointAccelerationLowerLimit();

  // Use default copy constructor.
  JointAccelerationLowerLimit(const JointAccelerationLowerLimit&) = default;

  // Use default copy coperator.
  JointAccelerationLowerLimit& operator=(const JointAccelerationLowerLimit&) = default;

  // Use default move constructor.
  JointAccelerationLowerLimit(JointAccelerationLowerLimit&&) noexcept = default;

  // Use default move assign coperator.
  JointAccelerationLowerLimit& operator=(JointAccelerationLowerLimit&&) noexcept 
      = default;

  bool isFeasible(const Robot& robot, ConstraintComponentData& data, 
                  const Eigen::Ref<const Eigen::VectorXd>& a, 
                  const Eigen::Ref<const Eigen::VectorXd>& f, 
                  const Eigen::Ref<const Eigen::VectorXd>& q, 
                  const Eigen::Ref<const Eigen::VectorXd>& v, 
                  const Eigen::Ref<const Eigen::VectorXd>& u) const override;

  void setSlackAndDual(
      const Robot& robot, ConstraintComponentData& data, const double dtau, 
      const Eigen::Ref<const Eigen::VectorXd>& a, 
      const Eigen::Ref<const Eigen::VectorXd>& f, 
      const Eigen::Ref<const Eigen::VectorXd>& q, 
      const Eigen::Ref<const Eigen::VectorXd>& v, 
      const Eigen::Ref<const Eigen::VectorXd>& u) const override;

  void augmentDualResidual(const Robot& robot, ConstraintComponentData& data, 
                           const double dtau, Eigen::Ref<Eigen::VectorXd> la,
                           Eigen::Ref<Eigen::VectorXd> lf, 
                           Eigen::Ref<Eigen::VectorXd> lq,  
                           Eigen::Ref<Eigen::VectorXd> lv) const override;

  void augmentDualResidual(const Robot& robot, ConstraintComponentData& data, 
                           const double dtau, 
                           Eigen::Ref<Eigen::VectorXd> lu) const override;

  void condenseSlackAndDual(const Robot& robot, ConstraintComponentData& data, 
                            const double dtau, 
                            const Eigen::Ref<const Eigen::VectorXd>& a, 
                            const Eigen::Ref<const Eigen::VectorXd>& f, 
                            const Eigen::Ref<const Eigen::VectorXd>& q, 
                            const Eigen::Ref<const Eigen::VectorXd>& v, 
                            Eigen::Ref<Eigen::MatrixXd> Caa,
                            Eigen::Ref<Eigen::MatrixXd> Cff, 
                            Eigen::Ref<Eigen::MatrixXd> Cqq,  
                            Eigen::Ref<Eigen::MatrixXd> Cvv, 
                            Eigen::Ref<Eigen::VectorXd> la,
                            Eigen::Ref<Eigen::VectorXd> lf, 
                            Eigen::Ref<Eigen::VectorXd> lq, 
                            Eigen::Ref<Eigen::VectorXd> lv) const override;

  void condenseSlackAndDual(const Robot& robot, ConstraintComponentData& data, 
                            const double dtau, 
                            const Eigen::Ref<const Eigen::VectorXd>& u, 
                            Eigen::Ref<Eigen::MatrixXd> Cuu, 
                            Eigen::Ref<Eigen::VectorXd> Cu) const override;

  void computeSlackAndDualDirection(
      const Robot& robot, ConstraintComponentData& data, const double dtau, 
      const Eigen::Ref<const Eigen::VectorXd>& da, 
      const Eigen::Ref<const Eigen::VectorXd>& df, 
      const Eigen::Ref<const Eigen::VectorXd>& dq, 
      const Eigen::Ref<const Eigen::VectorXd>& dv, 
      const Eigen::Ref<const Eigen::VectorXd>& du) const override; 

  double residualL1Nrom(
      const Robot& robot, ConstraintComponentData& data, 
      const double dtau, const Eigen::Ref<const Eigen::VectorXd>& a, 
      const Eigen::Ref<const Eigen::VectorXd>& f, 
      const Eigen::Ref<const Eigen::VectorXd>& q, 
      const Eigen::Ref<const Eigen::VectorXd>& v, 
      const Eigen::Ref<const Eigen::VectorXd>& u) const override;

  double residualSquaredNrom(
      const Robot& robot, ConstraintComponentData& data, 
      const double dtau, const Eigen::Ref<const Eigen::VectorXd>& a, 
      const Eigen::Ref<const Eigen::VectorXd>& f, 
      const Eigen::Ref<const Eigen::VectorXd>& q, 
      const Eigen::Ref<const Eigen::VectorXd>& v, 
      const Eigen::Ref<const Eigen::VectorXd>& u) const override;
  
  int dimc() const override;

private:
  int dimc_, dim_passive_;
  Eigen::VectorXd amin_;

};

} // namespace idocp

#endif // IDOCP_JOINT_ACCELERATION_LOWER_LIMIT_HPP_