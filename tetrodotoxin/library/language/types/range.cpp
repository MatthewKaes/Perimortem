// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/language/types/range.hpp"

#include "tetrodotoxin/library/language/constants/range.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

auto Types::Range::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Range::create_synthetic(arena, *this);
}

auto Types::Range::reserve(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved = carriers.reserve(program, *this, Llvm::Carriers::Kind::Range);
  if (!reserved) {
    return False;
  }

  return !*reserved || element.reserve(program);
}

auto Types::Range::complete(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool completed = element.complete(program);
  if (!completed) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete(program, *this, Llvm::Carriers::Kind::Range);
  return carrier_completed && complete_debug(program);
}

auto Types::Range::accepts_iteration(const Ttx::Concept::Layout& bindings) const
    -> Bool {
  auto binding = bindings.get_abstract(0);
  auto addressable = binding ? binding->select<Ttx::Model::Addressable>()
                             : Option<const Ttx::Model::Addressable&>();
  return bindings.get_size() == 1 && addressable &&
         &addressable->get_type().resolve() == &element.resolve();
}

auto Types::Range::begin_iteration(
    Llvm::Builder& body,
    const Ttx::Concept::Abstract& owner,
    const Ttx::Concept::Layout& bindings,
    const Ttx::Model::Pack& input) const -> Bool {
  auto binding = bindings.get_abstract(0);
  auto addressable = binding ? binding->select<Ttx::Model::Addressable>()
                             : Option<const Ttx::Model::Addressable&>();
  if (!accepts_iteration(bindings) || !addressable) {
    return False;
  }

  return body.begin_sequence(owner, *addressable, input);
}
