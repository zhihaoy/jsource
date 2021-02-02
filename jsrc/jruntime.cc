#include <stdexcept>

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

class system_error : public std::runtime_error
{
  int code_;

public:
  explicit system_error(JST& self, int condition) noexcept
      : runtime_error([&](int ev) {
          I ret;
          JErrorTextM(&self, ev, &ret);
          return reinterpret_cast<char*>(ret);
        }(condition)),
        code_(condition)
  {}

  auto code() const noexcept -> int { return code_; }
};

namespace py = pybind11;

using namespace pybind11::literals;

constexpr auto joutput = [](J, int, C* s) {
  py::print(reinterpret_cast<char*>(s), "end"_a = "");
};

constexpr auto jinput = [](J jt, C* s) {
  static std::string buf;
  buf.assign(py::module_::import(PYBIND11_BUILTINS_MODULE)
               .attr("input")(reinterpret_cast<char*>(s))
               .cast<std::string>());
  return reinterpret_cast<C*>(buf.data());
};

PYBIND11_MODULE(jruntime, m)
{
  m.doc() = "J Language Runtime";

  py::class_<JST, std::unique_ptr<JST, deleter>>(m, "Session")
    .def(py::init([](bool silent) {
           auto jt = JInit();
           if (silent)
             JSMX(jt, nullptr, nullptr, nullptr, nullptr, SMOPTMTH);
           else
             JSMX(jt, reinterpret_cast<void*>(+joutput), nullptr,
                  reinterpret_cast<void*>(+jinput), nullptr, SMOPTMTH);
           return jt;
         }),
         py::kw_only(), "silent"_a = false, "Create an empty J session")
    .def(
      "runsource",
      [](JST& self, char const* src) {
        auto r = JDo(&self, reinterpret_cast<C*>(const_cast<char*>(src)));
        if (r != 0)
          throw system_error(self, r);
      },
      "sentence"_a.none(false), "Compile and run some sentence in J");

  py::register_exception<system_error>(m, "JError", PyExc_RuntimeError);
  m.attr("__version__") = "0.1.0";
}

} // namespace j
