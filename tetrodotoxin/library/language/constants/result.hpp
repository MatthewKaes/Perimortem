// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/types/result.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Language::Constants {

// Result is one completed immutable value-or-error selection. Its payload Pack
// retains the exact folded alternative without copying producer identity.
class Result : public Constant {
 public:
  TTX_CONTRACT(Result, Constant);

  static auto create_value(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Model::Pack& payload) -> Perimortem::Core::Option<Result&>;

  static auto create_error(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Model::Pack& payload) -> Perimortem::Core::Option<Result&>;

  static auto create_fitted(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Model::Pack& source) -> Perimortem::Core::Option<Result&>;

  static auto select(Model::Pack& source) -> Perimortem::Core::Option<Result&>;

  constexpr auto get_type() const -> const Types::Result& override {
    return type;
  }

  constexpr auto get_kind() const -> Types::Result::Kind { return kind; }
  constexpr auto get_payload() const -> const Model::Pack& {
    return payload.get();
  }

  auto equals(const Constant& rhs) const -> Bool override;
  auto lower(Llvm::Builder& body) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

 private:
  constexpr Result(
      const Types::Result& type,
      Types::Result::Kind kind,
      Model::Pack& payload,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor)
      : Constant(anchor), type(type), kind(kind), payload(payload) {}

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      const Types::Result& type,
      Types::Result::Kind kind,
      Model::Pack& payload) -> Perimortem::Core::Option<Result&>;

  const Types::Result& type;
  Types::Result::Kind kind;
  Ttx::Concept::Reference<Model::Pack> payload;
};

}  // namespace Tetrodotoxin::Library::Language::Constants
