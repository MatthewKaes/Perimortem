// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/ast/value.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"

namespace Tetrodotoxin::Syntax::Type {
class Declaration;
}

namespace Tetrodotoxin::Syntax {

enum class DeclarationKind {
  Invalid,
  Value,
  Enum,
  Foreign,
  Object,
  Shader,
  Struct,
};

auto is_type_declaration(DeclarationKind kind) -> Bool;
auto to_scope_type(DeclarationKind kind) -> Lexical::Class::Type;

}  // namespace Tetrodotoxin::Syntax

namespace Tetrodotoxin::Syntax::Ast {

// A single named declaration: sigil? name : Type::Ref (= Value)?
class Definition {
  friend class Function;
  friend class Member;
  friend class Tetrodotoxin::Syntax::Type::Declaration;

 public:
  // Parses from the current stream position. Caller guarantees the current
  // token is a sigil or an identifier.
  static auto parse(Context& ctx) -> Definition;
  static auto parse_header(Context& ctx) -> Definition;

  auto get_sigil() const -> Lexical::Class { return sigil; }
  auto get_name() const -> Perimortem::Core::View::Bytes { return name; }
  auto get_type_ref() const -> const Type::Ref& { return type_ref; }
  auto get_alias_target() const -> const Type::Ref& { return alias_target; }
  auto get_form() const -> Lexical::Class { return form; }

  auto is_empty() const -> Bool {
    return form == Lexical::Class::Type::EndStatement;
  }
  auto is_value() const -> Bool { return form == Lexical::Class::Type::Assign; }
  auto is_alias() const -> Bool;
  auto get_declaration_kind() const -> DeclarationKind;
  auto is_type_declaration() const -> Bool {
    return Syntax::is_type_declaration(get_declaration_kind());
  }

  // Valid when is_value().
  auto get_value() const -> const Value& { return value; }

  auto is_valid() const -> Bool { return !name.is_empty(); }

 private:
  Lexical::Class sigil = Lexical::Class::Type::EndOfStream;
  Perimortem::Core::View::Bytes name;
  Type::Ref type_ref;
  Type::Ref alias_target;
  Lexical::Class form = Lexical::Class::Type::EndStatement;
  Value value;
};

}  // namespace Tetrodotoxin::Syntax::Ast
