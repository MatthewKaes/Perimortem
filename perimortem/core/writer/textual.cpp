// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/writer/textual.hpp"

#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;

template <typename text_buffer>
constexpr auto
    write_text(Access::Bytes& target, Count& cursor, const text_buffer& text)
        -> Bool {
  if (cursor + text.get_size() > target.get_size()) {
    return false;
  }

  auto data = target.get_data();
  Data::copy(data + cursor, text.get_data(), text.get_size());
  cursor += text.get_size();

  return true;
}

constexpr auto create_digit_table() -> Static::Bytes<200> {
  Static::Bytes<200> values;
  for (Count d1 = 0; d1 < 10; d1++) {
    for (Count d2 = 0; d2 < 10; d2++) {
      auto location = (d2 * 10 + d1) * 2;
      values[location + 0] = '0' + d2;
      values[location + 1] = '0' + d1;
    }
  }

  return values;
}

constexpr auto digit_buffer = create_digit_table();

template <typename storage_type, typename unsigned_type>
static constexpr auto decimal_length(storage_type value) -> Count {
  Count sign = 0;
  unsigned_type abs_value;

  // Capture the sign value for types that support it.
  if constexpr (storage_type(0) > storage_type(-1)) {
    if (value < storage_type(0)) {
      sign = 1;
      abs_value = unsigned_type(-value);
    } else {
      abs_value = unsigned_type(value);
    }
  } else {
    abs_value = unsigned_type(value);
  }

  // A comparision ladder seems to perform quite a bit better (~20%) than the
  // general path with dividing down the value to caculate log10.
  if (abs_value < 10ULL) {
    return sign + 1;
  } else if (abs_value < 100ULL) {
    return sign + 2;
  } else if (abs_value < 1'000ULL) {
    return sign + 3;
  } else if (abs_value < 10'000ULL) {
    return sign + 4;
  } else if (abs_value < 100'000ULL) {
    return sign + 5;
  } else if (abs_value < 1'000'000ULL) {
    return sign + 6;
  } else if (abs_value < 10'000'000ULL) {
    return sign + 7;
  } else if (abs_value < 100'000'000ULL) {
    return sign + 8;
  } else if (abs_value < 1'000'000'000ULL) {
    return sign + 9;
  } else if (abs_value < 10'000'000'000ULL) {
    return sign + 10;
  } else if (abs_value < 100'000'000'000ULL) {
    return sign + 11;
  } else if (abs_value < 1'000'000'000'000ULL) {
    return sign + 12;
  } else if (abs_value < 10'000'000'000'000ULL) {
    return sign + 13;
  } else if (abs_value < 100'000'000'000'000ULL) {
    return sign + 14;
  } else if (abs_value < 1'000'000'000'000'000ULL) {
    return sign + 15;
  } else if (abs_value < 10'000'000'000'000'000ULL) {
    return sign + 16;
  } else if (abs_value < 100'000'000'000'000'000ULL) {
    return sign + 17;
  } else if (abs_value < 1'000'000'000'000'000'000ULL) {
    return sign + 18;
  } else if (abs_value < 10'000'000'000'000'000'000ULL) {
    return sign + 19;
  }
  return sign + 20;
}

// Backwards-fill variant: caller supplies length so digits are placed directly
// into the output buffer at the correct offset without a scratch copy.
template <typename storage_type>
constexpr auto write_decimal(
    Access::Bytes& target,
    Count& cursor,
    storage_type value,
    Count length) -> Bool {
  if (cursor + length > target.get_size()) [[unlikely]] {
    return false;
  }

  auto data = target.get_data();
  if (value == 0) {
    data[cursor++] = '0';
    return true;
  }

  Bits_8* output_digits = data + cursor;
  storage_type abs_value;
  Count digits = length;

  if constexpr (storage_type(0) > storage_type(-1)) {
    if (value < storage_type(0)) {
      *output_digits++ = '-';
      digits--;
      abs_value = storage_type(-value);
    } else {
      abs_value = storage_type(value);
    }
  } else {
    abs_value = storage_type(value);
  }

  Count digit_position = digits - 1;
  while (abs_value >= 10) {
    auto two_digits = Count(abs_value % 100) << 1;
    abs_value /= 100;
    output_digits[digit_position] = digit_buffer.get_data()[two_digits + 1];
    output_digits[digit_position - 1] = digit_buffer.get_data()[two_digits];
    digit_position -= 2;
  }

  if (abs_value > 0) {
    output_digits[0] = '0' + Bits_8(abs_value);
  }

  cursor += length;
  return true;
}

