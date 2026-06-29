// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/format/syntax.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Format;
using namespace Tetrodotoxin::Lexical;

class MemberAlign {
 public:
  Count name = 0;
  Count type = 0;
};

static auto compute_align(
    View::Vector<Ast::Member*> members,
    Count start,
    Count end) -> MemberAlign {
  MemberAlign align;
  Bool has_values = False;

  for (Count i = start; i < end; i++) {
    const Ast::Member* member = members[i];

    if (!member) {
      continue;
    }

    const Ast::Definition& definition = member->get_definition();
    Count name_width = definition.get_name().get_size();

    if (name_width > align.name) {
      align.name = name_width;
    }

    if (definition.is_value()) {
      has_values = True;
      Count type_width =
          Tetrodotoxin::Format::measure(definition.get_type_ref());

      if (type_width > align.type) {
        align.type = type_width;
      }
    }
  }

  if (!has_values) {
    align.type = 0;
  }

  return align;
}

auto Tetrodotoxin::Format::format(Formatter& ctx, const Ast::Member& member)
    -> void {
  Tetrodotoxin::Format::format(
      ctx, member, false, member.get_definition().get_name().get_size(),
      Tetrodotoxin::Format::measure(member.get_definition().get_type_ref()));
}

auto Tetrodotoxin::Format::format(
    Formatter& ctx,
    const Ast::Member& member,
    Bool first,
    Count name_width,
    Count type_width) -> void {
  const Ast::Comment& comment = member.get_documentation();
  Bool has_comment = !comment.is_empty();

  if (has_comment && !first) {
    ctx.ensure_blank();
  }

  if (has_comment) {
    Tetrodotoxin::Format::format(ctx, comment);
  }

  if (member.is_disabled()) {
    ctx.do_indent();
    ctx << Class::Type::Disabled << " "_view;
  }

  View::Vector<Ast::Attribute> attrs = member.get_attributes();

  for (Count i = 0; i < attrs.get_size(); i++) {
    if (!member.is_disabled() || i != 0) {
      ctx.do_indent();
    }

    Tetrodotoxin::Format::format(ctx, attrs[i]);
    ctx.emit_newline();
  }

  ctx.do_indent();
  Tetrodotoxin::Format::format(
      ctx, member.get_definition(), name_width, type_width);
  ctx.emit_newline();
}

auto Tetrodotoxin::Format::format_block(
    Formatter& ctx,
    View::Vector<Ast::Member*> members) -> void {
  Bool first = True;
  Count i = 0;

  while (i < members.get_size()) {
    const Ast::Member* member = members[i];

    if (!member) {
      i++;
      continue;
    }

    Count group_end = i + 1;

    while (group_end < members.get_size()) {
      const Ast::Member* member = members[group_end];

      if (!member) {
        group_end++;
        continue;
      }

      if (!member->get_documentation().is_empty()) {
        break;
      }

      group_end++;
    }

    MemberAlign align = compute_align(members, i, group_end);

    for (Count j = i; j < group_end; j++) {
      if (members[j]) {
        Tetrodotoxin::Format::format(
            ctx, *members[j], first, align.name, align.type);
        first = False;
      }
    }

    i = group_end;
  }
}
