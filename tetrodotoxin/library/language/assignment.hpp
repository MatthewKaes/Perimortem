// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/code.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Assignment is one authored write statement. It retains the target Expression
// and source Pack directly because target selection and supplied value flow are
// different semantic facts. The operator records plain or compound write policy
// without constructing another arithmetic Expression.
class Assignment : public Ttx::Concept::Abstract {
 public:
  TTX_CONTRACT(
      Assignment,
      Ttx::Concept::Abstract,
      0xd35ac749f95f45a1,
      0x92d643f86ba1af35);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor) -> Perimortem::Core::Option<Assignment&>;

  Assignment(const Assignment&) = delete;
  Assignment(Assignment&&) = delete;
  auto operator=(const Assignment&) -> Assignment& = delete;
  auto operator=(Assignment&&) -> Assignment& = delete;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      const Ttx::Model::Type& access_scope) -> Bool;

  auto finalize() -> void;

  TTX_NAME("Assignment"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor { return anchor; }

  constexpr auto get_target() const -> const Expression& { return target; }

  constexpr auto get_source() const -> const Model::Pack& { return source; }

  constexpr auto get_operator() const -> Ttx::Lexical::Code::Type {
    return operation;
  }

 private:
  constexpr Assignment(
      Expression& target,
      Model::Pack& source,
      Ttx::Lexical::Code::Type operation,
      Ttx::Lexical::Anchor anchor)
      : target(target), source(source), operation(operation), anchor(anchor) {}

  Expression& target;
  Model::Pack& source;
  Ttx::Lexical::Code::Type operation;
  Ttx::Lexical::Anchor anchor;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language
