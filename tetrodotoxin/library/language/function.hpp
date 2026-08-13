// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/authored.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/signature.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Library::Language {

// Function is one Library defined Callable. Reservation fixes its graph
// identity while Definition supplies its exact host Type. Completion installs
// the signature and one authored Block. A reserved self Addressable at
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

  auto complete(Monograph& source, Ttx::Lexical::Cursor& cursor) -> Bool;

  auto link_signature(Tetrodotoxin::Language::Monograph& source) -> Bool;

  auto link_body(Tetrodotoxin::Language::Monograph& source) -> Bool;

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

  // Before Signature linking publishes the receiver Addressable, registration
  // still needs the authored receiver role. This query derives it from the
  // retained Signature shape; Callable::is_type_bound() becomes authoritative
  // once the parameter Layout exists.
  auto declares_self() const -> Bool;

  auto get_body() const -> Perimortem::Core::Option<const Flow::Block&>;

  constexpr auto is_complete() const -> Bool { return completed; }

 private:
  auto is_signature_linked() const -> Bool;

  Perimortem::Memory::Allocator::Arena& domain;
  Perimortem::Core::Option<Signature&> signature;
  Perimortem::Core::Option<Flow::Block&> body;
  Bool completed = False;
};

}  // namespace Tetrodotoxin::Library::Language
