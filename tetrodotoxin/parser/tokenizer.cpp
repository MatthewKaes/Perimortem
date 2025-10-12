// Perimortem Engine
// Copyright © Matt Kaes

#include "tokenizer.hpp"

#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <unordered_set>

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
  Context(const ByteView& source, TokenStream& tokens)
      : source(source), tokens(tokens) {};

  Location loc;
  const ByteView& source;
  TokenStream& tokens;
  Perimortem::Concepts::BitFlag<TtxState, uint64_t> options;
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
  const auto token = context.source.subspan(
      context.loc.source_index + 1 /* skip @ */,
      context.loc.parse_index - context.loc.source_index - 1);

  // Compiler configuration
  constexpr const char color_flag[] = "UseCppTheme";
  if (token.size() == sizeof(color_flag) - 1 &&
      token.data()[0] == color_flag[0] &&
      !std::memcmp(token.data(), color_flag, sizeof(color_flag) - 1)) {
    context.options += TtxState::CppTheme;
  }

  if (!token.empty())
    context.tokens.push_back({Classifier::Attribute, token, context.loc});

  context.loc.column += 1 /* @ */ + token.size();
}

auto parse_comment(Context& context) -> void {
  context.loc.source_index = context.loc.parse_index;

  context.loc.parse_index += 2;  // trim comment "//"

  // If there is a default space (which is standard) consume it from the
  // comment.
  if (peak_ahead(context, 0) == ' ')
    context.loc.parse_index++;

  uint32_t start_comment = context.loc.parse_index;

  while (can_parse(context) && context.source[context.loc.parse_index] != '\n')
    context.loc.parse_index++;

  context.tokens.push_back(
      {Classifier::Comment,
       context.source.subspan(start_comment,
                              context.loc.parse_index - start_comment),
       context.loc});

  // No need for column validation as we are about to end the file or start a
  // new line.
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
  context.tokens.push_back(
      {klass,
       context.source.subspan(
           context.loc.source_index,
           context.loc.parse_index - context.loc.source_index),
       context.loc});
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

auto parse_type(Context& context) -> void {
  // Can't be a number since that's handle in the main switch.
  while (is_class(peak_ahead(context, 1)))
    context.loc.parse_index++;

  // Consume the peak_ahead
  context.loc.parse_index++;
  context.tokens.push_back(
      {Classifier::Type,
       context.source.subspan(
           context.loc.source_index,
           context.loc.parse_index - context.loc.source_index),
       context.loc});
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

#define K_SECTION(size) \
  case size: {          \
    constexpr uint64_t section_bytes = size;
#define K_END(size) \
  break;            \
  }

#define KEYWORD_RECLASS(keyword)                                   \
  static_assert(sizeof(#keyword) - 1 == section_bytes);            \
  if (view.data()[0] == #keyword[0] &&                             \
      !std::memcmp(view.data(), #keyword, sizeof(#keyword) - 1)) { \
    klass = Classifier::K_##keyword;                               \
    break;                                                         \
  }

auto parse_identifier(Context& context) -> void {
  // Eat any unknown tokens including whitespace
  if (!is_identifier(peak_ahead(context, 0))) {
    context.loc.column++;
    context.loc.parse_index++;
    return;
  }

  // Can't be a number since that's handle in the main switch.
  Classifier klass = context.options.has(TtxState::ParamTokenizing) ? Classifier::Parameter : Classifier::Identifier;
  while (is_identifier(peak_ahead(context, 1)))
    context.loc.parse_index++;

  // Consume the peak_ahead
  context.loc.parse_index++;
  const auto view = context.source.subspan(
      context.loc.source_index,
      context.loc.parse_index - context.loc.source_index);
  switch (view.size()) {
    K_SECTION(2)
    KEYWORD_RECLASS(if)
    K_END()
    K_SECTION(3)
    KEYWORD_RECLASS(for)
    KEYWORD_RECLASS(new)
    K_END()
    K_SECTION(4)
    KEYWORD_RECLASS(else)
    KEYWORD_RECLASS(from)
    KEYWORD_RECLASS(true)
    static_assert(sizeof("func") - 1 == section_bytes);
    if (view.data()[0] == "func"[0] &&
        !std::memcmp(view.data(), "func", sizeof("func") - 1)) {
      klass = Classifier::K_func;
      context.options += TtxState::ParamTokenizing;  // enable function parsing
      break;
    }
    KEYWORD_RECLASS(type)
    KEYWORD_RECLASS(init)
    KEYWORD_RECLASS(this)
    K_END()
    K_SECTION(5)
    KEYWORD_RECLASS(false)
    KEYWORD_RECLASS(error)
    KEYWORD_RECLASS(debug)
    K_END()
    K_SECTION(6)
    KEYWORD_RECLASS(return)
    KEYWORD_RECLASS(import)
    K_END()
    K_SECTION(7)
    KEYWORD_RECLASS(library)
    KEYWORD_RECLASS(on_load)
    K_END()
  }

  context.tokens.push_back({klass, view, context.loc});
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

#define SIMPLE_TOKEN(klass, len)                                   \
  context.loc.parse_index += len;                                  \
  tokens.push_back({Classifier::klass,                             \
                    source.subspan(context.loc.source_index, len), \
                    context.loc});                                 \
  context.loc.column += len;

#define PARSE_SIMPLE(token, klass) \
  case token:                      \
    SIMPLE_TOKEN(klass, 1);        \
    break;

Tokenizer::Tokenizer(const ByteView& source) {
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
        break;

        // Comment or divide
      case '/':
        if (peak_ahead(context, 1) == '/') {
          parse_comment(context);
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
        if (source[context.loc.parse_index] == '"') {
          context.loc.parse_index++;  // closing quote
        }

        tokens.push_back(
            {Classifier::String,
             source.subspan(context.loc.source_index,
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

        // Simple spot tokens
        PARSE_SIMPLE('{', ScopeStart);
        PARSE_SIMPLE('}', ScopeEnd);
        PARSE_SIMPLE('(', GroupStart);
      case ')':
        context.loc.parse_index += 1;
        tokens.push_back({Classifier::GroupEnd,
                          source.subspan(context.loc.source_index, 1),
                          context.loc});
        context.loc.column += 1;
        context.options -=
            TtxState::ParamTokenizing;  // disable function parsing.
        break;
        PARSE_SIMPLE(']', IndexEnd);
        // PARSE_SIMPLE('+', AddOp); // Can't be simple due to +=
        // PARSE_SIMPLE('-', SubOp); // Can't be simple due to calls -> and -=
        // PARSE_SIMPLE('/', DivOp); // Can't be simple if comments are //
        PARSE_SIMPLE('*', MulOp);
        PARSE_SIMPLE('%', ModOp);
        PARSE_SIMPLE('&', AndOp);
        PARSE_SIMPLE('|', OrOp);
        PARSE_SIMPLE('.', AccessOp);
        PARSE_SIMPLE('!', NotOp);
        PARSE_SIMPLE('?', ValidOp);
        PARSE_SIMPLE(',', Seperator);
        PARSE_SIMPLE(':', Define);
        PARSE_SIMPLE(';', EndStatement);

      default:
        parse_identifier(context);
        break;
    }
  }

  // End of file
  options = context.options;
  context.loc.source_index = ++context.loc.parse_index;
  context.tokens.push_back({Classifier::EndOfStream, ByteView(), context.loc});
}

#define CLASS_DUMP(klass)                                \
  case Classifier::klass:                                \
    info_stream << std::left << std::setw(12) << #klass; \
    break;

auto Tokenizer::dump_tokens() -> std::string {
  std::stringstream info_stream;
  info_stream << "Token stream size: " << tokens.size() << std::endl;
  info_stream << std::left << std::setw(12) << "KLASS";
  info_stream << std::left << std::setw(8) << "LINE";
  info_stream << std::left << std::setw(8) << "COLUMN";
  info_stream << std::left << "  DATA" << std::endl;
  for (const auto& token : tokens) {
    switch (token.klass) {
      CLASS_DUMP(Comment)
      CLASS_DUMP(String)
      CLASS_DUMP(Numeric)
      CLASS_DUMP(Float)
      CLASS_DUMP(Attribute)
      CLASS_DUMP(Identifier)
      CLASS_DUMP(Type)
      CLASS_DUMP(ScopeStart)
      CLASS_DUMP(ScopeEnd)
      CLASS_DUMP(GroupStart)
      CLASS_DUMP(GroupEnd)
      CLASS_DUMP(IndexStart)
      CLASS_DUMP(IndexEnd)
      CLASS_DUMP(Seperator)
      CLASS_DUMP(Assign)
      CLASS_DUMP(AddAssign)
      CLASS_DUMP(SubAssign)
      CLASS_DUMP(Define)
      CLASS_DUMP(EndStatement)
      CLASS_DUMP(AddOp)
      CLASS_DUMP(SubOp)
      CLASS_DUMP(DivOp)
      CLASS_DUMP(MulOp)
      CLASS_DUMP(ModOp)
      CLASS_DUMP(LessOp)
      CLASS_DUMP(GreaterOp)
      CLASS_DUMP(LessEqOp)
      CLASS_DUMP(GreaterEqOp)
      CLASS_DUMP(CmpOp)
      CLASS_DUMP(CallOp)
      CLASS_DUMP(AccessOp)
      CLASS_DUMP(AndOp)
      CLASS_DUMP(OrOp)
      CLASS_DUMP(NotOp)
      CLASS_DUMP(ValidOp)
      CLASS_DUMP(K_this)
      CLASS_DUMP(K_if)
      CLASS_DUMP(K_for)
      CLASS_DUMP(K_else)
      CLASS_DUMP(K_return)
      CLASS_DUMP(K_func)
      CLASS_DUMP(K_type)
      CLASS_DUMP(K_import)
      CLASS_DUMP(K_from)
      CLASS_DUMP(K_library)
      CLASS_DUMP(K_debug)
      CLASS_DUMP(K_warning)
      CLASS_DUMP(K_error)
      CLASS_DUMP(K_true)
      CLASS_DUMP(K_false)
      CLASS_DUMP(K_new)
      CLASS_DUMP(K_init)
      default:
        // Unknown
        break;
    }

    info_stream << std::right << std::setw(8) << token.location.line;
    info_stream << std::right << std::setw(8) << token.location.column;
    info_stream << "  \""
                << std::string_view((char*)token.data.data(), token.data.size())
                << "\"" << std::endl;
  }

  return info_stream.str();
}