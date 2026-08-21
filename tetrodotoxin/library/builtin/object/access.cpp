// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/builtin/object/access.hpp"

#include "tetrodotoxin/library/language/types/object_storage.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

auto Builtin::Object::Access::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& result) -> Access& {
  auto& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  return domain.construct_from<Access>(
      [&]() -> Access { return Access(self, result); });
}

auto Builtin::Object::Access::accepts_receiver(
    const Abstract& receiver,
    const Abstract& host) const -> Bool {
  auto addressable = receiver.resolve().select<Language::Model::Addressable>();
  auto access_scope = host.resolve().select<Language::Model::Type>();
  return addressable && access_scope &&
         addressable->permits_write_from(*access_scope);
}

auto Builtin::Object::Access::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&> receiver_source) const -> Bool {
  auto parameter = get_parameters().get_abstract(0);
  auto receiver = parameter ? parameter->select<Ttx::Model::Addressable>()
                            : Core::Option<const Ttx::Model::Addressable&>();
  auto storage =
      receiver ? receiver->get_type().select<Language::Types::ObjectStorage>()
               : Core::Option<const Language::Types::ObjectStorage&>();
  auto fallback = storage ? storage->get_element_type().create_default(
                                body.get_program().get_arena())
                          : Core::Option<Language::Model::Pack&>();
  return receiver && receiver_source && fallback && fallback->lower(body) &&
         inputs.get_size() == 1 &&
         body.object_access(
             result, result_type, receiver->get_type(), *receiver_source,
             inputs.get_data()[0], *fallback);
}
