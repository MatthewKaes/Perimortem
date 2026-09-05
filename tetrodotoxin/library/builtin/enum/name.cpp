// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/enum/name.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/enumeration.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Enum::Name::create(
    Memory::Allocator::Arena& domain,
    const Language::Types::Enumeration& enumeration,
    const Language::Model::Type& result) -> Name& {
  Ttx::Model::Layouts::Addressable& self =
      Ttx::Model::Layouts::Addressable::create_synthetic(
          domain, "self"_view, enumeration);
  return domain.construct_from<Name>(
      [&]() -> Name { return Name(domain, self, enumeration, result); });
}

auto Builtin::Enum::Name::invoke(
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || !arguments.get_layout().is_empty());

  auto constant = receiver->select_identity<Language::Constants::Enumeration>();
  BAIL_IF(!constant || &constant->get_type() != &enumeration);
  Core::View::Bytes name = enumeration.find_case_name(constant->get_value());
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, name);
}

auto Builtin::Enum::Name::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(
             requirement, Language::Model::Invocation::requirement())
             ? TTX_INTERFACE_SATISFIED
             : Language::Model::Callable::negotiate(requirement);
}

void Builtin::Enum::Name::invoke(
    ttx_abstract operation,
    ttx_pack input,
    ttx_context context,
    ttx_pack_result result) const {
  if (!ttx_abstract_same(operation, Language::Model::Invocation::operation())) {
    result.operations->none(result);
    return;
  }
  auto inputs = Language::Model::Invocation::local_inputs(input);
  if (!inputs || inputs->size() != 1) {
    result.operations->none(result);
    return;
  }
  auto& arguments = Language::Model::Pack::create_completed(domain, {});
  Language::Model::Invocation::return_pack(
      invoke(*(*inputs)[0], arguments), context, result);
}
