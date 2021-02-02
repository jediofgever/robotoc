#ifndef IDOCP_UTILS_OCP_BENCHMARKER_HXX_
#define IDOCP_UTILS_OCP_BENCHMARKER_HXX_ 

#include "idocp/utils/ocp_benchmarker.hxx"

#include <iostream>
#include <chrono>


namespace idocp {
namespace ocpbenchmarker {

template <typename OCPSolverType>
inline void CPUTime(OCPSolverType& ocp_solver, const double t, 
                    const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
                    const int num_iteration, const bool line_search) {
  std::chrono::system_clock::time_point start_clock, end_clock;
  start_clock = std::chrono::system_clock::now();
  for (int i=0; i<num_iteration; ++i) {
    ocp_solver.updateSolution(t, q, v, line_search);
  }
  end_clock = std::chrono::system_clock::now();
  std::cout << "---------- OCP benchmark : CPU time ----------" << std::endl;
  std::cout << "total CPU time: " 
            << 1e-03 * std::chrono::duration_cast<std::chrono::microseconds>(
                  end_clock-start_clock).count() 
            << "[ms]" << std::endl;
  std::cout << "CPU time per update: " 
            << 1e-03 * std::chrono::duration_cast<std::chrono::microseconds>(
                  end_clock-start_clock).count() / num_iteration 
            << "[ms]" << std::endl;
  std::cout << "-----------------------------------" << std::endl;
  std::cout << std::endl;
}


template <typename OCPSolverType>
inline void Convergence(OCPSolverType& ocp_solver, const double t, 
                        const Eigen::VectorXd& q, const Eigen::VectorXd& v, 
                        const int num_iteration, const bool line_search) {
  std::cout << "---------- OCP benchmark : Convergence ----------" << std::endl;
  ocp_solver.computeKKTResidual(t, q, v);
  std::cout << "Initial KKT error = " << ocp_solver.KKTError() << std::endl;
  for (int i=0; i<num_iteration; ++i) {
    ocp_solver.updateSolution(t, q, v, line_search);
    ocp_solver.computeKKTResidual(t, q, v);
    if (ocp_solver.updateAugmentedLagrangian()) {
      std::cout << "update Augmentd Lagrangian!!" << std::endl;
      ocp_solver.computeKKTResidual(t, q, v);
    }
    std::cout << "KKT error after iteration " << i+1 << " = " 
              << ocp_solver.KKTError() << std::endl;
  }
  std::cout << "-----------------------------------" << std::endl;
  std::cout << std::endl;
}

} // namespace ocpbenchmarker
} // namespace idocp 

#endif // IDOCP_UTILS_OCP_BENCHMARKER_HXX_