// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/access/swizzle.hpp"

#include "tetrodotoxin/library/language/access/address.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/layouts/fluid.hpp"
#include "ttx/model/layouts/named.hpp"
#include "ttx/model/layouts/value.hpp"
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

static auto is_named(const Layout& layout) -> Bool {
  BAIL_IF(layout.is_empty());
  for (Count index = 0; index < layout.get_size(); index++) {
    auto name = layout.get_name(index);
    BAIL_IF(!name || name->is_empty());
  }
  return True;
}

static auto select_named(const Layout& layout, Core::View::Bytes name)
    -> Core::Option<Count> {
  BAIL_IF(name.is_empty());

  Core::Option<Count> selected;
  for (Count index = 0; index < layout.get_size(); index++) {
    auto slot_name = layout.get_name(index);
    if (!slot_name || *slot_name != name) {
      continue;
    }

    BAIL_IF(selected);
    selected = index;
  }
  return selected;
}

// Direct Pack selection retains exactly two facts: the real receiver producer
// and the selected source indices. The receiver continues to own value
// identity and slot-local descriptor fitting. This identity-free Layout only
// publishes the selected order and returns the original producer on reflection.
static auto create_layout(
    Memory::Allocator::Arena& domain,
    const Language::Model::Pack& receiver,
    Core::View::Vector<Count> selections) -> const Ttx::Concept::Layout& {
  class Layout final : public Ttx::Concept::Layout {
   public:
    constexpr Layout(
        const Language::Model::Pack& receiver,
        Core::View::Vector<Count> selections)
        : receiver(receiver), selections(selections) {}

    constexpr auto get_size() const -> Count override {
      return selections.get_size();
    }

    constexpr auto get_abstract(Count index) const
        -> Core::Option<const Abstract&> override {
      BAIL_IF(index >= get_size());
      return receiver.get_layout().get_abstract(selections[index]);
    }

    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override {
      BAIL_IF(source_index >= get_size() || target_index >= target.get_size());
      Count selected = selections[source_index];
      auto source_name = receiver.get_layout().get_name(selected);
      auto target_entry = target.get_abstract(target_index);
      BAIL_IF(!source_name || !target_entry);

      // Selection makes output positional, so lend the original source name to
      // this one real target entry only while its descriptor is checked. These
      // standard identity-free Layout values create no producer or retained
      // mapping beside the selected source index.
      Core::View::Bytes slot_names[] = {*source_name};
      Ttx::Model::Layouts::Value target_value(*target_entry);
      Ttx::Model::Layouts::Named target_slot(target_value, slot_names);
      return receiver.fits_entry(target_slot, selected, 0);
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
      return get_abstract(target_index)
          .visit(
              []() -> Utility::Result<const Abstract&, Errors> {
                return Errors::IncompatibleFit;
              },
              [](const Abstract& producer)
                  -> Utility::Result<const Abstract&, Errors> {
                return producer;
              });
    }

   private:
    const Language::Model::Pack& receiver;
    Core::View::Vector<Count> selections;
  };

  return domain.construct<Layout>(receiver, selections);
}

