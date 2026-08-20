// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/documents.hpp"

#include "perimortem/core/static/vector.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Puffer;
using namespace Tetrodotoxin;

static auto decode_hex(Unsigned_8 value) -> Option<Unsigned_8> {
  if (value >= '0' && value <= '9') {
    return Unsigned_8(value - '0');
  } else if (value >= 'A' && value <= 'F') {
    return Unsigned_8(value - 'A' + 10);
  } else if (value >= 'a' && value <= 'f') {
    return Unsigned_8(value - 'a' + 10);
  }

  return {};
}

static auto decode_file_uri(View::Bytes uri) -> Dynamic::Bytes {
  constexpr View::Bytes prefix = "file://"_view;
  if (uri.get_size() <= prefix.get_size() ||
      uri.slice(0, prefix.get_size()) != prefix) {
    return {};
  }

  Dynamic::Bytes path;
  View::Bytes encoded = uri.slice(prefix.get_size());
  for (Count index = 0; index < encoded.get_size(); index++) {
    if (encoded[index] != '%') {
      path.append(encoded[index]);
      continue;
    }

    if (index + 2 >= encoded.get_size()) {
      return {};
    }

    auto high = decode_hex(encoded[index + 1]);
    auto low = decode_hex(encoded[index + 2]);
    if (!high || !low) {
      return {};
    }

    path.append(Unsigned_8((*high << 4) | *low));
    index += 2;
  }

  Path normalized(path.get_view());
  return normalized.is_rooted() ? Dynamic::Bytes(normalized.get_view())
                                : Dynamic::Bytes();
}

static auto join_path(View::Bytes directory, View::Bytes file)
    -> Dynamic::Bytes {
  Dynamic::Bytes joined(directory);
  if (!joined.is_empty() && joined[joined.get_size() - 1] != '/') {
    joined.append('/');
  }
  joined.concat(file);
  return joined;
}

static auto relative_path(View::Bytes root, View::Bytes path)
    -> Dynamic::Bytes {
  if (path.get_size() <= root.get_size() ||
      path.slice(0, root.get_size()) != root || path[root.get_size()] != '/') {
    return {};
  }

  Path relative(path.slice(root.get_size() + 1));
  return relative.is_rooted() ? Dynamic::Bytes()
                              : Dynamic::Bytes(relative.get_view());
}

static auto manifest_claims(
    View::Bytes manifest_path,
    View::Bytes manifest_source,
    View::Bytes logical_route) -> Bool {
  Allocator::Arena transaction;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(
      transaction, manifest_source, manifest_path);
  Ttx::Lexical::Associations associations(transaction);
  Ttx::Lexical::Cursor cursor(tokenizer, errors, associations);
  Environment::Workspace context;
  Package::Dialect package;
  Static::Vector<Tetrodotoxin::Language::Dialect*, 1> dialects = {{&package}};
  auto interpreted = Tetrodotoxin::Language::Dialect::interpret_source(
      dialects.get_view(), cursor, context);
  BAIL_IF(!interpreted);

  auto manifest = interpreted->select<Package::Language::Monograph>();
  BAIL_IF(!manifest);
  return manifest->get_sources().contains(
      [&](const Package::Language::Source& source) {
        return source.get_source_path() == logical_route;
      });
}

struct PackageLocation {
  PackageLocation(View::Bytes root, View::Bytes route)
      : root(root), route(route) {}

  Dynamic::Bytes root;
  Dynamic::Bytes route;
};

static auto find_package(View::Bytes path, View::Bytes source)
    -> Option<PackageLocation> {
  Path normalized(path);
  View::Bytes directory = normalized.get_directory();
  View::Bytes file = normalized.get_file();
  while (!directory.is_empty()) {
    Dynamic::Bytes manifest_path = join_path(directory, "package.ttx"_view);
    if (File::exists(manifest_path.get_view())) {
      Dynamic::Bytes route = relative_path(directory, normalized.get_view());
      if (!route.is_empty()) {
        Dynamic::Bytes manifest_source;
        if (file == "package.ttx"_view &&
            manifest_path.get_view() == normalized.get_view()) {
          manifest_source = source;
        } else {
          auto read = File::read(manifest_path.get_view());
          if (read) {
            manifest_source = static_cast<Dynamic::Bytes&&>(*read);
          }
        }

        Bool claimed = route == "package.ttx"_view ||
                       (!manifest_source.is_empty() &&
                        manifest_claims(
                            manifest_path.get_view(),
                            manifest_source.get_view(), route.get_view()));
        if (claimed) {
          return PackageLocation(directory, route.get_view());
        }
      }
    }

    Path current(directory);
    View::Bytes parent = current.get_directory();
    if (parent == directory) {
      break;
    }
    directory = parent;
  }

  return {};
}

auto Lsp::Documents::find(View::Bytes uri) const -> Count {
  for (Count index = 0; index < records.get_size(); index++) {
    if (records[index].active && records[index].uri == uri) {
      return index;
    }
  }

  return Count(-1);
}

auto Lsp::Documents::find_session(View::Bytes root) const -> Count {
  for (Count index = 0; index < sessions.get_size(); index++) {
    if (sessions[index].active && sessions[index].root == root) {
      return index;
    }
  }

  return Count(-1);
}

auto Lsp::Documents::select_session(Document& document) -> Option<Session&> {
  BAIL_IF(document.package_root.is_empty());

  Count selected = find_session(document.package_root.get_view());
  BAIL_IF(selected == Count(-1));
  return sessions[selected];
}

