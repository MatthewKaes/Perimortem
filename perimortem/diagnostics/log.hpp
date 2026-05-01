// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/diagnostics/source.hpp"
#include "perimortem/core/view/structured.hpp"

namespace Perimortem::Diagnostics {
class Log {
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

    Data(const Core::View::Amorphous msg) : type(Type::View), bytes(msg) {}
    Data(Int msg) : type(Type::Int), number(msg) {}
    Data(Real_64 msg) : type(Type::View), real(msg) {}
    Data(Bool msg) : type(Type::View), boolean(msg) {}

    Type type;
    union {
      Core::View::Amorphous bytes;
      Int number;
      Real_64 real;
      Bool boolean;
    };
  };

  Log();
  ~Log();
  static auto set_level(Severity minimum = Severity::Info) -> void;
  static auto enable_stdout(Bool enable) -> void;
  static auto enable_log(Bool enable) -> void;
  static auto info(Core::View::Amorphous msg,
                   Core::View::Structured<Data> data = {},
                   const SourceInfo location = SourceInfo::current()) -> void;
  static auto debug(const Core::View::Amorphous msg,
                    Core::View::Structured<Data> data = {},
                    const SourceInfo location = SourceInfo::current()) -> void;
  static auto warning(const Core::View::Amorphous msg,
                      Core::View::Structured<Data> data = {},
                      const SourceInfo location = SourceInfo::current())
      -> void;
  static auto error(const Core::View::Amorphous msg,
                    Core::View::Structured<Data> data = {},
                    const SourceInfo location = SourceInfo::current()) -> void;
  static auto fatal(const Core::View::Amorphous msg,
                    Core::View::Structured<Data> data = {},
                    const SourceInfo location = SourceInfo::current()) -> void;

  static auto flush() -> void;

 private:
  static auto write_level(Severity level) -> void;
  static auto write_utc_stamp() -> void;
  static auto write_message(Core::View::Structured<Data> data) -> void;
};

}  // namespace Perimortem::Core::Log
