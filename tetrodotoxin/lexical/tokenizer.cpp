// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lexical/tokenizer.hpp"

#include "perimortem/core/perimortem.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lexical;

struct Location {
  Bits_32 source_index = 0;
  Bits_32 parse_index = 0;
  Bits_32 line = 1;
  Bits_32 column = 1;
};

constexpr auto is_whitespace(Bits_8 c) -> Bool {
  switch (c) {
  case ' ':
  case '\t':
  case '\n':
    return true;
  }
  return false;
}

constexpr auto is_attribute(Bits_8 c) -> Bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr auto is_class(Bits_8 c) -> Bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

constexpr auto is_identifier(Bits_8 c) -> Bool {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

constexpr auto is_num(Bits_8 c) -> Bool {
  return (c >= '0' && c <= '9') || c == '.';
}

struct Context {
  Context(
      const View::Bytes source,
      Perimortem::Memory::Managed::Vector<Token>& tokens)
      : source(source), tokens(tokens) {};

  Location loc;
  const View::Bytes source;
  Perimortem::Memory::Managed::Vector<Token>& tokens;
};

inline auto can_parse(Context& context) -> Bool {
  return context.loc.parse_index < context.source.get_size();
}

auto peak_ahead(Context& context, Bits_32 amount) -> Bits_8 {
  if (context.loc.parse_index + amount >= context.source.get_size()) {
    return 0;
  }
  return context.source[context.loc.parse_index + amount];
}

auto parse_attribute(Context& context) -> void {
  while (is_attribute(peak_ahead(context, 1))) {
    context.loc.parse_index++;
  }

  context.loc.parse_index++;
  const auto token = context.source.slice(
      context.loc.source_index,
      context.loc.parse_index - context.loc.source_index);

  if (!token.empty()) {
    context.tokens.insert(Token(
        token, Class::Type::Attribute, context.loc.line, context.loc.column));
  }
  context.loc.column += token.get_size();
}

auto parse_comment(Context& context) -> void {
  context.loc.source_index = context.loc.parse_index;

  // trim comment "//" and leading space.
  context.loc.parse_index += 2;

  // Skip one leading space if present.
  if (can_parse(context) && context.source[context.loc.parse_index] == ' ') {
    context.loc.parse_index++;
  }

  Bits_32 start_comment = context.loc.parse_index;
  while (can_parse(context) &&
         context.source[context.loc.parse_index] != '\n') {
    context.loc.parse_index++;
  }

  context.tokens.insert(Token(
      context.source.slice(
          start_comment, context.loc.parse_index - start_comment),
      Class::Type::Comment, context.loc.line, context.loc.column));
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

auto parse_disabled(Context& context, Bool strip_disabled) -> void {
  context.loc.source_index = context.loc.parse_index;
  context.loc.parse_index += 2;  // "/>"

  if (!strip_disabled) {
    context.tokens.insert(Token(
        context.source.slice(
            context.loc.source_index,
            context.loc.parse_index - context.loc.source_index),
        Class::Type::Disabled, context.loc.line, context.loc.column));
    context.loc.column += 2;
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
  }
}

auto parse_string(Context& context) -> void {
  context.loc.parse_index++;
  while (can_parse(context) &&
         context.source[context.loc.parse_index] != '\n' &&
         (context.source[context.loc.parse_index] != '"' ||
          context.source[context.loc.parse_index - 1] == '\\')) {
    context.loc.parse_index++;
  }
  if (can_parse(context) && context.source[context.loc.parse_index] == '"') {
    context.loc.parse_index++;
  }
  context.tokens.insert(Token(
      context.source.slice(
          context.loc.source_index,
          context.loc.parse_index - context.loc.source_index),
      Class::Type::String, context.loc.line, context.loc.column));
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

auto parse_embedded(Context& context) -> void {
  context.loc.parse_index += 2;  // "$["
  while (can_parse(context) && context.source[context.loc.parse_index] != ']' &&
         context.source[context.loc.parse_index] != '\n') {
    context.loc.parse_index++;
  }
  if (can_parse(context) && context.source[context.loc.parse_index] == ']') {
    context.loc.parse_index++;
  }
  context.tokens.insert(Token(
      context.source.slice(
          context.loc.source_index,
          context.loc.parse_index - context.loc.source_index),
      Class::Type::Embedded, context.loc.line, context.loc.column));
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

auto parse_number(Context& context) -> void {
  // 0x[FF FF ...] hex byte array literal.
  if (context.source[context.loc.source_index] == '0' &&
      peak_ahead(context, 1) == 'x' && peak_ahead(context, 2) == '[') {
    context.loc.parse_index += 3;
    while (can_parse(context) &&
           context.source[context.loc.parse_index] != ']') {
      context.loc.parse_index++;
    }
    if (can_parse(context)) {
      context.loc.parse_index++;
    }
    context.tokens.insert(Token(
        context.source.slice(
            context.loc.source_index,
            context.loc.parse_index - context.loc.source_index),
        Class::Type::Bytes, context.loc.line, context.loc.column));
    context.loc.column += context.loc.parse_index - context.loc.source_index;
    return;
  }

  Bool found_decimal = false;
  Class::Type klass = Class::Type::Numeric;

  char val = peak_ahead(context, 1);
  while (is_num(val)) {
    context.loc.parse_index++;
    if (val == '.') {
      if (found_decimal) {
        break;
      }
      found_decimal = true;
      klass = Class::Type::Float;
    }
    val = peak_ahead(context, 1);
  }

  context.loc.parse_index++;
  context.tokens.insert(Token(
      context.source.slice(
          context.loc.source_index,
          context.loc.parse_index - context.loc.source_index),
      klass, context.loc.line, context.loc.column));
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

auto parse_type(Context& context) -> void {
  while (is_class(peak_ahead(context, 1))) {
    context.loc.parse_index++;
  }

  context.loc.parse_index++;
  context.tokens.insert(Token(
      context.source.slice(
          context.loc.source_index,
          context.loc.parse_index - context.loc.source_index),
      Class::Type::Type, context.loc.line, context.loc.column));
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

static inline constexpr auto check_keyword(
    View::Bytes value,
    Class::Type default_value) -> Class::Type {
  static constexpr Pair<View::Bytes, Class::Type> data[] = {
    {"as"_view, Class::Type::As},
    {"if"_view, Class::Type::If},
    {"in"_view, Class::Type::In},
    {"for"_view, Class::Type::For},
    {"new"_view, Class::Type::New},
    {"case"_view, Class::Type::Case},
    {"else"_view, Class::Type::Else},
    {"enum"_view, Class::Type::Enum},
    {"func"_view, Class::Type::Func},
    {"init"_view, Class::Type::Init},
    {"self"_view, Class::Type::Self},
    {"true"_view, Class::Type::True},
    {"alias"_view, Class::Type::Alias},
    {"false"_view, Class::Type::False},
    {"match"_view, Class::Type::Match},
    {"using"_view, Class::Type::Using},
    {"while"_view, Class::Type::While},
    {"shader"_view, Class::Type::Shader},
    {"entity"_view, Class::Type::Entity},
    {"object"_view, Class::Type::Object},
    {"return"_view, Class::Type::Return},
    {"struct"_view, Class::Type::Struct},
    {"stack"_view, Class::Type::Temporary},
    {"frozen"_view, Class::Type::Constant},
    {"detail"_view, Class::Type::Detail},
    {"public"_view, Class::Type::Public},
    {"expose"_view, Class::Type::Dynamic},
    {"hidden"_view, Class::Type::Hidden},
    {"import"_view, Class::Type::Import},
    {"library"_view, Class::Type::Library},
    {"package"_view, Class::Type::Package},
  };

  using keyword_resolver = Table<Class::Type, data>;

  static_assert(
      keyword_resolver::find_or_default(
          "library"_view, Class::Type::EndOfStream) == Class::Type::Library);

  return keyword_resolver::find_or_default(value, default_value);
}

auto parse_identifier(Context& context) -> void {
  if (!is_identifier(peak_ahead(context, 0))) {
    context.loc.column++;
    context.loc.parse_index++;
    return;
  }

  while (is_identifier(peak_ahead(context, 1))) {
    context.loc.parse_index++;
  }

  context.loc.parse_index++;
  const auto view = context.source.slice(
      context.loc.source_index,
      context.loc.parse_index - context.loc.source_index);

  Class::Type klass = Class::Type::Addressable;
  klass = check_keyword(view, klass);

  context.tokens.insert(
      Token(view, klass, context.loc.line, context.loc.column));
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

#define SIMPLE_TOKEN(klass, len)                                               \
  context.loc.parse_index += len;                                              \
  tokens.insert(Token(                                                         \
      context.source.slice(context.loc.source_index, len), Class::Type::klass, \
      context.loc.line, context.loc.column));                                  \
  context.loc.column += len;

#define PARSE_SIMPLE(token, klass) \
  case token:                      \
    SIMPLE_TOKEN(klass, 1);        \
    break;

auto Tokenizer::parse(const View::Bytes source_, Bool strip_disabled) -> void {
  source = source_;
  tokens.reset();

  // Take an estimated best guess on the token count. This helps save on
  // resizes even if we end up oversized.
  tokens.reset(source.get_size() >> 2);
  Context context(source, tokens);
  while (context.loc.parse_index < source.get_size()) {
    context.loc.source_index = context.loc.parse_index;
    switch (source[context.loc.parse_index]) {
    case '\n':
      context.loc.line++;
      context.loc.column = 1;
      context.loc.parse_index++;
      break;

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

    case '+':
      if (peak_ahead(context, 1) == '=') {
        SIMPLE_TOKEN(AddAssign, 2);
        break;
      } else {
        SIMPLE_TOKEN(AddOp, 1);
        break;
      }

    case '=':
      if (peak_ahead(context, 1) == '=') {
        SIMPLE_TOKEN(CmpOp, 2);
        break;
      } else if (peak_ahead(context, 1) == '>') {
        SIMPLE_TOKEN(GeneratorOp, 2);
        break;
      } else {
        SIMPLE_TOKEN(Assign, 1);
        break;
      }

    case '<':
      if (peak_ahead(context, 1) == '=') {
        SIMPLE_TOKEN(LessEqOp, 2);
        break;
      } else {
        SIMPLE_TOKEN(LessOp, 1);
        break;
      }

    case '>':
      if (peak_ahead(context, 1) == '=') {
        SIMPLE_TOKEN(GreaterEqOp, 2);
        break;
      } else {
        SIMPLE_TOKEN(GreaterOp, 1);
        break;
      }

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

    case '"':
      parse_string(context);
      break;

    case '$':
      if (peak_ahead(context, 1) == '[') {
        parse_embedded(context);
      }
      break;

    case '[':
      SIMPLE_TOKEN(IndexStart, 1);
      break;

    case ']':
      context.loc.parse_index += 1;
      tokens.insert(Token(
          context.source.slice(context.loc.source_index, 1),
          Class::Type::IndexEnd, context.loc.line, context.loc.column));
      context.loc.column += 1;
      break;

    case ')':
      context.loc.parse_index += 1;
      tokens.insert(Token(
          context.source.slice(context.loc.source_index, 1),
          Class::Type::GroupEnd, context.loc.line, context.loc.column));
      context.loc.column += 1;
      break;

    case '.':
      if (peak_ahead(context, 1) == '[') {
        SIMPLE_TOKEN(SwizzleOp, 2);
        break;
      } else if (
          peak_ahead(context, 1) == '.' && peak_ahead(context, 2) == '.') {
        SIMPLE_TOKEN(RangeOp, 3);
        break;
      } else {
        SIMPLE_TOKEN(AddressOp, 1);
        break;
      }

      // Simple spot tokens
      PARSE_SIMPLE('{', ScopeStart);
      PARSE_SIMPLE('}', ScopeEnd);
      PARSE_SIMPLE('(', GroupStart);
      PARSE_SIMPLE('*', MulOp);
      PARSE_SIMPLE('%', ModOp);
      PARSE_SIMPLE('&', AndOp);
      PARSE_SIMPLE('|', OrOp);
      PARSE_SIMPLE('!', NotOp);
      PARSE_SIMPLE(',', PackingOp);
      PARSE_SIMPLE(':', Define);
      PARSE_SIMPLE(';', EndStatement);
      PARSE_SIMPLE('_', Discard);

    default:
      parse_identifier(context);
      break;
    }
  }

  context.loc.source_index = ++context.loc.parse_index;
  context.tokens.insert(Token(
      View::Bytes(), Class::Type::EndOfStream, context.loc.line,
      context.loc.column));
}
