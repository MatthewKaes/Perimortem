// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/tokenizer.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/perimortem.hpp"

#include "perimortem/utility/pair.hpp"
#include "perimortem/utility/table.hpp"

using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Perimortem::Core;
using namespace Ttx::Lexical;

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

constexpr auto is_numeric(Bits_8 c) -> Bool {
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
  static constexpr Static::Vector<ClassMapping, 30> data = {{
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
      Class::get_source_text(Class::Type::Dialect),
      Class::Type::Dialect,
    },
    {Class::get_source_text(Class::Type::Enum), Class::Type::Enum},
    {Class::get_source_text(Class::Type::Alias), Class::Type::Alias},
    {Class::get_source_text(Class::Type::Object), Class::Type::Object},
    {Class::get_source_text(Class::Type::Struct), Class::Type::Struct},
    {
      Class::get_source_text(Class::Type::Foreign),
      Class::Type::Foreign,
    },
  }};

  using keyword_resolver = Table<Class::Type, data>;
  return keyword_resolver::find_or_default(value, default_value);
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

  Location location;
  const View::Bytes source;
  Perimortem::Memory::Managed::Vector<Token>& tokens;
};

inline auto can_parse(Context& ctx) -> Bool {
  return ctx.location.parse_index < ctx.source.get_size();
}

auto peek_ahead(Context& ctx, Bits_32 amount) -> Bits_8 {
  if (ctx.location.parse_index + amount >= ctx.source.get_size()) {
    return 0;
  }
  return ctx.source[ctx.location.parse_index + amount];
}

auto parse_attribute(Context& ctx) -> void {
  while (is_attribute(peek_ahead(ctx, 1))) {
    ctx.location.parse_index++;
  }

  ctx.location.parse_index++;
  const auto token = ctx.source.slice(
      ctx.location.source_index,
      ctx.location.parse_index - ctx.location.source_index);

  if (!token.is_empty()) {
    Class::Type klass = check_directive(token, Class::Type::Attribute);
    ctx.tokens.insert(
        Token(token, klass, ctx.location.line, ctx.location.column));
  }
  ctx.location.column += token.get_size();
}

auto parse_comment(Context& ctx) -> void {
  ctx.location.source_index = ctx.location.parse_index;

  // trim comment marker and leading space.
  ctx.location.parse_index +=
      Class::get_source_text(Class::Type::Comment).get_size();

  // Skip one leading space if present.
  if (can_parse(ctx) && ctx.source[ctx.location.parse_index] == ' ') {
    ctx.location.parse_index++;
  }

  Bits_32 start_comment = ctx.location.parse_index;
  while (can_parse(ctx) &&
         ctx.source[ctx.location.parse_index] != '\n') {
    ctx.location.parse_index++;
  }

  ctx.tokens.insert(Token(
      ctx.source.slice(
          start_comment, ctx.location.parse_index - start_comment),
      Class::Type::Comment, ctx.location.line, ctx.location.column));
}

auto recursive_strip(Context& ctx) -> void {
  while (can_parse(ctx)) {
    switch (ctx.source[ctx.location.parse_index++]) {
    case '\n':
      ctx.location.line += 1;
      break;
    case '}':
      return;
    case '{':
      recursive_strip(ctx);
      break;
    default:
      break;
    }
  }
}

