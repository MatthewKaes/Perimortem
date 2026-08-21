// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/contiguous.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Language::Types::Contiguous::accepts_iteration(
    const Layout& bindings) const -> Bool {
  auto binding = bindings.get_abstract(0);
  auto addressable = binding ? binding->select<Ttx::Model::Addressable>()
                             : Option<const Ttx::Model::Addressable&>();
  return bindings.get_size() == 1 && addressable &&
         &addressable->get_type().resolve() == &get_element_type().resolve();
}

auto Language::Types::Contiguous::begin_iteration(
    Llvm::Builder& body,
    const Abstract& owner,
    const Layout& bindings,
    const Ttx::Model::Pack& input) const -> Bool {
  auto binding = bindings.get_abstract(0);
  auto addressable = binding ? binding->select<Ttx::Model::Addressable>()
                             : Option<const Ttx::Model::Addressable&>();
  if (!accepts_iteration(bindings) || !addressable) {
    return False;
  }

  return body.begin_sequence(owner, *addressable, input);
}
