// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/initializer.hpp"

#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto expression_fits(
    const Abstract& target,
    const Language::Expression& expression) -> Bool {
  const Abstract& selected = target.visit<Addressable>(
      [](const Addressable& addressable) -> const Abstract& {
        return addressable.get_type().resolve();
      },
      [](const Abstract& abstract) -> const Abstract& {
        return abstract.resolve();
      });
  return selected.visit<Type>(
      [&expression](const Type& type) { return expression.fits(type); },
      [](const Abstract&) { return False; });
}

auto Language::Initializer::is_next(const Ttx::Lexical::Cursor& cursor)
    -> Bool {
  return cursor.matches(Ttx::Lexical::Code::Type::Addressable) &&
         cursor.current().caculate_text(cursor.get_source_text()) == "new"_view;
}

auto Language::Initializer::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Ttx::Lexical::Cursor& cursor,
    const Abstract& source_context) -> Core::Option<Initializer&> {
  auto transaction = cursor.branch();
  BAIL_IF(!is_next(transaction));

  Ttx::Lexical::Token opening = transaction.consume();
  Memory::Managed::Vector<Reference<Expression>> inputs(domain);
  Memory::Managed::Vector<Core::View::Bytes> names(domain);
  Bool named = False;
  Ttx::Lexical::Token closing = opening;

  if (transaction.matches(Ttx::Lexical::Code::Type::PackingStart)) {
    transaction.consume();
    if (!transaction.matches(Ttx::Lexical::Code::Type::PackingEnd)) {
      // The first input selects one complete positional or named pack. The
      // private Cursor branch publishes nothing until every input succeeds.
      named = transaction.matches(Ttx::Lexical::Code::Type::AddressOp);
      while (True) {
        if (named) {
          BAIL_IF(!transaction.require(
              Ttx::Lexical::Code::Type::AddressOp,
              "Named Object initializer inputs require `.`."_view));

          Ttx::Lexical::Token name = transaction.current();
          if (name.get_code() != Ttx::Lexical::Code::Type::Addressable &&
              name.get_code() != Ttx::Lexical::Code::Type::Numeric &&
              name.get_code() != Ttx::Lexical::Code::Type::Hex) {
            transaction.create_token_error(
                "Named Object initializer inputs require one name."_view);
            return {};
          }
          transaction.consume();
          names.insert(name.caculate_text(transaction.get_source_text()));

          BAIL_IF(!transaction.require(
              Ttx::Lexical::Code::Type::Assign,
              "Named Object initializer inputs require `=`."_view));
        }

        auto expression = Parser::Expression::parse(
            domain, materializations, transaction, source_context);
        BAIL_IF(!expression);
        inputs.insert(*expression);

        if (!transaction.matches(Ttx::Lexical::Code::Type::PackingOp)) {
          break;
        }
        transaction.consume();
        if (transaction.matches(Ttx::Lexical::Code::Type::PackingEnd)) {
          break;
        }
      }
    }

    closing = transaction.require(
        Ttx::Lexical::Code::Type::PackingEnd,
        "Object initializer inputs require one closing `)`."_view);
    BAIL_IF(!closing);
  }

  Initializer& initializer = create_authored(
      domain, materializations, inputs.get_view(), names.get_view(), named,
      Ttx::Lexical::Anchor::create(
          opening, Ttx::Lexical::Span(opening, closing)));
  cursor.join(transaction);
  return initializer;
}

Language::Initializer::Initializer(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Core::View::Vector<Reference<Expression>> authored_inputs,
    Core::View::Vector<Core::View::Bytes> names,
    Bool named,
    Core::Option<Ttx::Lexical::Anchor> anchor)
    : Expression(anchor),
      domain(domain),
      materializations(materializations),
      inputs(domain),
      observations(domain),
      named(named),
      input_layout(inputs, observations, named) {
  inputs.reset(authored_inputs.get_size());
  observations.reset(authored_inputs.get_size());
  for (Count i = 0; i < authored_inputs.get_size(); i++) {
    Expression& input = authored_inputs.get_data()[i].get();
    inputs.insert(input);
    if (named) {
      observations.insert(domain.construct<Alias>(names[i], input));
    } else {
      observations.insert(input);
    }
  }
}

auto Language::Initializer::create_authored(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Core::View::Vector<Reference<Expression>> inputs,
    Core::View::Vector<Core::View::Bytes> names,
    Bool named,
    Ttx::Lexical::Anchor anchor) -> Initializer& {
  return Expression::create_authored<Initializer>(
      domain, anchor,
      [&](Core::Option<Ttx::Lexical::Anchor> source) -> Initializer {
        return Initializer(
            domain, materializations, inputs, names, named, source);
      });
}

constexpr auto Language::Initializer::InputLayout::get_abstract(
    Count index) const -> Core::Option<const Abstract&> {
  BAIL_IF(index >= observations.get_size());

  return observations.at(index).get();
}

