// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expressions/initializer.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_value_type(const Abstract& candidate)
    -> Core::Option<const Type&> {
  auto type = candidate.select<Type>();
  if (type) {
    return *type;
  }

  auto addressable = candidate.select<Addressable>();
  if (addressable) {
    return addressable->get_type();
  }

  const Abstract& resolved = candidate.resolve();
  addressable = resolved.select<Addressable>();
  return addressable ? Core::Option<const Type&>(addressable->get_type())
                     : resolved.select<Type>();
}

class InitializerValues final : public Language::Model::Pack {
 public:
  class Layout final : public Ttx::Concept::Layout {
   public:
    constexpr Layout(const InitializerValues& values) : values(values) {}

    auto get_size() const -> Count override {
      return values.entries.get_size();
    }

    auto get_abstract(Count index) const
        -> Core::Option<const Abstract&> override {
      BAIL_IF(index >= get_size());
      return values.entries.get_view().get_data()[index].get();
    }

    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source,
        Count target_index) const -> Bool override {
      BAIL_IF(source >= get_size() || target_index >= target.get_size());
      auto required = target.get_abstract(target_index);
      BAIL_IF(!required);
      auto type = select_value_type(*required);
      return type &&
             values.entries.get_view().get_data()[source].get().fits_into(
                 *type);
    }

    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override {
      BAIL_IF(!has_target_segment(target, target_offset));
      for (Count index = 0; index < get_size(); index++) {
        BAIL_IF(!fits_entry(target, index, target_offset + index));
      }
      return True;
    }

    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Utility::Result<const Abstract&, Errors> override {
      if (target_index >= get_size()) {
        return Errors::IndexOutOfBounds;
      }
      if (!has_target_segment(target, target_offset)) {
        return Errors::SizeMismatch;
      }
      if (!fits_at(target, target_offset)) {
        return Errors::IncompatibleFit;
      }
      return values.entries.get_view().get_data()[target_index].get();
    }

   private:
    const InitializerValues& values;
  };

  TTX_CONTRACT(
      InitializerValues,
      Language::Model::Pack,
      0xcbd30af87c70472c,
      0xa05533748abc2332);

  InitializerValues(
      Memory::Allocator::Arena& domain,
      Core::View::Vector<Reference<Language::Model::Pack>> source)
      : entries(domain), layout(*this), linked(True) {
    entries.reset(source.get_size());
    for (Count index = 0; index < source.get_size(); index++) {
      entries.insert(source.get_data()[index]);
      if (&source.get_data()[index].get().resolve() ==
          &Invalid::get_invalid()) {
        linked = False;
      }
    }
  }

  TTX_NAME("Initializer values"_view);
  TTX_EMPTY_DOCUMENTATION();
  TTX_INVALID_CONTEXT;

  auto link(
      Tetrodotoxin::Language::Monograph& source,
      const Abstract& lexical_context,
      Core::Option<const Type&> access_scope) -> Bool override {
    if (linked) {
      return True;
    }

    Bool failed = False;
    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      auto initializer =
          entry.get().select<Language::Expressions::Initializer>();
      Bool synthetic_initializer = initializer && !initializer->get_anchor();
      if (synthetic_initializer ||
          &entry.get().resolve() == &Invalid::get_invalid()) {
        failed |= !entry.get().link(source, lexical_context, access_scope);
      }
    }
    BAIL_IF(failed);

    linked = True;
    return True;
  }

  auto get_layout() const -> const Ttx::Concept::Layout& override {
    return layout;
  }

  auto resolve() const -> const Abstract& override {
    return linked ? static_cast<const Language::Model::Pack&>(*this)
                  : static_cast<const Abstract&>(Invalid::get_invalid());
  }

  auto finalize() -> void override {
    for (Reference<Language::Model::Pack> entry : entries.get_view()) {
      entry.get().finalize();
    }
  }

 private:
  Memory::Managed::Vector<Reference<Language::Model::Pack>> entries;
  Layout layout;
  Bool linked = False;
};

