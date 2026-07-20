// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/resolution/context.hpp"

#include "perimortem/core/data.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Puffer;

auto Resolution::Context::adopt_record(Dynamic::Object<Source::Record> record)
    -> Dynamic::Object<Source::Record> {
  Source::Record& adopted = *record;
  auto* existing = record_handles.find(&adopted);
  return existing == nullptr ? record_handles.insert(&adopted, record)->value
                             : existing->value;
}

auto Resolution::Context::persist_errors(
    View::Vector<Ttx::Lexical::Errors::Error> source) -> void {
  for (Count i = 0; i < source.get_size(); i++) {
    persist_error(source[i]);
  }
}

auto Resolution::Context::persist_error(
    const Ttx::Lexical::Errors::Error& error) -> void {
  Ttx::Lexical::Source source(
      persist(error.get_source_path()), persist(error.get_source()));
  View::Bytes message = persist(error.get_message());
  View::Bytes hint = persist(error.get_hint());

  if (!error.has_tokens()) {
    errors.insert(source, message, hint);
    return;
  }

  errors.insert_range(
      persist(error.get_start_token()), persist(error.get_end_token()), source,
      message, hint);
}

auto Resolution::Context::persist(View::Bytes bytes) -> View::Bytes {
  if (bytes.is_empty()) {
    return View::Bytes();
  }

  Unsigned_8* copy = error_arena.allocate(bytes.get_size());
  Data::copy(copy, bytes.get_data(), bytes.get_size());
  return View::Bytes(copy, bytes.get_size());
}

auto Resolution::Context::persist(const Ttx::Lexical::Token& token)
    -> const Ttx::Lexical::Token& {
  return error_arena.construct<Ttx::Lexical::Token>(
      persist(token.get_text()), token.get_code(), token.get_line(),
      token.get_column());
}
