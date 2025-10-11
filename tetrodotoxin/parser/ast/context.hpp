// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/error.hpp"
#include "parser/tokenizer.hpp"

#include <memory>

namespace Tetrodotoxin::Language::Parser {

#define TTX_ERROR(msg)                                                         \
  {                                                                            \
    std::stringstream __details;                                               \
    __details << msg << std::endl;                                             \
    ctx.errors.push_back(Error(ctx.source_map, __details.view(), ctx.source)); \
  }

#define TTX_TOKEN_FATAL(msg)           \
  {                                    \
    std::stringstream __details;       \
    __details << msg << ".";           \
    ctx.token_error(__details.view()); \
    return;                            \
  }

#define TTX_TOKEN_ERROR(msg)           \
  {                                    \
    std::stringstream __details;       \
    __details << msg << ".";           \
    ctx.token_error(__details.view()); \
  }

#define TTX_EXPECTED_CLASS_ERROR(expected, actual)                   \
  TTX_TOKEN_ERROR("Expected " << klass_name(expected) << " but got " \
                              << klass_name(actual));

struct Context {
  Context(const std::string_view& source_map,
          ByteView source,
          Tokenizer& tokenizer,
          Errors& errors)
      : tokenizer(tokenizer),
        errors(errors),
        source_map(source_map),
        source(source) {
    start_token = tokenizer.get_tokens().data();
    current_token = start_token;
    terminal_token = start_token + tokenizer.get_tokens().size() - 1;
  };

  inline auto get_line_range(Token& begin_token, Token& end_token) const
      -> std::optional<std::string_view> {
    uint32_t start = begin_token.location.source_index;
    while (start > 0 && source[start] != '\n')
      start--;

    // If we found a new line then go forward.
    if (start != 0)
      start++;

    uint32_t end = end_token.location.parse_index;
    while (end < source.size() && source[end] != '\n')
      end++;

    return std::string_view((char*)source.data() + start, end - start);
  }

  inline auto get_error_range(Token& begin_token, Token& end_token) const
      -> uint32_t {
    return (end_token.location.column + end_token.data.size()) -
           begin_token.location.column;
  }

  inline auto advance() -> const Token& {
    current_token = std::min(current_token + 1, terminal_token);
    return *current_token;
  }

  inline auto advance_past(Classifier target) -> const Token& {
    while (current().klass != target &&
           current().klass != Classifier::EndOfStream)
      advance();

    // Advance one past the target
    return advance();
  }

  // Advances until the current classifier is balanced.
  // Useful for quickly balancing parse errors on scopes.
  inline auto advance_balanced(Classifier open,
                               Classifier close,
                               ClassifierFlags terminals,
                               int start_count = 1) -> const Token& {
    while (start_count > 0 && current().klass != Classifier::EndOfStream &&
           !terminals.has(current().klass)) {
      advance();
      if (current().klass == open)
        start_count++;
      else if (current().klass == close)
        start_count--;
    }

    // Return the possibly balanced token.
    return current();
  }

  inline auto index() const -> uint64_t {
    return ((uint64_t)current_token - (uint64_t)start_token) / sizeof(Token);
  }

  inline auto token_error(std::string_view details) const -> void {
    // Try to look back one token if we are at EoF.
    int32_t offset = current().klass == Classifier::EndOfStream ? 1 : 0;
    range_error(details, *(current_token - offset), *(current_token - offset),
                *(current_token - offset));
  }

  inline auto range_error(const std::string_view& details,
                          const Token& begin_token,
                          const Token& end_token) const -> void {
    range_error(details, begin_token, begin_token, end_token);
  }

  inline auto range_error(std::string_view details,
                          const Token& token,
                          const Token& begin_token,
                          const Token& end_token) const -> void {
    std::optional<Location> loc = std::nullopt;
    std::optional<std::string_view> line_range = std::nullopt;
    std::optional<uint32_t> error_range = std::nullopt;
    if (begin_token.valid() && end_token.valid()) {
      loc = token.location;
      uint32_t start = begin_token.location.source_index;
      while (start > 0 && source[start] != '\n')
        start--;

      // If we found a new line then go forward.
      if (start != 0)
        start++;

      uint32_t end = end_token.location.parse_index;
      while (end < source.size() && source[end] != '\n')
        end++;

      line_range = std::string_view((char*)source.data() + start, end - start);
      error_range = end_token.location.parse_index -
                    begin_token.location.source_index;
    }

    errors.push_back(
        Error(source_map, details, source, loc, line_range, error_range));
  }

  inline auto current() const -> const Token& { return *current_token; }

  Tokenizer& tokenizer;
  Errors& errors;
  const std::string_view source_map;
  const ByteView source;

 private:
  const Token* current_token;
  const Token* terminal_token;
  const Token* start_token;
};

}  // namespace Tetrodotoxin::Language::Parser