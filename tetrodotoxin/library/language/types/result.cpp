// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/result.hpp"

#include "tetrodotoxin/library/language/constants/result.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library::Language;

auto Types::Result::create_default(Memory::Allocator::Arena& arena) const
    -> Core::Option<Model::Pack&> {
  auto selected = value.create_default(arena);
  BAIL_IF(!selected);
  auto created = Constants::Result::create_value(arena, *this, *selected);
  return created ? Core::Option<Model::Pack&>(*created)
                 : Core::Option<Model::Pack&>();
}

auto Types::Result::fold_propagation(Model::Pack& source) const
    -> Utility::Result<Core::Option<Model::Pack&>, Bool> {
  auto selected = Constants::Result::select(source);
  if (!selected || &selected->get_type() != this) {
    return False;
  }

  return selected->get_kind() == Kind::Value
             ? Core::Option<Model::Pack&>(
                   const_cast<Model::Pack&>(selected->get_payload()))
             : Core::Option<Model::Pack&>();
}

auto Types::Result::lower_propagation(
    Llvm::Builder& body,
    const Model::Pack& result,
    const Model::Pack& source,
    const Model::Pack& escape) const -> Bool {
  return body.propagate_result(*this, value, error, result, source, escape);
}

auto Types::Result::accepts(const Model::Pack& source) const -> Bool {
  if (source.fits(*this)) {
    return True;
  }

  Bool accepts_value = source.fits_into(value);
  Bool accepts_error = source.fits_into(error);
  return accepts_value != accepts_error;
}

auto Types::Result::create_fitted(
    Memory::Allocator::Arena& arena,
    Model::Pack& source) const -> Core::Option<Model::Pack&> {
  auto fitted = Constants::Result::create_fitted(arena, *this, source);
  return fitted ? Core::Option<Model::Pack&>(*fitted)
                : Core::Option<Model::Pack&>();
}

auto Types::Result::reserve(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved =
      carriers.reserve(program, *this, Llvm::Carriers::Kind::Result);
  if (!reserved) {
    return False;
  }

  if (!*reserved) {
    return True;
  }

  return value.reserve(program) && error.reserve(program) &&
         flag.reserve(program);
}

auto Types::Result::complete(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool alternatives = value.complete(program) && error.complete(program);
  if (!alternatives || !flag.complete(program)) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete(program, *this, Llvm::Carriers::Kind::Result);
  return carrier_completed && complete_debug(program);
}

auto Types::Result::validate_layout(Ttx::Lexical::Cursor& cursor) const
    -> Bool {
  if (!value.get_layout().is_empty() && !error.get_layout().is_empty()) {
    return True;
  }

  cursor.create_error(
      "Result alternatives must each produce one nonempty value Type."_view,
      "Replace the empty value or error Type before using this Result."_view);
  return False;
}
