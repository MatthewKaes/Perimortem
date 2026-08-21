// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/builtin/object/reserve.hpp"

#include "tetrodotoxin/library/language/types/object_storage.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

static auto create_entries(
    Language::Parameter& self,
    Language::Parameter& count)
    -> Core::Static::Vector<Reference<const Abstract>, 2> {
  const Core::Static::Vector<Reference<const Abstract>, 2> entries = {{
    Reference<const Abstract>(self),
    Reference<const Abstract>(count),
  }};
  return entries;
}

Builtin::Object::Reserve::Reserve(
    Language::Parameter& self,
    Language::Parameter& count,
    const Language::Model::Type& result)
    : parameter_entries(create_entries(self, count)),
      parameters(parameter_entries.get_view()),
      results(result, 1),
      result_type(result) {}

auto Builtin::Object::Reserve::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& count,
    const Language::Model::Type& result) -> Reserve& {
  auto& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  auto& count_parameter =
      Language::Parameter::create_synthetic(domain, "count"_view, count);
  return domain.construct_from<Reserve>(
      [&]() -> Reserve { return Reserve(self, count_parameter, result); });
}

auto Builtin::Object::Reserve::accepts_receiver(
    const Abstract& receiver,
    const Abstract& host) const -> Bool {
  auto addressable = receiver.resolve().select<Language::Model::Addressable>();
  auto access_scope = host.resolve().select<Language::Model::Type>();
  return addressable && access_scope &&
         addressable->permits_write_from(*access_scope);
}

auto Builtin::Object::Reserve::lower_call(
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
         inputs.get_size() == 2 &&
         body.object_reserve(
             result, result_type, receiver->get_type(), *receiver_source,
             inputs.get_data()[0], inputs.get_data()[1], *fallback);
}
