// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/library/builtin/fixed/access.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Builtin::Fixed::Access::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> Access& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<Access>(
      [&]() -> Access { return Access(self, result); });
}

auto Builtin::Fixed::Access::accepts_receiver(
    const Abstract& receiver,
    const Abstract& host) const -> Bool {
  auto addressable = receiver.resolve().select<Language::Model::Addressable>();
  auto access_scope = host.resolve().select<Language::Model::Type>();
  return addressable && access_scope &&
         addressable->permits_write_from(*access_scope);
}

auto Builtin::Fixed::Access::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&> receiver_source) const -> Bool {
  auto result_type = get_results().get_abstract(0).visit(
      []() -> Core::Option<const Ttx::Model::Type&> { return {}; },
      [](const Ttx::Concept::Abstract& selected)
          -> Core::Option<const Ttx::Model::Type&> {
        return selected.resolve().select<Ttx::Model::Type>();
      });
  auto parameter = get_parameters().get_abstract(0);
  auto receiver = parameter ? parameter->select<Ttx::Model::Addressable>()
                            : Core::Option<const Ttx::Model::Addressable&>();
  return result_type && receiver && receiver_source && inputs.get_size() == 1 &&
         body.borrow_fixed(
             result, *result_type, receiver->get_type(), *receiver_source,
             inputs.get_data()[0]);
}
