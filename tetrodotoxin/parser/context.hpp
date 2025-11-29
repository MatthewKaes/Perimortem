// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "parser/error.hpp"

#include "lexical/tokenizer.hpp"
#include "types/program.hpp"

#include <filesystem>
#include <format>
#include <memory>

namespace Tetrodotoxin::Parser {

struct Context {
  Context(Types::Library& library,
          std::filesystem::path source_map,
          Errors& errors)
      : library(library),
        tokenizer(library.tokenizer),
        errors(errors),
        source_map(std::filesystem::relative(source_map)),
        disk_path(std::filesystem::absolute(source_map)),
        source(tokenizer.get_source().get_view()) {
    start_token = tokenizer.get_tokens().data();
    current_token = start_token;
    terminal_token = start_token + tokenizer.get_tokens().size() - 1;
  };

  inline auto operator[](uint32_t index) const -> const Lexical::Token& {
    return tokenizer.get_tokens().data()[index];
  }

  inline auto get_allocator() -> Perimortem::Memory::Arena& {
    return library.allocator;
  }

  inline auto get_line_range(Lexical::Token& begin_token, Lexical::Token& end_token) const
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

  inline auto get_error_range(Lexical::Token& begin_token, Lexical::Token& end_token) const
      -> uint32_t {
    return (end_token.location.column + end_token.data.get_size()) -
           begin_token.location.column;
  }

  inline auto advance() -> const Lexical::Token& {
    current_token = std::min(current_token + 1, terminal_token);
    return *current_token;
  }

  inline auto advance_past(Lexical::Classifier target) -> const Lexical::Token& {
    while (current().klass != target &&
           current().klass != Lexical::Classifier::EndOfStream)
      advance();

    // Advance one past the target
    return advance();
  }

  // Advances until the current classifier is balanced.
  // Useful for quickly balancing parse errors on scopes.
  inline auto advance_balanced(Lexical::Classifier open,
                               Lexical::Classifier close,
                               Lexical::ClassifierFlags terminals,
                               int start_count = 1) -> const Lexical::Token& {
    while (start_count > 0 && current().klass != Lexical::Classifier::EndOfStream &&
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
    return ((uint64_t)current_token - (uint64_t)start_token) / sizeof(Lexical::Token);
  }

  inline auto check_klass(Lexical::Classifier expected, Lexical::Classifier actual) const
      -> bool {
    if (expected == actual)
      return true;

    token_error(std::format("Expected {} but got {}", Lexical::klass_name(expected),
                            Lexical::klass_name(actual)));
    return false;
  }

  inline auto check_klass(Lexical::Classifier expected) const -> bool {
    if (expected == current().klass)
      return true;

    token_error(std::format("Expected {} but got {}", Lexical::klass_name(expected),
                            Lexical::klass_name(current().klass)));
    return false;
  }

  inline auto generic_error(const std::string& details) const -> void {
    errors.push_back(Error(source_map, details, source));
  }

  inline auto token_error(const std::string& details) const -> void {
    // Try to look back one token if we are at EoF.
    int32_t offset = current().klass == Lexical::Classifier::EndOfStream ? 1 : 0;
    range_error(details, *(current_token - offset), *(current_token - offset),
                *(current_token - offset));
  }

  inline auto range_error(const std::string_view& details,
                          const Lexical::Token& begin_token,
                          const Lexical::Token& end_token) const -> void {
    range_error(details, begin_token, begin_token, end_token);
  }

  inline auto range_error(std::string_view details,
                          const Lexical::Token& token,
                          const Lexical::Token& begin_token,
                          const Lexical::Token& end_token) const -> void {
    std::optional<Lexical::Location> loc = std::nullopt;
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
      error_range =
          end_token.location.parse_index - begin_token.location.source_index;
    }

    errors.push_back(
        Error(source_map, details, source, loc, line_range, error_range));
  }

  inline auto current() const -> const Lexical::Token& { return *current_token; }

  Types::Library& library;
  const Lexical::Tokenizer& tokenizer;
  Errors& errors;
  const std::filesystem::path source_map;
  const std::filesystem::path disk_path;
  const std::string_view source;

 private:
  const Lexical::Token* current_token;
  const Lexical::Token* terminal_token;
  const Lexical::Token* start_token;
};

}  // namespace Tetrodotoxin::Parser