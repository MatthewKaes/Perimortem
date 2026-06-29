// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/resolution/record.hpp"

#include "tetrodotoxin/lexical/tokenizer.hpp"
#include "tetrodotoxin/syntax/context.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Resolution;

Record::Record(View::Bytes path)
    : source_path(path), source(), arena(), package() {}

auto Record::load_if_stale() -> Bool {
  if (!is_loaded()) {
    return reload();
  }

  File::State sync = source.sync_status(source_path.get_view());
  if (sync == File::State::Stale || sync == File::State::Conflict) {
    return reload();
  }

  return state != State::Failed;
}

auto Record::reload() -> Bool {
  arena.reset();
  package = Syntax::Package();
  state = State::Unloaded;
  error_count = 0;

  if (!source.read(source_path.get_view())) {
    fail();
    return False;
  }

  Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(source.get_view(), false);

  Syntax::Context context(tokenizer, source_path.get_view());
  package = Syntax::Package::parse(context);
  error_count = context.get_error_count();
  parse_count++;

  if (!package.is_valid() || error_count != 0) {
    fail();
    return False;
  }

  state = State::Parsed;
  return True;
}

auto Record::invalidate_resolution() -> void {
  if (state == State::Resolved) {
    state = State::Parsed;
  }
}

auto Record::begin_resolution() -> void {
  state = State::Resolving;
}

auto Record::finish_resolution() -> void {
  state = State::Resolved;
}

auto Record::fail() -> void {
  state = State::Failed;
  error_count++;
}