auto Lsp::Documents::upsert(View::Bytes uri, View::Bytes source) -> void {
  if (uri.is_empty()) {
    return;
  }

  Count slot = find(uri);
  if (slot == Count(-1)) {
    for (Count index = 0; index < records.get_size(); index++) {
      if (!records[index].active) {
        slot = index;
        records[index].active = True;
        records[index].uri = uri;
        break;
      }
    }
  }

  if (slot == Count(-1)) {
    return;
  }

  Document& document = records[slot];
  document.text = source;
  document.standalone_semantics = {};

  Dynamic::Bytes path = decode_file_uri(uri);
  if (document.package_root.is_empty() && !path.is_empty()) {
    auto package = find_package(path.get_view(), source);
    if (package) {
      document.package_root = package->root;
      document.logical_route = package->route;
    }
  }

  auto session = select_session(document);
  if (!session && !document.package_root.is_empty()) {
    Count selected = Count(-1);
    for (Count index = 0; index < sessions.get_size(); index++) {
      if (!sessions[index].active) {
        selected = index;
        break;
      }
    }

    if (selected != Count(-1)) {
      Session& created = sessions[selected];
      created.active = True;
      created.root = document.package_root;
      created.snapshots = Dynamic::Record<Package::Snapshots>();
      session = created;
    }
  }

  if (!session && !document.package_root.is_empty()) {
    document.package_root.clear();
    document.logical_route.clear();
  }

  if (session && session->snapshots) {
    (*session->snapshots)
        ->overlay(
            document.package_root.get_view(), document.logical_route.get_view(),
            source);
    session->semantics = {};
  }
}

auto Lsp::Documents::erase(View::Bytes uri) -> void {
  Count slot = find(uri);
  if (slot == Count(-1)) {
    return;
  }

  Document& document = records[slot];
  auto session = select_session(document);
  if (session && session->snapshots) {
    (*session->snapshots)
        ->remove_overlay(
            document.package_root.get_view(),
            document.logical_route.get_view());
    session->semantics = {};
  }

  Dynamic::Bytes root = document.package_root;
  document.active = False;
  document.uri.clear();
  document.text.clear();
  document.package_root.clear();
  document.logical_route.clear();
  document.standalone_semantics = {};

  if (root.is_empty()) {
    return;
  }

  Bool retained = False;
  for (const Document& candidate : records.get_view()) {
    retained |= candidate.active && candidate.package_root == root;
  }

  if (!retained) {
    Count selected = find_session(root.get_view());
    if (selected != Count(-1)) {
      sessions[selected].active = False;
      sessions[selected].root.clear();
      sessions[selected].snapshots = {};
      sessions[selected].semantics = {};
    }
  }
}

auto Lsp::Documents::get_text(View::Bytes uri) const -> Dynamic::Bytes {
  Count slot = find(uri);
  return slot == Count(-1) ? Dynamic::Bytes() : records[slot].text;
}

auto Lsp::Documents::get_semantics(Document& document) -> SemanticWorkspace& {
  auto session = select_session(document);
  if (session && session->snapshots) {
    if (!session->semantics) {
      session->semantics = Dynamic::Record<SemanticWorkspace>(
          SemanticWorkspace::Mode::Package, *session->snapshots,
          document.logical_route.get_view(), document.text.get_view(),
          document.package_root.get_view(), packages_root.get_view());
    }

    return **session->semantics;
  }

  if (!document.standalone_semantics) {
    document.standalone_semantics = Dynamic::Record<SemanticWorkspace>(
        SemanticWorkspace::Mode::Standalone,
        Dynamic::Record<Package::Snapshots>(), document.uri.get_view(),
        document.text.get_view());
  }

  return **document.standalone_semantics;
}

auto Lsp::Documents::get_diagnostics(View::Bytes uri) -> Option<Diagnostics> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));

  Document& document = records[slot];
  SemanticWorkspace& semantics = get_semantics(document);
  View::Bytes source_name = document.package_root.is_empty()
                                ? document.uri.get_view()
                                : document.logical_route.get_view();
  return Diagnostics(semantics.get_errors(), source_name);
}

static auto utf_16_position_to_byte(
    View::Bytes source,
    Count target_line,
    Count target_character) -> Option<Count> {
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
    for (Count index = 1; index < width; index++) {
      if ((source[offset + index] & 0xC0) != 0x80) {
        width = 1;
        code_units = 1;
        break;
      }
    }
    offset += width;
    units += code_units;
  }

  return units == target_character ? Option<Count>(offset) : Option<Count>();
}

auto Lsp::Documents::find_semantic(
    View::Bytes uri,
    Count line,
    Count utf_16_character) -> Option<const Ttx::Concept::Abstract&> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));

  Document& document = records[slot];
  auto offset =
      utf_16_position_to_byte(document.text.get_view(), line, utf_16_character);
  BAIL_IF(!offset);

  View::Bytes source_name = document.package_root.is_empty()
                                ? document.uri.get_view()
                                : document.logical_route.get_view();
  return get_semantics(document).find(source_name, *offset);
}

auto Lsp::Documents::invalidate(View::Bytes uri) -> void {
  Dynamic::Bytes path = decode_file_uri(uri);
  if (path.is_empty()) {
    return;
  }

  for (Count index = 0; index < sessions.get_size(); index++) {
    Session& session = sessions[index];
    if (!session.active) {
      continue;
    }

    Bool member = path.get_size() > session.root.get_size() &&
                  path.get_view().slice(0, session.root.get_size()) ==
                      session.root.get_view() &&
                  path[session.root.get_size()] == '/';
    Bool standard_member = !packages_root.is_empty() &&
                           path.get_size() > packages_root.get_size() &&
                           path.get_view().slice(0, packages_root.get_size()) ==
                               packages_root.get_view() &&
                           path[packages_root.get_size()] == '/';
    if (member || standard_member) {
      session.semantics = {};
    }
  }
}
