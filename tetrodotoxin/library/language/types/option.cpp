// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/types/option.hpp"

#include "tetrodotoxin/library/language/constants/option.hpp"
#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Library::Language;

static auto select_option_constant(Model::Pack& source)
    -> Option<Constants::Option&> {
  auto direct = source.select<Constants::Option>();
  if (direct) {
    return *direct;
  }

  const Ttx::Concept::Layout& layout = source.get_layout();
  BAIL_IF(layout.get_size() != 1);
  return layout.get_abstract(0).visit(
      []() -> Option<Constants::Option&> { return {}; },
      [](const Ttx::Concept::Abstract& selected) -> Option<Constants::Option&> {
        auto pack =
            const_cast<Ttx::Concept::Abstract&>(selected).select<Model::Pack>();
        return pack ? pack->select<Constants::Option>()
                    : Option<Constants::Option&>();
      });
}

auto Types::Option::create_default(Perimortem::Memory::Allocator::Arena& arena)
    const -> Perimortem::Core::Option<Model::Pack&> {
  return Constants::Option::create_absent(arena, *this);
}

auto Types::Option::fold_propagation(Model::Pack& source) const -> Perimortem::
    Utility::Result<Perimortem::Core::Option<Model::Pack&>, Bool> {
  auto selected = select_option_constant(source);
  if (!selected || &selected->get_type() != this) {
    return False;
  }

  auto payload = selected->get_payload();
  return payload ? Perimortem::Core::Option<Model::Pack&>(
                       const_cast<Model::Pack&>(*payload))
                 : Perimortem::Core::Option<Model::Pack&>();
}

auto Types::Option::lower_propagation(
    Llvm::Builder& body,
    const Model::Pack& result,
    const Model::Pack& source,
    const Model::Pack& escape) const -> Bool {
  return body.propagate_option(*this, element, result, source, escape);
}

auto Types::Option::accepts(const Model::Pack& source) const -> Bool {
  return source.get_layout().is_empty() || source.fits_into(element);
}

auto Types::Option::create_fitted(
    Perimortem::Memory::Allocator::Arena& arena,
    Model::Pack& source) const -> Perimortem::Core::Option<Model::Pack&> {
  auto fitted = Constants::Option::create_fitted(arena, *this, source);
  return fitted ? Perimortem::Core::Option<Model::Pack&>(*fitted)
                : Perimortem::Core::Option<Model::Pack&>();
}

auto Types::Option::reserve(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto reserved =
      carriers.reserve(program, *this, Llvm::Carriers::Kind::Option);
  if (!reserved) {
    return False;
  }

  if (!*reserved) {
    return True;
  }

  Bool element_reserved = element.reserve(program);
  if (!element_reserved) {
    return False;
  }

  return flag.reserve(program);
}

auto Types::Option::complete(Llvm::Program& program) const -> Bool {
  const auto& carriers = program.get_carriers();
  auto began = carriers.begin_completion(program, *this);
  if (!began) {
    return False;
  }

  if (!*began) {
    return True;
  }

  Bool element_completed = element.complete(program);
  if (!element_completed) {
    return False;
  }

  Bool flag_completed = flag.complete(program);
  if (!flag_completed) {
    return False;
  }

  Bool carrier_completed =
      carriers.complete(program, *this, Llvm::Carriers::Kind::Option);
  return carrier_completed && complete_debug(program);
}

auto Types::Option::validate_layout(Ttx::Lexical::Cursor& cursor) const
    -> Bool {
  if (!element.get_layout().is_empty()) {
    return True;
  }

  cursor.create_error(
      "Option requires one nonempty element Type."_view,
      "Replace the empty element before using this Option."_view);
  return False;
}
