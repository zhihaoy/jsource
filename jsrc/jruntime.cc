#include <complex>
#include <stdexcept>

#include <pybind11/numpy.h>
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

inline auto
toj(char const* s)
{
  return reinterpret_cast<C*>(const_cast<char*>(s));
}

inline auto
fromj(C* s)
{
  return reinterpret_cast<char*>(s);
}

static constexpr auto joutput = [](J, int, C* s) {
  py::print(fromj(s), "end"_a = "");
};

static constexpr auto jinput = [](J jt, C* s) {
  static std::string buf;
  buf.assign(py::module_::import(PYBIND11_BUILTINS_MODULE)
               .attr("input")(fromj(s))
               .cast<std::string>());
  return toj(buf.data());
};

struct nonesuch
{};

template<class T> inline constexpr auto scalar_type_name = nonesuch{};
template<> inline constexpr auto scalar_type_name<bool> = "bool_";
template<> inline constexpr auto scalar_type_name<int64_t> = "int64";
template<> inline constexpr auto scalar_type_name<double> = "float64";
template<>
inline constexpr auto scalar_type_name<std::complex<double>> = "complex128";

template<class T>
inline auto
noun_to_ndarray(I rank, I const* shape, void const* data)
{
  return py::array_t(std::vector(shape, shape + rank),
                     reinterpret_cast<T const*>(data));
}

template<class T>
inline auto
noun_to_numpy(I rank, I const* shape, void const* data) -> py::object
{
  if (rank == 0) {
    auto np = py::module_::import("numpy");
    return np.attr(scalar_type_name<T>)(*reinterpret_cast<T const*>(data));
  } else {
    return noun_to_ndarray<T>(rank, shape, data);
  }
}

inline auto
noun_to_python(I datatype, I rank, I const* shape, void const* data)
  -> py::object
{
  switch (CTTZ(datatype)) {
  case B01X:
    return noun_to_numpy<bool>(rank, shape, data);
  case LITX:
    return py::none();
  case INTX:
    return noun_to_numpy<int64_t>(rank, shape, data);
  case FLX:
    return noun_to_numpy<double>(rank, shape, data);
  case CMPXX:
    return noun_to_numpy<std::complex<double>>(rank, shape, data);
  case BOXX:
    return py::none();
  case C2TX:
    return py::none();
  case C4TX:
    return py::none();
  default:
    throw py::value_error{"unsupported datatype"};
  }
}

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
      "__getitem__",
      [](JST& self, char const* name) {
        I jtype, jrank, jshape, jdata;
        if (auto r = JGetM(&self, toj(name), &jtype, &jrank, &jshape, &jdata);
            r != 0)
          throw system_error(self, r);

        return noun_to_python(jtype, jrank, reinterpret_cast<I*>(jshape),
                              reinterpret_cast<void*>(jdata));
      },
      "")
    .def(
      "runsource",
      [](JST& self, char const* src) {
        if (auto r = JDo(&self, toj(src)); r != 0)
          throw system_error(self, r);
      },
      "sentence"_a.none(false), "Compile and run some sentence in J");

  py::register_exception<system_error>(m, "JError", PyExc_RuntimeError);
  m.attr("__version__") = "0.1.0";
}

} // namespace j
