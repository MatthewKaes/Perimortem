// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/builtin/view/slice.hpp"

#include "tetrodotoxin/library/language/constants/bytes.hpp"
#include "tetrodotoxin/library/language/constants/unsigned.hpp"
#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library;

static auto create_parameter_entries(
    Language::Parameter& self,
    Language::Parameter& start,
    Language::Parameter& count)
    -> Core::Static::Vector<Reference<const Abstract>, 3> {
  const Core::Static::Vector<Reference<const Abstract>, 3> entries = {{
    Reference<const Abstract>(self),
    Reference<const Abstract>(start),
    Reference<const Abstract>(count),
  }};
  return entries;
}

Builtin::View::Slice::Slice(
    Language::Parameter& self,
    Language::Parameter& start,
    Language::Parameter& count,
    const Language::Model::Type& result)
    : parameter_entries(create_parameter_entries(self, start, count)),
      parameters(parameter_entries.get_view()),
      results(result, 1),
      result_type(result) {}

auto Builtin::View::Slice::create(
    Memory::Allocator::Arena& domain,
    const Language::Model::Type& receiver,
    const Language::Model::Type& count,
    const Language::Model::Type& result) -> Slice& {
  Language::Parameter& self =
      Language::Parameter::create_synthetic(domain, "self"_view, receiver);
  Language::Parameter& start =
      Language::Parameter::create_synthetic(domain, "start"_view, count);
  Language::Parameter& size =
      Language::Parameter::create_synthetic(domain, "count"_view, count);
  return domain.construct_from<Slice>(
      [&]() -> Slice { return Slice(self, start, size, result); });
}

auto Builtin::View::Slice::lower_call(
    Llvm::Builder& body,
    const Ttx::Model::Pack& result,
    Core::View::Vector<LLVMValueRef> inputs,
    Core::Option<const Ttx::Model::Pack&>) const -> Bool {
  BAIL_IF(inputs.get_size() != 3);

  auto receiver = get_parameters().get_abstract(0);
  auto receiver_parameter =
      receiver ? receiver->select<Ttx::Model::Addressable>()
               : Core::Option<const Ttx::Model::Addressable&>();
  return receiver_parameter &&
         body.slice_view(
             result, result_type, receiver_parameter->get_type(), inputs[0],
             inputs[1], inputs[2]);
}

static auto select_unsigned(const Ttx::Model::Pack& values, Count index)
    -> Core::Option<Unsigned_64> {
  auto produced = values.get_produced(index);
  BAIL_IF(!produced);
  auto constant = produced->producer.select<Language::Constants::Unsigned>();
  if (constant) {
    return constant->get_value();
  }

  // The argument Pack retains its authored producer identity. Following that
  // producer through ordinary folding keeps const Locals usable without
  // copying their values into the Callable.
  auto expression = const_cast<Ttx::Model::Pack&>(produced->producer)
                        .select<Language::Expression>();
  BAIL_IF(!expression);

  Core::Option<Language::Model::Pack&> folded;
  expression->fold().visit(
      [&](const Core::Option<Language::Model::Pack&>& selected) {
        folded = selected;
      },
      [](const Language::Expression::Error&) {});
  BAIL_IF(!folded);

  auto selected = folded->get_produced(produced->local_index);
  BAIL_IF(!selected);
  constant = selected->producer.select<Language::Constants::Unsigned>();
  return constant ? Core::Option<Unsigned_64>(constant->get_value())
                  : Core::Option<Unsigned_64>();
}

auto Builtin::View::Slice::fold_call(
    Memory::Allocator::Arena& domain,
    Core::Option<const Language::Model::Pack&> receiver,
    const Language::Model::Pack& arguments) const
    -> Core::Option<Language::Model::Pack&> {
  BAIL_IF(!receiver || arguments.get_layout().get_size() != 2);

  auto bytes = receiver->select<Language::Constants::Bytes>();
  auto start = select_unsigned(arguments, 0);
  auto count = select_unsigned(arguments, 1);
  BAIL_IF(
      !bytes || !start || !count || *start > Unsigned_64(Count(-1)) ||
      *count > Unsigned_64(Count(-1)));

  Core::View::Bytes selected =
      bytes->get_value().slice(Count(*start), Count(*count));
  return Language::Constants::Bytes::create_synthetic(
      domain, result_type, selected);
}
