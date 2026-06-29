// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/context.hpp"

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "tetrodotoxin/syntax/error.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Core::Diagnostics;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

static auto error_expected(Context& ctx, Class::Type type) -> void {
  Static::Bytes<256> error_buffer;
  Writer::Textual error(error_buffer.get_access());
  error << "Expected "_view;
  error << Class(type).get_name();
  error << " but got "_view;
  error << ctx.get_current().get_class().get_name();

  ctx.create_error(View::Bytes(error));
}

auto Context::expect(Class::Type type) -> Bool {
  if (matches(type)) {
    return True;
  }

  error_expected(*this, type);
  return False;
}

auto Context::require(Class::Type type) -> Bool {
  if (matches(type)) {
    advance();
    return True;
  }

  error_expected(*this, type);
  advance();
  return False;
}

auto Context::recover_to_matching(Class::Type open, Class::Type close) -> void {
  if (!matches(open)) {
    return;
  }

  Count depth = 0;
  while (!is_done()) {
    Class::Type type = get_current().get_class().get_type();
    if (type == open) {
      depth++;
    } else if (type == close) {
      depth--;
    }

    advance();

    if (depth == 0) {
      return;
    }
  }
}

auto Context::recover_to_statement() -> void {
  while (!is_done()) {
    switch (get_current().get_class().get_type()) {
    case Class::Type::EndStatement:
      advance();
      return;

    case Class::Type::ScopeEnd:
      return;

    default:
      advance();
      break;
    }
  }
}

auto Context::create_error(View::Bytes message, View::Bytes hint) -> void {
  create_error(get_current(), get_current(), message, hint);
}

auto Context::create_error(
    const Token& start,
    const Token& end,
    View::Bytes message,
    View::Bytes hint) -> void {
  error_count++;
  Count previous_size = diagnostics.get_size();
  diagnostics.resize(previous_size + Error::max_render_size);

  Count written = Error::render(
      diagnostics.get_access().slice(previous_size, Error::max_render_size),
      path, start, end, source, message, hint, color_enabled);

  diagnostics.resize(previous_size + written);
  View::Bytes output = diagnostics.get_view().slice(previous_size, written);
  Log::error(
      output, Diagnostics::Source(path, start.get_line(), start.get_column()));
}
