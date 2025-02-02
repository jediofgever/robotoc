#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "robotoc/mpc/biped_walk_foot_step_planner.hpp"


namespace robotoc {
namespace python {

namespace py = pybind11;

PYBIND11_MODULE(biped_walk_foot_step_planner, m) {
  py::class_<BipedWalkFootStepPlanner, ContactPlannerBase, 
             std::shared_ptr<BipedWalkFootStepPlanner>>(m, "BipedWalkFootStepPlanner")
    .def(py::init<const Robot&>(),
         py::arg("biped_robot"))
    .def("set_gait_pattern", 
          static_cast<void (BipedWalkFootStepPlanner::*)(const Eigen::Vector3d&, const double, const bool)>(&BipedWalkFootStepPlanner::setGaitPattern),
          py::arg("step_length"), py::arg("step_yaw"), py::arg("enable_double_support_phase")) 
    .def("set_gait_pattern", 
          static_cast<void (BipedWalkFootStepPlanner::*)(const Eigen::Vector3d&, const double, const double, const double, const double)>(&BipedWalkFootStepPlanner::setGaitPattern),
          py::arg("v_com_cmd"), py::arg("yaw_rate_cmd"), 
          py::arg("t_swing"), py::arg("t_stance"), py::arg("gain")) 
    .def("init", &BipedWalkFootStepPlanner::init,
          py::arg("q"))
    .def("plan", &BipedWalkFootStepPlanner::plan,
          py::arg("q"), py::arg("v"), py::arg("contact_status"), py::arg("planning_steps"))
    .def("contact_position", 
          static_cast<const std::vector<Eigen::Vector3d>& (BipedWalkFootStepPlanner::*)(const int) const>(&BipedWalkFootStepPlanner::contactPosition),
          py::arg("step"))
    .def("contact_position", 
          static_cast<const std::vector<std::vector<Eigen::Vector3d>>& (BipedWalkFootStepPlanner::*)() const>(&BipedWalkFootStepPlanner::contactPosition))
    .def("com", 
          static_cast<const Eigen::Vector3d& (BipedWalkFootStepPlanner::*)(const int) const>(&BipedWalkFootStepPlanner::com),
          py::arg("step"))
    .def("com", 
          static_cast<const std::vector<Eigen::Vector3d>& (BipedWalkFootStepPlanner::*)() const>(&BipedWalkFootStepPlanner::com))
    .def("R", 
          static_cast<const Eigen::Matrix3d& (BipedWalkFootStepPlanner::*)(const int) const>(&BipedWalkFootStepPlanner::R),
          py::arg("step"))
    .def("R", 
          static_cast<const std::vector<Eigen::Matrix3d>& (BipedWalkFootStepPlanner::*)() const>(&BipedWalkFootStepPlanner::R))
    .def("__str__", [](const BipedWalkFootStepPlanner& self) {
        std::stringstream ss;
        ss << self;
        return ss.str();
      });
}

} // namespace python
} // namespace robotoc