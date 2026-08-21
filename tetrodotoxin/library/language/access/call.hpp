// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Call owns one complete postfix invocation and its argument Pack. The receiver
// Expression determines Static or Self selection from the exact
// semantic result produced during linking. Composite registration guarantees
// one Callable per name and receiver role, so Call validates one selected
// signature rather than searching an overload set.
class Call : public Expression {
 public:
  TTX_CONTRACT(Call, Expression);

  static auto parse(
      const Ttx::Concept::Abstract& context,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      Expression& receiver,
      Perimortem::Core::View::Bytes name,
      Language::Model::Pack& arguments) -> Call&;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  auto link_restored(
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_result() const -> const Ttx::Concept::Abstract& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_value_type(Count index) const
      -> const Ttx::Concept::Abstract& override;
  auto get_produced(Count index) const
      -> Perimortem::Core::Option<Ttx::Model::Pack::Produced> override;
  auto get_layout() const -> const Ttx::Concept::Layout& override;
  auto resolve() const -> const Ttx::Concept::Abstract& override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> void override;
  auto lower(Llvm::Builder& body) const -> Bool override;

  auto get_callable() const -> Perimortem::Core::Option<const Model::Callable&>;

  constexpr auto get_arguments() const -> const Language::Model::Pack& {
    return arguments;
  }

  constexpr auto get_receiver() const -> const Expression& { return receiver; }

  constexpr auto get_name_token() const -> Ttx::Lexical::Token {
    return name_token;
  }

 private:
  // Input retains the one parameter mapping proven during linking. Lowering
  // consumes this evidence without repeating named or positional fitting.
  class Input {
   public:
    constexpr Input(
        const Ttx::Model::Addressable& parameter,
        const Ttx::Model::Pack& source,
        Count offset,
        Count size)
        : parameter(parameter), source(source), offset(offset), size(size) {}

    constexpr auto get_parameter() const -> const Ttx::Model::Addressable& {
      return parameter.get();
    }

    constexpr auto get_source() const -> const Ttx::Model::Pack& {
      return source.get();
    }

    constexpr auto get_offset() const -> Count { return offset; }

    constexpr auto get_size() const -> Count { return size; }

   private:
    Ttx::Concept::Reference<const Ttx::Model::Addressable> parameter;
    Ttx::Concept::Reference<const Ttx::Model::Pack> source;
    Count offset;
    Count size;
  };

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
        arguments(arguments),
        fitted_inputs(domain) {}

  auto fit_inputs(
      const Model::Callable& selected,
      Perimortem::Core::Option<const Ttx::Concept::Layout&> inputs) -> Bool;

  auto evaluate() -> Perimortem::Utility::
      Result<Perimortem::Core::Option<Model::Pack&>, Error> override;

  Perimortem::Memory::Allocator::Arena& domain;
  Expression& receiver;
  Ttx::Lexical::Token name_token;
  Perimortem::Core::View::Bytes name;
  Language::Model::Pack& arguments;
  Perimortem::Core::Option<Ttx::Concept::Reference<const Model::Callable>>
      callable;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> input_layout;
  Perimortem::Core::Option<const Ttx::Concept::Layout&> output;
  Perimortem::Memory::Managed::Vector<Input> fitted_inputs;
};

}  // namespace Tetrodotoxin::Library::Language::Access
