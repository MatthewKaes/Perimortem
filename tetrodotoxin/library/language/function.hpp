// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/monograph.hpp"
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
// identity, source owner, and exact host Type before completion installs the
// signature and authored Expression roots from one complete definition. A
// reserved self Addressable at parameter entry zero records receiver
// invocation and has that exact host Type.
class Function : public Ttx::Model::Callable {
 private:
  Function(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Model::Type& host,
      Materializations& materializations);

 public:
  TTX_CONTRACT(
      Function,
      Ttx::Model::Callable,
      0x6c76a9165a2640bf,
      0xbbebc5e45cc6bd0f);

  static auto reserve(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition,
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Model::Type& host,
      Materializations& materializations)
      -> Perimortem::Core::Option<Function&>;

  Function(const Function&) = delete;
  Function(Function&&) = delete;
  auto operator=(const Function&) -> Function& = delete;
  auto operator=(Function&&) -> Function& = delete;

  auto complete(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto link_signature() -> Bool;

  auto link_body() -> Bool;

  auto link() -> Bool;

  auto finalize() -> Bool;

  TTX_NAME(definition.get_name());

  TTX_DOCUMENTATION(definition.get_documentation());

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_parameters() const -> const Ttx::Concept::Layout& override;

  auto get_results() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_source() const
      -> const Tetrodotoxin::Language::Monograph& {
    return source;
  }

  constexpr auto get_host() const -> const Ttx::Model::Type& { return host; }

  constexpr auto get_span() const -> Ttx::Lexical::Span { return span; }

  auto get_signature() const -> Perimortem::Core::Option<const Signature&>;

  auto get_expressions()
      -> Perimortem::Core::View::Vector<Ttx::Concept::Reference<Expression>>;

  auto get_expressions() const -> Perimortem::Core::View::Vector<
      Ttx::Concept::Reference<const Expression>>;

  constexpr auto get_return_token() const -> Ttx::Lexical::Token {
    return return_token;
  }

  constexpr auto get_return_span() const -> Ttx::Lexical::Span {
    return return_span;
  }

  auto get_return_expression() const
      -> Perimortem::Core::Option<const Expression&>;

  constexpr auto is_complete() const -> Bool { return completed; }

  auto is_signature_linked() const -> Bool;

  constexpr auto is_linked() const -> Bool { return linked; }

 private:
  Perimortem::Memory::Allocator::Arena& domain;
  Tetrodotoxin::Language::Definition& definition;
  Tetrodotoxin::Language::Monograph& source;
  const Ttx::Model::Type& host;
  Materializations& materializations;
  Ttx::Lexical::Span span;
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
