// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/isa/context.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// Registry stores the semantic instruction sets installed for one Tetrodotoxin
// toolchain. The registry stores stateless ISA instances so it's safe to use
// across multiple toolchains on multiple threads, however VM results are thread
// locked as of now as all their computations use a shared cluster memory space
// that is thread localized.
//
// Preamble ISAs such as Puffer Boot are not part of this table. This registry
// contains the body ISAs that execute after a caller has prepared imports for a
// source record.
class Registry {
 public:
  using EvaluateFunction =
      Ttx::Type* (*)(Ttx::Lexical::Cursor& cursor, Context& context);

  class Entry {
   public:
    Entry() = default;
    Entry(
        Perimortem::Core::View::Bytes name,
        Registry::EvaluateFunction evaluator)
        : name(name), evaluator(evaluator) {}

    constexpr auto get_name() const -> Perimortem::Core::View::Bytes {
      return name;
    }
    constexpr auto get_evaluator() const -> EvaluateFunction {
      return evaluator;
    };
    constexpr auto is_valid() const -> Bool {
      return !name.is_empty() && evaluator != nullptr;
    }

   private:
    Perimortem::Core::View::Bytes name;
    EvaluateFunction evaluator = nullptr;
  };

  Registry() = default;

  auto install(Perimortem::Core::View::Bytes name, EvaluateFunction evaluator)
      -> Bool;

  auto find(Perimortem::Core::View::Bytes name) const -> const Entry*;
  auto require_installed(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Lexical::Token& name) const -> Bool;
  auto require_installed(
      Ttx::Lexical::Cursor& cursor,
      Perimortem::Core::View::Bytes name) const -> Bool;
  auto get_installed() const -> Perimortem::Core::View::Vector<Entry>;

  constexpr auto get_size() const -> Count { return installed_count; }

 private:
  Perimortem::Core::Static::Vector<Entry, 64> installed;
  Count installed_count = 0;
};

}  // namespace Tetrodotoxin::Isa
