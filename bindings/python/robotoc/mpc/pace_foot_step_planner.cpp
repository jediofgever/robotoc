#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "robotoc/mpc/pace_foot_step_planner.hpp"


namespace robotoc {
namespace python {

namespace py = pybind11;

PYBIND11_MODULE(pace_foot_step_planner, m) {
  py::class_<PaceFootStepPlanner, ContactPlannerBase, 
             std::shared_ptr<PaceFootStepPlanner>>(m, "PaceFootStepPlanner")
    .def(py::init<const Robot&>(),
         py::arg("quadruped_robot"))
    .def("set_gait_pattern", 
          static_cast<void (PaceFootStepPlanner::*)(const Eigen::Vector3d&, const double, const bool)>(&PaceFootStepPlanner::setGaitPattern),
          py::arg("step_length"), py::arg("step_yaw"), py::arg("enable_stance_phase")) 
    .def("set_gait_pattern", 
          static_cast<void (PaceFootStepPlanner::*)(const Eigen::Vector3d&, const double, const double, const double, const double)>(&PaceFootStepPlanner::setGaitPattern),
          py::arg("v_com_cmd"), py::arg("yaw_rate_cmd"), 
          py::arg("t_swing"), py::arg("t_stance"), py::arg("gain")) 
    .def("init", &PaceFootStepPlanner::init,
          py::arg("q"))
    .def("plan", &PaceFootStepPlanner::plan,
          py::arg("q"), py::arg("v"), py::arg("contact_status"), py::arg("planning_steps"))
    .def("contact_position", 
          static_cast<const std::vector<Eigen::Vector3d>& (PaceFootStepPlanner::*)(const int) const>(&PaceFootStepPlanner::contactPosition),
          py::arg("step"))
    .def("contact_position", 
          static_cast<const std::vector<std::vector<Eigen::Vector3d>>& (PaceFootStepPlanner::*)() const>(&PaceFootStepPlanner::contactPosition))
    .def("com", 
          static_cast<const Eigen::Vector3d& (PaceFootStepPlanner::*)(const int) const>(&PaceFootStepPlanner::com),
          py::arg("step"))
    .def("com", 
          static_cast<const std::vector<Eigen::Vector3d>& (PaceFootStepPlanner::*)() const>(&PaceFootStepPlanner::com))
    .def("R", 
          static_cast<const Eigen::Matrix3d& (PaceFootStepPlanner::*)(const int) const>(&PaceFootStepPlanner::R),
          py::arg("step"))
    .def("R", 
          static_cast<const std::vector<Eigen::Matrix3d>& (PaceFootStepPlanner::*)() const>(&PaceFootStepPlanner::R))
    .def("__str__", [](const PaceFootStepPlanner& self) {
        std::stringstream ss;
        ss << self;
        return ss.str();
      });
}

} // namespace python
} // namespace robotoc