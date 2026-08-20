// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/flow/block.hpp"
#include "tetrodotoxin/library/language/model/callable.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/library/language/signature.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language {

// Function is one completely parsed Library Callable. Definition supplies its
// exact host context, Signature supplies its callable shape, and one authored
// Block supplies its body. A leading self parameter records receiver
// invocation and has that exact host Type.
class Function : public Model::Callable {
 private:
  Function(
      Tetrodotoxin::Language::Definition& definition,
      Signature& signature);

 public:
  TTX_CONTRACT(Function, Model::Callable);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Function&>;

  static auto restore(
      Archive::Reader& reader,
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Concept::Abstract& host) -> Perimortem::Core::Option<Function&>;

  Function(const Function&) = delete;
  Function(Function&&) = delete;
  auto operator=(const Function&) -> Function& = delete;
  auto operator=(Function&&) -> Function& = delete;

  auto link_declaration_signature(Ttx::Lexical::Cursor& cursor)
      -> Bool override;

  auto link_declaration_body(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored_declaration_signature() -> Bool override;

  auto finalize_declaration(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto reserve_declaration(Llvm::Program& program) const -> Bool override;

  auto complete_declaration(Llvm::Program& program) const -> Bool override;

  auto lower_declaration(Llvm::Program& program) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

  TTX_DOCUMENTATION(get_definition().get_documentation());
  TTX_NAME(definition.get_name());

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_anchor() const -> Ttx::Lexical::Anchor {
    return definition.get_anchor();
  }

  constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return get_anchor();
  }

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto get_parameters() const -> const Ttx::Concept::Layout& override;

  auto get_results() const -> const Ttx::Concept::Layout& override;

  constexpr auto get_host() const -> const Model::Type& {
    return static_cast<const Model::Type&>(get_definition().get_host());
  }

  // Before Signature linking publishes the receiver Addressable, registration
  // still needs the authored receiver role. This query derives it from the
  // retained Signature shape. Callable::is_type_bound() becomes authoritative
  // once the parameter Layout exists.
  auto declares_self() const -> Bool override;

  auto get_body() const -> Perimortem::Core::Option<const Flow::Block&>;

 private:
  auto is_signature_linked() const -> Bool;

  Tetrodotoxin::Language::Definition& definition;
  Signature& signature;
  Perimortem::Core::Option<Flow::Block&> body;
};

}  // namespace Tetrodotoxin::Library::Language
