// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/singleton.hpp"
#include "perimortem/memory/view/bytes.hpp"

#include <source_location>

namespace Perimortem::System {
class Diagnostics : public Memory::Managed::Singleton<Diagnostics> {
 public:
  enum class Severity {
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
  };

  struct Data {
   public:
    enum class Type {
      View,
      Int,
      Double,
      Bool,
    };

    Data(const Memory::View::Bytes msg) : type(Type::View), bytes(msg) {}
    Data(Int msg) : type(Type::Int), number(msg) {}
    Data(Real_64 msg) : type(Type::View), real(msg) {}
    Data(Bool msg) : type(Type::View), boolean(msg) {}

    Type type;
    union {
      Memory::View::Bytes bytes;
      Int number;
      Real_64 real;
      Bool boolean;
    };
  };

  Diagnostics();
  ~Diagnostics();
  auto set_level(Severity minimum = Severity::Info) -> void;
  auto enable_stdout(Bool enable) -> void;
  auto enable_log(Bool enable) -> void;
  auto info(const Memory::View::Bytes msg,
            std::initializer_list<Data> data = {},
            const std::source_location location =
                std::source_location::current()) -> void;
  auto debug(const Memory::View::Bytes msg,
             std::initializer_list<Data> data = {},
             const std::source_location location =
                 std::source_location::current()) -> void;
  auto warning(const Memory::View::Bytes msg,
               std::initializer_list<Data> data = {},
               const std::source_location location =
                   std::source_location::current()) -> void;
  auto error(const Memory::View::Bytes msg,
             std::initializer_list<Data> data = {},
             const std::source_location location =
                 std::source_location::current()) -> void;
  auto fatal(const Memory::View::Bytes msg,
             std::initializer_list<Data> data = {},
             const std::source_location location =
                 std::source_location::current()) -> void;

  auto flush() -> void;

 private:
  auto write_level(Severity level) -> void;
  auto write_utc_stamp() -> void;
  auto write_message(std::initializer_list<Data> data) -> void;

  Severity log_level = Severity::Info;
  Bool flush_to_stdout = false;
  Bool flush_to_log = false;
  Byte log_file[1 << 9];
  Byte log_buffer[4 << 12];
  Count write_ptr = 0;
};

}  // namespace Perimortem::System
