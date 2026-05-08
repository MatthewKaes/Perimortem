// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/serialization/textual/stream.hpp"
#include "perimortem/utility/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Serialization;

template <typename text_buffer>
constexpr auto write_text(Access::Bytes& data,
                          Count& ptr_location,
                          const text_buffer& text) -> Bool {
  if (ptr_location + text.get_size() > data.get_size()) {
    return false;
  }

  Data::copy(data.get_data() + ptr_location, text.get_data(), text.get_size());
  ptr_location += text.get_size();

  return true;
}

template <typename storage_type>
constexpr auto decimal_length(storage_type value) -> Count {
  Count digits = 0;

  // Sign detection for signed values.
  if constexpr (storage_type(0) > storage_type(-1)) {
    if (value < storage_type(0)) {
      digits += 1;
      value = -value;
    }
  }

  while (true) {
    // Comparision ladder which seems faster than the divide.
    if (value < 10) {
      return digits + 1;
    } else if (value < 100) {
      return digits + 2;
    } else if (value < 1'000) {
      return digits + 3;
    } else if (value < 10'000) {
      return digits + 4;
    } else if (value < 100'000) {
      return digits + 5;
    }

    digits += 5;
    value /= 100'000;
  }
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

template <typename storage_type>
constexpr auto write_decimal(Access::Bytes& data,
                             Count& ptr_location,
                             storage_type value,
                             Count length) -> Bool {
  if (ptr_location + length > data.get_size()) {
    return false;
  }

  if (value == 0) {
    if (ptr_location < data.get_size()) {
      data.get_data()[ptr_location++] = '0';
      return true;
    }

    return false;
  }

  // Sign detection for signed values.
  if constexpr (storage_type(0) > storage_type(-1)) {
    if (value < storage_type(0)) {
      data.get_data()[ptr_location++] = '-';
      value = -value;
      length -= 1;
    }
  }

  Count i;
  for (i = 0; i < length - 1; i += 2) {
    auto two_digits = (value % 100) << 1;
    value /= 100;
    memcpy(data.get_data() + ptr_location + length - i - 2,
           digit_buffer.get_data() + two_digits, 2);
  }

  // Left over digit
  if (value != 0) {
    data.get_data()[ptr_location] = '0' + value;
  }

  ptr_location += length;

  return true;
}

template <typename storage_type>
constexpr auto write_decimal(Access::Bytes& data,
                             Count& ptr_location,
                             storage_type value) -> Bool {
  auto length = decimal_length(value);
  return write_decimal(data, ptr_location, value, length);
}

auto Textual::Stream::operator<<(const Byte character) -> Textual::Stream& {
  valid_state &= ptr_location < data.get_size();
  if (valid_state) {
    data.get_data()[ptr_location++] = character;
  }

  return *this;
}

auto Textual::Stream::operator<<(const Bool boolean) -> Textual::Stream& {
  if (boolean) {
    valid_state &= write_text(data, ptr_location, "true"_view);
  } else {
    valid_state &= write_text(data, ptr_location, "false"_view);
  }

  return *this;
}

auto Textual::Stream::operator<<(const Half half) -> Textual::Stream& {
  valid_state &= write_decimal(data, ptr_location, half);
  return *this;
}

auto Textual::Stream::operator<<(const UHalf unsigned_half)
    -> Textual::Stream& {
  valid_state &= write_decimal(data, ptr_location, unsigned_half);
  return *this;
}

auto Textual::Stream::operator<<(const Int integer) -> Textual::Stream& {
  valid_state &= write_decimal(data, ptr_location, integer);
  return *this;
}

auto Textual::Stream::operator<<(const UInt unsigned_integer)
    -> Textual::Stream& {
  valid_state &= write_decimal(data, ptr_location, unsigned_integer);
  return *this;
}

auto Textual::Stream::operator<<(const Long full) -> Textual::Stream& {
  valid_state &= write_decimal(data, ptr_location, full);
  return *this;
}

auto Textual::Stream::operator<<(const ULong unsigned_full)
    -> Textual::Stream& {
  valid_state &= write_decimal(data, ptr_location, unsigned_full);
  return *this;
}

auto Textual::Stream::operator<<(const Real_32 real_32) -> Textual::Stream& {
  write_real(Real_64(real_32), __FLT_EPSILON__);
  return *this;
}

auto Textual::Stream::operator<<(const Real_64 real_64) -> Textual::Stream& {
  write_real(real_64, __DBL_EPSILON__);
  return *this;
}

// TODO: This code is awful and should be optimized but it does a decent enough
// job without having to pull in bulky headers.
auto Textual::Stream::write_real(Real_64 real, Real_64 precision) -> void {
  if (!valid_state) {
    return;
  }

  constexpr auto max_length = 32;
  SignedBits_64 decimal_portion = SignedBits_64(real);
  auto length = decimal_length(decimal_portion);
  valid_state &= write_decimal(data, ptr_location, decimal_portion, length);

  valid_state &= ptr_location < data.get_size();
  if (!valid_state) {
    return;
  }

  // Always write a decimal point.
  data.get_data()[ptr_location++] = '.';
  length += 1;

  auto fract = real - decimal_portion;

  // Trailing zero and negative values.
  if (fract <= 0) {
    fract *= -1;
    if (fract < precision) {
      valid_state &= ptr_location < data.get_size();
      if (!valid_state) {
        return;
      }

      data.get_data()[ptr_location++] = '0';
      length += 1;
    }
  }

  // Slowly write the rest of fractional part until we hit a limit.
  while (length < max_length && fract > precision) {
    if (ptr_location >= data.get_size()) {
      valid_state &= ptr_location < data.get_size();
      return;
    }

    fract *= 10;
    precision *= 10;
    length += 1;
    Byte value = Byte(fract);
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

    data.get_data()[ptr_location++] = '0' + value;
  }
}

auto Textual::Stream::operator<<(const Core::View::Bytes raw)
    -> Textual::Stream& {
  valid_state &= write_text(data, ptr_location, raw);
  return *this;
}
