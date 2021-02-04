#include <complex>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

#ifdef _WIN32
#define JRUNTIME_API __declspec(dllimport)
#endif

extern "C"
{
#include "j.h"
#include "jlib.h"
#undef move
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
using namespace std::literals;

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

template<class T> inline constexpr auto codec_name = nonesuch{};
template<> inline constexpr auto codec_name<unsigned char> = "utf-8";
template<> inline constexpr auto codec_name<char16_t> = "utf-16";
template<> inline constexpr auto codec_name<char32_t> = "utf-32";

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

template<class T>
inline auto
noun_to_numpy_str(I rank, I const* shape, void const* data) -> py::object
{
  if (rank > 1)
    throw py::value_error{"cannot convert multidimensional string"};
  auto n = rank ? shape[0] : 1;
  auto mem = py::memoryview::from_memory(data, n * sizeof(T));
  return py::module_::import("numpy").attr("str_")(mem, codec_name<T>);
}

inline auto noun_to_python(I, I, I const*, void const*) -> py::object;

inline auto
noun_to_tuple(I rank, I const* shape, void const* data)
{
  if (rank > 1)
    throw py::value_error{"cannot convert multidimensional tuple"};
  auto w = reinterpret_cast<A const*>(data);
  auto n = rank ? shape[0] : 1;
  auto tu = py::tuple(n);
  for (decltype(n) i = 0; i < n; ++i) {
    auto a = w[i];
    tu[i] = noun_to_python(AT(a), AR(a), AS(a), AV(a));
  }
  return tu;
}

auto
noun_to_python(I datatype, I rank, I const* shape, void const* data)
  -> py::object
{
  switch (CTTZ(datatype)) {
  case B01X:
    return noun_to_numpy<bool>(rank, shape, data);
  case LITX:
    return noun_to_numpy_str<unsigned char>(rank, shape, data);
  case INTX:
    return noun_to_numpy<int64_t>(rank, shape, data);
  case FLX:
    return noun_to_numpy<double>(rank, shape, data);
  case CMPXX:
    return noun_to_numpy<std::complex<double>>(rank, shape, data);
  case BOXX:
    return noun_to_tuple(rank, shape, data);
  case C2TX:
    return noun_to_numpy_str<char16_t>(rank, shape, data);
  case C4TX:
    return noun_to_numpy_str<char32_t>(rank, shape, data);
  default:
    throw py::value_error{"unsupported datatype"};
  }
}

static constexpr auto getitem = [](JST& self, char const* name) {
  I jtype, jrank, jshape, jdata;
  if (auto r = JGetM(&self, toj(name), &jtype, &jrank, &jshape, &jdata); r != 0)
    throw system_error(self, r);

  return noun_to_python(jtype, jrank, reinterpret_cast<I*>(jshape),
                        reinterpret_cast<void*>(jdata));
};

static constexpr auto runsource = [](JST& self, char const* src) {
  if (auto r = JDo(&self, toj(src)); r != 0)
    throw system_error(self, r);
};

static constexpr auto create = [](bool silent) {
  auto jt = JInit();
  if (silent)
    JSMX(jt, nullptr, nullptr, nullptr, nullptr, SMOPTMTH);
  else
    JSMX(jt, reinterpret_cast<void*>(+joutput), nullptr,
         reinterpret_cast<void*>(+jinput), nullptr, SMOPTMTH);
  return jt;
};

static constexpr auto eval = [](JST& self, std::string_view src) {
  auto cmd = "eval_jpyd_=:"s;
  cmd += src;
  runsource(self, cmd.data());
  struct defer
  {
    J jt;
    ~defer() { JDo(jt, toj("0!:100 '4!:55<''eval_jpyd_'''")); }
  } _{&self};
  return getitem(self, "eval_jpyd_");
};

PYBIND11_MODULE(jruntime, m)
{
  m.doc() = "J Language Runtime";

  py::class_<JST, std::unique_ptr<JST, deleter>>(m, "Session")
    .def(py::init(create), py::kw_only(), "silent"_a = false,
         "Create an empty J session")
    .def("__getitem__", getitem, "Retrieve J nouns as Python and NumPy objects")
    .def("runsource", runsource, "sentence"_a.none(false),
         "Compile and run some sentence in J")
    .def("eval", eval, "sentence"_a.none(false),
         "Evaluate a sentence and return its result to Python");

  py::register_exception<system_error>(m, "JError", PyExc_RuntimeError);
  m.attr("__version__") = "0.1.0";
}

} // namespace j
