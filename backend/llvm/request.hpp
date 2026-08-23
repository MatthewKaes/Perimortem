// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/perimortem.hpp"

#include "backend/llvm/abi/unit.hpp"
#include "backend/llvm/representation/debug.hpp"
#include "backend/llvm/target.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/lexical/errors.hpp"

namespace Tetrodotoxin::Backend::Llvm {

// Request borrows one completed Library graph and the exact authored source
// facts used by this compilation. Target and Debug Level are request policy
// rather than semantic graph facts.
class Request {
 public:
  constexpr Request(
      const Tetrodotoxin::Library::Language::Monograph& monograph,
      Ttx::Lexical::Errors& errors,
      Perimortem::Core::View::Bytes source_path,
      Perimortem::Core::View::Bytes source_text,
      Target target,
      Representation::Debug::Level debug_level,
      Abi::Unit unit = {})
      : monograph(monograph),
        errors(errors),
        source_path(source_path),
        source_text(source_text),
        target(target),
        debug_level(debug_level),
        unit(unit.bind(monograph)) {}

  constexpr auto get_monograph() const
      -> const Tetrodotoxin::Library::Language::Monograph& {
    return monograph;
  }

  constexpr auto get_errors() const -> Ttx::Lexical::Errors& { return errors; }

  constexpr auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

  constexpr auto get_source_text() const -> Perimortem::Core::View::Bytes {
    return source_text;
  }

  constexpr auto get_target() const -> Target { return target; }
  constexpr auto get_debug_level() const -> Representation::Debug::Level {
    return debug_level;
  }

  constexpr auto get_unit() const -> const Abi::Unit& { return unit; }

 private:
  const Tetrodotoxin::Library::Language::Monograph& monograph;
  Ttx::Lexical::Errors& errors;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Bytes source_text;
  Target target;
  Representation::Debug::Level debug_level;
  Abi::Unit unit;
};

}  // namespace Tetrodotoxin::Backend::Llvm