static auto select_accessible_field(
    const Abstract& candidate,
    Core::Option<const Type&> access_scope)
    -> Core::Option<const Language::Field&> {
  auto field = candidate.select<Language::Field>();
  BAIL_IF(
      !field || field->get_writability() != Language::Writability::Internal ||
      !Language::Access::Address::is_accessible(*field, access_scope));

  // Object construction consumes the target's real authored Field order, but
  // shares Address's one publication and host authority decision. Static and
  // const Fields remain declaration owned and never enter the supplied
  // construction Layout.
  return *field;
}

auto Language::Expressions::Initializer::is_next(
    const Ttx::Lexical::Cursor& cursor) -> Bool {
  return cursor.matches(Ttx::Lexical::Code::Type::New);
}

auto Language::Expressions::Initializer::parse(
    Memory::Allocator::Arena& domain,
    Language::Monograph& source,
    Ttx::Lexical::Cursor& cursor) -> Core::Option<Initializer&> {
  auto transaction = cursor.branch();
  BAIL_IF(!is_next(transaction));

  Ttx::Lexical::Token opening = transaction.consume();
  BAIL_IF(!transaction.require(
      Ttx::Lexical::Code::Type::BracketStart,
      "Library `new` requires `[` before its Object Type."_view));
  auto target_reference = TypeReference::parse(source, transaction);
  BAIL_IF(!target_reference);
  Ttx::Lexical::Token type_closing = transaction.require(
      Ttx::Lexical::Code::Type::BracketEnd,
      "Library `new` requires `]` after its Object Type."_view);
  BAIL_IF(!type_closing);

  Language::Model::Pack* arguments = nullptr;
  Ttx::Lexical::Token closing = type_closing;
  if (transaction.matches(Ttx::Lexical::Code::Type::PackingStart)) {
    Ttx::Lexical::Token argument_opening = transaction.current();
    auto parsed =
        Language::Model::Parser::Pack::parse(domain, source, transaction, True);
    BAIL_IF(!parsed);
    if (parsed->get_layout().is_empty()) {
      transaction.create_expression_error(
          Ttx::Lexical::Span(argument_opening, transaction.peek(-1)),
          "Object initializer arguments cannot be empty."_view,
          "Omit the argument list when every state Field should use its "
          "default."_view);
      return {};
    }
    arguments = &*parsed;
    closing = transaction.peek(-1);
  } else {
    // The omitted form owns an independent empty Pack. Sharing one static
    // empty Layout would also share its staged link and finalization lifetime
    // across otherwise unrelated initializer transactions.
    arguments = &Language::Model::Pack::create_empty(domain);
  }

  Ttx::Lexical::Anchor anchor = Ttx::Lexical::Anchor::create(
      opening, Ttx::Lexical::Span(opening, closing));
  Initializer& initializer = Expression::create_authored<Initializer>(
      domain, anchor,
      [&](Core::Option<Ttx::Lexical::Anchor> source) -> Initializer {
        return Initializer(domain, *target_reference, *arguments, source);
      });
  cursor.join(transaction);
  return initializer;
}

auto Language::Expressions::Initializer::create_synthetic(
    Memory::Allocator::Arena& domain,
    const Type& type,
    Core::View::Vector<Reference<Model::Pack>> values) -> Initializer& {
  auto& arguments = Model::Pack::create_empty(domain);
  auto& completed = domain.construct<InitializerValues>(domain, values);
  Initializer& initializer = Expression::create_synthetic<Initializer>(
      domain, [&](Core::Option<Ttx::Lexical::Anchor> source) -> Initializer {
        return Initializer(domain, {}, arguments, source);
      });
  initializer.expected_type = Reference<const Type>(type);
  initializer.completed_values = Reference<Model::Pack>(completed);
  return initializer;
}

Language::Expressions::Initializer::Initializer(
    Memory::Allocator::Arena& domain,
    Core::Option<TypeReference> target_reference,
    Language::Model::Pack& arguments,
    Core::Option<Ttx::Lexical::Anchor> anchor)
    : Expression(anchor),
      domain(domain),
      target_reference(target_reference),
      arguments(arguments) {}

