#ifndef IDOCP_FORWARD_RICCATI_RECURSION_FACTORIZER_HXX_ 
#define IDOCP_FORWARD_RICCATI_RECURSION_FACTORIZER_HXX_

#include "idocp/ocp/forward_riccati_recursion_factorizer.hpp"

#include <cassert>

namespace idocp {

inline ForwardRiccatiRecursionFactorizer::ForwardRiccatiRecursionFactorizer(
    const Robot& robot) 
  : has_floating_base_(robot.hasFloatingBase()),
    dimv_(robot.dimv()),
    NApBKt_(Eigen::MatrixXd::Zero(2*robot.dimv(), 2*robot.dimv())) {
}


inline ForwardRiccatiRecursionFactorizer::ForwardRiccatiRecursionFactorizer() 
  : has_floating_base_(false),
    dimv_(0),
    NApBKt_() {
}


inline ForwardRiccatiRecursionFactorizer::~ForwardRiccatiRecursionFactorizer() {
}


inline void ForwardRiccatiRecursionFactorizer::factorizeStateTransition(
    const SplitRiccatiFactorization& riccati, const SplitKKTMatrix& kkt_matrix, 
    const SplitKKTResidual& kkt_residual, const double dtau,
    SplitRiccatiFactorization& riccati_next) {
//   assert(dtau >= 0);
//   if (has_floating_base_) {
//     riccati_next.Pi.template topRows<6>().noalias() 
//         = kkt_matrix.Fqq().template topLeftCorner<6, 6>() 
//             * riccati.Pi.template topRows<6>();
//     riccati_next.Pi.middleRows(6, dimv_-6) = riccati.Pi.middleRows(6, dimv_-6);
//     riccati_next.Pi.template topRows<6>().noalias() 
//         += kkt_matrix.Fqv().template topLeftCorner<6, 6>() 
//             * riccati.Pi.template middleRows<6>(dimv_);
//     riccati_next.Pi.middleRows(6, dimv_-6).noalias() 
//         += dtau * riccati.Pi.middleRows(dimv_+6, dimv_-6);
//   }
//   else {
//     riccati_next.Pi.topRows(dimv_) 
//         = riccati.Pi.topRows(dimv_) + dtau * riccati.Pi.bottomRows(dimv_);
//   }
//   riccati_next.Pi.bottomRows(dimv_).noalias() 
//       = kkt_matrix.Fxx().bottomRows(dimv_) * riccati.Pi;
//   riccati_next.pi = kkt_residual.Fx();
//   if (has_floating_base_) {
//     riccati_next.pi.template head<6>().noalias() 
//         += kkt_matrix.Fqq().template topLeftCorner<6, 6>() 
//             * riccati.pi.template head<6>();
//     riccati_next.pi.segment(6, dimv_-6).noalias() 
//         += riccati.pi.segment(6, dimv_-6);
//     riccati_next.pi.template head<6>().noalias() 
//         += kkt_matrix.Fqv().template topLeftCorner<6, 6>() 
//             * riccati.pi.template segment<6>(dimv_);
//     riccati_next.pi.segment(6, dimv_-6).noalias() 
//         += dtau * riccati.pi.segment(dimv_+6, dimv_-6);
//   }
//   else {
//     riccati_next.pi.head(dimv_).noalias() += riccati.pi.head(dimv_);
//     riccati_next.pi.head(dimv_).noalias() += dtau * riccati.pi.tail(dimv_);
//   }
//   riccati_next.pi.tail(dimv_).noalias() 
//       += kkt_matrix.Fxx().bottomRows(dimv_) * riccati.pi;
}


inline void ForwardRiccatiRecursionFactorizer::
factorizeStateConstraintFactorization(const SplitRiccatiFactorization& riccati, 
                                      const SplitKKTMatrix& kkt_matrix, 
                                      const double dtau, 
                                      SplitRiccatiFactorization& riccati_next) {
//   assert(dtau >= 0);
//   if (has_floating_base_) {
//     NApBKt_.template leftCols<6>().noalias() 
//         = riccati.N.template leftCols<6>() 
//             * kkt_matrix.Fqq().template topLeftCorner<6, 6>().transpose();
//     NApBKt_.middleCols(6, dimv_-6) = riccati.N.middleCols(6, dimv_-6);
//     NApBKt_.template leftCols<6>().noalias() 
//         += riccati.N.template middleCols<6>(dimv_) 
//             * kkt_matrix.Fqv().template topLeftCorner<6, 6>().transpose();
//     NApBKt_.middleCols(6, dimv_-6).noalias() 
//         += dtau * riccati.N.rightCols(dimv_-6);
//   }
//   else {
//     NApBKt_.leftCols(dimv_).noalias() 
//         = riccati.N.leftCols(dimv_) + dtau * riccati.N.rightCols(dimv_);
//   }
//   NApBKt_.rightCols(dimv_).noalias() 
//       = riccati.N * kkt_matrix.Fxx().bottomRows(dimv_).transpose();
//   if (has_floating_base_) {
//     riccati_next.N.template topRows<6>().noalias() 
//         = kkt_matrix.Fqq().template topLeftCorner<6, 6>() 
//             * NApBKt_.template topRows<6>();
//     riccati_next.N.middleRows(6, dimv_-6) = NApBKt_.middleRows(6, dimv_-6);
//     riccati_next.N.template topRows<6>().noalias() 
//         += kkt_matrix.Fqv().template topLeftCorner<6, 6>() 
//             * NApBKt_.template middleRows<6>(dimv_);
//     riccati_next.N.middleRows(6, dimv_-6).noalias() 
//         += dtau * NApBKt_.bottomRows(dimv_-6);
//   }
//   else {
//     riccati_next.N.topRows(dimv_).noalias()
//         = NApBKt_.topRows(dimv_) + dtau * NApBKt_.bottomRows(dimv_);
//   }
//   riccati_next.N.bottomRows(dimv_).noalias() 
//       = kkt_matrix.Fxx().bottomRows(dimv_) * NApBKt_;
}

} // namespace idocp

#endif // IDOCP_FORWARD_RICCATI_RECURSION_FACTORIZER_HXX_ 