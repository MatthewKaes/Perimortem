// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/result.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Tetrodotoxin::Library::Language;

auto Constants::Result::persist(Archive::Writer& writer) const -> Bool {
  auto record = writer.begin(Archive::Tag::ConstantResult);
  BAIL_IF(
      !writer.write(get_type().get_name()) ||
      !writer.write(get_type().get_value_type().get_name()) ||
      !writer.write(get_type().get_error_type().get_name()));
  writer.write(Unsigned_8(get_kind()));
  BAIL_IF(
      !Model::Pack::persist_folded(writer, get_payload()) ||
      !writer.finish(record));
  return True;
}

auto Constants::Result::create(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Types::Result::Kind kind,
    Model::Pack& payload) -> Core::Option<Result&> {
  const Model::Type& selected = kind == Types::Result::Kind::Value
                                    ? type.get_value_type()
                                    : type.get_error_type();
  BAIL_IF(!payload.fits_into(selected));
  const Layout& layout = payload.get_layout();
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    BAIL_IF(!entry || !entry->is<Constant>());
  }

  return Expression::create_synthetic<Result>(
      domain, [&](auto source) -> Result {
        return Result(type, kind, payload, source);
      });
}

auto Constants::Result::create_value(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Model::Pack& payload) -> Core::Option<Result&> {
  return create(domain, type, Types::Result::Kind::Value, payload);
}

auto Constants::Result::create_error(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Model::Pack& payload) -> Core::Option<Result&> {
  return create(domain, type, Types::Result::Kind::Error, payload);
}

auto Constants::Result::select(Model::Pack& source) -> Core::Option<Result&> {
  auto direct = source.select<Result>();
  if (direct) {
    return *direct;
  }

  const Layout& layout = source.get_layout();
  BAIL_IF(layout.get_size() != 1);
  return layout.get_abstract(0).visit(
      []() -> Core::Option<Result&> { return {}; },
      [](const Abstract& selected) -> Core::Option<Result&> {
        auto pack = const_cast<Abstract&>(selected).select<Model::Pack>();
        return pack ? pack->select<Result>() : Core::Option<Result&>();
      });
}

auto Constants::Result::create_fitted(
    Memory::Allocator::Arena& domain,
    const Types::Result& type,
    Model::Pack& source) -> Core::Option<Result&> {
  auto retained = select(source);
  if (retained && &retained->get_type() == &type) {
    return *retained;
  }

  Model::Pack* payload = &source;
  auto expression = source.select<Expression>();
  if (expression) {
    Core::Option<Model::Pack&> folded;
    expression->fold().visit(
        [&](const Core::Option<Model::Pack&>& selected) { folded = selected; },
        [](const Expression::Error&) {});
    BAIL_IF(!folded);
    payload = &*folded;
  }

  retained = select(*payload);
  if (retained && &retained->get_type() == &type) {
    return *retained;
  }

  Bool value = payload->fits_into(type.get_value_type());
  Bool error = payload->fits_into(type.get_error_type());
  BAIL_IF(value == error);
  return value ? create_value(domain, type, *payload)
               : create_error(domain, type, *payload);
}

auto Constants::Result::equals(const Constant& rhs) const -> Bool {
  auto selected = rhs.select<Constants::Result>();
  return selected && has_same_type(rhs) && kind == selected->kind &&
                 have_equal_values(payload.get(), selected->payload.get())
             ? True
             : False;
}

auto Constants::Result::lower(Llvm::Builder& body) const -> Bool {
  Bool lowered = prepare_carrier(body) && payload.get().lower(body);
  return lowered && body.result(get_type(), *this, payload.get());
}
