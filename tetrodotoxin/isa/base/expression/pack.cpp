// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/expression/pack.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/isa/base/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Base::Expression::Pack::evaluate(Cursor& cursor, Base::Context& context)
    -> const Base::Expression::Pack* {
  Bool has_pack = cursor.require(
      Code::Type::PackingStart, "Expected `(` before expression pack."_view);
  if (!has_pack) {
    return nullptr;
  }

  Managed::Vector<Entry> entries(context.get_arena());
  Managed::Vector<Value> values(context.get_arena());
  while (!cursor.is_one_of(
      {{Code::Type::Terminal, Code::Type::PackingEnd}})) {
    View::Bytes name;
    if (cursor.matches(Code::Type::AddressOp)) {
      cursor.consume();
      const Token* token = cursor.require(
          Code::Type::Addressable,
          "Expected expression pack entry name."_view);
      if (token == nullptr) {
        return nullptr;
      }

      Bool has_assignment = cursor.require(
          Code::Type::Assign,
          "Expected `=` after expression pack entry name."_view);
      if (!has_assignment) {
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
    if (!cursor.matches(Code::Type::PackingOp)) {
      break;
    }

    cursor.consume();
  }

  Bool has_pack_end = cursor.require(
      Code::Type::PackingEnd, "Expected `)` after expression pack."_view);
  if (!has_pack_end) {
    return nullptr;
  }

  return &context.get_arena().construct<Base::Expression::Pack>(
      entries.get_view(), values.get_view());
}

auto Base::Expression::Pack::schema(
    Allocator::Arena& arena,
    View::Vector<Ttx::Member> value_schema) const -> Ttx::Layout {
  Count entry_count =
      entries.is_empty() ? values.get_size() : entries.get_size();
  if (value_schema.get_size() != entry_count) {
    return Ttx::Layout();
  }

  Managed::Vector<Ttx::Member> members(arena);
  for (Count i = 0; i < entry_count; i++) {
    View::Bytes name =
        entries.is_empty() ? View::Bytes() : entries[i].get_name();
    members.insert(Ttx::Member(name, value_schema[i].get_type()));
  }

  return Ttx::Layout(members.get_view());
}
