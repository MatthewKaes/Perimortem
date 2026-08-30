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

auto Builtin::View::IsEmpty::invoke_abi(
    const ttx_abstract* callable,
    const ttx_pack* receiver,
    const ttx_pack* arguments) -> const ttx_pack* {
  const auto& selected =
      static_cast<const IsEmpty&>(Ttx::Concept::Abstract::from_abi(callable));
  auto source = receiver ? Core::Option<const Language::Model::Pack&>(
                               Language::Model::Pack::from_abi(receiver))
                         : Core::Option<const Language::Model::Pack&>();
  const auto& inputs = Language::Model::Pack::from_abi(arguments);
  auto result = selected.invoke(source, inputs);
  return result ? result->get_abi() : nullptr;
}

const ttx_library_invocation_operations
    Builtin::View::IsEmpty::invocation_operations = {
      .interface = {.negotiate = ttx_library_invocation_relation},
      .invoke = invoke_abi,
};

auto Builtin::View::IsEmpty::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_library_invocation_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), &invocation_operations.interface)
             : Language::Model::Callable::negotiate_interface(requirement);
}
