// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/authored.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/signature.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/span.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Function is one Library defined Callable. Reservation fixes its graph
// identity while Definition supplies its exact host Type. Completion installs
// the signature and authored Expression roots. A reserved self Addressable at
// parameter entry zero records receiver invocation and has that exact host
// Type.
class Function : public Authored<Ttx::Model::Callable> {
  using Base = Authored<Ttx::Model::Callable>;

 private:
  Function(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition);

 public:
  TTX_CONTRACT(Function, Base, 0x6c76a9165a2640bf, 0xbbebc5e45cc6bd0f);

  static auto reserve(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Function&>;

  Function(const Function&) = delete;
  Function(Function&&) = delete;
  auto operator=(const Function&) -> Function& = delete;
  auto operator=(Function&&) -> Function& = delete;

  auto complete(
      Ttx::Lexical::Cursor& cursor,
      Materializations& materializations) -> Bool;

  auto link_signature(Tetrodotoxin::Language::Monograph& source) -> Bool;

  auto link_body(
      Tetrodotoxin::Language::Monograph& source,
      Materializations& materializations) -> Bool;

  auto finalize(Tetrodotoxin::Language::Monograph& source) -> Bool;

  TTX_DOCUMENTATION(get_definition().get_documentation());

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_parameters() const -> const Ttx::Concept::Layout& override;

  auto get_results() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_host() const -> const Ttx::Model::Type& {
    return static_cast<const Ttx::Model::Type&>(get_definition().get_host());
  }

  auto get_signature() const -> Perimortem::Core::Option<const Signature&>;

  auto get_expressions() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Expression>>;

  auto get_return_expression() const
      -> Perimortem::Core::Option<const Expression&>;

  constexpr auto is_complete() const -> Bool { return completed; }

 private:
  auto is_signature_linked() const -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Core::Option<Signature&> signature;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Expression>>
      expressions;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<const Expression>>
      expression_observations;
  Ttx::Lexical::Token return_token;
  Ttx::Lexical::Span return_span;
  Perimortem::Core::Option<Ttx::Concept::Reference<Expression>>
      return_expression;
  Bool completed = False;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Library::Language
