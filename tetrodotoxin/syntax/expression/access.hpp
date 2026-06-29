// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/syntax/expression/expression.hpp"
#include "tetrodotoxin/syntax/pack.hpp"

namespace Tetrodotoxin::Syntax::Expression {

class FieldAccess : public Expression {
 public:
  FieldAccess(Expression* base, Perimortem::Core::View::Bytes name)
      : Expression(Lexical::Class::Type::AddressOp, name), base(base) {}

  auto get_kind() const -> Kind override { return Kind::FieldAccess; }
  auto get_left() const -> const Expression* override { return base; }

 private:
  Expression* base = nullptr;
};

class Call : public Expression {
 public:
  Call(Expression* base, Perimortem::Core::View::Bytes name, Pack* args)
      : Expression(Lexical::Class::Type::CallOp, name),
        base(base),
        args(args) {}

  auto get_kind() const -> Kind override { return Kind::Call; }
  auto get_left() const -> const Expression* override { return base; }
  auto get_right() const -> const Expression* override { return args; }

 private:
  Expression* base = nullptr;
  Pack* args = nullptr;
};

class Index : public Expression {
 public:
  Index(Expression* base, Expression* index)
      : Expression(Lexical::Class::Type::IndexStart),
        base(base),
        index(index) {}

  auto get_kind() const -> Kind override { return Kind::Index; }
  auto get_left() const -> const Expression* override { return base; }
  auto get_right() const -> const Expression* override { return index; }

 private:
  Expression* base = nullptr;
  Expression* index = nullptr;
};

class IndexSlice : public Expression {
 public:
  IndexSlice(Expression* base, Expression* start, Expression* count)
      : Expression(Lexical::Class::Type::SliceOp),
        base(base),
        start(start),
        count(count) {}

  auto get_kind() const -> Kind override { return Kind::IndexSlice; }
  auto get_left() const -> const Expression* override { return base; }
  auto get_start() const -> const Expression* { return start; }
  auto get_count() const -> const Expression* { return count; }

 private:
  Expression* base = nullptr;
  Expression* start = nullptr;
  Expression* count = nullptr;
};

class Swizzle : public Expression {
 public:
  Swizzle(Expression* base, Perimortem::Core::View::Vector<Expression*> fields)
      : Expression(Lexical::Class::Type::SwizzleOp),
        base(base),
        fields(fields) {}

  auto get_kind() const -> Kind override { return Kind::Swizzle; }
  auto get_left() const -> const Expression* override { return base; }
  auto get_args() const
      -> Perimortem::Core::View::Vector<Expression*> override {
    return fields;
  }

 private:
  Expression* base = nullptr;
  Perimortem::Core::View::Vector<Expression*> fields;
};

class Slice : public Expression {
 public:
  Slice(Expression* base, Expression* start, Expression* count)
      : Expression(Lexical::Class::Type::SliceOp),
        base(base),
        start(start),
        count(count) {}

  auto get_kind() const -> Kind override { return Kind::Slice; }
  auto get_left() const -> const Expression* override { return base; }
  auto get_start() const -> const Expression* { return start; }
  auto get_count() const -> const Expression* { return count; }

 private:
  Expression* base = nullptr;
  Expression* start = nullptr;
  Expression* count = nullptr;
};

auto parse_access_chain(Context& ctx, Expression* base) -> Expression*;

}  // namespace Tetrodotoxin::Syntax::Expression
