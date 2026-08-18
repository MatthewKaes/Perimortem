// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/documents.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Puffer;

auto Lsp::Documents::find(View::Bytes uri) const -> Count {
  for (Count i = 0; i < records.get_size(); i++) {
    if (records[i].active && records[i].uri == uri) {
      return i;
    }
  }

  return Count(-1);
}

auto Lsp::Documents::upsert(View::Bytes uri, View::Bytes source) -> void {
  if (uri.is_empty()) {
    return;
  }

  Count slot = find(uri);
  if (slot == Count(-1)) {
    for (Count i = 0; i < records.get_size(); i++) {
      if (!records[i].active) {
        slot = i;
        records[i].active = True;
        records[i].uri = uri;
        break;
      }
    }
  }

  if (slot != Count(-1)) {
    records[slot].text = source;
    records[slot].semantics = {};
  }
}

auto Lsp::Documents::erase(View::Bytes uri) -> void {
  Count slot = find(uri);
  if (slot != Count(-1)) {
    records[slot].active = False;
    records[slot].uri.clear();
    records[slot].text.clear();
    records[slot].semantics = {};
  }
}

auto Lsp::Documents::get_text(View::Bytes uri) const -> Dynamic::Bytes {
  Count slot = find(uri);
  if (slot == Count(-1)) {
    return Dynamic::Bytes();
  }

  return records[slot].text;
}

static auto get_semantics(Lsp::Document& document) -> Lsp::SemanticDocument& {
  if (!document.semantics) {
    document.semantics = Dynamic::Object<Lsp::SemanticDocument>(
        document.uri.get_view(), document.text.get_view());
  }
  return **document.semantics;
}

auto Lsp::Documents::get_errors(View::Bytes uri)
    -> Option<const Ttx::Lexical::Errors&> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));
  return get_semantics(records[slot]).get_errors();
}

static auto utf_16_position_to_byte(
    View::Bytes source,
    Count target_line,
    Count target_character) -> Option<Count> {
  // LSP positions count UTF-16 code units while TTX Anchors count source bytes.
  // Invalid or partial UTF-8 returns absence so a malformed editor position
  // cannot select a neighboring semantic identity.
  Count offset = 0;
  Count line = 0;
  while (line < target_line && offset < source.get_size()) {
    if (source[offset++] == '\n') {
      line++;
    }
  }
  if (line != target_line) {
    return {};
  }

  Count units = 0;
  while (offset < source.get_size() && source[offset] != '\n' &&
         units < target_character) {
    Unsigned_8 lead = source[offset];
    Count width = 1;
    Count code_units = 1;
    if (lead >= 0xC2 && lead <= 0xDF) {
      width = 2;
    } else if (lead >= 0xE0 && lead <= 0xEF) {
      width = 3;
    } else if (lead >= 0xF0 && lead <= 0xF4) {
      width = 4;
      code_units = 2;
    }

    if (units + code_units > target_character ||
        offset + width > source.get_size()) {
      return {};
    }
    for (Count i = 1; i < width; i++) {
      if ((source[offset + i] & 0xC0) != 0x80) {
        width = 1;
        code_units = 1;
        break;
      }
    }
    offset += width;
    units += code_units;
  }

  if (units != target_character) {
    return {};
  }
  return offset;
}

auto Lsp::Documents::find_semantic(
    View::Bytes uri,
    Count line,
    Count utf_16_character) -> Option<const Ttx::Concept::Abstract&> {
  Count slot = find(uri);
  if (slot == Count(-1)) {
    return {};
  }

  Document& document = records[slot];
  auto offset =
      utf_16_position_to_byte(document.text.get_view(), line, utf_16_character);
  if (!offset) {
    return {};
  }

  return get_semantics(document).find(*offset);
}
