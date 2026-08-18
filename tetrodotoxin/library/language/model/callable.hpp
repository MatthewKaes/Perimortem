// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/model/callable.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Callable derives Library's Static or Self receiver role from the one real
// parameter Layout instead of retaining a second marker on each Function.
class Callable : public Ttx::Model::Callable {
 public:
  TTX_CONTRACT(Callable, Ttx::Model::Callable);

  // A selected Self Callable may impose receiver authority beyond exact Type
  // binding. Ordinary invocations accept the resolved receiver unchanged.
  // Borrowing built-ins use this boundary to require writable storage without
  // teaching Call about a concrete declaration or built-in kind.
  virtual auto accepts_receiver(
      const Ttx::Concept::Abstract&,
      const Ttx::Concept::Abstract&) const -> Bool {
    return True;
  }

  // The owning declaration context asks each retained Callable to cross its
  // closure barriers. Signatures settle before Fields may invoke them, while
  // bodies wait until every initializer has linked. Bodyless and generated
  // Callables keep the neutral behavior.
  virtual auto link_declaration_signature(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto link_declaration_body(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto finalize_declaration(Ttx::Lexical::Cursor&) -> Bool {
    return True;
  }

  virtual auto reserve_declaration(Llvm::Program& program) const -> Bool;

  virtual auto complete_declaration(Llvm::Program& program) const -> Bool;

  virtual auto lower_declaration(Llvm::Program&) const -> Bool { return True; }

  virtual auto lower_call(
      Llvm::Builder& body,
      const Ttx::Model::Pack& result,
      Perimortem::Core::View::Vector<LLVMValueRef> inputs,
      Perimortem::Core::Option<const Ttx::Model::Pack&> receiver_source) const
      -> Bool;

  virtual constexpr auto get_declaration_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> {
    return {};
  }

  // Registration needs receiver role before an authored Signature has linked
  // its Addressable entries. Completed and generated Callables derive the same
  // answer from their parameter Layout, while an authored owner may answer from
  // its retained Signature shape.
  virtual auto declares_self() const -> Bool { return is_type_bound(); }

  constexpr auto get_type_binding() const
      -> Perimortem::Core::Option<const Type&> {
    auto first = get_parameters().get_abstract(0);
    if (!first) {
      return {};
    }

    auto parameter = first->select<Addressable>();
    if (!parameter || parameter->get_name() != "self"_view) {
      return {};
    }
    return parameter->get_type();
  }

  constexpr auto is_type_bound() const -> Bool {
    return Bool(get_type_binding());
  }

  constexpr auto is_type_bound(const Type& receiver) const -> Bool {
    auto binding = get_type_binding();
    return binding && &*binding == &receiver;
  }
};

}  // namespace Tetrodotoxin::Library::Language::Model
