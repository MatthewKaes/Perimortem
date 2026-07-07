// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/expression/pack.hpp"

#include "perimortem/memory/managed/vector.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Expression::Pack::evaluate(Cursor& cursor, Context& context)
    -> const Pack* {
  if (!cursor.require(
          Class::Type::PackingStart,
          "Expected `(` before expression pack."_view)) {
    return nullptr;
  }

  Managed::Vector<Entry> entries(context.get_arena());
  Managed::Vector<Value> values(context.get_arena());
  while (!cursor.is_one_of(
      {{Class::Type::EndOfStream, Class::Type::PackingEnd}})) {
    View::Bytes name;
    if (cursor.matches(Class::Type::AddressOp)) {
      cursor.consume();
      const Token* token = cursor.require(
          Class::Type::Addressable,
          "Expected expression pack entry name."_view);
      if (token == nullptr) {
        return nullptr;
      }

      if (!cursor.require(
              Class::Type::Assign,
              "Expected `=` after expression pack entry name."_view)) {
        return nullptr;
      }
      name = token->get_text();
    }

    Value value = Value::evaluate(cursor, context);
    if (value.is_empty()) {
      return nullptr;
    }
    entries.insert(Entry(name, value));
    values.insert(value);

    if (!cursor.matches(Class::Type::PackingOp)) {
      break;
    }
    cursor.consume();
  }

  if (!cursor.require(
          Class::Type::PackingEnd,
          "Expected `)` after expression pack."_view)) {
    return nullptr;
  }

  return &context.get_arena().construct<Pack>(
      entries.get_view(), values.get_view());
}

auto Expression::Pack::schema(
    Allocator::Arena& arena,
    View::Vector<Ttx::Type::Member> value_schema) const -> Ttx::Layout {
  Count entry_count =
      entries.is_empty() ? values.get_size() : entries.get_size();
  if (value_schema.get_size() != entry_count) {
    return Ttx::Layout();
  }

  Managed::Vector<Ttx::Type::Member> members(arena);
  for (Count i = 0; i < entry_count; i++) {
    const Ttx::Type* type = value_schema[i].get_type();
    if (type == nullptr) {
      return Ttx::Layout();
    }

    View::Bytes name =
        entries.is_empty() ? View::Bytes() : entries[i].get_name();
    members.insert(Ttx::Type::Member(name, *type));
  }

  return Ttx::Layout(members.get_view());
}
