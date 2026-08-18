// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/types/option.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Option is one completed immutable optional value. A present value retains
// its complete folded payload Pack without copying any producer identity.
class Option : public Constant {
 public:
  auto lower(Llvm::Builder& body) const -> Bool override;

  TTX_CONTRACT(Option, Constant);

  static auto create_absent(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Option& type) -> Option&;

  static auto create_present(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Option& type,
      Model::Pack& payload) -> Perimortem::Core::Option<Option&>;

  static auto create_fitted(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Option& type,
      Model::Pack& source) -> Perimortem::Core::Option<Option&>;

  constexpr auto get_type() const -> const Types::Option& override {
    return type;
  }

  constexpr auto get_kind() const -> Types::Option::Kind { return kind; }

  auto get_payload() const -> Perimortem::Core::Option<const Model::Pack&>;

  auto equals(const Constant& rhs) const -> Bool override;

 private:
  constexpr Option(
      const Types::Option& type,
      Types::Option::Kind kind,
      Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>> payload,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type), kind(kind), payload(payload) {}

  const Types::Option& type;
  Types::Option::Kind kind;
  Perimortem::Core::Option<Ttx::Concept::Reference<Model::Pack>> payload;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