auto Language::Expressions::Initializer::get_type() const -> const Abstract& {
  return expected_type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Type>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Language::Expressions::Initializer::fits(const Type& target) const
    -> Bool {
  return expected_type && &expected_type->get() == &target;
}

auto Language::Expressions::Initializer::finalize() -> void {
  // The initializer owns the complete argument flow. Finalize its real Pack in
  // source order before folding the initializer node itself. No second
  // expression inventory exists beside the Pack's canonical Layout.
  arguments.finalize();
  completed_values.visit(
      []() {}, [](Reference<Model::Pack>& values) { values.get().finalize(); });
  Expression::finalize();
}

auto Language::Expressions::Initializer::get_completed_values() const
    -> Core::Option<const Model::Pack&> {
  return completed_values.visit(
      []() -> Core::Option<const Model::Pack&> { return {}; },
      [](const Reference<Model::Pack>& selected)
          -> Core::Option<const Model::Pack&> { return selected.get(); });
}

auto Language::Expressions::Initializer::supplies(
    Core::Option<const Type&> access_scope,
    const Types::Object& target,
    const Field& field) const -> Bool {
  return Bool(select_supplied(access_scope, target, field));
}

auto Language::Expressions::Initializer::select_supplied(
    Core::Option<const Type&> access_scope,
    const Types::Object& target,
    const Field& field) const -> Core::Option<const Model::Pack&> {
  const Layout& inputs = arguments.get_layout();
  if (inputs.get_name(0)) {
    for (Count i = 0; i < inputs.get_size(); i++) {
      auto name = inputs.get_name(i);
      if (name && *name == field.get_name()) {
        return inputs.get_abstract(i).visit(
            []() -> Core::Option<const Model::Pack&> { return {}; },
            [](const Abstract& selected) {
              return selected.select<Model::Pack>();
            });
      }
    }
    return {};
  }

  Count input = 0;
  for (const Reference<Abstract>& selected : target.get_addressables()) {
    auto selected_field = select_accessible_field(selected.get(), access_scope);
    if (!selected_field) {
      continue;
    }

    if (input >= inputs.get_size()) {
      break;
    }

    if (&*selected_field == &field) {
      return inputs.get_abstract(input).visit(
          []() -> Core::Option<const Model::Pack&> { return {}; },
          [](const Abstract& selected) {
            return selected.select<Model::Pack>();
          });
    }
    input++;
  }
  return {};
}

auto Language::Expressions::Initializer::has_mandatory_cycle(
    Core::Option<const Type&> access_scope,
    const Types::Object& target,
    Core::View::Vector<Reference<const Types::Object>> path) const -> Bool {
  for (Count i = 0; i < path.get_size(); i++) {
    if (&path.get_data()[i].get() == &target) {
      return True;
    }
  }

  Memory::Managed::Vector<Reference<const Types::Object>> next_path(domain);
  next_path.reset(path.get_size() + 1);
  for (Count i = 0; i < path.get_size(); i++) {
    next_path.insert(path.get_data()[i]);
  }
  next_path.insert(target);

  // Only an omitted Field uses its authored initializer. Following those
  // exact Initializer edges distinguishes a mandatory cycle from one broken
  // by a supplied value. Each nested declaration contributes its own host
  // scope, so an outer constructor never lends authority to another Object.
  for (const Reference<Abstract>& selected : target.get_addressables()) {
    auto selected_field = selected.get().select<Field>();
    if (!selected_field ||
        selected_field->get_writability() != Writability::Internal) {
      continue;
    }
    const Field& field = *selected_field;
    if (supplies(access_scope, target, field)) {
      continue;
    }

    auto field_initializer = field.get_initializer();
    if (!field_initializer) {
      continue;
    }

    auto nested_initializer = field_initializer->select<Initializer>();
    auto nested_target = field.get_type().resolve().select<Types::Object>();
    if (nested_initializer && nested_target &&
        nested_initializer->has_mandatory_cycle(
            field.get_host(), *nested_target, next_path.get_view())) {
      return True;
    }
  }

  return False;
}

auto Language::Expressions::Initializer::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Core::Option<const Type&> access_scope) -> Bool {
  if (expected_type && completed_values) {
    BAIL_IF(
        !completed_values->get().link(source, lexical_context, access_scope));
    return Expression::link(source, lexical_context, access_scope);
  }

  auto context = access_scope.visit(
      []() -> Core::Option<const Types::Composite&> { return {}; },
      [](const Type& selected) { return selected.select<Types::Composite>(); });
  if (!context || !target_reference) {
    source.report(
        get_anchor(),
        "Object initializer requires one declaring Composite context."_view,
        "Retain `new[Type]` only on one Library declaration."_view);
    return False;
  }

  const Abstract& selected = context->resolve_type(*target_reference);
  auto target = selected.select<Types::Object>();
  if (!target || target->get_layout().is_empty()) {
    source.report(
        target_reference->get_anchor(),
        "Object initializer requires one nonempty Object Type."_view,
        "Name an Object Type with at least one state value in `new[Type]`."_view);
    return False;
  }

  if (expected_type) {
    if (&expected_type->get() == &*target) {
      return True;
    }

    source.report(
        get_anchor(),
        "Object initializer cannot change its expected Type."_view,
        "Keep the authored initializer on its original declaration."_view);
    return False;
  }

  BAIL_IF(!arguments.link(source, lexical_context, access_scope));

  const Layout& inputs = arguments.get_layout();

  Memory::Managed::Vector<Reference<const Abstract>> fitted_fields(domain);
  fitted_fields.reset(inputs.get_size());

  // The receiving host selects the real accessible Field range. Named inputs
  // preserve their source order while the fitted target follows authored Field
  // order. Positional inputs select the same accessible range directly.
  if (inputs.get_name(0)) {
    for (const Reference<Abstract>& selected : target->get_addressables()) {
      auto selected_field =
          select_accessible_field(selected.get(), access_scope);
      if (!selected_field) {
        continue;
      }
      const Field& field = *selected_field;
      for (Count input_index = 0; input_index < inputs.get_size();
           input_index++) {
        auto name = inputs.get_name(input_index);
        if (name && *name == field.get_name()) {
          fitted_fields.insert(field);
        }
      }
    }
  } else {
    Count input = 0;
    for (const Reference<Abstract>& selected : target->get_addressables()) {
      auto selected_field =
          select_accessible_field(selected.get(), access_scope);
      if (!selected_field) {
        continue;
      }
      if (input >= inputs.get_size()) {
        break;
      }
      fitted_fields.insert(*selected_field);
      input++;
    }
  }

  Ttx::Model::Layouts::Fluid target_layout(fitted_fields.get_view());
  Bool fitted = arguments.fits(target_layout);
  if (!fitted) {
    source.report(
        get_anchor(),
        "Object initializer inputs do not fit the initialization Layout."_view,
        "Use unique accessible Fields with values accepted by their Types."_view);
    return False;
  }

  if (has_mandatory_cycle(access_scope, *target, {})) {
    source.report(
        get_anchor(),
        "Object initializer contains a mandatory initialization cycle."_view,
        "Break the cycle with one terminating authored value."_view);
    return False;
  }

  Memory::Managed::Vector<Reference<Model::Pack>> values(domain);
  values.reset(target->get_layout().get_size());
  for (const Reference<Abstract>& selected : target->get_addressables()) {
    auto selected_field = selected.get().select<Field>();
    if (!selected_field ||
        selected_field->get_writability() != Writability::Internal) {
      continue;
    }
    const Field& field = *selected_field;

    auto supplied = select_supplied(access_scope, *target, field);
    if (supplied) {
      values.insert(const_cast<Model::Pack&>(*supplied));
      continue;
    }

    auto authored = field.get_initializer();
    if (authored) {
      values.insert(const_cast<Model::Pack&>(*authored));
      continue;
    }

    auto fallback = Library::Dialect::create_default(domain, field.get_type());
    if (!fallback) {
      source.report(
          get_anchor(),
          "Object initializer cannot complete one omitted state Field."_view,
          "Break a mandatory default cycle or supply an exact Field value."_view);
      return False;
    }
    values.insert(*fallback);
  }

  auto& completed =
      domain.construct<InitializerValues>(domain, values.get_view());
  BAIL_IF(!completed.link(source, lexical_context, access_scope));
  if (!completed.fits_into(*target)) {
    source.report(
        get_anchor(),
        "Object initializer completed values do not fit the Object Layout."_view,
        "Keep one exact value for every state Field in authored order."_view);
    return False;
  }

  expected_type = Reference<const Type>(*target);
  completed_values = Reference<Model::Pack>(completed);
  return True;
}
