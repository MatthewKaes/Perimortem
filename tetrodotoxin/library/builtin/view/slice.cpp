// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/view/slice.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/fold.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

static auto create_parameter_entries(
    Ttx::Model::Layouts::Addressable& self,
    Ttx::Model::Layouts::Addressable& start,
    Ttx::Model::Layouts::Addressable& count)
    -> Core::Static::Vector<const Abstract*, 3> {
  const Core::Static::Vector<const Abstract*, 3> entries = {{
    &self,
    &start,
    &count,
  }};
  return entries;
}

Builtin::View::Slice::Slice(
    Memory::Allocator::Arena& domain,
    Ttx::Model::Layouts::Addressable& self,
    Ttx::Model::Layouts::Addressable& start,
    Ttx::Model::Layouts::Addressable& count,
    const Language::Model::Type& result)
    : domain(domain),
      parameter_entries(create_parameter_entries(self, start, count)),
      parameters(parameter_entries.get_view()),
      results(result, 1),
      result_type(result) {}

auto Builtin::View::Slice::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& count,
    const Language::Model::Type& result) -> Slice& {
  Ttx::Model::Layouts::Addressable& self =
      Ttx::Model::Layouts::Addressable::create_synthetic(
          domain, "self"_view, receiver);
  Ttx::Model::Layouts::Addressable& start =
      Ttx::Model::Layouts::Addressable::create_synthetic(
          domain, "start"_view, count);
  Ttx::Model::Layouts::Addressable& size =
      Ttx::Model::Layouts::Addressable::create_synthetic(
          domain, "count"_view, count);
  return domain.construct_from<Slice>(
      [&]() -> Slice { return Slice(domain, self, start, size, result); });
}

static auto select_unsigned(
    const Tetrodotoxin::Library::Language::Model::Pack& values,
    Count index) -> Core::Option<U64> {
  auto producer = values.get_layout().get_abstract(index);
  BAIL_IF(!producer);
  auto constant = producer->select<Language::Constants::Unsigned>();
  if (constant) {
    return constant->get_value();
  }

  // The argument Pack retains its authored producer identity. Following that
  // producer through ordinary folding keeps const Locals usable without
  // copying their values into the Callable.
  auto source = Language::Model::Pack::from(const_cast<Abstract&>(*producer));
  BAIL_IF(!source);
  auto folded = Language::query_folded_pack(*source);
  BAIL_IF(!folded);

  auto selected = folded->get_layout().get_abstract(0);
  BAIL_IF(!selected);
  constant = selected->select<Language::Constants::Unsigned>();
  return constant ? Core::Option<U64>(constant->get_value())
                  : Core::Option<U64>();
}

auto Builtin::View::Slice::invoke(
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || arguments.get_layout().get_size() != 2);

  auto bytes = receiver->select_identity<Language::Constants::Bytes>();
  auto start = select_unsigned(arguments, 0);
  auto count = select_unsigned(arguments, 1);
  BAIL_IF(
      !bytes || !start || !count || *start > U64(Count(-1)) ||
      *count > U64(Count(-1)));

  Core::View::Bytes selected =
      bytes->get_value().slice(Count(*start), Count(*count));
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, selected);
}

auto Builtin::View::Slice::invoke_abi(
    const ttx_abstract* callable,
    const ttx_pack* receiver,
    const ttx_pack* arguments) -> const ttx_pack* {
  const auto& selected =
      static_cast<const Slice&>(Ttx::Concept::Abstract::from_abi(callable));
  auto source = receiver ? Core::Option<const Language::Model::Pack&>(
                               Language::Model::Pack::from_abi(receiver))
                         : Core::Option<const Language::Model::Pack&>();
  const auto& inputs = Language::Model::Pack::from_abi(arguments);
  auto result = selected.invoke(source, inputs);
  return result ? result->get_abi() : nullptr;
}

const ttx_library_invocation_operations
    Builtin::View::Slice::invocation_operations = {
      .interface = {.negotiate = ttx_library_invocation_relation},
      .invoke = invoke_abi,
};

auto Builtin::View::Slice::negotiate_interface(
    const ttx_abstract* requirement) const -> ttx_interface {
  return requirement == ttx_library_invocation_requirement()
             ? ttx_interface_satisfied(
                   requirement, get_abi(), &invocation_operations.interface)
             : Language::Model::Callable::negotiate_interface(requirement);
}
