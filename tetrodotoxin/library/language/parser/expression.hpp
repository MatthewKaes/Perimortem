// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Parser {

// Expression consumes one complete Library value operand. Postfix Access owns
// its receiver chain before mathematical Operations participate in precedence.
// Each concrete owner parses its complete grammar and constructs its source
// identity. Semantic Types and Addressables connect only when the retained
// owner links the complete Expression graph.
class Expression {
 public:
  Expression() = delete;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Core::Option<Language::Expression&>;

  // Parses one tighter operand with private diagnostics. The caller position
  // advances only when the complete operand succeeds.
  static auto parse_operand(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context,
      Ttx::Lexical::Code::Type operation)
      -> Perimortem::Core::Option<Language::Expression&>;

  // Parses one prefix operand with private diagnostics. Postfix operations
  // remain inside the operand while binary operations remain outside it.
  static auto parse_prefix_operand(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Core::Option<Language::Expression&>;
};

}  // namespace Tetrodotoxin::Library::Language::Parser

// Ordinary operation parsers share their complete source transaction while
// keeping direct and speculative cursor policies explicit at each definition.
#define TTX_DIRECT_BINARY_PARSE(                                            \
    type, token_type, malformed, repair, missing_anchor)                    \
  auto Tetrodotoxin::Library::Language::Operations::type::parse(            \
      Perimortem::Memory::Allocator::Arena& domain,                         \
      Tetrodotoxin::Library::Language::Materializations& materializations,  \
      Ttx::Lexical::Cursor& cursor,                                         \
      const Ttx::Concept::Abstract& source_context,                         \
      Tetrodotoxin::Library::Language::Expression& left)                    \
      -> Perimortem::Core::Option<                                          \
          Tetrodotoxin::Library::Language::Expression&> {                   \
    Ttx::Lexical::Token opening = cursor.consume();                         \
    auto right =                                                            \
        Tetrodotoxin::Library::Language::Parser::Expression::parse_operand( \
            domain, materializations, cursor, source_context,               \
            Ttx::Lexical::Code::Type::token_type);                          \
    Ttx::Lexical::Span span(opening, cursor.peek(-1));                      \
    if (!right) {                                                           \
      cursor.create_expression_error(span, malformed, repair);              \
      return {};                                                            \
    }                                                                       \
    const auto& left_anchor = left.get_anchor();                            \
    const auto& right_anchor = right->get_anchor();                         \
    if (!left_anchor || !right_anchor) {                                    \
      cursor.create_expression_error(span, missing_anchor);                 \
      return {};                                                            \
    }                                                                       \
    auto anchor = Ttx::Lexical::Anchor::create(                             \
        opening, left_anchor->get_span(), right_anchor->get_span());        \
    return create_authored(domain, materializations, left, *right, anchor); \
  }

#define TTX_TRANSACTIONAL_BINARY_PARSE(                                     \
    type, token_type, malformed, repair, missing_anchor)                    \
  auto Tetrodotoxin::Library::Language::Operations::type::parse(            \
      Perimortem::Memory::Allocator::Arena& domain,                         \
      Tetrodotoxin::Library::Language::Materializations& materializations,  \
      Ttx::Lexical::Cursor& cursor,                                         \
      const Ttx::Concept::Abstract& source_context,                         \
      Tetrodotoxin::Library::Language::Expression& left)                    \
      -> Perimortem::Core::Option<                                          \
          Tetrodotoxin::Library::Language::Expression&> {                   \
    auto transaction = cursor.branch();                                     \
    Ttx::Lexical::Token opening = transaction.consume();                    \
    auto right =                                                            \
        Tetrodotoxin::Library::Language::Parser::Expression::parse_operand( \
            domain, materializations, transaction, source_context,          \
            Ttx::Lexical::Code::Type::token_type);                          \
    Ttx::Lexical::Span span(opening, transaction.peek(-1));                 \
    if (!right) {                                                           \
      transaction.create_expression_error(span, malformed, repair);         \
      return {};                                                            \
    }                                                                       \
    const auto& left_anchor = left.get_anchor();                            \
    const auto& right_anchor = right->get_anchor();                         \
    if (!left_anchor || !right_anchor) {                                    \
      transaction.create_expression_error(span, missing_anchor);            \
      return {};                                                            \
    }                                                                       \
    auto anchor = Ttx::Lexical::Anchor::create(                             \
        opening, left_anchor->get_span(), right_anchor->get_span());        \
    auto& result =                                                          \
        create_authored(domain, materializations, left, *right, anchor);    \
    cursor.join(transaction);                                               \
    return result;                                                          \
  }

#define TTX_DIRECT_UNARY_PARSE(type, malformed, repair, missing_anchor)    \
  auto Tetrodotoxin::Library::Language::Operations::type::parse(           \
      Perimortem::Memory::Allocator::Arena& domain,                        \
      Tetrodotoxin::Library::Language::Materializations& materializations, \
      Ttx::Lexical::Cursor& cursor,                                        \
      const Ttx::Concept::Abstract& source_context)                        \
      -> Perimortem::Core::Option<                                         \
          Tetrodotoxin::Library::Language::Expression&> {                  \
    Ttx::Lexical::Token opening = cursor.consume();                        \
    auto operand = Tetrodotoxin::Library::Language::Parser::Expression::   \
        parse_prefix_operand(                                              \
            domain, materializations, cursor, source_context);             \
    Ttx::Lexical::Span span(opening, cursor.peek(-1));                     \
    if (!operand) {                                                        \
      cursor.create_expression_error(span, malformed, repair);             \
      return {};                                                           \
    }                                                                      \
    const auto& operand_anchor = operand->get_anchor();                    \
    if (!operand_anchor) {                                                 \
      cursor.create_expression_error(span, missing_anchor);                \
      return {};                                                           \
    }                                                                      \
    auto anchor = Ttx::Lexical::Anchor::create(                            \
        opening, Ttx::Lexical::Span(opening), operand_anchor->get_span()); \
    return create_authored(domain, materializations, *operand, anchor);    \
  }
