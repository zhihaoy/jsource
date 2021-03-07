#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <tuple>

#include "dragonbox/dragonbox.h"

extern "C"
{
  void dtoaroundtrip(int64_t len, unsigned char *buf, double x);
  void dtoafixed(int64_t len, unsigned char *buf, double x, int64_t precision);
}

void
dtoaroundtrip(int64_t len, unsigned char *buf, double x)
{
  using namespace jkj::dragonbox;

  auto p = reinterpret_cast<char *>(buf);
  auto end = p + len;

  auto add_leading_zeros = [](char *p, char *last, auto offset) {
    auto len = std::distance(p, last);
    std::move(p, last, p + 2 - offset);
    p[0] = '0';
    p[1] = '.';
    return std::fill_n(p + 2, -offset, '0') + len;
  };

  auto insert_decimal_point = [](char *p, char *last, auto offset) {
    auto end_of_integer_part = p + offset;
    std::move(end_of_integer_part, last, std::next(end_of_integer_part));
    *end_of_integer_part = '.';
    return std::next(last);
  };

  auto add_trailing_zeros = [](char *p, char *last, auto offset) {
    return std::fill_n(last, offset - std::distance(p, last), '0');
  };

  /**
   * no more than 4 leading zeros (0 before the decimal point counts 1)
   * no more than 4 trailing zeros
   * if a significant is 17 digits long, it's okay to prepend it with up to 4
   * leading zeros, but if a significant is 16 digits long already, appending
   * any trailing zero would be misleading
   */
  auto adjust_decimal_point =
    [](auto decimal_length,
       auto exponent) -> decltype(std::tuple(decimal_length, exponent)) {
    if ((-4 < decimal_length + exponent && exponent <= 0) ||
        (0 < exponent && exponent <= 4 && decimal_length + exponent <= 16))
      return {decimal_length + exponent, 0};
    else
      return {1, exponent + decimal_length - 1};
  };

  auto add_j_exponent_part = [end](char *p, auto exponent) {
    *p++ = 'e';
    unsigned exp;
    if (exponent < 0) {
      *p++ = '_';
      exp = -exponent;
    } else {
      exp = exponent;
    }
    auto [ptr, ec] = std::to_chars(p, end, exp);
    return ptr;
  };

  auto br = ieee754_bits(x);
  assert(br.is_finite());

  if (br.is_nonzero()) {
    if (br.is_negative())
      *p++ = '_';

    auto [significant, exponent] =
      to_decimal(x, jkj::dragonbox::policy::sign::ignore);
    auto [ptr, ec] = std::to_chars(p, end, significant);
    auto decimal_length = std::distance(p, ptr);
    auto [offset, exp] = adjust_decimal_point(decimal_length, exponent);

    if (offset < 1)
      p = add_leading_zeros(p, ptr, offset); // 0.0031415
    else if (offset < decimal_length)
      p = insert_decimal_point(p, ptr, offset); // 3.1415
    else if (offset > decimal_length)
      p = add_trailing_zeros(p, ptr, offset); // 31415000
    else
      p = ptr; // 31415

    if (exp != 0)
      p = add_j_exponent_part(p, exp);
  } else {
    *p++ = '0';
  }

  *p = '\0';
}

void
dtoafixed(int64_t len, unsigned char *buf, double x, int64_t precision)
{
  auto p = reinterpret_cast<char *>(buf);
  static char const fmt[][6] = {
    "",      "%.1g",  "%.2g",  "%.3g",  "%.4g",  "%.5g",  "%.6g",
    "%.7g",  "%.8g",  "%.9g",  "%.10g", "%.11g", "%.12g", "%.13g",
    "%.14g", "%.15g", "%.16g", "%.17g", "%.18g", "%.19g", "%.20g",
  };
  std::snprintf(p, len, fmt[precision], x);
}