auto Writer::Textual::set_pointer(Count location) -> void {
  cursor = location;
}

auto Writer::Textual::operator<<(const Bool flag) -> Writer::Textual& {
  if (flag) {
    valid_state &= write_text(source, cursor, "true"_view);
  } else {
    valid_state &= write_text(source, cursor, "false"_view);
  }

  return *this;
}

auto Writer::Textual::operator<<(const Bits_8 value) -> Writer::Textual& {
  valid_state &= write_decimal(
      source, cursor, value, decimal_length<Bits_8, Bits_8>(value));

  return *this;
}

auto Writer::Textual::operator<<(const Bits_16 value) -> Writer::Textual& {
  valid_state &= write_decimal(
      source, cursor, value, decimal_length<Bits_16, Bits_16>(value));
  return *this;
}

auto Writer::Textual::operator<<(const Bits_32 value) -> Writer::Textual& {
  valid_state &= write_decimal(
      source, cursor, value, decimal_length<Bits_32, Bits_32>(value));
  return *this;
}

auto Writer::Textual::operator<<(const Bits_64 value) -> Writer::Textual& {
  valid_state &= write_decimal(
      source, cursor, value, decimal_length<Bits_64, Bits_64>(value));
  return *this;
}

auto Writer::Textual::operator<<(const Signed_8 character) -> Writer::Textual& {
  valid_state &= cursor < source.get_size();
  if (valid_state) {
    auto data = source.get_data();
    data[cursor++] = character;
  }

  return *this;
}

auto Writer::Textual::operator<<(const Signed_16 value) -> Writer::Textual& {
  valid_state &= write_decimal(
      source, cursor, value, decimal_length<Signed_16, Bits_16>(value));
  return *this;
}

auto Writer::Textual::operator<<(const Signed_32 value) -> Writer::Textual& {
  valid_state &= write_decimal(
      source, cursor, value, decimal_length<Signed_32, Bits_32>(value));
  return *this;
}

auto Writer::Textual::operator<<(const Signed_64 value) -> Writer::Textual& {
  valid_state &= write_decimal(
      source, cursor, value, decimal_length<Signed_64, Bits_64>(value));
  return *this;
}

auto Writer::Textual::operator<<(const Real_32 real_32) -> Writer::Textual& {
  write_real(Real_64(real_32), __FLT_EPSILON__);
  return *this;
}

auto Writer::Textual::operator<<(const Real_64 real_64) -> Writer::Textual& {
  write_real(real_64, __DBL_EPSILON__);
  return *this;
}

// TODO: This code is awful and should be optimized but it does a decent enough
// job without having to pull in bulky headers.
auto Writer::Textual::write_real(Real_64 real, Real_64 precision) -> void {
  if (!valid_state) {
    return;
  }

  constexpr auto max_length = 32;
  Signed_64 decimal_portion = Signed_64(real);
  auto length = decimal_length<Signed_64, Signed_64>(decimal_portion);
  valid_state &= write_decimal(source, cursor, decimal_portion, length);

  valid_state &= cursor < source.get_size();
  if (!valid_state) {
    return;
  }

  // Always write a decimal point.
  auto data = source.get_data();
  data[cursor++] = '.';
  length += 1;

  auto fract = real - decimal_portion;

  // Trailing zero and negative values.
  if (fract <= 0) {
    fract *= -1;
    if (fract < precision) {
      valid_state &= cursor < source.get_size();
      if (!valid_state) {
        return;
      }

      data[cursor++] = '0';
      length += 1;
    }
  }

  // Slowly write the rest of fractional part until we hit a limit.
  while (length < max_length && fract > precision) {
    if (cursor >= source.get_size()) {
      valid_state &= cursor < source.get_size();
      return;
    }

    fract *= 10;
    precision *= 10;
    length += 1;
    Bits_8 value = Bits_8(fract);
    fract -= value;

    // Deal with percision rolloff.
    if (fract > 0.99999) {
      fract = 0;
      value++;

      // If we some how are rounding a 9 due to percision issues.
      if (value > 10) {
        return;
      }
    }

    data[cursor++] = '0' + value;
  }
}

auto Writer::Textual::operator<<(const View::Bytes raw) -> Writer::Textual& {
  valid_state &= write_text(source, cursor, raw);
  return *this;
}
