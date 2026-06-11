// // Perimortem Engine
// // Copyright © Matt Kaes

// #pragma once

// #include "perimortem/core/view/bytes.hpp"
// #include "perimortem/core/access/bytes.hpp"

// #include "perimortem/memory/dynamic/bytes.hpp"

// namespace Perimortem::Serialization {

// class Stream {
//  public:
//   auto write(Core::View::Bytes text) -> void;
//   auto write(Bits_32 rune) -> void;
//   auto write_repeat(Bits_32 rune, Count n) -> void;
//   auto write_repeat(Bits_8 b, Count n) -> void;

//   constexpr auto get_size() const -> Count { return storage.get_size(); }

//   constexpr auto get_view() const -> const Core::View::Bytes {
//     return storage.get_view();
//   }

//   constexpr auto get_access() -> Core::Access::Bytes {
//     return storage.get_access();
//   }

//   constexpr auto get_data() -> Bits_8* {
//     return storage.get_access().get_data();
//   }

//   constexpr auto get_data() const -> const Bits_8* {
//     return storage.get_view().get_data();
//   }

//   constexpr operator Core::View::Bytes() const { return get_view(); }
//   constexpr operator Core::Access::Bytes() { return get_access(); }

//   class Cursor {
//    public:
//     explicit Cursor(Core::View::Bytes data);
//     auto read_rune() -> Bits_32;
//     auto is_done() const -> Bool;

//    private:
//     Core::View::Bytes data;
//     Count position;
//   };

//   auto get_cursor() const -> Cursor;

//  private:
//   Memory::Dynamic::Bytes storage;
// };

// }  // namespace Perimortem::Serialization
