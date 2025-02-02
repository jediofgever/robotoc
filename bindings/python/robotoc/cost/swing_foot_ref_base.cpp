#include <pybind11/pybind11.h>
#include <pybind11/eigen.h>
#include <pybind11/numpy.h>

#include "robotoc/cost/swing_foot_cost.hpp"


namespace robotoc {
namespace python {

class PySwingFootRefBase : public SwingFootRefBase {
public:
  // Inherit the constructors
  using SwingFootRefBase::SwingFootRefBase;

  void update_x3d_ref(const ContactStatus& contact_status, 
                      Eigen::VectorXd& x3d_ref) const override {
    PYBIND11_OVERRIDE_PURE(void, SwingFootRefBase, 
                           update_x3d_ref, 
                           contact_status, x3d_ref);
  }
};

namespace py = pybind11;

PYBIND11_MODULE(swing_foot_ref_base, m) {
  py::class_<SwingFootRefBase, PySwingFootRefBase,
             std::shared_ptr<SwingFootRefBase>>(m, "SwingFootRefBase")
    .def(py::init<>())
    .def("update_x3d_ref", &SwingFootRefBase::update_x3d_ref,
          py::arg("contact_status"), py::arg("x3d_ref"));
}

} // namespace python
} // namespace robotoc