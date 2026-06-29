// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/dialect/shader/quad.hpp"

#include "perimortem/core/reader/textual.hpp"

#include "tetrodotoxin/syntax/ast/member.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Dialect::Shader;

using Expression = Syntax::Expression::Expression;

static constexpr Static::Vector<Vec2Literal, QuadMesh::vertex_count>
    default_quad_positions = {{
      {-1.0f, -1.0f},
      {1.0f, -1.0f},
      {1.0f, 1.0f},
      {-1.0f, -1.0f},
      {1.0f, 1.0f},
      {-1.0f, 1.0f},
    }};

static constexpr Static::Vector<Vec2Literal, QuadMesh::vertex_count>
    default_quad_uvs = {{
      {0.0f, 0.0f},
      {1.0f, 0.0f},
      {1.0f, 1.0f},
      {0.0f, 0.0f},
      {1.0f, 1.0f},
      {0.0f, 1.0f},
    }};

static auto parse_real_literal(View::Bytes text, Real_32* out) -> Bool {
  if (!out) {
    return False;
  }

  Reader::Textual reader(text);
  Real_32 value = reader.read_real_32();
  if (!reader.is_valid() || !reader.is_empty()) {
    return False;
  }

  *out = value;
  return True;
}

static auto parse_real_expr(const Expression* expr, Real_32* out) -> Bool {
  if (!expr || !out) {
    return False;
  }

  switch (expr->get_class().get_type()) {
  case Lexical::Class::Type::Float:
  case Lexical::Class::Type::Numeric:
    return parse_real_literal(expr->get_text(), out);

  case Lexical::Class::Type::SubOp: {
    if (expr->get_left()) {
      return False;
    }
    Real_32 value = 0.0f;
    if (!parse_real_expr(expr->get_right(), &value)) {
      return False;
    }
    *out = -value;
    return True;
  }

  default:
    return False;
  }
}

static auto field_expr(
    View::Vector<Syntax::Pack::Field> fields,
    View::Bytes name) -> const Expression* {
  for (Count i = 0; i < fields.get_size(); i++) {
    if (fields[i].name == name) {
      return fields[i].value;
    }
  }

  return nullptr;
}

static auto parse_vec2_expr(const Expression* expr, Vec2Literal* out) -> Bool {
  if (!expr || !out ||
      expr->get_class() != Lexical::Class::Type::PackingStart) {
    return False;
  }

  const auto* pack = static_cast<const Syntax::Pack*>(expr);
  View::Vector<Syntax::Pack::Field> fields = pack->get_fields();
  const Expression* x = field_expr(fields, "x"_view);
  const Expression* y = field_expr(fields, "y"_view);
  if (x && y) {
    return parse_real_expr(x, &out->x) && parse_real_expr(y, &out->y);
  }

  View::Vector<Expression*> args = pack->get_args();
  if (args.get_size() < 2) {
    return False;
  }

  return parse_real_expr(args[0], &out->x) && parse_real_expr(args[1], &out->y);
}

static auto find_member(const Syntax::Type::Declaration& type, View::Bytes name)
    -> const Syntax::Ast::Member* {
  View::Vector<Syntax::Ast::Member*> members = type.get_scope();
  for (Count i = 0; i < members.get_size(); i++) {
    if (members[i] && members[i]->get_definition().get_name() == name) {
      return members[i];
    }
  }

  return nullptr;
}

static auto read_vec2_array_member(
    const Syntax::Type::Declaration& type,
    View::Bytes name,
    Static::Vector<Vec2Literal, QuadMesh::vertex_count>& out) -> void {
  const Syntax::Ast::Member* member = find_member(type, name);
  if (!member) {
    return;
  }

  const Expression* expr = member->get_definition().get_value().get_expr();
  if (!expr || expr->get_class() != Lexical::Class::Type::PackingStart) {
    return;
  }

  View::Vector<Expression*> args = expr->get_args();
  for (Count i = 0; i < args.get_size() && i < QuadMesh::vertex_count; i++) {
    Vec2Literal parsed;
    if (parse_vec2_expr(args[i], &parsed)) {
      out[i] = parsed;
    }
  }
}

auto QuadMesh::default_mesh() -> QuadMesh {
  QuadMesh mesh;
  mesh.positions = default_quad_positions;
  mesh.uvs = default_quad_uvs;
  return mesh;
}

auto QuadMesh::from_type(const Syntax::Type::Declaration& type) -> QuadMesh {
  QuadMesh mesh = default_mesh();
  read_vec2_array_member(type, "quad_positions"_view, mesh.positions);
  read_vec2_array_member(type, "quad_uvs"_view, mesh.uvs);
  return mesh;
}
