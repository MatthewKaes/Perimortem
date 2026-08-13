// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/expressions/initializer.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "tetrodotoxin/library/language/model/parser/pack.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/fluid.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_accessible_field(
    const Abstract& candidate,
    Core::Option<const Type&> access_scope)
    -> Core::Option<const Language::Field&> {
  auto field = candidate.select<Language::Field>();
  BAIL_IF(
      !field || field->get_writability() == Language::Writability::Constant ||
      !Language::Access::Address::is_accessible(*field, access_scope));

  // Object construction consumes the target's real authored Field order, but
  // shares Address's one publication and host authority decision. A const Field
  // always uses its one declaration owned initializer and therefore never
  // enters the supplied construction Layout.
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
  Language::Model::Pack* arguments = nullptr;
  Ttx::Lexical::Token closing = opening;
  if (transaction.matches(Ttx::Lexical::Code::Type::PackingStart)) {
    auto parsed =
        Language::Model::Parser::Pack::parse(domain, source, transaction, True);
    BAIL_IF(!parsed);
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
        return Initializer(domain, *arguments, source);
      });
  cursor.join(transaction);
  return initializer;
}

Language::Expressions::Initializer::Initializer(
    Memory::Allocator::Arena& domain,
    Language::Model::Pack& arguments,
    Core::Option<Ttx::Lexical::Anchor> anchor)
    : Expression(anchor), domain(domain), arguments(arguments) {}

auto Language::Expressions::Initializer::get_type() const -> const Abstract& {
  return expected_type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Types::Object>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Language::Expressions::Initializer::finalize() -> void {
  // The initializer owns the complete argument flow. Finalize its real Pack in
  // source order before folding the initializer node itself. No second
  // expression inventory exists beside the Pack's canonical Layout.
  arguments.finalize();
  Expression::finalize();
}

auto Language::Expressions::Initializer::supplies(
    Core::Option<const Type&> access_scope,
    const Types::Object& target,
    const Field& field) const -> Bool {
  const Layout& inputs = arguments.get_layout();
  if (inputs.get_name(0)) {
    for (Count i = 0; i < inputs.get_size(); i++) {
      auto name = inputs.get_name(i);
      if (name && *name == field.get_name()) {
        return True;
      }
    }
    return False;
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
      return True;
    }
    input++;
  }
  return False;
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
    if (!selected_field) {
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
  auto receiver = lexical_context.select<Addressable>();
  if (!receiver) {
    source.report(
        get_anchor(),
        "Object initializer requires its receiving Addressable transaction."_view,
        "Retain `new` only on one typed Library declaration."_view);
    return False;
  }

  auto target = receiver->get_type().resolve().select<Types::Object>();
  if (!target) {
    source.report(
        get_anchor(), "Object initializer requires one exact Object Type."_view,
        "Name an Object Type on the declaration that receives `new`."_view);
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

  // Every omitted Field must contribute its own authored initializer. The
  // transaction checks the complete Object before retaining its expected Type.
  Bool failed = False;
  for (const Reference<Abstract>& selected : target->get_addressables()) {
    auto selected_field = selected.get().select<Field>();
    if (!selected_field) {
      continue;
    }
    const Field& field = *selected_field;
    if (!supplies(access_scope, *target, field) && !field.get_initializer()) {
      source.report(
          get_anchor(), "Object initializer omits one required Field."_view,
          "Supply every Field that has no authored initializer."_view);
      failed = True;
    }
  }
  BAIL_IF(failed);

  if (has_mandatory_cycle(access_scope, *target, {})) {
    source.report(
        get_anchor(),
        "Object initializer contains a mandatory initialization cycle."_view,
        "Break the cycle with one terminating authored value."_view);
    return False;
  }

  expected_type = Reference<const Types::Object>(*target);
  return True;
}
