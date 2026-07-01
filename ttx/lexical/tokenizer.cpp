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

static constexpr auto is_attribute(Bits_8 c) -> Bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static constexpr auto is_class(Bits_8 c) -> Bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

static constexpr auto is_identifier(Bits_8 c) -> Bool {
  return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

static constexpr auto is_numeric(Bits_8 c) -> Bool {
  return (c >= '0' && c <= '9') || c == '.';
}

static constexpr auto is_hex(Bits_8 c) -> Bool {
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

// Context is the tokenizer cursor for one source view and one token stream.
// It owns the mutable scan coordinates because token helpers walk different
// shapes before they know the final token class, but all of them emit tokens
// with the same line, column, and source-slice rules.
//
// Keep grammar and token classification in the parse helpers. Context should
// stay limited to cursor movement, source slicing, line and column
// accounting, and token emission. If a helper needs the current token text,
// it should emit through add_current_token so token-start arithmetic stays
// here.
class Context {
 public:
  Context(const View::Bytes source, Managed::Vector<Token>& tokens)
      : source(source), tokens(tokens) {};

  constexpr auto can_parse() const -> Bool {
    return parse_index < source.get_size();
  }

  // The main parse loop guards current-token dispatch with can_parse.
  // Reading through the raw pointer keeps that hot path from repeating the
  // same bounds check for every switch and helper.
  constexpr auto current() const -> Bits_8 {
    return source.get_data()[parse_index];
  }

  constexpr auto get_parse_index() const -> Bits_32 { return parse_index; }

  constexpr auto get_source() const -> View::Bytes { return source; }

  constexpr auto slice(Count start, Count size) const -> View::Bytes {
    return source.slice(start, size);
  }

  // Keyword and directive tables need the token bytes before the final class
  // is known. Keep that as one current-token view instead of exposing token
  // start and token size as separate pieces of state.
  constexpr auto current_token_text() const -> View::Bytes {
    return source.slice(token_start, parse_index - token_start);
  }

  // Lookahead often asks about a byte that might be past the end of the
  // source. View::Bytes owns that protected access, so Context does not need
  // to duplicate the same end-of-source logic.
  constexpr auto peek_ahead(Bits_32 amount) const -> Bits_8 {
    return source[parse_index + amount];
  }

  // Token helpers advance parse_index before they know the final class.
  // Capturing the start here lets add_current_token build the final source
  // slice without leaking token-start state to every helper.
  auto begin_token() -> void { token_start = parse_index; }

  auto advance_parse(Count amount = 1) -> void { parse_index += amount; }

  auto backup_parse() -> void { parse_index--; }

  auto advance_column(Count amount = 1) -> void { column += amount; }

  auto advance_column_to_parse() -> void {
    column += parse_index - token_start;
  }

  auto consume_line_break() -> void {
    line++;
    column = 1;
    parse_index++;
  }

  auto count_line_break() -> void { line++; }

  auto add_current_token(Class::Type klass) -> void {
    add_token(source.slice(token_start, parse_index - token_start), klass);
  }

  auto add_token(View::Bytes text, Class::Type klass) -> void {
    tokens.insert(Token(text, klass, line, column));
  }

  auto add_end_of_stream() -> void {
    token_start = ++parse_index;
    add_token(View::Bytes(), Class::Type::EndOfStream);
  }

 private:
  Bits_32 token_start = 0;
  Bits_32 parse_index = 0;
  Bits_32 line = 1;
  Bits_32 column = 1;
  const View::Bytes source;
  Managed::Vector<Token>& tokens;
};

static auto parse_attribute(Context& ctx) -> void {
  while (is_attribute(ctx.peek_ahead(1))) {
    ctx.advance_parse();
  }

  ctx.advance_parse();
  const auto token = ctx.current_token_text();

  if (!token.is_empty()) {
    Class::Type klass = check_directive(token, Class::Type::Attribute);
    ctx.add_token(token, klass);
  }
  ctx.advance_column_to_parse();
}

static auto parse_comment(Context& ctx) -> void {
  // trim comment marker and leading space.
  ctx.advance_parse(Class::get_source_text(Class::Type::Comment).get_size());

  // Skip one leading space if present.
  if (ctx.can_parse() && ctx.current() == ' ') {
    ctx.advance_parse();
  }

  Bits_32 start_comment = ctx.get_parse_index();
  while (ctx.can_parse() && ctx.current() != '\n') {
    ctx.advance_parse();
  }

  ctx.add_token(
      ctx.slice(start_comment, ctx.get_parse_index() - start_comment),
      Class::Type::Comment);
}

static auto recursive_strip(Context& ctx) -> void {
  while (ctx.can_parse()) {
    Bits_8 current = ctx.current();
    ctx.advance_parse();
    switch (current) {
    case '\n':
      ctx.count_line_break();
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

static auto parse_disabled(Context& ctx, Bool strip_disabled) -> void {
  Count marker_size = Class::get_source_text(Class::Type::Disabled).get_size();
  ctx.advance_parse(marker_size);

  if (!strip_disabled) {
    ctx.add_current_token(Class::Type::Disabled);
    ctx.advance_column(marker_size);
  } else {
    // The parser pass is more expensive than tokenization so if we can strip
    // out disabled code then optimize by removing the tokens.
    while (ctx.can_parse() && ctx.current() != '\n') {
      if (ctx.current() == '{') {
        recursive_strip(ctx);
        continue;
      }
      ctx.advance_parse();
    }
  }
}

static auto string_quote_is_escaped(View::Bytes source, Count position)
    -> Bool {
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

static auto parse_string(Context& ctx) -> void {
  ctx.advance_parse();
  while (ctx.can_parse() && ctx.current() != '\n' &&
         (ctx.current() != '"' ||
          string_quote_is_escaped(ctx.get_source(), ctx.get_parse_index()))) {
    ctx.advance_parse();
  }
  if (ctx.can_parse() && ctx.current() == '"') {
    ctx.advance_parse();
  }
  ctx.add_current_token(Class::Type::String);
  ctx.advance_column_to_parse();
}

static auto parse_embedded(Context& ctx) -> void {
  ctx.advance_parse(Class::get_source_text(Class::Type::Embedded).get_size());
  while (ctx.can_parse() && ctx.current() != ']' && ctx.current() != '\n') {
    ctx.advance_parse();
  }
  if (ctx.can_parse() && ctx.current() == ']') {
    ctx.advance_parse();
  }
  ctx.add_current_token(Class::Type::Embedded);
  ctx.advance_column_to_parse();
}

static auto parse_number(Context& ctx) -> void {
  // If we start with zero then check if we have a valid hex sequence.
  if (ctx.current() == '0' && ctx.peek_ahead(1) == 'x') {
    switch (ctx.peek_ahead(2)) {
    // 0x[FF FF ...] hex byte array literal (any whitespace is fine)
    case '[': {
      ctx.advance_parse(Class::get_source_text(Class::Type::Bytes).get_size());
      while (ctx.can_parse() && ctx.current() != ']') {
        ctx.advance_parse();
      }
      if (ctx.can_parse()) {
        ctx.advance_parse();
      }
      ctx.add_current_token(Class::Type::Bytes);
      ctx.advance_column_to_parse();
      return;
    }

      // 0xFF... hex integer literal.
    case '0' ... '9':
    case 'a' ... 'f':
    case 'A' ... 'F': {
      ctx.advance_parse(2);  // consume '0x'
      Bits_8 hex_char = ctx.peek_ahead(1);
      while (is_hex(hex_char)) {
        ctx.advance_parse();
        hex_char = ctx.peek_ahead(1);
      }
      ctx.advance_parse();
      ctx.add_current_token(Class::Type::Numeric);
      ctx.advance_column_to_parse();
      return;
    }

      // Not a valid hex sequence
    default:
      break;
    }
  }

  Bool found_decimal = false;
  Class::Type klass = Class::Type::Numeric;

  char numeric_char = ctx.peek_ahead(1);
  while (is_numeric(numeric_char)) {
    ctx.advance_parse();
    if (numeric_char == '.') {
      // Don't consume a '.' that starts a RangeOp '...'.
      if (ctx.peek_ahead(1) == '.') {
        ctx.backup_parse();
        break;
      }
      if (found_decimal) {
        ctx.backup_parse();
        break;
      }
      found_decimal = true;
      klass = Class::Type::Float;
    }
    numeric_char = ctx.peek_ahead(1);
  }

  ctx.advance_parse();
  ctx.add_current_token(klass);
  ctx.advance_column_to_parse();
}

static auto parse_type(Context& ctx) -> void {
  while (is_class(ctx.peek_ahead(1))) {
    ctx.advance_parse();
  }

  ctx.advance_parse();
  ctx.add_current_token(Class::Type::Type);
  ctx.advance_column_to_parse();
}

static auto parse_unknown(Context& ctx) -> void {
  ctx.advance_parse();
  ctx.add_current_token(Class::Type::Unknown);
  ctx.advance_column();
}

static auto parse_identifier(Context& ctx) -> void {
  if (!is_identifier(ctx.peek_ahead(0))) {
    parse_unknown(ctx);
    return;
  }

  while (is_identifier(ctx.peek_ahead(1))) {
    ctx.advance_parse();
  }

  ctx.advance_parse();
  const auto view = ctx.current_token_text();

  // Start by assuming the addressable isn't a compiler provided symbol.
  Class::Type klass = Class::Type::Addressable;
  klass = check_keyword(view, klass);

  ctx.add_token(view, klass);
  ctx.advance_column_to_parse();
}

template <Class::Type klass>
static auto parse_simple(Context& ctx) -> void {
  constexpr Count token_length = Class::get_source_text(klass).get_size();
  ctx.advance_parse(token_length);
  ctx.add_current_token(klass);
  ctx.advance_column(token_length);
}

auto Tokenizer::parse(const View::Bytes source_, Bool strip_disabled) -> void {
  source = source_;
  tokens.reset();

  // Take an estimated best guess on the token count. This helps save on
  // resizes even if we end up oversized.
  tokens.reset(source.get_size() >> 2);
  Context ctx(source, tokens);
  while (ctx.can_parse()) {
    ctx.begin_token();
    switch (ctx.current()) {
    case '\n':
      ctx.consume_line_break();
      break;

    case ' ':
    case '\t':
    case '\r':
      ctx.advance_column();
      ctx.advance_parse();
      break;

    case '/':
      if (ctx.peek_ahead(1) == '/') {
        parse_comment(ctx);
        break;
      } else if (ctx.peek_ahead(1) == '>') {
        parse_disabled(ctx, strip_disabled);
        break;
      } else {
        parse_simple<Class::Type::DivOp>(ctx);
        break;
      }

    case '-':
      if (ctx.peek_ahead(1) == '>') {
        parse_simple<Class::Type::CallOp>(ctx);
        break;
      } else if (ctx.peek_ahead(1) == '=') {
        parse_simple<Class::Type::SubAssign>(ctx);
        break;
      } else {
        parse_simple<Class::Type::SubOp>(ctx);
        break;
      }

    case '+':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Class::Type::AddAssign>(ctx);
        break;
      } else {
        parse_simple<Class::Type::AddOp>(ctx);
        break;
      }

    case '=':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Class::Type::CmpOp>(ctx);
        break;
      } else {
        parse_simple<Class::Type::Assign>(ctx);
        break;
      }

    case '<':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Class::Type::LessEqOp>(ctx);
        break;
      } else {
        parse_simple<Class::Type::LessOp>(ctx);
        break;
      }

    case '>':
      if (ctx.peek_ahead(1) == '=') {
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
      if (ctx.peek_ahead(1) == '[') {
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
      if (ctx.peek_ahead(1) == '[') {
        parse_simple<Class::Type::SwizzleOp>(ctx);
        break;
      } else if (ctx.peek_ahead(1) == '.' && ctx.peek_ahead(2) == '.') {
        parse_simple<Class::Type::RangeOp>(ctx);
        break;
      } else {
        parse_simple<Class::Type::AddressOp>(ctx);
        break;
      }

    case '!':
      if (ctx.peek_ahead(1) == '=') {
        parse_simple<Class::Type::NotEqOp>(ctx);
      } else {
        parse_simple<Class::Type::NotOp>(ctx);
      }
      break;

    case ':':
      if (ctx.peek_ahead(1) == '[') {
        parse_simple<Class::Type::SliceOp>(ctx);
      } else if (ctx.peek_ahead(1) == ':') {
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

  ctx.add_end_of_stream();
}
