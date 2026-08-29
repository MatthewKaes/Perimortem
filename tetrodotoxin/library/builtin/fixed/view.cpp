// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/fixed/view.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "ttx/bootstrap/concept/constant.hpp"

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

auto Builtin::Fixed::View::fold(
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || !arguments.get_layout().is_empty());

  auto bytes = receiver->select_identity<Language::Constants::Bytes>();
  BAIL_IF(!bytes);
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, bytes->get_value(), bytes->get_resource());
}

auto Builtin::Fixed::View::fold_abi(
    const ttx_abstract* callable,
    const ttx_pack* receiver,
    const ttx_pack* arguments) -> const ttx_abstract* {
  const auto& selected =
      static_cast<const View&>(Ttx::Concept::Abstract::from_abi(callable));
  auto source = receiver ? Core::Option<const Language::Model::Pack&>(
                               Language::Model::Pack::from_abi(receiver))
                         : Core::Option<const Language::Model::Pack&>();
  const auto& inputs = Language::Model::Pack::from_abi(arguments);
  auto result = selected.fold(source, inputs);
  auto identity = result ? result->get_identity()
                         : Core::Option<const Ttx::Concept::Abstract&>();
  return identity && Ttx::Concept::Constant::prove(*identity)
             ? identity->get_abi()
             : ttx_none();
}

const ttx_library_fold_call_operations Builtin::Fixed::View::fold_operations = {
  .interface = {.negotiate = ttx_library_fold_call_relation},
  .fold = fold_abi,
};

auto Builtin::Fixed::View::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_library_fold_call_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), &fold_operations.interface)
             : Language::Model::Callable::negotiate_interface(requirement);
}
