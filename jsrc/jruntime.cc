#include <algorithm>
#include <complex>
#include <cstring>
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
#undef str
#undef shape
}

namespace j {

struct deleter
{
  void operator()(J p) const noexcept { JFree(p); }
};

using pointer = std::unique_ptr<JST, deleter>;

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
  py::gil_scoped_acquire _;
  py::print(fromj(s), "end"_a = "");
};

static constexpr auto jinput = [](J jt, C* s) {
  static std::string buf;
  try {
    py::gil_scoped_acquire _;
    buf.assign(py::module_::import(PYBIND11_BUILTINS_MODULE)
                 .attr("input")(fromj(s))
                 .cast<std::string>());
  } catch (py::error_already_set&) {
    jt->recurstate = RECSTATEIDLE;
    throw;
  }
  return toj(buf.data());
};

struct nonesuch
{};

template<class... T> struct always_false : std::false_type
{};
template<> struct always_false<class dont_use> : std::true_type
{};

template<class T> inline constexpr auto scalar_type_name = nonesuch{};
template<> inline constexpr auto scalar_type_name<bool> = "bool_";
template<> inline constexpr auto scalar_type_name<int64_t> = "int64";
template<> inline constexpr auto scalar_type_name<double> = "float64";
template<>
inline constexpr auto scalar_type_name<std::complex<double>> = "complex128";

template<class T> inline constexpr auto scalar_type_enum = nonesuch{};
template<> inline constexpr auto scalar_type_enum<bool> = B01;
template<> inline constexpr auto scalar_type_enum<int64_t> = INT;
template<> inline constexpr auto scalar_type_enum<double> = FL;
template<> inline constexpr auto scalar_type_enum<std::complex<double>> = CMPX;

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

inline void
setup(JST& self, outputtype fnout = nullptr, inputtype fnin = nullptr)
{
  // when setting SMOPTMTH, stack position is reinitialized for each JDo
  JSMX(&self, reinterpret_cast<void*>(fnout), nullptr,
       reinterpret_cast<void*>(fnin), nullptr, SMOPTMTH << 8);
}

template<class T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
inline auto
gc_array(JST& self, I type, I n, I rank, T const* shape)
{
  static_assert(std::numeric_limits<T>::digits ==
                std::numeric_limits<I>::digits);

  if (auto p = Jga(&self, type, n, rank,
                   reinterpret_cast<I*>(const_cast<T*>(shape)))) {
    p->c |= ACINPLACE;
    return p;
  } else {
    throw system_error(self, self.jerr);
  }
}

inline auto
gc_array(JST& self, I type, I n)
{
  I shape[] = {n};
  return gc_array(self, type, n, 1, shape);
}

template<class T>
inline void
copy(py::array_t<T> const& a, A w)
{
  if constexpr (std::is_same_v<T, bool>)
    std::copy_n(a.data(), a.size(), BAV(w));
  else if constexpr (std::is_floating_point_v<T>)
    std::copy_n(a.data(), a.size(), DAV(w));
  else if constexpr (std::is_integral_v<T> and std::is_signed_v<T>)
    std::copy_n(a.data(), a.size(), IAV(w));
  else if constexpr (py::detail::is_complex<T>())
    std::transform(a.data(), a.data() + a.size(), ZAV(w), [](auto& cmpx) {
      return Z{real(cmpx), imag(cmpx)};
    });
  else
    static_assert(always_false<T>(), "no compatible J datatype");
}

template<class T>
inline auto
numpy_to_j(JST& self, py::array& arr)
{
  auto w =
    gc_array(self, scalar_type_enum<T>, arr.size(), arr.ndim(), arr.shape());
  auto c = arr.dtype().attr("char").cast<char>();
  switch (c) {
  case '?':
    copy<bool>(arr, w);
    break;
  case 'b':
    copy<int8_t>(arr, w);
    break;
  case 'h':
    copy<int16_t>(arr, w);
    break;
  case 'l':
    copy<int32_t>(arr, w);
    break;
  case 'q':
    copy<int64_t>(arr, w);
    break;
  case 'f':
    copy<float>(arr, w);
    break;
  case 'd':
    copy<double>(arr, w);
    break;
  case 'F':
    copy<std::complex<float>>(arr, w);
    break;
  case 'D':
    copy<std::complex<double>>(arr, w);
    break;
  default:
    throw py::value_error{"unrecognized dtype"};
  }
  return w;
}

template<class T>
inline auto
numpy_str_to_j(JST& self, py::array& arr)
{
  if (arr.ndim() == 0) {
    auto w = [&] {
      if constexpr (std::is_same_v<T, unsigned char>)
        return gc_array(self, LIT, arr.itemsize());
      else if constexpr (std::is_same_v<T, char32_t>)
        return gc_array(self, C4T, arr.itemsize() / C4TSIZE);
    }();
    std::memcpy(voidAV(w), arr.data(), arr.itemsize());
    return w;
  } else {
    throw py::value_error{"cannot convert ndarray of strings"};
  }
}