auto Language::Initializer::InputLayout::fits_at(
    const Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));

  for (Count input_index = 0; input_index < get_size(); input_index++) {
    Count matched_index = input_index;
    if (named) {
      Core::View::Bytes input_name =
          observations.at(input_index).get().get_name();
      BAIL_IF(input_name.is_empty());

      Count matches = 0;
      for (Count other = 0; other < get_size(); other++) {
        if (observations.at(other).get().get_name() == input_name) {
          matches++;
        }
      }
      BAIL_IF(matches != 1);

      matches = 0;
      for (Count target_index = 0; target_index < get_size(); target_index++) {
        auto candidate = target.get_abstract(target_offset + target_index);
        if (candidate && candidate->get_name() == input_name) {
          matched_index = target_index;
          matches++;
        }
      }
      BAIL_IF(matches != 1);
    }

    Bool fits = target.get_abstract(target_offset + matched_index)
                    .visit(
                        []() { return False; },
                        [&](const Abstract& selected) {
                          return expression_fits(
                              selected, inputs.at(input_index).get());
                        });
    BAIL_IF(!fits);
  }

  return True;
}

auto Language::Initializer::InputLayout::get_fitted_at(
    const Layout& target,
    Count target_offset,
    Count target_index) const -> Utility::Result<const Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }

  if (!named) {
    return observations.at(target_index).get();
  }

  auto target_edge = target.get_abstract(target_offset + target_index);
  if (!target_edge) {
    return Errors::IncompatibleFit;
  }

  Core::View::Bytes target_name = target_edge->get_name();
  for (Count input_index = 0; input_index < get_size(); input_index++) {
    const Abstract& observation = observations.at(input_index).get();
    if (observation.get_name() == target_name) {
      return observation;
    }
  }

  return Errors::IncompatibleFit;
}

auto Language::Initializer::get_type() const -> const Abstract& {
  return expected_type.visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Reference<const Types::Object>& selected) -> const Abstract& {
        return selected.get();
      });
}

auto Language::Initializer::supplies(
    const Field& receiver,
    const Types::Object& target,
    const Field& field) const -> Bool {
  if (named) {
    for (Count i = 0; i < observations.get_size(); i++) {
      if (observations.at(i).get().get_name() == field.get_name()) {
        return True;
      }
    }
    return False;
  }

  auto accessible = &receiver.get_host() == &target
                        ? target.get_fields()
                        : target.get_public_fields();
  for (Count i = 0; i < inputs.get_size() && i < accessible.get_size(); i++) {
    if (&accessible.get_data()[i].get() == &field) {
      return True;
    }
  }
  return False;
}

auto Language::Initializer::has_mandatory_cycle(
    const Field& receiver,
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
  // by a supplied value.
  auto fields = target.get_fields();
  for (Count i = 0; i < fields.get_size(); i++) {
    const Field& field = fields.get_data()[i].get();
    if (supplies(receiver, target, field)) {
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
            field, *nested_target, next_path.get_view())) {
      return True;
    }
  }

  return False;
}

auto Language::Initializer::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& context,
    Materializations& materializations) -> Bool {
  auto receiver = context.select<Field>();
  if (!receiver || &materializations != &this->materializations) {
    source.report(
        get_anchor(),
        "Object initializer requires its receiving Field transaction."_view,
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

  // Arguments link in source order before fitting observes any result Type.
  // A failed child therefore leaves this Initializer without a published
  // Object edge.
  Bool failed = False;
  for (Count i = 0; i < inputs.get_size(); i++) {
    failed |= !inputs[i].get().link(source, context, materializations);
  }
  BAIL_IF(failed);

  auto accessible = &receiver->get_host() == &*target
                        ? target->get_fields()
                        : target->get_public_fields();
  Memory::Managed::Vector<Reference<const Abstract>> fitted_fields(domain);
  fitted_fields.reset(inputs.get_size());

  // The receiving host selects the real accessible Field range. Named inputs
  // preserve their source order while the fitted target follows authored Field
  // order. Positional inputs select the same accessible range directly.
  if (named) {
    for (Count field_index = 0; field_index < accessible.get_size();
         field_index++) {
      const Field& field = accessible.get_data()[field_index].get();
      for (Count input_index = 0; input_index < observations.get_size();
           input_index++) {
        if (observations[input_index].get().get_name() == field.get_name()) {
          fitted_fields.insert(field);
        }
      }
    }
  } else {
    for (Count i = 0; i < inputs.get_size() && i < accessible.get_size(); i++) {
      fitted_fields.insert(accessible.get_data()[i].get());
    }
  }

  Bool fitted = False;
  if (named) {
    Layouts::Named target_layout(fitted_fields.get_view());
    fitted = input_layout.fits(target_layout);
  } else {
    Layouts::Fluid target_layout(fitted_fields.get_view());
    fitted = input_layout.fits(target_layout);
  }
  if (!fitted) {
    source.report(
        get_anchor(),
        "Object initializer inputs do not fit the initialization Layout."_view,
        "Use unique accessible Fields with values accepted by their Types."_view);
    return False;
  }

  auto fields = target->get_fields();
  // Every omitted Field must contribute its own authored initializer. The
  // transaction checks the complete Object before retaining its expected Type.
  for (Count i = 0; i < fields.get_size(); i++) {
    const Field& field = fields.get_data()[i].get();
    if (!supplies(*receiver, *target, field) && !field.get_initializer()) {
      source.report(
          get_anchor(), "Object initializer omits one required Field."_view,
          "Supply every Field that has no authored initializer."_view);
      failed = True;
    }
  }
  BAIL_IF(failed);

  if (has_mandatory_cycle(*receiver, *target, {})) {
    source.report(
        get_anchor(),
        "Object initializer contains a mandatory initialization cycle."_view,
        "Break the cycle with one terminating authored value."_view);
    return False;
  }

  expected_type = Reference<const Types::Object>(*target);
  return True;
}
