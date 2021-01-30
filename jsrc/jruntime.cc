#include <pybind11/pybind11.h>

#ifdef _WIN32
#define JRUNTIME_API __declspec(dllimport)
#endif

extern "C"
{
#include "j.h"
#include "jlib.h"
}

namespace j {

struct deleter
{
  void operator()(J p) const noexcept { JFree(p); }
};

namespace py = pybind11;

PYBIND11_MODULE(jruntime, m)
{
  m.doc() = R"(
        J Language Runtime
        ------------------
        .. currentmodule:: jruntime
        .. autosummary::
           :toctree: _generate
    )";

  py::class_<JST, std::unique_ptr<JST, deleter>>(m, "Session")
    .def(py::init([]() { return JInit(); }));

  m.attr("__version__") = "0.1.0";
}

} // namespace j