inline auto
numpy_to_j(JST& self, py::object& data)
{
  auto arr = py::array(data);
  switch (arr.dtype().kind()) {
  case 'b':
    return numpy_to_j<bool>(self, arr);
  case 'i':
    return numpy_to_j<int64_t>(self, arr);
  case 'f':
    return numpy_to_j<double>(self, arr);
  case 'c':
    return numpy_to_j<std::complex<double>>(self, arr);
  case 'S':
    return numpy_str_to_j<unsigned char>(self, arr);
  case 'U':
    return numpy_str_to_j<char32_t>(self, arr);
  default:
    throw py::value_error{"unsupported dtype"};
  }
}

inline auto
object_to_j(JST& self, py::object data) -> A
{
  if (py::isinstance<py::tuple>(data)) {
    auto tu = data.cast<py::tuple>();
    auto n = len(tu);
    auto w = gc_array(self, BOX, n);
    for (decltype(n) i = 0; i < n; ++i)
      AAV(w)[i] = object_to_j(self, tu[i]);
    return w;
  } else {
    return numpy_to_j(self, data);
  }
}

static constexpr auto setitem = [](JST& self, char const* name,
                                   py::object data) {
  if (auto r = Jassoc(&self, toj(name), object_to_j(self, std::move(data)));
      r != 0)
    throw system_error(self, r);
};

static constexpr auto runsource = [](JST& self, char const* src) {
  if (auto r = JDo(&self, toj(src)); r != 0)
    throw system_error(self, r);
};

inline void
runsource_(JST& self, std::string const& src)
{
  runsource(self, src.data());
}

inline void
setitem_(JST& self, std::string const& name, py::object data)
{
  setitem(self, name.data(), std::move(data));
}

inline void
simulate_jfe(JST& self, py::object binpath)
{
  auto Path = py::module_::import("pathlib").attr("Path");
  auto sys = py::module_::import("sys");
  auto bin = Path(binpath).attr("resolve")();

  setitem(self, "ARGV_z_", py::make_tuple(""_s));
  setitem(self, "BINPATH_z_", bin.attr("as_posix")());
  {
    py::gil_scoped_release _;
    runsource(self, "0!:0<BINPATH,'/profile.ijs'");
  }

  if (sys.attr("stdout").attr("isatty")().cast<bool>())
    runsource(self, "0 0$boxdraw_j_ 0");
  else
    runsource(self, "0 0$boxdraw_j_ 1");
}

static constexpr auto create = [](bool silent, py::object binpath) {
  auto jt = pointer(JInit());

  if (silent)
    setup(*jt);
  else
    setup(*jt, joutput, jinput);

  if (not binpath.is_none())
    simulate_jfe(*jt, binpath);

  return jt;
};

static constexpr auto eval = [](JST& self, std::string_view src) {
  auto cmd = "eval_jpyd_=:"s;
  cmd += src;
  {
    py::gil_scoped_release _;
    runsource_(self, cmd);
  }
  struct defer
  {
    J jt;
    ~defer() { JDo(jt, toj("0!:100 '4!:55<''eval_jpyd_'''")); }
  } _{&self};
  return getitem(self, "eval_jpyd_");
};

struct scope
{
  // the reference can be dangling if the context manager is __enter__()'ed
  // after the session object is gone
  JST& self;
  py::kwargs kwds;

  void enter()
  {
    for (auto& [name, value] : kwds) {
      setitem_(self, py::reinterpret_borrow<py::str>(name),
               py::reinterpret_borrow<py::object>(value));
    }
  }

  void exit(py::args)
  {
    auto cmd = "0!:100 '4!:55<''{}'''"_s;
    for (auto& [name, _] : kwds)
      runsource_(self, cmd.format(name));
  }

  static auto make(JST& self, py::kwargs kwds)
  {
    return scope{self, std::move(kwds)};
  }
};

PYBIND11_MODULE(jruntime, m)
{
  m.doc() = "J Language Runtime";

  py::class_<scope>(m, "_Scope", py::module_local())
    .def("__enter__", &scope::enter)
    .def("__exit__", &scope::exit);

  py::class_<JST, pointer>(m, "Session")
    .def(py::init(create), py::kw_only(), "silent"_a = false,
         "binpath"_a = py::none(), "Create an J session which may simulate JFE")
    .def("__getitem__", getitem, "Retrieve J nouns as Python and NumPy objects")
    .def("__setitem__", setitem, "Assign Python and NumPy objects to J nouns")
    .def("let", scope::make, "Bind J names to Python values in a scope")
    .def("runsource", runsource, "sentence"_a.none(false),
         py::call_guard<py::gil_scoped_release>(),
         "Compile and run some sentence in J")
    .def("eval", eval, "sentence"_a.none(false),
         "Evaluate a sentence and return its result to Python");

  py::register_exception<system_error>(m, "JError", PyExc_RuntimeError);
  m.attr("__version__") = "0.1.3";
}

} // namespace j
