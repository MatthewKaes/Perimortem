// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/fixed/view.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Fixed::View::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> View& {
  Ttx::Model::Layouts::Addressable& self =
      Ttx::Model::Layouts::Addressable::create_synthetic(
          domain, "self"_view, receiver);
  return domain.construct_from<View>(
      [&]() -> View { return View(domain, self, result); });
}

auto Builtin::Fixed::View::invoke(
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || !arguments.get_layout().is_empty());

  auto bytes = receiver->select_identity<Language::Constants::Bytes>();
  BAIL_IF(!bytes);
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, bytes->get_value(), bytes->get_resource());
}

auto Builtin::Fixed::View::negotiate(ttx_abstract requirement) const
    -> ttx_interface_relation {
  return ttx_abstract_same(
             requirement, Language::Model::Invocation::requirement())
             ? TTX_INTERFACE_SATISFIED
             : Language::Model::Callable::negotiate(requirement);
}

void Builtin::Fixed::View::invoke(
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
