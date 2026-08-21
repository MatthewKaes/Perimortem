// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/access.hpp"

#include "tetrodotoxin/library/builtin/view/is_empty.hpp"
#include "tetrodotoxin/library/builtin/view/size.hpp"
#include "tetrodotoxin/library/builtin/view/slice.hpp"
#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

Types::Access::Access(
    Perimortem::Memory::Allocator::Arena& domain,
    View::Bytes name,
    const Model::Type& element,
    const Model::Type& size_type,
    const Model::Type& flag_type,
    const Model::Type& view_type)
    : name(name), element(element) {
  auto& get_size = Builtin::View::Size::create(domain, *this, size_type);
  auto& is_empty = Builtin::View::IsEmpty::create(domain, *this, flag_type);
  auto& slice =
      Builtin::View::Slice::create(domain, *this, size_type, view_type);
  publish_callable(domain, get_size, True);
  publish_callable(domain, is_empty, True);
  publish_callable(domain, slice, True);
}

auto Types::Access::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Bytes::create_synthetic(arena, *this, {});
}

auto Types::Access::reserve(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved =
      carriers.reserve(program, *this, Llvm::Carriers::Kind::Access);
  if (!reserved) {
    return False;
  }

  if (!*reserved) {
    return True;
  }

  Bool element_reserved = element.reserve(program);
  if (!element_reserved) {
    return False;
  }

  return reserve_callables(program);
}

auto Types::Access::complete(Llvm::Program& program) const -> Bool {
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

  if (!complete_callables(program)) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete(program, *this, Llvm::Carriers::Kind::Access);
  return carrier_completed && complete_debug(program);
}
