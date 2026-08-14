// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Call owns one complete postfix `-> name(arguments)` invocation. Its one
// receiver Expression determines Static or Self selection from the exact
// semantic result produced during linking. Composite registration guarantees
// one Callable per name and receiver role, so Call validates one selected
// signature rather than searching an overload set.
class Call : public Expression {
 public:
  TTX_CONTRACT(Call, Expression, 0x1bc2984aba944842, 0x9ed57eef00ed826f);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto finalize() -> void override;

  auto get_callable() const
      -> Perimortem::Core::Option<const Ttx::Model::Callable&>;

  constexpr auto get_arguments() const -> const Language::Model::Pack& {
    return arguments;
  }

  constexpr auto get_receiver() const -> const Expression& { return receiver; }

  constexpr auto get_name_token() const -> Ttx::Lexical::Token {
    return name_token;
  }

 private:
  constexpr Call(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      Ttx::Lexical::Token name_token,
      Perimortem::Core::View::Bytes name,
      Language::Model::Pack& arguments,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        domain(domain),
        receiver(receiver),
        name_token(name_token),
        name(name),
        arguments(arguments) {}

  Perimortem::Memory::Allocator::Arena& domain;
  Expression& receiver;
  Ttx::Lexical::Token name_token;
  Perimortem::Core::View::Bytes name;
  Language::Model::Pack& arguments;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Ttx::Model::Callable>>
      callable;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> inputs;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> output;
};

}  // namespace Tetrodotoxin::Library::Language::Access
