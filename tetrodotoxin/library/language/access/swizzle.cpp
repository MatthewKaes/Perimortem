// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/swizzle.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/type.hpp"

using namespace Perimortem;
using namespace Tetrodotoxin::Library;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;

static auto select_type(const Abstract& output) -> Core::Option<const Type&> {
  auto direct = output.select<Type>();
  if (direct) {
    return direct;
  }

  return output.resolve().select<Type>();
}

static auto select_addressable(const Layout& layout, Core::View::Bytes name)
    -> Core::Option<const Addressable&> {
  Core::Option<const Abstract&> selected;
  for (Count index = 0; index < layout.get_size(); index++) {
    auto entry = layout.get_abstract(index);
    if (!entry || entry->get_name() != name) {
      continue;
    }

    BAIL_IF(selected);
    selected = *entry;
  }

  BAIL_IF(!selected || name.is_empty());
  auto direct = selected->select<Addressable>();
  return direct ? direct : selected->resolve().select<Addressable>();
}

auto Language::Access::Swizzle::parse(
    Memory::Allocator::Arena& domain,
    Materializations&,
    Cursor& cursor,
    const Abstract&,
    Expression& receiver) -> Core::Option<Expression&> {
  auto transaction = cursor.branch();
  Token opening = transaction.consume();
  Memory::Managed::Vector<Token> tokens(domain);
  Memory::Managed::Vector<Core::View::Bytes> names(domain);

  // Tokens retain the exact authored selections while stable spellings support
  // lookup after the parsing Cursor and its source view leave this transaction.
  while (!transaction.matches(Code::Type::BracketEnd)) {
    Token name = transaction.require(
        Code::Type::Addressable,
        "Swizzle requires an addressable name or one closing bracket."_view);
    BAIL_IF(!name);

    tokens.insert(name);
    names.insert(
        domain.proxy(name.caculate_text(transaction.get_source_text())));
    if (!transaction.matches(Code::Type::PackingOp)) {
      break;
    }

    transaction.consume();
    if (transaction.matches(Code::Type::BracketEnd)) {
      break;
    }
  }

  Token closing = transaction.require(
      Code::Type::BracketEnd,
      "Swizzle requires one closing bracket after its selected names."_view);
  BAIL_IF(!closing);

  const auto& receiver_anchor = receiver.get_anchor();
  if (!receiver_anchor) {
    transaction.create_expression_error(
        Span(opening, closing),
        "Swizzle requires one authored receiver Anchor."_view);
    return {};
  }

  Anchor anchor =
      Anchor::create(opening, receiver_anchor->get_span(), Span(closing));
  Swizzle& swizzle = Expression::create_authored<Swizzle>(
      domain, anchor, [&](auto source) -> Swizzle {
        return Swizzle(
            domain, receiver, tokens.get_view(), names.get_view(), source);
      });
  cursor.join(transaction);
  return swizzle;
}

auto Language::Access::Swizzle::link(
    Tetrodotoxin::Language::Monograph& source,
    const Abstract& lexical_context,
    Materializations& materializations,
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  BAIL_IF(
      !receiver.link(source, lexical_context, materializations, access_scope));

  auto receiver_type = select_type(receiver.get_type());
  if (!receiver_type) {
    source.report(
        get_anchor(), "Swizzle receiver did not produce one Type."_view,
        "Select names only from a value with a complete Layout."_view);
    return False;
  }

  Memory::Managed::Vector<Reference<const Abstract>> candidates(
      selected.get_arena());
  // The receiver Type real Layout owns the candidate set. Caller scope only
  // filters authority and never supplies another receiver or implicit lookup.
  for (Core::View::Bytes name : names) {
    auto candidate = select_addressable(receiver_type->get_layout(), name);
    if (!candidate || !Address::is_accessible(*candidate, access_scope)) {
      source.report(
          get_anchor(),
          "Swizzle did not find one readable Addressable for every name."_view,
          "Select exact names admitted by the receiver Type Layout."_view);
      return False;
    }

    candidates.insert(Reference<const Abstract>(*candidate));
  }

  if (linked) {
    Bool changed = selected.get_size() != candidates.get_size();
    for (Count index = 0; !changed && index < selected.get_size(); index++) {
      changed = &selected.at(index).get() != &candidates.at(index).get();
    }
    if (changed) {
      source.report(
          get_anchor(),
          "Swizzle cannot change one of its selected Addressables."_view,
          "Keep each authored name bound to the same exact identity."_view);
      return False;
    }

    return True;
  }

  // Selection is fully proven before retained result state changes. The Fluid
  // Layout then borrows the exact Addressables without creating an aggregate
  // Type, copied member facts, or a second registration surface.
  selected.reset(candidates.get_size());
  for (const Reference<const Abstract>& candidate : candidates.get_view()) {
    selected.insert(candidate);
  }
  results = Ttx::Model::Layouts::Fluid(selected.get_view());
  linked = True;

  // Swizzle completion is its real Layout rather than a scalar Type. The
  // single selection query below remains available without making empty or
  // multiple selections fail semantic linking.
  return True;
}

auto Language::Access::Swizzle::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Access::Swizzle::get_type() const -> const Abstract& {
  if (!linked || selected.get_size() != 1) {
    return Invalid::get_invalid();
  }

  return selected.at(0).get().visit<Addressable>(
      [](const Addressable& addressable) -> const Abstract& {
        return addressable.get_type();
      },
      [](const Abstract&) -> const Abstract& {
        return Invalid::get_invalid();
      });
}

auto Language::Access::Swizzle::get_inputs() const -> const Layout& {
  return inputs;
}

auto Language::Access::Swizzle::fits(const Ttx::Model::Type& target) const
    -> Bool {
  if (!linked) {
    return False;
  }

  if (selected.get_size() == 1 && Expression::fits(target)) {
    return True;
  }

  return results.fits(target.get_layout());
}
