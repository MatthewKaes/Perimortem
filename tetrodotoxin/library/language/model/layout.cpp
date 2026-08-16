// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/layout.hpp"

#include "tetrodotoxin/library/language/model/parser/layout.hpp"
#include "tetrodotoxin/library/language/model/type.hpp"
#include "tetrodotoxin/library/language/parameter.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/model/addressable.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Ttx::Model;
using namespace Tetrodotoxin::Library;

static auto select_entry_type(const Abstract& entry) -> Option<const Type&> {
  auto type = entry.select<Type>();
  if (type) {
    return *type;
  }

  return entry.select<Addressable>().visit(
      []() -> Option<const Type&> { return {}; },
      [](const Addressable& addressable) -> Option<const Type&> {
        return addressable.get_type();
      });
}

static auto resolves_for_fitting(const Abstract& entry) -> const Abstract& {
  // Authored Layouts retain direct Type and Parameter edges before every Type
  // necessarily resolves. Those identities are already canonical. Resolving
  // first would collapse distinct staged Types to the shared Invalid object.
  if (entry.is<Type>()) {
    return entry;
  }

  auto direct_addressable = entry.select<Addressable>();
  if (direct_addressable) {
    return direct_addressable->get_type();
  }

  const Abstract& represented = entry.resolve();
  return represented.visit<Addressable>(
      [](const Addressable& addressable) -> const Abstract& {
        return addressable.get_type();
      },
      [](const Abstract& abstract) -> const Abstract& { return abstract; });
}

static auto get_slot_name(const Ttx::Concept::Layout& layout, Count index)
    -> Option<View::Bytes> {
  auto explicit_name = layout.get_name(index);
  if (explicit_name) {
    return explicit_name;
  }

  return layout.get_abstract(index).visit(
      []() -> Option<View::Bytes> { return {}; },
      [](const Abstract& selected) -> Option<View::Bytes> {
        View::Bytes name = selected.get_name();
        return name.is_empty() ? Option<View::Bytes>()
                               : Option<View::Bytes>(name);
      });
}

auto Language::Model::Layout::interpret_parameters(
    Cursor& cursor,
    const Abstract& host) -> Option<Layout&> {
  return interpret(cursor, host, True);
}

auto Language::Model::Layout::interpret(Cursor& cursor, const Abstract& host)
    -> Option<Layout&> {
  return interpret(cursor, host, False);
}

auto Language::Model::Layout::interpret(
    Cursor& cursor,
    const Abstract& host,
    Bool parameters) -> Option<Layout&> {
  // Parser::Layout owns the bracket and separator grammar. This owner retains
  // delayed Type routes so declaration order does not become a parse rule.
  Allocator::Arena& domain = cursor.get_arena();
  Token opening = cursor.current();
  Managed::Vector<Slot> slots(domain);
  auto closing = Parser::Layout::parse(
      cursor,
      [&](Cursor& entry, Count index, Option<Token> name_token) -> Bool {
        if (entry.matches(Code::Type::Self)) {
          if (!parameters || index != 0 || !name_token ||
              name_token->get_code() != Code::Type::Self) {
            entry.create_token_error(
                "Library `self` must be the first Function parameter."_view);
            return False;
          }

          Token self = entry.consume();
          slots.insert(Slot({}, Anchor::create(Span(self)), "self"_view));
          return True;
        }

        if (!entry.matches(Code::Type::Type)) {
          entry.create_token_error(
              "Library Layout entries require one Type reference."_view);
          return False;
        }

        Token slot_opening = name_token ? entry.peek(-3) : entry.current();
        auto type = TypeReference::parse(host, entry);
        BAIL_IF(!type);

        View::Bytes name;
        Anchor slot_anchor = type->get_anchor();
        if (name_token) {
          name = name_token->caculate_text(entry.get_source_text());
          slot_anchor = Anchor::create(
              *name_token,
              Span(slot_opening, type->get_anchor().get_span().get_end()));
        }

        slots.insert(Slot(*type, slot_anchor, name));
        return True;
      });
  BAIL_IF(!closing);

  if (parameters && !slots.is_empty() && slots.at(0).name.is_empty()) {
    cursor.create_expression_error(
        slots.at(0).anchor,
        "Library Function parameters require one Named Layout."_view,
        "Use `[]` for no parameters or name every entry as `.name : Type`."_view);
    return {};
  }

  Anchor anchor = Anchor::create(opening, Span(opening, *closing));
  Layout& layout = domain.construct_from<Layout>(
      [&]() -> Layout { return Layout(domain, slots, anchor); });
  return layout;
}

auto Language::Model::Layout::link_parameters(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& host) -> Bool {
  return link(cursor, host, True);
}

auto Language::Model::Layout::link_types(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& host) -> Bool {
  return link(cursor, host, False);
}

