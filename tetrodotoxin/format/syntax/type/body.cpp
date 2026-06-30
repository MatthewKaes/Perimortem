// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/format/syntax.hpp"
#include "tetrodotoxin/syntax/ast/declaration.hpp"
#include "tetrodotoxin/syntax/ast/definition.hpp"
#include "tetrodotoxin/syntax/ast/function.hpp"
#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/type/declaration.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

using TypeFilter = Bool (*)(const Type::Declaration&);

static auto begin_group(Formatter& ctx, Bool& emitted) -> void {
  if (emitted) {
    ctx.ensure_blank();
  }
  emitted = True;
}

static auto format_member_group(
    Formatter& ctx,
    View::Vector<Ast::Member*> members,
    Bool& emitted) -> void {
  if (members.is_empty()) {
    return;
  }

  begin_group(ctx, emitted);
  Tetrodotoxin::Format::format_block(ctx, members);
}

static auto format_function_group(
    Formatter& ctx,
    View::Vector<Ast::Function*> functions,
    Bool& emitted) -> void {
  if (functions.is_empty()) {
    return;
  }

  begin_group(ctx, emitted);
  Tetrodotoxin::Format::format_block(ctx, functions);
}

static auto accepts_any_type(const Type::Declaration&) -> Bool {
  return True;
}

static auto accepts_foreign_type(const Type::Declaration& type) -> Bool {
  return type.get_kind() == DeclarationKind::Foreign;
}

static auto accepts_package_body_type(const Type::Declaration& type) -> Bool {
  return type.get_kind() != DeclarationKind::Foreign;
}

static auto format_type_group(
    Formatter& ctx,
    View::Vector<Type::Declaration*> types,
    TypeFilter accepts,
    Bool& emitted) -> void {
  Bool first_type = True;

  for (Count i = 0; i < types.get_size(); i++) {
    if (!types[i]) {
      continue;
    }

    if (!accepts(*types[i])) {
      continue;
    }

    if (!emitted) {
      emitted = True;
    } else {
      ctx.ensure_blank();
    }

    Tetrodotoxin::Format::format(ctx, *types[i], first_type);
    first_type = False;
    emitted = True;
  }
}

static auto format_package_order(Formatter& ctx, const Type::Body& body)
    -> void {
  Bool emitted = False;

  // Foreign declarations are ABI promises, so package formatting keeps them
  // before ordinary declarations where source reviewers can see the boundary.
  format_type_group(ctx, body.get_types(), accepts_foreign_type, emitted);
  format_member_group(ctx, body.get_members(), emitted);
  format_function_group(ctx, body.get_functions(), emitted);
  format_type_group(ctx, body.get_types(), accepts_package_body_type, emitted);
}

static auto format_type_order(Formatter& ctx, const Type::Body& body) -> void {
  Bool emitted = False;

  format_member_group(ctx, body.get_members(), emitted);
  format_function_group(ctx, body.get_functions(), emitted);
  format_type_group(ctx, body.get_types(), accepts_any_type, emitted);
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Type::Body& body,
    BodyOrder order) -> void {
  if (order == BodyOrder::Package) {
    format_package_order(ctx, body);
    return;
  }

  format_type_order(ctx, body);
}
