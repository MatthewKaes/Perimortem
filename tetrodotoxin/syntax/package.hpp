// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/lexical/tokenizer.hpp"
#include "tetrodotoxin/syntax/ast/comment.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/dialect/dialect.hpp"
#include "tetrodotoxin/syntax/type/body.hpp"

namespace Tetrodotoxin::Syntax {

// Root AST node for a single TTX source file.
class Package {
 public:
  static auto detect_kind(Perimortem::Core::View::Vector<Lexical::Token> tokens)
      -> Lexical::Class;
  static auto parse(
      const Lexical::Tokenizer& tokenizer,
      Perimortem::Core::View::Bytes source_path) -> Package;
  static auto parse(Context& context) -> Package;

  auto get_documentation() const -> const Ast::Comment& {
    return documentation;
  }

  auto get_source_path() const -> Perimortem::Core::View::Bytes {
    return source_path;
  }

  auto get_imports() const -> Perimortem::Core::View::Vector<Ast::Import> {
    return imports;
  }

  auto get_members() const -> Perimortem::Core::View::Vector<Ast::Member*> {
    return body.get_members();
  }

  auto get_functions() const -> Perimortem::Core::View::Vector<Ast::Function*> {
    return body.get_functions();
  }

  auto get_types() const -> Perimortem::Core::View::Vector<Type::Declaration*> {
    return body.get_types();
  }

  auto get_body() const -> const Type::Body& { return body; }

  auto is_valid() const -> Bool {
    return kind != Lexical::Class::Type::Unknown;
  }

  auto get_kind() const -> Lexical::Class { return kind; }
  auto get_dialect() const -> const Dialect::Dialect& { return *dialect; }

 private:
  Ast::Comment documentation;
  Perimortem::Core::View::Bytes source_path;
  Perimortem::Core::View::Vector<Ast::Import> imports;
  Type::Body body;
  Lexical::Class kind = Lexical::Class::Type::Unknown;
  const Dialect::Dialect* dialect = nullptr;
};

}  // namespace Tetrodotoxin::Syntax
