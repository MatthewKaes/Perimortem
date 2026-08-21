// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/object_storage.hpp"

#include "tetrodotoxin/library/builtin/object/access.hpp"
#include "tetrodotoxin/library/builtin/object/capacity.hpp"
#include "tetrodotoxin/library/builtin/object/clone.hpp"
#include "tetrodotoxin/library/builtin/object/is_shared.hpp"
#include "tetrodotoxin/library/builtin/object/reserve.hpp"
#include "tetrodotoxin/library/builtin/object/view.hpp"
#include "tetrodotoxin/library/language/constants/object.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

Types::ObjectStorage::ObjectStorage(
    Perimortem::Memory::Allocator::Arena& domain,
    Perimortem::Core::View::Bytes name,
    const Model::Type& element,
    const Model::Type& size_type,
    const Model::Type& flag_type,
    const Model::Type& view_type,
    const Model::Type& access_type)
    : name(name), element(element) {
  auto& capacity = Builtin::Object::Capacity::create(domain, *this, size_type);
  auto& clone = Builtin::Object::Clone::create(domain, *this);
  auto& is_shared = Builtin::Object::IsShared::create(domain, *this, flag_type);
  auto& view = Builtin::Object::View::create(domain, *this, view_type);
  auto& access = Builtin::Object::Access::create(domain, *this, access_type);
  auto& reserve =
      Builtin::Object::Reserve::create(domain, *this, size_type, access_type);
  publish_callable(domain, capacity, True);
  publish_callable(domain, clone, True);
  publish_callable(domain, is_shared, True);
  publish_callable(domain, view, True);
  publish_callable(domain, access, True);
  publish_callable(domain, reserve, True);
}

auto Types::ObjectStorage::create_default(
    Perimortem::Memory::Allocator::Arena& arena) const -> Option<Model::Pack&> {
  return Constants::Object::create(arena, *this);
}

auto Types::ObjectStorage::reserve(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved =
      carriers.reserve(program, *this, Llvm::Carriers::Kind::ObjectStorage);
  if (!reserved) {
    return False;
  }

  if (!*reserved) {
    return True;
  }

  return element.reserve(program) && reserve_callables(program);
}

auto Types::ObjectStorage::complete(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool completed = element.complete(program) && complete_callables(program);
  if (!completed) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete(program, *this, Llvm::Carriers::Kind::ObjectStorage);
  return carrier_completed && complete_debug(program);
}
