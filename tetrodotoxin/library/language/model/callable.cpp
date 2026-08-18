// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/callable.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;

static auto reserve_layout(Llvm::Program& program, const Layout& layout)
    -> Bool {
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    if (!entry) {
      return False;
    }

    auto addressable = entry->select<Language::Model::Addressable>();
    auto type = entry->resolve().select<Language::Model::Type>();
    if (addressable) {
      type = addressable->get_type();
    }

    if (!type || !type->reserve(program)) {
      return False;
    }
  }

  return True;
}

static auto complete_layout(Llvm::Program& program, const Layout& layout)
    -> Bool {
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    if (!entry) {
      return False;
    }

    auto addressable = entry->select<Language::Model::Addressable>();
    auto type = entry->resolve().select<Language::Model::Type>();
    if (addressable) {
      type = addressable->get_type();
    }

    if (!type || !type->complete(program)) {
      return False;
    }
  }

  return True;
}

auto Language::Model::Callable::reserve_declaration(
    Llvm::Program& program) const -> Bool {
  Bool parameters = reserve_layout(program, get_parameters());
  if (!parameters) {
    return False;
  }

  return reserve_layout(program, get_results());
}

auto Language::Model::Callable::complete_declaration(
    Llvm::Program& program) const -> Bool {
  Bool parameters = complete_layout(program, get_parameters());
  if (!parameters) {
    return False;
  }

  return complete_layout(program, get_results());
}

auto Language::Model::Callable::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Perimortem::Core::View::Vector<LLVMValueRef> inputs,
    Perimortem::Core::Option<const Ttx::Model::Pack&>) const -> Bool {
  return body.invoke(result, *this, inputs);
}
