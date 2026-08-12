// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/model/pack.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Parser {

// Expression consumes one complete Library value-flow operand. Parentheses may
// therefore produce an empty, named, or multi-value Pack without inventing a
// carrier Expression. Postfix Access and scalar Operations prove the narrower
// Expression category only when their own grammar requires it.
class Expression {
 public:
  Expression() = delete;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Model::Pack&>;

  // Parses one tighter operand with private diagnostics. The caller position
  // advances only when the complete operand succeeds.
  static auto parse_operand(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor,
      Ttx::Lexical::Code::Type operation)
      -> Perimortem::Core::Option<Language::Model::Pack&>;

  // Parses one prefix operand with private diagnostics. Postfix operations
  // remain inside the operand while binary operations remain outside it.
  static auto parse_prefix_operand(
      Perimortem::Memory::Allocator::Arena& domain,
      Monograph& source,
      Ttx::Lexical::Cursor& cursor)
      -> Perimortem::Core::Option<Language::Model::Pack&>;
};

}  // namespace Tetrodotoxin::Library::Language::Parser

// Scalar Operations accept Pack operands at their grammar boundary, then
// retain the exact Expression identities proved here. A parenthesized single
// positional value is already that Expression; named and multi-value Packs
// remain honest Packs and require an operation that defines their shape.
#define TTX_DIRECT_BINARY_PARSE(type, token_type, malformed, repair)        \
  auto Tetrodotoxin::Library::Language::Operations::type::parse(            \
      Perimortem::Memory::Allocator::Arena& domain,                         \
      Tetrodotoxin::Library::Language::Monograph& source,                   \
      Ttx::Lexical::Cursor& cursor,                                         \
      Tetrodotoxin::Library::Language::Model::Pack& left,                   \
      Ttx::Lexical::Span left_span)                                         \
      -> Perimortem::Core::Option<                                          \
          Tetrodotoxin::Library::Language::Expression&> {                   \
    Ttx::Lexical::Token opening = cursor.consume();                         \
    Ttx::Lexical::Token right_start = cursor.current();                     \
    auto right =                                                            \
        Tetrodotoxin::Library::Language::Parser::Expression::parse_operand( \
            domain, source, cursor, Ttx::Lexical::Code::Type::token_type);  \
    Ttx::Lexical::Span span(opening, cursor.peek(-1));                      \
    if (!right) {                                                           \
      cursor.create_expression_error(span, malformed, repair);              \
      return {};                                                            \
    }                                                                       \
    Ttx::Lexical::Span right_span(right_start, cursor.peek(-1));            \
    auto left_expression =                                                  \
        left.select<Tetrodotoxin::Library::Language::Expression>();         \
    auto right_expression =                                                 \
        right->select<Tetrodotoxin::Library::Language::Expression>();       \
    if (!left_expression || !right_expression) {                            \
      cursor.create_expression_error(                                       \
          Ttx::Lexical::Anchor::create(opening, left_span, right_span),     \
          "Library scalar operation requires one Expression from each "     \
          "operand Pack."_view,                                             \
          "Use one unlabelled scalar value; named and multi-value Packs "   \
          "require an operation that defines their shape."_view);           \
      return {};                                                            \
    }                                                                       \
    auto anchor =                                                           \
        Ttx::Lexical::Anchor::create(opening, left_span, right_span);       \
    return create_authored(                                                 \
        domain, *left_expression, *right_expression, anchor);               \
  }

#define TTX_TRANSACTIONAL_BINARY_PARSE(type, token_type, malformed, repair)   \
  auto Tetrodotoxin::Library::Language::Operations::type::parse(              \
      Perimortem::Memory::Allocator::Arena& domain,                           \
      Tetrodotoxin::Library::Language::Monograph& source,                     \
      Ttx::Lexical::Cursor& cursor,                                           \
      Tetrodotoxin::Library::Language::Model::Pack& left,                     \
      Ttx::Lexical::Span left_span)                                           \
      -> Perimortem::Core::Option<                                            \
          Tetrodotoxin::Library::Language::Expression&> {                     \
    auto transaction = cursor.branch();                                       \
    Ttx::Lexical::Token opening = transaction.consume();                      \
    Ttx::Lexical::Token right_start = transaction.current();                  \
    auto right =                                                              \
        Tetrodotoxin::Library::Language::Parser::Expression::parse_operand(   \
            domain, source, transaction,                                      \
            Ttx::Lexical::Code::Type::token_type);                            \
    Ttx::Lexical::Span span(opening, transaction.peek(-1));                   \
    if (!right) {                                                             \
      transaction.create_expression_error(span, malformed, repair);           \
      return {};                                                              \
    }                                                                         \
    Ttx::Lexical::Span right_span(right_start, transaction.peek(-1));         \
    auto left_expression =                                                    \
        left.select<Tetrodotoxin::Library::Language::Expression>();           \
    auto right_expression =                                                   \
        right->select<Tetrodotoxin::Library::Language::Expression>();         \
    if (!left_expression || !right_expression) {                              \
      transaction.create_expression_error(                                    \
          Ttx::Lexical::Anchor::create(opening, left_span, right_span),       \
          "Library scalar operation requires one Expression from each "       \
          "operand Pack."_view,                                               \
          "Use one unlabelled scalar value; named and multi-value Packs "     \
          "require an operation that defines their shape."_view);             \
      return {};                                                              \
    }                                                                         \
    auto anchor =                                                             \
        Ttx::Lexical::Anchor::create(opening, left_span, right_span);         \
    auto& result =                                                            \
        create_authored(domain, *left_expression, *right_expression, anchor); \
    cursor.join(transaction);                                                 \
    return result;                                                            \
  }

#define TTX_DIRECT_UNARY_PARSE(type, malformed, repair)                   \
  auto Tetrodotoxin::Library::Language::Operations::type::parse(          \
      Perimortem::Memory::Allocator::Arena& domain,                       \
      Tetrodotoxin::Library::Language::Monograph& source,                 \
      Ttx::Lexical::Cursor& cursor)                                       \
      -> Perimortem::Core::Option<                                        \
          Tetrodotoxin::Library::Language::Expression&> {                 \
    Ttx::Lexical::Token opening = cursor.consume();                       \
    Ttx::Lexical::Token operand_start = cursor.current();                 \
    auto operand = Tetrodotoxin::Library::Language::Parser::Expression::  \
        parse_prefix_operand(domain, source, cursor);                     \
    Ttx::Lexical::Span span(opening, cursor.peek(-1));                    \
    if (!operand) {                                                       \
      cursor.create_expression_error(span, malformed, repair);            \
      return {};                                                          \
    }                                                                     \
    Ttx::Lexical::Span operand_span(operand_start, cursor.peek(-1));      \
    auto expression =                                                     \
        operand->select<Tetrodotoxin::Library::Language::Expression>();   \
    if (!expression) {                                                    \
      cursor.create_expression_error(                                     \
          Ttx::Lexical::Anchor::create(                                   \
              opening, Ttx::Lexical::Span(opening), operand_span),        \
          "Library scalar operation requires one Expression operand "     \
          "Pack."_view,                                                   \
          "Use one unlabelled scalar value; named and multi-value Packs " \
          "require an operation that defines their shape."_view);         \
      return {};                                                          \
    }                                                                     \
    auto anchor = Ttx::Lexical::Anchor::create(                           \
        opening, Ttx::Lexical::Span(opening), operand_span);              \
    return create_authored(domain, *expression, anchor);                  \
  }
