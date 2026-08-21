// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/builtin/fixed/view.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;

auto Builtin::Fixed::View::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> View& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<View>(
      [&]() -> View { return View(self, result); });
}

auto Builtin::Fixed::View::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&> receiver_source) const -> Bool {
  BAIL_IF(!receiver_source || inputs.get_size() != 1);

  auto bytes = receiver_source->select<Language::Constants::Bytes>();
  if (bytes) {
    return body.bytes_value(result_type, result, bytes->get_value());
  }

  auto parameter = get_parameters().get_abstract(0);
  auto receiver = parameter ? parameter->select<Ttx::Model::Addressable>()
                            : Core::Option<const Ttx::Model::Addressable&>();
  return receiver && body.borrow_fixed(
                         result, result_type, receiver->get_type(),
                         *receiver_source, inputs.get_data()[0]);
}

auto Builtin::Fixed::View::fold_call(
    Memory::Allocator::Arena& domain,
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || !arguments.get_layout().is_empty());

  auto bytes = receiver->select<Language::Constants::Bytes>();
  BAIL_IF(!bytes);
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, bytes->get_value());
}
