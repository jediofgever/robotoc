#ifndef IDOCP_CONSTRAINT_COMPONENT_DATA_HXX_
#define IDOCP_CONSTRAINT_COMPONENT_DATA_HXX_

#include "idocp/constraints/constraint_component_data.hpp"

#include <cmath>
#include <stdexcept>
#include <iostream>


namespace idocp {

inline ConstraintComponentData::ConstraintComponentData(const int dimc,
                                                        const double barrier)
  : slack(Eigen::VectorXd::Constant(dimc, std::sqrt(barrier))),
    dual(Eigen::VectorXd::Constant(dimc, std::sqrt(barrier))),
    residual(Eigen::VectorXd::Zero(dimc)),
    cmpl(Eigen::VectorXd::Zero(dimc)),
    dslack(Eigen::VectorXd::Zero(dimc)),
    ddual(Eigen::VectorXd::Zero(dimc)),
    r(),
    J(),
    dimc_(dimc) {
  try {
    if (dimc <= 0) {
      throw std::out_of_range(
          "Invalid argment: dimc must be positive!");
    }
    if (barrier <= 0) {
      throw std::out_of_range(
          "Invalid argment: barrirer must be positive!");
    }
  }
  catch(const std::exception& e) {
    std::cerr << e.what() << '\n';
    std::exit(EXIT_FAILURE);
  }
}


inline ConstraintComponentData::ConstraintComponentData()
  : slack(),
    dual(),
    residual(),
    cmpl(),
    dslack(),
    ddual(),
    r(),
    J(),
    dimc_(0) {
}


inline ConstraintComponentData::~ConstraintComponentData() {
}


inline double ConstraintComponentData::squaredNormKKTResidual() const {
  return (residual.squaredNorm() + cmpl.squaredNorm());
}


inline double ConstraintComponentData::l1NormConstraintViolation() const {
  return residual.template lpNorm<1>();
}


inline int ConstraintComponentData::dimc() const {
  return dimc_;
}


inline bool ConstraintComponentData::checkDimensionalConsistency() const {
  if (slack.size() != dimc_) {
    return false;
  }
  if (dual.size() != dimc_) {
    return false;
  }
  if (residual.size() != dimc_) {
    return false;
  }
  if (cmpl.size() != dimc_) {
    return false;
  }
  if (dslack.size() != dimc_) {
    return false;
  }
  if (ddual.size() != dimc_) {
    return false;
  }
  return true;
}


inline bool ConstraintComponentData::isApprox(
    const ConstraintComponentData& other) const {
  if (!slack.isApprox(other.slack)) {
    return false;
  }
  if (!dual.isApprox(other.dual)) {
    return false;
  }
  if (!residual.isApprox(other.residual)) {
    return false;
  }
  if (!cmpl.isApprox(other.cmpl)) {
    return false;
  }
  if (!dslack.isApprox(other.dslack)) {
    return false;
  }
  if (!ddual.isApprox(other.ddual)) {
    return false;
  }
  return true;
}

} // namespace idocp

#endif // IDOCP_CONSTRAINT_COMPONENT_DATA_HXX_