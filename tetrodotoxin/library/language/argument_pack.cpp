// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/argument_pack.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/language/parser/expression.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/alias.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto expression_fits(
    const Abstract& target,
    const Language::Expression& expression) -> Bool {
  const Abstract& selected = target.visit<Ttx::Model::Alias>(
      [](const Ttx::Model::Alias& alias) -> const Abstract& {
        return alias.resolve();
      },
      [](const Abstract& direct) -> const Abstract& { return direct; });
  const Abstract& target_type = selected.visit<Addressable>(
      [](const Addressable& addressable) -> const Abstract& {
        return addressable.get_type();
      },
      [](const Abstract& abstract) -> const Abstract& { return abstract; });
  auto direct = target_type.select<Type>();
  if (direct) {
    return expression.fits(*direct);
  }

  return target_type.resolve().visit<Type>(
      [&expression](const Type& type) { return expression.fits(type); },
      [](const Abstract&) { return False; });
}

Language::ArgumentPack::ArgumentPack(
    Core::View::Bytes source,
    Core::View::Vector<Reference<Expression>> authored_expressions,
    Core::View::Vector<Token> authored_labels,
    Bool named,
    Anchor anchor)
    : source(source),
      expressions(authored_expressions),
      labels(authored_labels),
      named(named),
      anchor(anchor) {}

auto Language::ArgumentPack::parse(
    Memory::Allocator::Arena& domain,
    Materializations& materializations,
    Cursor& cursor,
    const Abstract& source_context) -> Core::Option<ArgumentPack&> {
  auto transaction = cursor.branch();
  Token opening = transaction.require(
      Code::Type::PackingStart,
      "Library argument pack requires an opening `(`."_view);
  BAIL_IF(!opening);

  Memory::Managed::Vector<Reference<Expression>> expressions(domain);
  Memory::Managed::Vector<Token> labels(domain);
  Bool named = False;
  if (!transaction.matches(Code::Type::PackingEnd)) {
    // The first entry fixes one complete positional or named shape. Later
    // entries preserve that shape so punctuation never becomes value flow.
    named = transaction.matches(Code::Type::AddressOp);
    while (True) {
      if (named) {
        BAIL_IF(!transaction.require(
            Code::Type::AddressOp,
            "Named Library arguments require `.` before every name."_view));

        Token label = transaction.current();
        if (label.get_code() != Code::Type::Addressable &&
            label.get_code() != Code::Type::Numeric &&
            label.get_code() != Code::Type::Hex) {
          transaction.create_token_error(
              "Named Library arguments require one addressable or indexed "
              "name."_view);
          return {};
        }
        transaction.consume();
        labels.insert(label);

        BAIL_IF(!transaction.require(
            Code::Type::Assign,
            "Named Library arguments require `=` before their value."_view));
      } else if (transaction.matches(Code::Type::AddressOp)) {
        transaction.create_token_error(
            "Positional and named Library arguments cannot share one pack."_view);
        return {};
      }

      auto argument = Parser::Expression::parse(
          domain, materializations, transaction, source_context);
      BAIL_IF(!argument);
      expressions.insert(*argument);

      if (!transaction.matches(Code::Type::PackingOp)) {
        break;
      }
      transaction.consume();
      if (transaction.matches(Code::Type::PackingEnd)) {
        break;
      }
    }
  }

  Token closing = transaction.require(
      Code::Type::PackingEnd,
      "Library argument pack requires one closing `)`."_view);
  BAIL_IF(!closing);

  // The caller Arena contract keeps parser inputs alive for the graph lifetime.
  // Retaining that stable view lets exact Tokens produce labels without
  // copying names or manufacturing semantic objects for lexical spelling.
  Core::View::Bytes source = transaction.get_source_text();
  Anchor anchor = Anchor::create(opening, Span(opening, closing));
  ArgumentPack& arguments = domain.construct_from<ArgumentPack>([&]() {
    return ArgumentPack(
        source, expressions.get_view(), labels.get_view(), named, anchor);
  });
  cursor.join(transaction);
  return arguments;
}

auto Language::ArgumentPack::create_empty(
    Memory::Allocator::Arena& domain,
    Anchor anchor) -> ArgumentPack& {
  return domain.construct_from<ArgumentPack>(
      [&]() { return ArgumentPack({}, {}, {}, False, anchor); });
}

constexpr auto Language::ArgumentPack::get_abstract(Count index) const
    -> Core::Option<const Abstract&> {
  BAIL_IF(index >= expressions.get_size());

  return expressions.get_data()[index].get();
}

auto Language::ArgumentPack::has_unique_labels() const -> Bool {
  BAIL_IF(!named || labels.get_size() != expressions.get_size());

  for (Count index = 0; index < labels.get_size(); index++) {
    Core::View::Bytes label = get_label_spelling(index);
    BAIL_IF(label.is_empty());

    for (Count other = index + 1; other < labels.get_size(); other++) {
      BAIL_IF(label == get_label_spelling(other));
    }
  }

  return True;
}

auto Language::ArgumentPack::fits_at(const Layout& target, Count target_offset)
    const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));
  BAIL_IF(named && !has_unique_labels());

  for (Count expression_index = 0; expression_index < get_size();
       expression_index++) {
    Count matched_index = expression_index;
    if (named) {
      Core::View::Bytes label = get_label_spelling(expression_index);
      Count matches = 0;
      for (Count target_index = 0; target_index < get_size(); target_index++) {
        auto candidate = target.get_abstract(target_offset + target_index);
        if (candidate && candidate->get_name() == label) {
          matched_index = target_index;
          matches++;
        }
      }
      BAIL_IF(matches != 1);
    }

    Bool fits =
        target.get_abstract(target_offset + matched_index)
            .visit(
                []() { return False; },
                [&](const Abstract& selected) {
                  return expression_fits(
                      selected, expressions.get_data()[expression_index].get());
                });
    BAIL_IF(!fits);
  }

  return True;
}

auto Language::ArgumentPack::get_fitted_at(
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
    return expressions.get_data()[target_index].get();
  }

  auto target_edge = target.get_abstract(target_offset + target_index);
  if (!target_edge) {
    return Errors::IncompatibleFit;
  }

  Core::View::Bytes target_name = target_edge->get_name();
  for (Count expression_index = 0; expression_index < get_size();
       expression_index++) {
    if (get_label_spelling(expression_index) == target_name) {
      return expressions.get_data()[expression_index].get();
    }
  }

  return Errors::IncompatibleFit;
}

auto Language::ArgumentPack::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Materializations& materializations,
    Core::Option<const Type&> access_scope) -> Bool {
  // Arguments link in authored order. Fitting starts only after every child
  // has had the opportunity to publish its exact result Type.
  Bool failed = False;
  for (Count i = 0; i < expressions.get_size(); i++) {
    failed |= !expressions.get_data()[i].get().link(
        source, lexical_context, materializations, access_scope);
  }

  return !failed;
}
