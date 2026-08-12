// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/addressable.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Address evaluates one value receiver and selects one Addressable from its
// Type Layout. The selected address may use stack storage or a receiver base,
// while target generation owns its concrete representation.
class Address : public Expression {
 public:
  TTX_CONTRACT(Address, Expression, 0xdf9d470a5b024335, 0xb38fc893710f519e);

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Expression& receiver) -> Perimortem::Core::Option<Expression&>;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      Expression& receiver,
      const Ttx::Model::Addressable& addressable) -> Address&;

  // The receiver Layout is already the complete candidate range for `.`.
  // Semantic linking and completion tooling apply this predicate to each
  // Addressable rather than copying Layout entries into a member registry.
  static auto is_accessible(
      const Ttx::Model::Addressable& candidate,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Ttx::Concept::Abstract& lexical_context,
      Perimortem::Core::Option<const Ttx::Model::Type&> access_scope = {})
      -> Bool override;

  TTX_NAME(name);

  auto get_documentation() const -> const Ttx::Concept::Documentation& override;
  auto get_type() const -> const Ttx::Concept::Abstract& override;
  auto get_result() const -> const Ttx::Concept::Abstract& override;
  auto finalize() -> void override;

  constexpr auto get_receiver() const -> const Expression& { return receiver; }

  constexpr auto get_name_token() const -> Ttx::Lexical::Token {
    return name_token;
  }

 private:
  constexpr Address(
      Expression& receiver,
      Ttx::Lexical::Token name_token,
      Perimortem::Core::View::Bytes name,
      Perimortem::Core::Option<
          Ttx::Concept::Reference<const Ttx::Model::Addressable>> addressable,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Expression(anchor),
        receiver(receiver),
        name_token(name_token),
        name(name),
        addressable(addressable) {}

  Expression& receiver;
  Ttx::Lexical::Token name_token;
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::Option<
      Ttx::Concept::Reference<const Ttx::Model::Addressable>>
      addressable;
};

}  // namespace Tetrodotoxin::Library::Language::Access
