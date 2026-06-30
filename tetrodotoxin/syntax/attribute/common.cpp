// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/attribute/common.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Syntax;

using FieldMapping = Pair<View::Bytes, Attribute::FieldKind>;
static constexpr Static::Vector<FieldMapping, 4> field_names = {{
  {"kind"_view, Attribute::FieldKind::Kind},
  {"name"_view, Attribute::FieldKind::Name},
  {"set"_view, Attribute::FieldKind::Set},
  {"slot"_view, Attribute::FieldKind::Slot},
}};

using FieldTable = Table<Attribute::FieldKind, field_names>;

static auto lookup_field(View::Bytes name) -> Attribute::FieldKind {
  return FieldTable::find_or_default(name, Attribute::FieldKind::Unknown);
}

auto Attribute::has_field(View::Vector<Pack::Field> fields, FieldKind kind)
    -> Bool {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (lookup_field(fields[i].name) == kind) {
      return True;
    }
  }

  return False;
}

static auto has_positional_fields(View::Vector<Pack::Field> fields) -> Bool {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields[i].name.is_empty()) {
      return True;
    }
  }

  return False;
}

auto Attribute::require_named_fields(
    Context& ctx,
    const Ast::Attribute& attr,
    View::Bytes hint) -> void {
  if (has_positional_fields(attr.get_fields())) {
    ctx.error("Expected named attribute fields"_view, hint);
  }
}

auto Attribute::require_field(
    Context& ctx,
    const Ast::Attribute& attr,
    FieldKind kind,
    View::Bytes hint) -> void {
  if (!has_field(attr.get_fields(), kind)) {
    ctx.error("Expected attribute field"_view, hint);
  }
}

auto Attribute::is_value_member(const Ast::Member* member) -> Bool {
  return member != nullptr;
}

auto Attribute::reject_layout_attribute(Context& ctx, const Ast::Attribute&)
    -> void {
  ctx.error(
      "Invalid layout attribute"_view,
      "Only @builtin is currently defined for layout fields"_view);
}

auto Attribute::ignore_member_attribute(
    Context&,
    const Target&,
    const Ast::Attribute&) -> void {}

auto Attribute::ignore_layout_attribute(Context&, const Ast::Attribute&)
    -> void {}
