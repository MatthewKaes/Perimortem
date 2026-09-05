// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/view/is_empty.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/flag.hpp"
#include "tetrodotoxin/library/language/model/types/flag.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::View::IsEmpty::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> IsEmpty& {
  Ttx::Model::Layouts::Addressable& self =
      Ttx::Model::Layouts::Addressable::create_synthetic(
          domain, "self"_view, receiver);
  return domain.construct_from<IsEmpty>(
      [&]() -> IsEmpty { return IsEmpty(domain, self, result); });
}

auto Builtin::View::IsEmpty::invoke(
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  auto value = receiver
                   ? receiver->select_identity<Language::Constants::Bytes>()
                   : Core::Option<const Language::Constants::Bytes&>();
  auto flag = result_type.resolve().select<Language::Model::Types::Flag>();
  BAIL_IF(!value || !flag || !arguments.get_layout().is_empty());
  return Language::Constants::Flag::create_synthetic(
      domain, *flag, value->get_value().is_empty());
}

auto Builtin::View::IsEmpty::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(
             requirement, Language::Model::Invocation::requirement())
             ? TTX_INTERFACE_SATISFIED
             : Language::Model::Callable::negotiate(requirement);
}

void Builtin::View::IsEmpty::invoke(
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
