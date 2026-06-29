// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/lexical/tokenizer.hpp"

#include "perimortem/core/static/vector.hpp"
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

constexpr auto is_hex(Bits_8 c) -> Bool {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

using ClassMapping = Pair<View::Bytes, Class::Type>;

static constexpr auto check_keyword(
    View::Bytes value,
    Class::Type default_value) -> Class::Type {
  static constexpr Static::Vector<ClassMapping, 25> data = {{
    {Class::get_source_text(Class::Type::If), Class::Type::If},
    {Class::get_source_text(Class::Type::In), Class::Type::In},
    {Class::get_source_text(Class::Type::Or), Class::Type::Or},
    {Class::get_source_text(Class::Type::And), Class::Type::And},
    {Class::get_source_text(Class::Type::For), Class::Type::For},
    {Class::get_source_text(Class::Type::Break), Class::Type::Break},
    {Class::get_source_text(Class::Type::Continue), Class::Type::Continue},
    {Class::get_source_text(Class::Type::New), Class::Type::New},
    {Class::get_source_text(Class::Type::Case), Class::Type::Case},
    {Class::get_source_text(Class::Type::Else), Class::Type::Else},
    {
      Class::get_source_text(Class::Type::External),
      Class::Type::External,
    },
    {Class::get_source_text(Class::Type::Init), Class::Type::Init},
    {Class::get_source_text(Class::Type::Self), Class::Type::Self},
    {Class::get_source_text(Class::Type::True), Class::Type::True},
    {Class::get_source_text(Class::Type::Func), Class::Type::Func},
    {Class::get_source_text(Class::Type::False), Class::Type::False},
    {Class::get_source_text(Class::Type::Match), Class::Type::Match},
    {Class::get_source_text(Class::Type::While), Class::Type::While},
    {
      Class::get_source_text(Class::Type::Return),
      Class::Type::Return,
    },
    {
      Class::get_source_text(Class::Type::Temporary),
      Class::Type::Temporary,
    },
    {
      Class::get_source_text(Class::Type::Public),
      Class::Type::Public,
    },
    {
      Class::get_source_text(Class::Type::Dynamic),
      Class::Type::Dynamic,
    },
    {
      Class::get_source_text(Class::Type::Hidden),
      Class::Type::Hidden,
    },
    {Class::get_source_text(Class::Type::Import), Class::Type::Import},
    {
      Class::get_source_text(Class::Type::Package),
      Class::Type::Package,
    },
  }};

  using keyword_resolver = Table<Class::Type, data>;
  return keyword_resolver::find_or_default(value, default_value);
}

static constexpr auto check_builtins(
    View::Bytes value,
    Class::Type default_value) -> Class::Type {
  static constexpr Static::Vector<ClassMapping, 8> data = {{
    {Class::get_source_text(Class::Type::Enum), Class::Type::Enum},
    {Class::get_source_text(Class::Type::Alias), Class::Type::Alias},
    {Class::get_source_text(Class::Type::Shader), Class::Type::Shader},
    {Class::get_source_text(Class::Type::Entity), Class::Type::Entity},
    {Class::get_source_text(Class::Type::Object), Class::Type::Object},
    {Class::get_source_text(Class::Type::Struct), Class::Type::Struct},
    {
      Class::get_source_text(Class::Type::Foreign),
      Class::Type::Foreign,
    },
    {
      Class::get_source_text(Class::Type::Library),
      Class::Type::Library,
    },
  }};

  using builtin_resolver = Table<Class::Type, data>;
  return builtin_resolver::find_or_default(value, default_value);
}

static constexpr auto check_directive(
    View::Bytes value,
    Class::Type default_value) -> Class::Type {
  static constexpr Static::Vector<ClassMapping, 5> data = {{
    {
      Class::get_source_text(Class::Type::CompileIf),
      Class::Type::CompileIf,
    },
    {
      Class::get_source_text(Class::Type::ConstPublic),
      Class::Type::ConstPublic,
    },
    {
      Class::get_source_text(Class::Type::ConstDynamic),
      Class::Type::ConstDynamic,
    },
    {
      Class::get_source_text(Class::Type::ConstHidden),
      Class::Type::ConstHidden,
    },
    {
      Class::get_source_text(Class::Type::ConstTemporary),
      Class::Type::ConstTemporary,
    },
  }};

  using directive_resolver = Table<Class::Type, data>;
  return directive_resolver::find_or_default(value, default_value);
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

auto peek_ahead(Context& context, Bits_32 amount) -> Bits_8 {
  if (context.loc.parse_index + amount >= context.source.get_size()) {
    return 0;
  }
  return context.source[context.loc.parse_index + amount];
}

auto parse_attribute(Context& context) -> void {
  while (is_attribute(peek_ahead(context, 1))) {
    context.loc.parse_index++;
  }

  context.loc.parse_index++;
  const auto token = context.source.slice(
      context.loc.source_index,
      context.loc.parse_index - context.loc.source_index);

  if (!token.is_empty()) {
    Class::Type klass = check_directive(token, Class::Type::Attribute);
    context.tokens.insert(
        Token(token, klass, context.loc.line, context.loc.column));
  }
  context.loc.column += token.get_size();
}

auto parse_comment(Context& context) -> void {
  context.loc.source_index = context.loc.parse_index;

  // trim comment marker and leading space.
  context.loc.parse_index +=
      Class::get_source_text(Class::Type::Comment).get_size();

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

auto recursive_strip(Context& context) -> void {
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
  Count marker_size = Class::get_source_text(Class::Type::Disabled).get_size();
  context.loc.parse_index += marker_size;

  if (!strip_disabled) {
    context.tokens.insert(Token(
        context.source.slice(
            context.loc.source_index,
            context.loc.parse_index - context.loc.source_index),
        Class::Type::Disabled, context.loc.line, context.loc.column));
    context.loc.column += marker_size;
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

auto string_quote_is_escaped(View::Bytes source, Count position) -> Bool {
  Count slash_count = 0;

  while (position > 0) {
    position--;
    if (source[position] != '\\') {
      break;
    }

    slash_count++;
  }

  return (slash_count & 1) != 0;
}

auto parse_string(Context& context) -> void {
  context.loc.parse_index++;
  while (can_parse(context) &&
         context.source[context.loc.parse_index] != '\n' &&
         (context.source[context.loc.parse_index] != '"' ||
          string_quote_is_escaped(context.source, context.loc.parse_index))) {
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
  context.loc.parse_index +=
      Class::get_source_text(Class::Type::Embedded).get_size();
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
  // If we start with zero then check if we have a valid hex sequence.
  if (context.source[context.loc.source_index] == '0' &&
      peek_ahead(context, 1) == 'x') {
    switch (peek_ahead(context, 2)) {
    // 0x[FF FF ...] hex byte array literal (any whitespace is fine)
    case '[': {
      context.loc.parse_index +=
          Class::get_source_text(Class::Type::Bytes).get_size();
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

      // 0xFF... hex integer literal.
    case '0' ... '9':
    case 'a' ... 'f':
    case 'A' ... 'F': {
      context.loc.parse_index += 2;  // consume '0x'
      Bits_8 h = peek_ahead(context, 1);
      while (is_hex(h)) {
        context.loc.parse_index++;
        h = peek_ahead(context, 1);
      }
      context.loc.parse_index++;
      context.tokens.insert(Token(
          context.source.slice(
              context.loc.source_index,
              context.loc.parse_index - context.loc.source_index),
          Class::Type::Numeric, context.loc.line, context.loc.column));
      context.loc.column += context.loc.parse_index - context.loc.source_index;
      return;
    }

      // Not a valid hex sequence
    default:
      break;
    }
  }

  Bool found_decimal = false;
  Class::Type klass = Class::Type::Numeric;

  char val = peek_ahead(context, 1);
  while (is_num(val)) {
    context.loc.parse_index++;
    if (val == '.') {
      // Don't consume a '.' that starts a RangeOp '...'.
      if (peek_ahead(context, 1) == '.') {
        context.loc.parse_index--;
        break;
      }
      if (found_decimal) {
        context.loc.parse_index--;
        break;
      }
      found_decimal = true;
      klass = Class::Type::Float;
    }
    val = peek_ahead(context, 1);
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
  while (is_class(peek_ahead(context, 1))) {
    context.loc.parse_index++;
  }

  context.loc.parse_index++;
  const auto view = context.source.slice(
      context.loc.source_index,
      context.loc.parse_index - context.loc.source_index);

  // Start by assuming it's a Type and isn't a compiler provided symbol.
  Class::Type klass = Class::Type::Type;
  klass = check_builtins(view, klass);

  context.tokens.insert(
      Token(view, klass, context.loc.line, context.loc.column));
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

auto parse_unknown(Context& context) -> void {
  context.tokens.insert(Token(
      context.source.slice(context.loc.source_index, 1), Class::Type::Unknown,
      context.loc.line, context.loc.column));
  context.loc.column++;
  context.loc.parse_index++;
}

auto parse_identifier(Context& context) -> void {
  if (!is_identifier(peek_ahead(context, 0))) {
    parse_unknown(context);
    return;
  }

  while (is_identifier(peek_ahead(context, 1))) {
    context.loc.parse_index++;
  }

  context.loc.parse_index++;
  const auto view = context.source.slice(
      context.loc.source_index,
      context.loc.parse_index - context.loc.source_index);

  // Start by assuming the addressable isn't a compiler provided symbol.
  Class::Type klass = Class::Type::Addressable;
  klass = check_keyword(view, klass);

  context.tokens.insert(
      Token(view, klass, context.loc.line, context.loc.column));
  context.loc.column += context.loc.parse_index - context.loc.source_index;
}

template <Class::Type klass>
auto parse_simple(Context& context) -> void {
  constexpr Count len = Class::get_source_text(klass).get_size();
  context.loc.parse_index += len;
  context.tokens.insert(Token(
      context.source.slice(context.loc.source_index, len), klass,
      context.loc.line, context.loc.column));
  context.loc.column += len;
}

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

    case ' ':
    case '\t':
    case '\r':
      context.loc.column++;
      context.loc.parse_index++;
      break;

    case '/':
      if (peek_ahead(context, 1) == '/') {
        parse_comment(context);
        break;
      } else if (peek_ahead(context, 1) == '>') {
        parse_disabled(context, strip_disabled);
        break;
      } else {
        parse_simple<Class::Type::DivOp>(context);
        break;
      }

    case '-':
      if (peek_ahead(context, 1) == '>') {
        parse_simple<Class::Type::CallOp>(context);
        break;
      } else if (peek_ahead(context, 1) == '=') {
        parse_simple<Class::Type::SubAssign>(context);
        break;
      } else {
        parse_simple<Class::Type::SubOp>(context);
        break;
      }

    case '+':
      if (peek_ahead(context, 1) == '=') {
        parse_simple<Class::Type::AddAssign>(context);
        break;
      } else if (peek_ahead(context, 1) == ':') {
        parse_simple<Class::Type::SliceOp>(context);
        break;
      } else {
        parse_simple<Class::Type::AddOp>(context);
        break;
      }

    case '=':
      if (peek_ahead(context, 1) == '=') {
        parse_simple<Class::Type::CmpOp>(context);
        break;
      } else {
        parse_simple<Class::Type::Assign>(context);
        break;
      }

    case '<':
      if (peek_ahead(context, 1) == '=') {
        parse_simple<Class::Type::LessEqOp>(context);
        break;
      } else {
        parse_simple<Class::Type::LessOp>(context);
        break;
      }

    case '>':
      if (peek_ahead(context, 1) == '=') {
        parse_simple<Class::Type::GreaterEqOp>(context);
        break;
      } else {
        parse_simple<Class::Type::GreaterOp>(context);
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

    // All Addressable names are snake_case.
    case 'a' ... 'z':
      parse_identifier(context);
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
      if (peek_ahead(context, 1) == '[') {
        parse_embedded(context);
      } else {
        parse_unknown(context);
      }
      break;

    case '[':
      parse_simple<Class::Type::IndexStart>(context);
      break;

    case ']':
      parse_simple<Class::Type::IndexEnd>(context);
      break;

    case ')':
      parse_simple<Class::Type::PackingEnd>(context);
      break;

    case '.':
      if (peek_ahead(context, 1) == '[') {
        parse_simple<Class::Type::SwizzleOp>(context);
        break;
      } else if (
          peek_ahead(context, 1) == '.' && peek_ahead(context, 2) == '.') {
        parse_simple<Class::Type::RangeOp>(context);
        break;
      } else {
        parse_simple<Class::Type::AddressOp>(context);
        break;
      }

    case '!':
      if (peek_ahead(context, 1) == '=') {
        parse_simple<Class::Type::NotEqOp>(context);
      } else {
        parse_simple<Class::Type::NotOp>(context);
      }
      break;

    case ':':
      parse_simple<Class::Type::Define>(context);
      break;

      // Simple spot tokens
    case '{':
      parse_simple<Class::Type::ScopeStart>(context);
      break;
    case '}':
      parse_simple<Class::Type::ScopeEnd>(context);
      break;
    case '(':
      parse_simple<Class::Type::PackingStart>(context);
      break;
    case '*':
      parse_simple<Class::Type::MulOp>(context);
      break;
    case '%':
      parse_simple<Class::Type::ModOp>(context);
      break;
    case '&':
      parse_simple<Class::Type::AndOp>(context);
      break;
    case '|':
      parse_simple<Class::Type::OrOp>(context);
      break;
    case ';':
      parse_simple<Class::Type::EndStatement>(context);
      break;
    case '_':
      parse_simple<Class::Type::Discard>(context);
      break;
    case ',':
      parse_simple<Class::Type::PackingOp>(context);
      break;

      // We failed to parse so log the unknown token as we don't want to drop it
      // from the stream. Sometimes a format or another command is issued to the
      // LSP speculatively and it's super annoying if the formatter drops
      // partial tokens that were a work in progress.
    default:
      parse_unknown(context);
      break;
    }
  }

  context.loc.source_index = ++context.loc.parse_index;
  context.tokens.insert(Token(
      View::Bytes(), Class::Type::EndOfStream, context.loc.line,
      context.loc.column));
}
