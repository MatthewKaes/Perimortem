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

enum class TypeGroup {
  All,
  Foreign,
  NonForeign,
};

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

static auto format_type_group(
    Formatter& ctx,
    View::Vector<Type::Declaration*> types,
    TypeGroup group,
    Bool& emitted) -> void {
  Bool first_type = True;

  for (Count i = 0; i < types.get_size(); i++) {
    if (!types[i]) {
      continue;
    }

    if (group == TypeGroup::Foreign && !types[i]->is_foreign()) {
      continue;
    }

    if (group == TypeGroup::NonForeign && types[i]->is_foreign()) {
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

  format_type_group(ctx, body.get_types(), TypeGroup::Foreign, emitted);
  format_member_group(ctx, body.get_members(), emitted);
  format_function_group(ctx, body.get_functions(), emitted);
  format_type_group(ctx, body.get_types(), TypeGroup::NonForeign, emitted);
}

static auto format_type_order(Formatter& ctx, const Type::Body& body) -> void {
  Bool emitted = False;

  format_member_group(ctx, body.get_members(), emitted);
  format_function_group(ctx, body.get_functions(), emitted);
  format_type_group(ctx, body.get_types(), TypeGroup::All, emitted);
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
