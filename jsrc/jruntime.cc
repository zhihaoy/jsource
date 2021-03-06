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
  m.doc() = "Python extension that interacts with the J language";

  py::class_<scope>(m, "_Scope", py::module_local())
    .def("__enter__", &scope::enter)
    .def("__exit__", &scope::exit);

  py::class_<JST, pointer>(m, "Session")
    .def(py::init(create), py::kw_only(), "silent"_a = false,
         "binpath"_a = py::none(),
         R"doc(
         Create a J session

         Associate J's input callback with :py:func:`input` and output
         callback with :py:func:`print`.
         If ``binpath`` is :py:data:`None`, the session starts with an
         empty locale.  Otherwise, the session loads the J standard
         library from the parent directory of ``binpath`` as if it is
         a J front-end (jconsole, jqt, etc.).

         Parameters
         ----------
         binpath
             Path-like object that points to the `bin` directory under
             the J standard library
         )doc")
    .def("__getitem__", getitem,
         R"doc(
         Retrieve J nouns as Python and NumPy objects

         If ``arg0`` refers to a J noun, determine the Python side
         types using the following table:

         +--------+------------+-------------------------------------------+
         | Rank   | J datatype | Python type                               |
         +========+============+===========================================+
         | *N*    | boolean    | :py:class:`np.bool_ <numpy.bool_>`        |
         +--------+------------+-------------------------------------------+
         | *N*    | integer    | :py:class:`np.int64 <numpy.int_>`         |
         +--------+------------+-------------------------------------------+
         | *N*    | floating   | :py:class:`np.float64 <numpy.double>`     |
         +--------+------------+-------------------------------------------+
         | *N*    | complex    | :py:class:`np.complex128 <numpy.cdouble>` |
         +--------+------------+-------------------------------------------+
         | 0 or 1 | literal    | :py:class:`np.str_ <numpy.str_>`          |
         +--------+------------+-------------------------------------------+
         | 0 or 1 | unicode    | :py:class:`np.str_ <numpy.str_>`          |
         +--------+------------+-------------------------------------------+
         | 0 or 1 | boxed      | :py:class:`tuple`                         |
         +--------+------------+-------------------------------------------+

         J literals or Unicode arrays of rank 0 or 1 are converted to
         :ref:`NumPy scalars <arrays.scalars.built-in>` that act as
         Python strings.  An unboxed J atom of other types is
         converted to a NumPy scalar that acts as a 0-dimensional
         :py:class:`ndarray <numpy.ndarray>`.  A boxed J array of rank
         0 or 1 is deemed an object of product type and is converted
         to a tuple recursively.

         Parameters
         ----------
         arg0
             Name of the noun

         Returns
         -------
         Union[tuple, numpy.ndarray]
             The converted object

         Raises
         ------
         UnicodeDecodeError
             If attempts to convert a literal that is not a valid
             UTF-8 string

         ValueError
             If attempts to convert a J array of unsupported
             dimensions or datatypes

         JError
             If ``arg0`` does not refer to a J noun

         Examples
         --------
         >>> j.runsource("foo =: ((<'hello');(3.1415;4j1);i.6)")
         >>> j['foo']
         (('hello',), (3.1415, (4+1j)), array([0, 1, 2, 3, 4, 5], dtype=int64))
         >>> j['bar']
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         jruntime.JError: domain error

         See Also
         --------
         :ref:`arrays.scalars`,
         :j:`noun <AET#Noun>`,
         :j:`atom <AET#Atom>`,
         :j:`array <AET#Array>`
         )doc")
    .def("__setitem__", setitem, u8R"doc(
         Assign Python and NumPy objects to J nouns

         Create a J array and assign it to a noun named by the value
         of ``arg0`` in the J session.  Determine the type of the
         array using the following table:

         +--------------------------------------------------------+------------+------+
         | Python types                                           | J datatype | Rank |
         +========================================================+============+======+
         | :term:`array_like` object of :py:class:`bool`          | boolean    | *N*  |
         +--------------------------------------------------------+------------+------+
         | :term:`array_like` object of :py:class:`numpy.integer` | integer    | *N*  |
         +--------------------------------------------------------+------------+------+
         | :term:`array_like` object of :py:class:`float`         | floating   | *N*  |
         +--------------------------------------------------------+------------+------+
         | :term:`array_like` object of :py:class:`complex`       | complex    | *N*  |
         +--------------------------------------------------------+------------+------+
         | :py:class:`bytes`                                      | literals   | 1    |
         +--------------------------------------------------------+------------+------+
         | :py:class:`str`                                        | unicode    | 1    |
         +--------------------------------------------------------+------------+------+
         | :py:class:`tuple`                                      | boxed      | 1    |
         +--------------------------------------------------------+------------+------+

         If ``arg1`` is a tuple, create a boxed array recursively.
         Otherwise, the Python object to convert should be an
         :term:`array-like <array_like>` object of a supported
         :term:`dtype`.  If the object is an :term:`array scalar`,
         create an unboxed atom; otherwise, the array to make has
         a rank greater than 0.

         When converting from integers, if the dtype ends up being
         :py:class:`np.uint64 <numpy.uint>` or :py:class:`object`,
         they are not supported.  These can happen when converting
         from :py:class:`int`, which is an integer type of infinite
         precision in Python.

         Parameters
         ----------
         arg0
             Name for the noun
         arg1
             Value for the noun

         Raises
         ------
         ValueError
             If attempts to convert from unsupported :term:`dtype`

         JError
             If ``arg0`` is not a valid name

         Examples
         --------
         >>> j['x'] = (np.arange(12).reshape(3, 4), ('world',), 6-3j)
         >>> j.runsource('x')
         ┌─────────┬───────┬────┐
         │0 1  2  3│┌─────┐│6j_3│
         │4 5  6  7││world││    │
         │8 9 10 11│└─────┘│    │
         └─────────┴───────┴────┘
         >>> j['y'] = 2 ** 31
         >>> j.runsource('y')
         2147483648
         >>> j['y'] = 2 ** 63
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         ValueError: unsupported dtype
         )doc")
    .def("let", scope::make,
         R"doc(
         Bind J names to Python values in a scope

         Emulates lexical bindings in other languages, except that
         the "scope" for J is established using a :ref:`with <with>`
         statement in Python.

         Parameters
         ----------
         kwargs
             Nouns in name-value pairs

         Returns
         -------
         _Scope
             Context manager that makes each noun in ``kwargs``
             available in the ``self`` J session until reaching the
             end of a :ref:`with <with>` statement

         Warning
         -------
         Do not enter the scope after the lifetime of ``self`` has
         ended.

         Examples
         --------
         >>> with j.let(v1=np.arange(12).reshape(4, 3) / 2, tag='3x4'):
         ...     z = j.eval("('sum ',tag);+/ v1")
         >>> z
         ('sum 3x4', array([ 9., 11., 13.]))
         >>> j['v1']
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         jruntime.JError: domain error

         See Also
         --------
         `let <https://www.cs.utexas.edu/ftp/garbage/cs345/schintro-v14/schintro_54.html>`_
         )doc")
    .def("runsource", runsource, "sentence"_a.none(false),
         py::call_guard<py::gil_scoped_release>(),
         R"doc(
         Compile and run some sentence in J

         Execute the J code in ``sentence`` and echo the result,
         if any, using :py:func:`print`.  If the sentence expects
         a *body*, iteratively read more lines using :py:func:`input`
         until encountering a `terminator`.

         Parameters
         ----------
         sentence
             One line of J code

         Raises
         ------
         JError
             If the sentence fail to execute

         Examples
         --------
         >>> j.runsource('v =: i.12')
         >>> j.runsource("f =: 3 : '_5 + x'")
         >>> j.runsource('f v')
         _5 _4 _3 _2 _1 0 1 2 3 4 5 6
         >>> j.runsource('v f')
         |syntax error
         |       v f
         Traceback (most recent call last):
           File "<stdin>", line 1, in <module>
         jruntime.JError: syntax error

         See Also
         --------
         :j:`Sentences <Words#Sentence>`,
         :j:`multiline explicit definition <cor#Notes>`
         )doc")
    .def("eval", eval, "sentence"_a.none(false),
         R"doc(
         Evaluate a sentence and return its result to Python

         Unlike :py:meth:`runsource`, which may echo the result,
         :py:meth:`eval` returns the results from J to Python as if
         creating a temporary noun and getting the noun using
         :py:meth:`__getitem__`.
         Raises any exception that :py:meth:`runsource` and
         :py:meth:`__getitem__` may raise.

         Parameters
         ----------
         sentence
             One line of J code

         Returns
         -------
         Union[tuple, numpy.ndarray]
             The converted result object

         Examples
         --------
         >>> j.runsource("('matrix';2 2 $ 0.1 0.2 0.3 0.4)")
         ┌──────┬───────┐
         │matrix│0.1 0.2│
         │      │0.3 0.4│
         └──────┴───────┘
         >>> j.eval("z =: ('matrix';2 2 $ 0.1 0.2 0.3 0.4)")
         ('matrix', array([[0.1, 0.2],
                [0.3, 0.4]]))
         >>> j.eval('>0 } z')
         'matrix'
         >>> j.eval('>1 } z')
         array([[0.1, 0.2],
                [0.3, 0.4]])
         )doc");

  py::register_exception<system_error>(m, "JError", PyExc_RuntimeError).doc() =
    "Exception raised when the J engine reports an error.";
  m.attr("__version__") = "0.1.4";
}

} // namespace j
