// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "tetrodotoxin/library/language/model/addressable.hpp"
#include "ttx/model/callable.hpp"

namespace Tetrodotoxin::Library::Language::Model {

// Callable derives Library's Static or Self receiver role from the one real
// parameter Layout instead of retaining a second marker on each Function.
class Callable : public Ttx::Model::Callable {
 public:
  TTX_CONTRACT(Callable, Ttx::Model::Callable);

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