auto Language::Model::Layout::link(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& host,
    Bool parameters) -> Bool {
  // Linking settles every slot before exposing the Layout. Parameter Layouts
  // replace their authored slot with one real Parameter identity while result
  // Layouts retain the selected Type itself.
  Bool failed = False;
  for (Count i = 0; i < slots.get_size(); i++) {
    Slot& slot = slots[i];
    const Type* type = nullptr;
    if (!slot.type_reference) {
      // Self is derived from the exact host because its reserved spelling is a
      // receiver role rather than a route that another context may intercept.
      if (!parameters || i != 0 || slot.name != "self"_view) {
        cursor.create_expression_error(
            slot.anchor,
            "Only a leading Function parameter may derive its Type from "
            "`self`."_view,
            "Use an authored Type reference for every other Layout entry."_view);
        failed = True;
        continue;
      }
      auto host_type = host.select<Type>();
      if (!host_type) {
        cursor.create_expression_error(
            slot.anchor, "Library `self` requires one exact host Type."_view);
        failed = True;
        continue;
      }
      type = &*host_type;
    } else {
      auto selected = slot.type_reference->resolve_authored(cursor, host);
      if (!selected) {
        failed = True;
        continue;
      }
      auto selected_type = selected->select<Type>();
      if (!selected_type) {
        cursor.create_expression_error(
            slot.get_type_anchor(),
            "Library Layout Type route did not resolve to one stable Type."_view,
            "Publish the named Type in this logical context before linking."_view);
        failed = True;
        continue;
      }
      type = &*selected_type;
    }

    if (type->get_layout().is_empty()) {
      cursor.create_expression_error(
          slot.get_type_anchor(),
          parameters
              ? "Function parameter cannot bind an empty Type Layout."_view
              : "Function result cannot name an empty Type Layout."_view,
          parameters
              ? "Remove the parameter or use a Type with one value leaf."_view
              : "Write `[]` when the Function produces no values."_view);
      failed = True;
      continue;
    }

    if (!parameters) {
      // Repeated phase entry may observe the same identity but must never move
      // an already published slot to a newly selected Type.
      if (slot.edge) {
        if (&slot.edge->get() != type) {
          cursor.create_expression_error(
              slot.get_type_anchor(),
              "Repeated Layout linking selected a different Type identity."_view,
              "Preserve the original resolved Type edge across completion."_view);
          failed = True;
        }
      } else {
        slot.edge = Reference<const Abstract>(*type);
      }
      continue;
    }

    if (slot.edge) {
      auto parameter = slot.edge->get().select<Language::Parameter>();
      if (!parameter || &parameter->get_type() != type) {
        cursor.create_expression_error(
            slot.get_type_anchor(),
            "Repeated parameter linking selected a different semantic edge."_view,
            "Preserve the original Parameter and resolved Type identity."_view);
        failed = True;
      }
      continue;
    }

    auto parameter =
        Language::Parameter::create_authored(domain, slot.name, *type);
    if (!parameter) {
      cursor.create_expression_error(
          slot.anchor,
          "Function parameter could not retain its authored Layout entry."_view,
          "Use one named nonempty Type for each Function parameter."_view);
      failed = True;
      continue;
    }
    slot.edge = Reference<const Abstract>(*parameter);
  }

  return !failed && is_linked();
}

auto Language::Model::Layout::resolve_named(View::Bytes route) const
    -> const Abstract& {
  if (!is_linked()) {
    return Invalid::get_invalid();
  }

  for (Count i = 0; i < get_size(); i++) {
    auto name = get_name(i);
    auto entry = get_abstract(i);
    if (name && *name == route && entry) {
      return *entry;
    }
  }

  return Invalid::get_invalid();
}

auto Language::Model::Layout::validate_publication(
    Ttx::Lexical::Cursor& cursor,
    const Abstract& host) const -> Bool {
  auto context = host.select<Language::Model::Type>();
  if (!is_linked() || !context) {
    cursor.create_expression_error(
        anchor,
        "A published Function requires one linked Library Type context."_view,
        "Link every authored Layout entry on its exact Function host before "
        "publication."_view);
    return False;
  }

  Bool valid = True;
  for (Count i = 0; i < slots.get_size(); i++) {
    const Slot& slot = slots.at(i);
    auto type = select_entry_type(slot.edge->get());
    if (!type) {
      cursor.create_expression_error(
          slot.get_type_anchor(),
          "A published Function Layout retains an invalid semantic entry."_view,
          "Retain the exact Parameter or Type selected during linking."_view);
      valid = False;
      continue;
    }

    // Publication repeats the authored query through the host's public graph.
    // A private Type remains usable locally but cannot leak through a readable
    // Function signature merely because linking retained its identity.
    Bool reachable = slot.type_reference.visit(
        [&]() {
          return Bool(i == 0 && slot.name == "self"_view && &*type == &host);
        },
        [&](const TypeReference& reference) {
          const Abstract* selected = nullptr;
          reference.resolve(*context).visit(
              [&](const Abstract& resolved) { selected = &resolved; },
              [](const TypeReference::Failure&) {});
          return Bool(selected != nullptr && &selected->resolve() == &*type);
        });
    if (!reachable) {
      cursor.create_expression_error(
          slot.get_type_anchor(),
          "Externally readable Function publishes an unreachable Type "
          "route."_view,
          "Keep the Function private or publish its authored Type route."_view);
      valid = False;
    }
  }

  return valid;
}

