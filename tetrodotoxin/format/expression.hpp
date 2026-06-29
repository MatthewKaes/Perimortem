// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/format/formatter.hpp"
#include "tetrodotoxin/syntax/expression/expression.hpp"

namespace Tetrodotoxin::Syntax {
class Pack;
}

namespace Tetrodotoxin::Syntax::Expression {
class Addressable;
class Binary;
class Call;
class Data;
class FieldAccess;
class Index;
class IndexSlice;
class Literal;
class Slice;
class Swizzle;
class TypeAccess;
class Unary;
}  // namespace Tetrodotoxin::Syntax::Expression

namespace Tetrodotoxin::Format {

using ExpressionNode = Tetrodotoxin::Syntax::Expression::Expression;

auto format(Formatter& ctx, const ExpressionNode* expr) -> void;
auto measure(const ExpressionNode* expr) -> Count;

auto format(Formatter& ctx, const Syntax::Expression::Addressable& expr)
    -> void;
auto measure(const Syntax::Expression::Addressable& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::Binary& expr) -> void;
auto measure(const Syntax::Expression::Binary& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::Call& expr) -> void;
auto measure(const Syntax::Expression::Call& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::Data& expr) -> void;
auto measure(const Syntax::Expression::Data& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::FieldAccess& expr)
    -> void;
auto measure(const Syntax::Expression::FieldAccess& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::Index& expr) -> void;
auto measure(const Syntax::Expression::Index& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::IndexSlice& expr) -> void;
auto measure(const Syntax::Expression::IndexSlice& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::Literal& expr) -> void;
auto measure(const Syntax::Expression::Literal& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::Slice& expr) -> void;
auto measure(const Syntax::Expression::Slice& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::Swizzle& expr) -> void;
auto measure(const Syntax::Expression::Swizzle& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::TypeAccess& expr) -> void;
auto measure(const Syntax::Expression::TypeAccess& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Expression::Unary& expr) -> void;
auto measure(const Syntax::Expression::Unary& expr) -> Count;
auto format(Formatter& ctx, const Syntax::Pack& pack) -> void;
auto measure(const Syntax::Pack& pack) -> Count;

}  // namespace Tetrodotoxin::Format
