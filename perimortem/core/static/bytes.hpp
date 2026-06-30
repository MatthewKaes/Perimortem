// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.hpp"
#include "perimortem/core/math.hpp"

namespace Perimortem::Core::Static {

// Used to convert string literals into non-null terminated bytes.
template <Count literal_size>
class Bytes {
 public:
  constexpr Bytes() {}

  // This awful syntax allows cpp to enable direct value initialization:
  // Static::Bytes<3>{0x01, 0x02, 0x03}.
  template <typename... raw_bytes>
    requires(sizeof...(raw_bytes) == literal_size)
  constexpr Bytes(raw_bytes... values) : source_block{Bits_8(values)...} {}

  // Used for passing bytes directly that are already packed.
  constexpr Bytes(const Bits_8 (&source)[literal_size]) {
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        source_block[i] = source[i];
      }
    } else {
      Data::copy(source_block, source, literal_size);
    }
  }

  // Initializes a buffer from a source view, either taking a slice of the data
  // or zero extending the ouput if the buffer is larger than the input.
  constexpr Bytes(const View::Bytes& source) {
    const Count size = Math::min(literal_size, source.get_size());
    if consteval {
      Count i = 0;
      for (; i < size; i++) {
        source_block[i] = source.get_data()[i];
      }

      // Zero out the rest of the array so we don't expose junk data.
      for (; i < literal_size; i++) {
        source_block[i] = 0;
      }
    } else {
      Data::copy(source_block, source.get_data(), size);
      Data::set(source_block + size, 0, literal_size - size);
    }
  }

  // Allows for generating data that would be a pain to manually write out.
  constexpr Bytes(Bits_8 (*generator)(Count)) {
    for (Count i = 0; i < literal_size; i++) {
      source_block[i] = generator(i);
    }
  }

  // Fast read function that assumes the read range is valid.
  static constexpr auto read_range(const Bits_8* source) {
    Bytes data;
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        data.source_block[i] = source[i];
      }
    } else {
      if constexpr (literal_size < 16) {
        for (Count i = 0; i < literal_size; i++) {
          data.source_block[i] = source[i];
        }
      } else {
        Data::copy(data.source_block, source, literal_size);
      }
    }

    return data;
  }

  constexpr operator View::Bytes() const { return get_view(); }
  constexpr operator Access::Bytes() { return get_access(); }

  constexpr auto operator==(const View::Bytes& rhs) -> Bool {
    if (rhs.get_size() != literal_size) {
      return False;
    }

    return Data::compare(source_block, rhs.get_data(), literal_size);
  }

  constexpr auto operator!=(const View::Bytes& rhs) -> Bool {
    return !(*this == rhs);
  }

  constexpr auto operator[](Count index) -> Bits_8& {
    return source_block[index];
  }

  constexpr auto operator[](Count index) const -> const Bits_8& {
    return source_block[index];
  }

  constexpr auto slice(Count start, Count size) const -> View::Bytes {
    if (start >= get_size()) {
      return View::Bytes();
    }

    return View::Bytes(
        source_block + start, Math::min(size, get_size() - start));
  }

  constexpr auto get_size() const -> Count { return literal_size; }
  constexpr auto get_capacity() const -> Count { return literal_size; }
  constexpr auto get_view() const -> const View::Bytes {
    return View::Bytes(source_block, literal_size);
  }
  constexpr auto get_data() const -> const Bits_8* { return source_block; }
  constexpr auto get_data() -> Bits_8* { return source_block; }
  constexpr auto get_access() -> Access::Bytes {
    return Access::Bytes(source_block, literal_size);
  }

  constexpr auto hash() const -> Bits_64 {
    return Core::Hash(get_view()).get_value();
  }

 private:
  Bits_8 source_block[literal_size]{};
};

}  // namespace Perimortem::Core::Static