auto parse_disabled(Context& ctx, Bool strip_disabled) -> void {
  ctx.location.source_index = ctx.location.parse_index;
  Count marker_size = Class::get_source_text(Class::Type::Disabled).get_size();
  ctx.location.parse_index += marker_size;

  if (!strip_disabled) {
    ctx.tokens.insert(Token(
        ctx.source.slice(
            ctx.location.source_index,
            ctx.location.parse_index - ctx.location.source_index),
        Class::Type::Disabled, ctx.location.line, ctx.location.column));
    ctx.location.column += marker_size;
  } else {
    // The parser pass is more expensive than tokenization so if we can strip
    // out disabled code then optimize by removing the tokens.
    while (can_parse(ctx) &&
           ctx.source[ctx.location.parse_index] != '\n') {
      if (ctx.source[ctx.location.parse_index] == '{') {
        recursive_strip(ctx);
        continue;
      }
      ctx.location.parse_index++;
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

auto parse_string(Context& ctx) -> void {
  ctx.location.parse_index++;
  while (can_parse(ctx) &&
         ctx.source[ctx.location.parse_index] != '\n' &&
         (ctx.source[ctx.location.parse_index] != '"' ||
          string_quote_is_escaped(ctx.source, ctx.location.parse_index))) {
    ctx.location.parse_index++;
  }
  if (can_parse(ctx) && ctx.source[ctx.location.parse_index] == '"') {
    ctx.location.parse_index++;
  }
  ctx.tokens.insert(Token(
      ctx.source.slice(
          ctx.location.source_index,
          ctx.location.parse_index - ctx.location.source_index),
      Class::Type::String, ctx.location.line, ctx.location.column));
  ctx.location.column += ctx.location.parse_index - ctx.location.source_index;
}

auto parse_embedded(Context& ctx) -> void {
  ctx.location.parse_index +=
      Class::get_source_text(Class::Type::Embedded).get_size();
  while (can_parse(ctx) && ctx.source[ctx.location.parse_index] != ']' &&
         ctx.source[ctx.location.parse_index] != '\n') {
    ctx.location.parse_index++;
  }
  if (can_parse(ctx) && ctx.source[ctx.location.parse_index] == ']') {
    ctx.location.parse_index++;
  }
  ctx.tokens.insert(Token(
      ctx.source.slice(
          ctx.location.source_index,
          ctx.location.parse_index - ctx.location.source_index),
      Class::Type::Embedded, ctx.location.line, ctx.location.column));
  ctx.location.column += ctx.location.parse_index - ctx.location.source_index;
}

auto parse_number(Context& ctx) -> void {
  // If we start with zero then check if we have a valid hex sequence.
  if (ctx.source[ctx.location.source_index] == '0' &&
      peek_ahead(ctx, 1) == 'x') {
    switch (peek_ahead(ctx, 2)) {
    // 0x[FF FF ...] hex byte array literal (any whitespace is fine)
    case '[': {
      ctx.location.parse_index +=
          Class::get_source_text(Class::Type::Bytes).get_size();
      while (can_parse(ctx) &&
             ctx.source[ctx.location.parse_index] != ']') {
        ctx.location.parse_index++;
      }
      if (can_parse(ctx)) {
        ctx.location.parse_index++;
      }
      ctx.tokens.insert(Token(
          ctx.source.slice(
              ctx.location.source_index,
              ctx.location.parse_index - ctx.location.source_index),
          Class::Type::Bytes, ctx.location.line, ctx.location.column));
      ctx.location.column += ctx.location.parse_index - ctx.location.source_index;
      return;
    }

      // 0xFF... hex integer literal.
    case '0' ... '9':
    case 'a' ... 'f':
    case 'A' ... 'F': {
      ctx.location.parse_index += 2;  // consume '0x'
      Bits_8 hex_char = peek_ahead(ctx, 1);
      while (is_hex(hex_char)) {
        ctx.location.parse_index++;
        hex_char = peek_ahead(ctx, 1);
      }
      ctx.location.parse_index++;
      ctx.tokens.insert(Token(
          ctx.source.slice(
              ctx.location.source_index,
              ctx.location.parse_index - ctx.location.source_index),
          Class::Type::Numeric, ctx.location.line, ctx.location.column));
      ctx.location.column += ctx.location.parse_index - ctx.location.source_index;
      return;
    }

      // Not a valid hex sequence
    default:
      break;
    }
  }

  Bool found_decimal = false;
  Class::Type klass = Class::Type::Numeric;

  char numeric_char = peek_ahead(ctx, 1);
  while (is_numeric(numeric_char)) {
    ctx.location.parse_index++;
    if (numeric_char == '.') {
      // Don't consume a '.' that starts a RangeOp '...'.
      if (peek_ahead(ctx, 1) == '.') {
        ctx.location.parse_index--;
        break;
      }
      if (found_decimal) {
        ctx.location.parse_index--;
        break;
      }
      found_decimal = true;
      klass = Class::Type::Float;
    }
    numeric_char = peek_ahead(ctx, 1);
  }

  ctx.location.parse_index++;
  ctx.tokens.insert(Token(
      ctx.source.slice(
          ctx.location.source_index,
          ctx.location.parse_index - ctx.location.source_index),
      klass, ctx.location.line, ctx.location.column));
  ctx.location.column += ctx.location.parse_index - ctx.location.source_index;
}

auto parse_type(Context& ctx) -> void {
  while (is_class(peek_ahead(ctx, 1))) {
    ctx.location.parse_index++;
  }

  ctx.location.parse_index++;
  const auto view = ctx.source.slice(
      ctx.location.source_index,
      ctx.location.parse_index - ctx.location.source_index);

  ctx.tokens.insert(
      Token(view, Class::Type::Type, ctx.location.line, ctx.location.column));
  ctx.location.column += ctx.location.parse_index - ctx.location.source_index;
}

auto parse_unknown(Context& ctx) -> void {
  ctx.tokens.insert(Token(
      ctx.source.slice(ctx.location.source_index, 1), Class::Type::Unknown,
      ctx.location.line, ctx.location.column));
  ctx.location.column++;
  ctx.location.parse_index++;
}

auto parse_identifier(Context& ctx) -> void {
  if (!is_identifier(peek_ahead(ctx, 0))) {
    parse_unknown(ctx);
    return;
  }

  while (is_identifier(peek_ahead(ctx, 1))) {
    ctx.location.parse_index++;
  }

  ctx.location.parse_index++;
  const auto view = ctx.source.slice(
      ctx.location.source_index,
      ctx.location.parse_index - ctx.location.source_index);

  // Start by assuming the addressable isn't a compiler provided symbol.
  Class::Type klass = Class::Type::Addressable;
  klass = check_keyword(view, klass);

  ctx.tokens.insert(
      Token(view, klass, ctx.location.line, ctx.location.column));
  ctx.location.column += ctx.location.parse_index - ctx.location.source_index;
}

template <Class::Type klass>
auto parse_simple(Context& ctx) -> void {
  constexpr Count token_length = Class::get_source_text(klass).get_size();
  ctx.location.parse_index += token_length;
  ctx.tokens.insert(Token(
      ctx.source.slice(ctx.location.source_index, token_length), klass,
      ctx.location.line, ctx.location.column));
  ctx.location.column += token_length;
}

auto Tokenizer::parse(const View::Bytes source_, Bool strip_disabled) -> void {
  source = source_;
  tokens.reset();

  // Take an estimated best guess on the token count. This helps save on
  // resizes even if we end up oversized.
  tokens.reset(source.get_size() >> 2);
  Context ctx(source, tokens);
  while (ctx.location.parse_index < source.get_size()) {
    ctx.location.source_index = ctx.location.parse_index;
    switch (source[ctx.location.parse_index]) {
    case '\n':
      ctx.location.line++;
      ctx.location.column = 1;
      ctx.location.parse_index++;
      break;

    case ' ':
    case '\t':
    case '\r':
      ctx.location.column++;
      ctx.location.parse_index++;
      break;

    case '/':
      if (peek_ahead(ctx, 1) == '/') {
        parse_comment(ctx);
        break;
      } else if (peek_ahead(ctx, 1) == '>') {
        parse_disabled(ctx, strip_disabled);
        break;
      } else {
        parse_simple<Class::Type::DivOp>(ctx);
        break;
      }

    case '-':
      if (peek_ahead(ctx, 1) == '>') {
        parse_simple<Class::Type::CallOp>(ctx);
        break;
      } else if (peek_ahead(ctx, 1) == '=') {
        parse_simple<Class::Type::SubAssign>(ctx);
        break;
      } else {
        parse_simple<Class::Type::SubOp>(ctx);
        break;
      }

    case '+':
      if (peek_ahead(ctx, 1) == '=') {
        parse_simple<Class::Type::AddAssign>(ctx);
        break;
      } else {
        parse_simple<Class::Type::AddOp>(ctx);
        break;
      }

    case '=':
      if (peek_ahead(ctx, 1) == '=') {
        parse_simple<Class::Type::CmpOp>(ctx);
        break;
      } else {
        parse_simple<Class::Type::Assign>(ctx);
        break;
      }

    case '<':
      if (peek_ahead(ctx, 1) == '=') {
        parse_simple<Class::Type::LessEqOp>(ctx);
        break;
      } else {
        parse_simple<Class::Type::LessOp>(ctx);
        break;
      }

    case '>':
      if (peek_ahead(ctx, 1) == '=') {
        parse_simple<Class::Type::GreaterEqOp>(ctx);
        break;
      } else {
        parse_simple<Class::Type::GreaterOp>(ctx);
        break;
      }

    case '@':
      parse_attribute(ctx);
      break;

    // Tested faster to unroll number parsing here in the switch rather than
    // try and push it to the default.
    case '0' ... '9':
      parse_number(ctx);
      break;

    // All Addressable names are snake_case.
    case 'a' ... 'z':
      parse_identifier(ctx);
      break;

    // Originally types had @ but it was clunky, so @ was moved to
    // Attributes and all Types are simply capitalized.
    case 'A' ... 'Z':
      parse_type(ctx);
      break;

    case '"':
      parse_string(ctx);
      break;

    case '$':
      if (peek_ahead(ctx, 1) == '[') {
        parse_embedded(ctx);
      } else {
        parse_unknown(ctx);
      }
      break;

    case '[':
      parse_simple<Class::Type::IndexStart>(ctx);
      break;

    case ']':
      parse_simple<Class::Type::IndexEnd>(ctx);
      break;

    case ')':
      parse_simple<Class::Type::PackingEnd>(ctx);
      break;

    case '.':
      if (peek_ahead(ctx, 1) == '[') {
        parse_simple<Class::Type::SwizzleOp>(ctx);
        break;
      } else if (
          peek_ahead(ctx, 1) == '.' && peek_ahead(ctx, 2) == '.') {
        parse_simple<Class::Type::RangeOp>(ctx);
        break;
      } else {
        parse_simple<Class::Type::AddressOp>(ctx);
        break;
      }

    case '!':
      if (peek_ahead(ctx, 1) == '=') {
        parse_simple<Class::Type::NotEqOp>(ctx);
      } else {
        parse_simple<Class::Type::NotOp>(ctx);
      }
      break;

    case ':':
      if (peek_ahead(ctx, 1) == '[') {
        parse_simple<Class::Type::SliceOp>(ctx);
      } else if (peek_ahead(ctx, 1) == ':') {
        parse_simple<Class::Type::TypeAccessOp>(ctx);
      } else {
        parse_simple<Class::Type::Define>(ctx);
      }
      break;

      // Simple spot tokens
    case '{':
      parse_simple<Class::Type::ScopeStart>(ctx);
      break;
    case '}':
      parse_simple<Class::Type::ScopeEnd>(ctx);
      break;
    case '(':
      parse_simple<Class::Type::PackingStart>(ctx);
      break;
    case '*':
      parse_simple<Class::Type::MulOp>(ctx);
      break;
    case '%':
      parse_simple<Class::Type::ModOp>(ctx);
      break;
    case '&':
      parse_simple<Class::Type::AndOp>(ctx);
      break;
    case '|':
      parse_simple<Class::Type::OrOp>(ctx);
      break;
    case ';':
      parse_simple<Class::Type::EndStatement>(ctx);
      break;
    case '_':
      parse_simple<Class::Type::Discard>(ctx);
      break;
    case ',':
      parse_simple<Class::Type::PackingOp>(ctx);
      break;

      // We failed to parse so log the unknown token as we don't want to drop it
      // from the stream. Sometimes a format or another command is issued to the
      // LSP speculatively and it's super annoying if the formatter drops
      // partial tokens that were a work in progress.
    default:
      parse_unknown(ctx);
      break;
    }
  }

  ctx.location.source_index = ++ctx.location.parse_index;
  ctx.tokens.insert(Token(
      View::Bytes(), Class::Type::EndOfStream, ctx.location.line,
      ctx.location.column));
}