auto Language::Model::Layout::declares_self() const -> Bool {
  if (slots.is_empty()) {
    return False;
  }

  const Slot& first = slots.at(0);
  return Bool(!first.type_reference && first.name == "self"_view);
}

auto Language::Model::Layout::is_linked() const -> Bool {
  for (Count i = 0; i < slots.get_size(); i++) {
    if (!slots.at(i).edge) {
      return False;
    }
  }
  return True;
}

auto Language::Model::Layout::is_named() const -> Bool {
  return slots.is_empty() || !slots.at(0).name.is_empty();
}

auto Language::Model::Layout::get_slot(Count index) const -> const Slot* {
  return index < slots.get_size() ? &slots.at(index) : nullptr;
}

auto Language::Model::Layout::get_size() const -> Count {
  return slots.get_size();
}

auto Language::Model::Layout::get_abstract(Count index) const
    -> Option<const Abstract&> {
  const Slot* slot = get_slot(index);
  BAIL_IF(slot == nullptr || !slot->edge);
  return slot->edge->get();
}

auto Language::Model::Layout::get_name(Count index) const
    -> Option<View::Bytes> {
  BAIL_IF(!is_named());

  const Slot* slot = get_slot(index);
  BAIL_IF(slot == nullptr || slot->name.is_empty());
  return slot->name;
}

auto Language::Model::Layout::fits_value(
    const Ttx::Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  auto source = get_abstract(source_index);
  auto destination = target.get_abstract(target_index);
  return Bool(
      source && destination &&
      &resolves_for_fitting(*source) == &resolves_for_fitting(*destination));
}

auto Language::Model::Layout::fits_entry(
    const Ttx::Concept::Layout& target,
    Count source_index,
    Count target_index) const -> Bool {
  BAIL_IF(source_index >= get_size() || target_index >= target.get_size());

  if (!is_named()) {
    return fits_value(target, source_index, target_index);
  }

  auto source_name = get_name(source_index);
  auto target_name = get_slot_name(target, target_index);
  return source_name && target_name && *source_name == *target_name &&
         fits_value(target, source_index, target_index);
}

auto Language::Model::Layout::has_unique_names() const -> Bool {
  for (Count i = 0; i < get_size(); i++) {
    auto name = get_name(i);
    BAIL_IF(!name);
    for (Count other = i + 1; other < get_size(); other++) {
      auto candidate = get_name(other);
      BAIL_IF(!candidate || *candidate == *name);
    }
  }
  return True;
}

auto Language::Model::Layout::fits_at(
    const Ttx::Concept::Layout& target,
    Count target_offset) const -> Bool {
  BAIL_IF(!has_target_segment(target, target_offset));

  if (!is_named()) {
    // Positional Layouts preserve authored order. Named Layouts instead match
    // within one equal sized target segment so names may reorder without
    // reaching outside the receiving declaration's boundary.
    for (Count i = 0; i < get_size(); i++) {
      BAIL_IF(!fits_value(target, i, target_offset + i));
    }
    return True;
  }

  BAIL_IF(!has_unique_names());
  for (Count source_index = 0; source_index < get_size(); source_index++) {
    auto source_name = get_name(source_index);
    BAIL_IF(!source_name);

    Count selected = 0;
    Count matches = 0;
    for (Count target_index = 0; target_index < get_size(); target_index++) {
      auto target_name = get_slot_name(target, target_offset + target_index);
      if (target_name && *target_name == *source_name) {
        selected = target_index;
        matches++;
      }
    }
    BAIL_IF(
        matches != 1 ||
        !fits_value(target, source_index, target_offset + selected));
  }

  return True;
}

auto Language::Model::Layout::get_fitted_at(
    const Ttx::Concept::Layout& target,
    Count target_offset,
    Count target_index) const -> Result<const Abstract&, Errors> {
  if (target_index >= get_size()) {
    return Errors::IndexOutOfBounds;
  }
  if (!has_target_segment(target, target_offset)) {
    return Errors::SizeMismatch;
  }
  if (!fits_at(target, target_offset)) {
    return Errors::IncompatibleFit;
  }

  if (!is_named()) {
    return get_abstract(target_index)
        .visit(
            []() -> Result<const Abstract&, Errors> {
              return Errors::IncompatibleFit;
            },
            [](const Abstract& selected) -> Result<const Abstract&, Errors> {
              return selected;
            });
  }

  auto target_name = get_slot_name(target, target_offset + target_index);
  if (!target_name) {
    return Errors::IncompatibleFit;
  }

  for (Count source_index = 0; source_index < get_size(); source_index++) {
    auto source_name = get_name(source_index);
    if (source_name && *source_name == *target_name) {
      return get_abstract(source_index)
          .visit(
              []() -> Result<const Abstract&, Errors> {
                return Errors::IncompatibleFit;
              },
              [](const Abstract& selected) -> Result<const Abstract&, Errors> {
                return selected;
              });
    }
  }

  return Errors::IncompatibleFit;
}
