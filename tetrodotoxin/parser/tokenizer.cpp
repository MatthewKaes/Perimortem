// Perimortem Engine
// Copyright © Matt Kaes

#include "tokenizer.hpp"

#include "concepts/narrow_resolver.hpp"

#include <cmath>
#include <cstring>
#include <sstream>

using namespace Perimortem::Concepts;
using namespace Tetrodotoxin::Language::Parser;

constexpr auto is_whitespace(uint8_t c) -> bool {
  switch (c) {
    // Skip whitespace
    case ' ':
    case '\t':
    case '\n':
      return true;
  }

  return false;
}

constexpr auto is_attribute(uint8_t c) -> bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr auto is_class(uint8_t c) -> bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9');
}

constexpr auto is_identifier(uint8_t c) -> bool {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

constexpr auto is_num(uint8_t c) -> bool {
  return (c >= '0' && c <= '9') || c == '.';
}

// Used for tracking during parsing
struct Context {
  Context(const std::string_view& source, TokenStream& tokens)
      : source(source), tokens(tokens) {};

  Location loc;
  const std::string_view& source;
  TokenStream& tokens;
  Perimortem::Concepts::BitFlag<TtxState> options;
};

inline auto can_parse(Context& context) -> bool {
  return context.loc.parse_index < context.source.size();
}

auto peak_ahead(Context& context, uint32_t amount) -> Byte {
  if (context.loc.parse_index + amount >= context.source.size())
    return 0;

  return context.source[context.loc.parse_index + amount];
}

auto parse_attribute(Context& context) -> void {
  // Consume all the valid tokens greedily
  while (is_attribute(peak_ahead(context, 1)))
    context.loc.parse_index++;

  context.loc.parse_index++;
  const auto token = context.source.substr(
      context.loc.source_index + 1 /* skip @ */,
      context.loc.parse_index - context.loc.source_index - 1);

  // Compiler configuration
  if (!context.options.has(TtxState::DisableCommands)) {
    constexpr const char color_flag[] = "UseCppTheme";
    if (token.size() == sizeof(color_flag) - 1 &&
        token.data()[0] == color_flag[0] &&
        !std::memcmp(token.data(), color_flag, sizeof(color_flag) - 1)) {
      context.options += TtxState::CppTheme;
    }
  }

  if (!token.empty())
    context.tokens.push_back({Classifier::Attribute, token, context.loc});

  context.loc.column += 1 /* @ */ + token.size();
}

auto parse_comment(Context& context) -> void {
  context.loc.source_index = context.loc.parse_index;

  context.loc.parse_index += 2;  // trim comment "//"

  uint32_t start_comment = context.loc.parse_index;

  while (can_parse(context) && context.source[context.loc.parse_index] != '\n')
    context.loc.parse_index++;

  context.tokens.push_back(
      {Classifier::Comment,
       context.source.substr(start_comment,
                             context.loc.parse_index - start_comment),
       context.loc});

  // No need for column validation as we are about to end the file or start a
  // new line.
}

auto recursive_strip(Context& context) {
  while (can_parse(context)) {
    switch (context.source[context.loc.parse_index++]) {
      case '\n':
        context.loc.line += 1;
        break;
      case '}':
        return;
      case '{':
        recursive_strip(context);
        break;
      default:
        break;
    }
  }
}

auto parse_disabled(Context& context, bool strip_disabled) -> void {
  context.loc.source_index = context.loc.parse_index;
  context.loc.parse_index += 2;  // "/>"

  // Strip disabled lines if requested.
  if (!strip_disabled) {
    context.tokens.push_back(
        {Classifier::Disabled,
         context.source.substr(
             context.loc.source_index,
             context.loc.parse_index - context.loc.source_index),
         context.loc});
    context.loc.column += 2;
    context.options += TtxState::DisableCommands;
  } else {
    // The parser pass is more expensive than tokenization so if we can strip
    // out disabled code then optimize by removing the tokens.
    while (can_parse(context) &&
           context.source[context.loc.parse_index] != '\n') {
      if (context.source[context.loc.parse_index] == '{') {
        recursive_strip(context);
        continue;
      }

      context.loc.parse_index++;
    }

    // No need for column validation as we are about to end the file or start a
    // new line.
  }
}

auto parse_number(Context& context) -> void {
  bool found_dec = false;
  Classifier klass = Classifier::Numeric;

  char val = peak_ahead(context, 1);
  while (is_num(val)) {
    context.loc.parse_index++;
    if (val == '.') {
      if (found_dec)
        break;

      found_dec = true;
      klass = Classifier::Float;
    }

    val = peak_ahead(context, 1);
  }

  // Consume the peak_ahead
  context.loc.parse_index++;
  context.tokens.push_back({klass,
                            context.source.substr(context.loc.source_index,
                                                  context.loc.parse_index -
                                                      context.loc.source_index),
                            context.loc});
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

auto parse_type(Context& context) -> void {
  // Can't be a number since that's handle in the main switch.
  while (is_class(peak_ahead(context, 1)))
    context.loc.parse_index++;

  // Consume the peak_ahead
  context.loc.parse_index++;
  context.tokens.push_back({Classifier::Type,
                            context.source.substr(context.loc.source_index,
                                                  context.loc.parse_index -
                                                      context.loc.source_index),
                            context.loc});
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

static inline constexpr auto check_keyword(std::string_view value,
                                           Classifier default_value)
    -> Classifier {
  static constexpr TablePair<std::string_view, Classifier> data[] = {
      {"as", Classifier::As},           {"if", Classifier::If},
      {"for", Classifier::For},         {"new", Classifier::New},
      {"else", Classifier::Else},       {"func", Classifier::Func},
      {"init", Classifier::Init},       {"self", Classifier::Self},
      {"true", Classifier::True},       {"alis", Classifier::Alias},
      {"debug", Classifier::Debug},     {"error", Classifier::Error},
      {"false", Classifier::False},     {"using", Classifier::Using},
      {"while", Classifier::While},     {"entity", Classifier::Entity},
      {"object", Classifier::Object},   {"return", Classifier::Return},
      {"struct", Classifier::Struct},   {"library", Classifier::Library},
      {"on_load", Classifier::OnLoad},  {"package", Classifier::Package},
      {"warning", Classifier::Warning},
  };

  using keyword_resolver = NarrowResolver<Classifier, array_size(data), data>;
  static_assert(sizeof(keyword_resolver::sparse_table) <= 4800,
                "Keyword sparse table should be less than 4800 bytes. "
                "Use keywords only 8 characters or shorter.");

  return keyword_resolver::find_or_default(value, default_value);
}

template <bool forced_identifier>
auto parse_identifier(Context& context) -> void {
  // Eat any unknown tokens including whitespace
  if (!forced_identifier) {
    if (!is_identifier(peak_ahead(context, 0))) {
      context.loc.column++;
      context.loc.parse_index++;
      return;
    }
  }

  while (is_identifier(peak_ahead(context, 1)))
    context.loc.parse_index++;

  // Consume the peak_ahead
  context.loc.parse_index++;
  const auto view =
      context.source.substr(context.loc.source_index,
                            context.loc.parse_index - context.loc.source_index);

  // Can't be a number since that's handle in the main switch.
  Classifier klass = context.options.has(TtxState::ParamTokenizing)
                         ? Classifier::Parameter
                         : Classifier::Identifier;

  if (!forced_identifier) {
    // Check if we need to reclass as a keyword.
    klass =
        check_keyword(std::string_view((char*)view.data(), view.size()), klass);

    if (klass == Classifier::Func)
      context.options += TtxState::ParamTokenizing;
  }

  context.tokens.push_back({klass, view, context.loc});
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

#define SIMPLE_TOKEN(klass, len)                                  \
  context.loc.parse_index += len;                                 \
  tokens.push_back({Classifier::klass,                            \
                    source.substr(context.loc.source_index, len), \
                    context.loc});                                \
  context.loc.column += len;

#define PARSE_SIMPLE(token, klass) \
  case token:                      \
    SIMPLE_TOKEN(klass, 1);        \
    break;

Tokenizer::Tokenizer(const std::string_view& source_, bool strip_disabled)
    : source(source_) {
  // Take an estimated best guess on the token count. This helps save on
  // resizes even if we end up oversized.
  // Assume larger files are more likely to have comments and have a
  // lower proportion of actual tokens.
  tokens.reserve(pow(source.size(), 0.8));
  constexpr const uint32_t tag_size = sizeof("[***]") - 1;

  // Tokenizer Loop
  Context context(source, tokens);
  while (context.loc.parse_index < source.size()) {
    context.loc.source_index = context.loc.parse_index;
    switch (source[context.loc.parse_index]) {
      // Newline handling
      case '\n':
        context.loc.line++;
        context.loc.column = 1;
        context.loc.parse_index++;
        context.options -= TtxState::DisableCommands;
        break;

        // Comment or divide
      case '/':
        if (peak_ahead(context, 1) == '/') {
          parse_comment(context);
          break;
        } else if (peak_ahead(context, 1) == '>') {
          parse_disabled(context, strip_disabled);
          break;
        } else {
          SIMPLE_TOKEN(DivOp, 1);
          break;
        }

        // Call, sub assign, or sub
      case '-':
        if (peak_ahead(context, 1) == '>') {
          SIMPLE_TOKEN(CallOp, 2);
          break;
        } else if (peak_ahead(context, 1) == '=') {
          SIMPLE_TOKEN(SubAssign, 2);
          break;
        } else {
          SIMPLE_TOKEN(SubOp, 1);
          break;
        }

        // Add assign or add
      case '+':
        if (peak_ahead(context, 1) == '=') {
          SIMPLE_TOKEN(AddAssign, 2);
          break;
        } else {
          SIMPLE_TOKEN(AddOp, 1);
          break;
        }

        // Assign or equal
      case '=':
        if (peak_ahead(context, 1) == '=') {
          SIMPLE_TOKEN(CmpOp, 2);
          break;
        } else {
          SIMPLE_TOKEN(Assign, 1);
          break;
        }

        // Less
      case '<':
        if (peak_ahead(context, 1) == '=') {
          SIMPLE_TOKEN(LessEqOp, 2);
          break;
        } else {
          SIMPLE_TOKEN(LessOp, 1);
          break;
        }

        // Greater
      case '>':
        if (peak_ahead(context, 1) == '=') {
          SIMPLE_TOKEN(GreaterEqOp, 2);
          break;
        } else {
          SIMPLE_TOKEN(GreaterOp, 1);
          break;
        }

        // Attribute
      case '@':
        parse_attribute(context);
        break;

        // Tested faster to unroll number parsing here in the switch rather than
        // try and push it to the default.
      case '0' ... '9':
        parse_number(context);
        break;

        // Originally types had @ but it was clunky, so @ was moved to
        // Attributes and all Types are simply capitalized.
      case 'A' ... 'Z':
        parse_type(context);
        break;

        // String
      case '"':
        context.loc.source_index = context.loc.parse_index;
        context.loc.parse_index++;
        while (can_parse(context) && source[context.loc.parse_index] != '\n' &&
               (source[context.loc.parse_index] != '"' ||
                source[context.loc.parse_index - 1] == '\\'))
          context.loc.parse_index++;

        // Consume the closing quote if we found it.
        if (can_parse(context) && source[context.loc.parse_index] == '"') {
          context.loc.parse_index++;  // closing quote
        }

        tokens.push_back(
            {Classifier::String,
             source.substr(context.loc.source_index,
                           context.loc.parse_index - context.loc.source_index),
             context.loc});
        context.loc.column +=
            context.loc.parse_index - context.loc.source_index;
        break;

        // Index start or tags
      case '[':
        if (context.loc.parse_index + tag_size >= source.size()) {
          SIMPLE_TOKEN(IndexStart, 1);
          break;
        } else if (std::memcmp("[***]", source.data() + context.loc.parse_index,
                               tag_size) == 0) {
          SIMPLE_TOKEN(Temporary, tag_size);
          break;
        } else if (std::memcmp("[=>>]", source.data() + context.loc.parse_index,
                               tag_size) == 0) {
          SIMPLE_TOKEN(Dynamic, tag_size);
          break;
        } else if (std::memcmp("[=!=]", source.data() + context.loc.parse_index,
                               tag_size) == 0) {
          SIMPLE_TOKEN(Hidden, tag_size);
          break;
        } else if (std::memcmp("[=/=]", source.data() + context.loc.parse_index,
                               tag_size) == 0) {
          SIMPLE_TOKEN(Constant, tag_size);
          break;
        }

        SIMPLE_TOKEN(IndexStart, 1);
        break;

      case ')':
        context.loc.parse_index += 1;
        tokens.push_back({Classifier::GroupEnd,
                          source.substr(context.loc.source_index, 1),
                          context.loc});
        context.loc.column += 1;
        context.options -=
            TtxState::ParamTokenizing;  // disable function parsing.
        break;

      // leading underscores force an indentifier so we can optimize for that
      // case.
      case '_':
        parse_identifier<true>(context);
        break;

        // Simple spot tokens
        PARSE_SIMPLE('{', ScopeStart);
        PARSE_SIMPLE('}', ScopeEnd);
        PARSE_SIMPLE('(', GroupStart);
        PARSE_SIMPLE(']', IndexEnd);
        PARSE_SIMPLE('*', MulOp);
        PARSE_SIMPLE('%', ModOp);
        PARSE_SIMPLE('&', AndOp);
        PARSE_SIMPLE('|', OrOp);
        PARSE_SIMPLE('.', AccessOp);
        PARSE_SIMPLE('!', NotOp);
        PARSE_SIMPLE(',', Seperator);
        PARSE_SIMPLE(':', Define);
        PARSE_SIMPLE(';', EndStatement);

      default:
        // Parse an identifier that could be a keyword.
        parse_identifier<false>(context);
        break;
    }
  }

  // End of file
  options = context.options;
  context.loc.source_index = ++context.loc.parse_index;
  context.tokens.push_back(
      {Classifier::EndOfStream, std::string_view(), context.loc});
}
