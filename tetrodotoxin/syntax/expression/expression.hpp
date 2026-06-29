// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "tetrodotoxin/lexical/class.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/type/ref.hpp"

namespace Tetrodotoxin::Syntax::Expression {

using Syntax::Type::Ref;

enum class Kind {
  Expression,
  Literal,
  Addressable,
  TypeAccess,
  Data,
  Unary,
  Binary,
  FieldAccess,
  Call,
  Index,
  IndexSlice,
  Swizzle,
  Slice,
  Pack,
};

class Expression {
 public:
  Expression() = default;
  explicit Expression(Lexical::Class klass) : klass(klass) {}
  Expression(Lexical::Class klass, Perimortem::Core::View::Bytes text)
      : klass(klass), text(text) {}

  static auto parse(Context& ctx) -> Expression*;
  static auto make(Context& ctx, Lexical::Class klass) -> Expression*;
  template <typename T, typename... Args>
  static auto construct(Context& ctx, Args&&... args) -> T* {
    return new (ctx.get_arena().allocate(sizeof(T)))
        T(static_cast<Args&&>(args)...);
  }

  virtual auto get_kind() const -> Kind { return Kind::Expression; }
  auto get_class() const -> Lexical::Class { return klass; }
  auto get_text() const -> Perimortem::Core::View::Bytes { return text; }
  virtual auto get_left() const -> const Expression* { return nullptr; }
  virtual auto get_right() const -> const Expression* { return nullptr; }
  virtual auto get_args() const -> Perimortem::Core::View::Vector<Expression*> {
    return Perimortem::Core::View::Vector<Expression*>();
  }
  virtual auto get_type_ref() const -> const Ref&;

 protected:
  Lexical::Class klass = Lexical::Class::Type::EndOfStream;
  Perimortem::Core::View::Bytes text;
};

}  // namespace Tetrodotoxin::Syntax::Expression