auto Language::Access::Swizzle::parse(
    Memory::Allocator::Arena& domain,
    Language::Monograph&,
    Cursor& cursor,
    Language::Model::Pack& receiver,
    Span receiver_span) -> Core::Option<Expression&> {
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

  Anchor anchor = Anchor::create(opening, receiver_span, Span(closing));
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
    Core::Option<const Ttx::Model::Type&> access_scope) -> Bool {
  BAIL_IF(!receiver.link(source, lexical_context, access_scope));

  const Ttx::Concept::Layout& receiver_layout = receiver.get_layout();
  Bool direct_selection = names.is_empty() || is_named(receiver_layout);
  Memory::Managed::Vector<Count> selected_indices(domain);
  Memory::Managed::Vector<Reference<const Abstract>> candidates(domain);

  if (direct_selection) {
    // A named Pack's Layout carries its authored slot names independently from
    // producer identity. The source indices must survive selection because a
    // multi-result producer such as Call can occupy several differently typed
    // slots while remaining one exact Abstract.
    for (Core::View::Bytes name : names) {
      auto selected = select_named(receiver_layout, name);
      if (!selected) {
        source.report(
            get_anchor(),
            "Swizzle did not find one producer for every named Pack slot."_view,
            "Select exact names published by the receiver Pack Layout."_view);
        return False;
      }
      selected_indices.insert(*selected);
    }
  } else {
    auto receiver_expression = receiver.select<Expression>();
    auto receiver_type = receiver_expression.visit(
        []() -> Core::Option<const Ttx::Model::Type&> { return {}; },
        [](Expression& expression) {
          return select_type(expression.get_type());
        });
    if (!receiver_expression || !receiver_type) {
      source.report(
          get_anchor(),
          "Swizzle receiver did not expose a named Pack or one Type."_view,
          "Select names from named Pack flow or a typed scalar value."_view);
      return False;
    }

    // The receiver Type's real Layout owns the candidate set. Caller scope
    // only filters authority and never supplies another receiver or lookup.
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
  }

  if (output) {
    Bool changed = False;
    if (direct_selection) {
      changed |= selections.get_size() != selected_indices.get_size();
      for (Count index = 0; !changed && index < selections.get_size();
           index++) {
        changed = selections.at(index) != selected_indices.at(index);
      }
    } else {
      changed |= projections.get_size() != candidates.get_size();
      for (Count index = 0; !changed && index < projections.get_size();
           index++) {
        const Abstract& projection = projections.at(index).get();
        const Abstract& candidate = candidates.at(index).get();
        auto expression = projection.select<Expression>();
        changed = !expression || &expression->get_result() != &candidate;
      }
    }
    if (changed) {
      source.report(
          get_anchor(),
          "Swizzle cannot change one of its selected values."_view,
          "Keep each authored name bound to the same source slot and exact "
          "producer."_view);
      return False;
    }

    return True;
  }

  Memory::Managed::Vector<Reference<const Abstract>> created(domain);
  created.reset(candidates.get_size());
  if (direct_selection) {
    selections.reset(selected_indices.get_size());
    for (Count selected : selected_indices.get_view()) {
      selections.insert(selected);
    }
    output = create_layout(domain, receiver, selections.get_view());
  } else {
    // Type-backed selection must evaluate a member relative to its scalar
    // receiver. These Address Expressions are the selected value producers,
    // not copied Field identities or an aggregate result carrier.
    Expression& receiver_expression = *receiver.select<Expression>();
    for (const Reference<const Abstract>& candidate : candidates.get_view()) {
      const Addressable& addressable =
          static_cast<const Addressable&>(candidate.get());
      Address& projection =
          Address::create_synthetic(domain, receiver_expression, addressable);
      BAIL_IF(!projection.link(source, lexical_context, access_scope));
      created.insert(projection);
    }

    projections.reset(created.get_size());
    for (Reference<const Abstract> projection : created.get_view()) {
      projections.insert(projection);
    }
    output =
        domain.construct<Ttx::Model::Layouts::Fluid>(projections.get_view());
  }
  return True;
}

auto Language::Access::Swizzle::get_documentation() const
    -> const Documentation& {
  return Documentation::get_empty();
}

auto Language::Access::Swizzle::get_type() const -> const Abstract& {
  if (!output || output->get_size() != 1) {
    return Invalid::get_invalid();
  }

  return output->get_abstract(0).visit(
      []() -> const Abstract& { return Invalid::get_invalid(); },
      [](const Abstract& output) -> const Abstract& {
        return output.visit<Language::Model::Pack>(
            [](const Language::Model::Pack& producer) -> const Abstract& {
              return producer.get_type();
            },
            [](const Abstract&) -> const Abstract& {
              return Invalid::get_invalid();
            });
      });
}

auto Language::Access::Swizzle::get_layout() const
    -> const Ttx::Concept::Layout& {
  return *output;
}

auto Language::Access::Swizzle::resolve() const -> const Abstract& {
  if (!output) {
    return Invalid::get_invalid();
  }

  return static_cast<const Ttx::Model::Pack&>(*this);
}

auto Language::Access::Swizzle::finalize() -> void {
  // Swizzle evaluates its receiver once. Type-backed projection Expressions
  // describe selected members; they are not another evaluation inventory.
  receiver.finalize();
  Expression::finalize();
}
