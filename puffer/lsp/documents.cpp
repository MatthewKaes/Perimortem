// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "puffer/lsp/documents.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"
#include "perimortem/system/version.hpp"

#include "tetrodotoxin/app/dialect.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/expressions/identifier.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"
#include "tetrodotoxin/package/storage.hpp"
#include "tetrodotoxin/scene/dialect.hpp"
#include "ttx/concept/invalid.hpp"
#include "ttx/lexical/associations.hpp"
#include "ttx/lexical/cursor.hpp"
#include "ttx/lexical/errors.hpp"
#include "ttx/lexical/tokenizer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Puffer;
using namespace Tetrodotoxin;

static auto decode_hex(U8 value) -> Option<U8> {
  if (value >= '0' && value <= '9') {
    return U8(value - '0');
  } else if (value >= 'A' && value <= 'F') {
    return U8(value - 'A' + 10);
  } else if (value >= 'a' && value <= 'f') {
    return U8(value - 'a' + 10);
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

    path.append(U8((*high << 4) | *low));
    index += 2;
  }

  Path normalized(path.get_view());
  return normalized.is_rooted() ? Dynamic::Bytes(normalized.get_view())
                                : Dynamic::Bytes();
}

static auto encode_file_uri(View::Bytes path) -> Dynamic::Bytes {
  constexpr View::Bytes hexadecimal = "0123456789ABCDEF"_view;
  Dynamic::Bytes uri("file://"_view);
  for (Count index = 0; index < path.get_size(); index++) {
    U8 byte = path[index];
    Bool unreserved = (byte >= 'A' && byte <= 'Z') ||
                      (byte >= 'a' && byte <= 'z') ||
                      (byte >= '0' && byte <= '9') || byte == '-' ||
                      byte == '.' || byte == '_' || byte == '~' || byte == '/';
    if (unreserved) {
      uri.append(byte);
      continue;
    }

    uri.append('%');
    uri.concat(hexadecimal.slice(byte >> 4, 1));
    uri.concat(hexadecimal.slice(byte & 0x0F, 1));
  }
  return uri;
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
    Environment::Toolchain& toolchain,
    View::Bytes manifest_path,
    View::Bytes manifest_source,
    View::Bytes logical_route) -> Bool {
  Allocator::Arena transaction;
  Ttx::Lexical::Errors errors;
  Ttx::Lexical::Tokenizer tokenizer(
      transaction, manifest_source, manifest_path);
  Ttx::Lexical::Associations associations(transaction);
  Ttx::Lexical::Cursor cursor(tokenizer, errors, associations);
  Environment::Workspace context(toolchain);
  auto interpreted = Tetrodotoxin::Language::Dialect::interpret_source(
      toolchain.get_dialects(), cursor, context);
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

static auto find_package(
    Environment::Toolchain& toolchain,
    Dynamic::Record<Package::Snapshots> snapshots,
    View::Bytes path,
    View::Bytes source) -> Option<PackageLocation> {
  Path normalized(path);
  View::Bytes directory = normalized.get_directory();
  View::Bytes file = normalized.get_file();
  while (!directory.is_empty()) {
    Dynamic::Bytes manifest_path = join_path(directory, "package.ttx"_view);
    Allocator::Arena acquisition;
    auto storage =
        File::exists(manifest_path.get_view())
            ? Package::Storage::open(acquisition, directory, snapshots)
            : Option<Package::Storage>();
    Option<Package::Content&> manifest;
    if (storage) {
      storage->read("package.ttx"_view)
          .visit(
              [&](Package::Content& content) { manifest = content; },
              [](const Package::Storage::Failure&) {});
    }
    if (manifest) {
      Dynamic::Bytes route = relative_path(directory, normalized.get_view());
      if (!route.is_empty()) {
        Dynamic::Bytes manifest_source;
        if (file == "package.ttx"_view &&
            manifest_path.get_view() == normalized.get_view()) {
          manifest_source = source;
        } else {
          manifest_source = manifest->get_contents();
        }

        Bool claimed = route == "package.ttx"_view ||
                       (!manifest_source.is_empty() &&
                        manifest_claims(
                            toolchain, manifest_path.get_view(),
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

class PackageRequest {
 public:
  PackageRequest(View::Bytes identity, Version version, View::Bytes root)
      : identity(identity), version(version), root(root) {}

  auto matches(View::Bytes selected_identity, Version selected_version) const
      -> Bool {
    return identity == selected_identity && version == selected_version;
  }

  constexpr auto get_root() const -> View::Bytes { return root; }

 private:
  Dynamic::Bytes identity;
  Version version;
  Dynamic::Bytes root;
};

static auto contains_request(
    View::Vector<PackageRequest> requests,
    View::Bytes identity,
    Version version) -> Bool {
  return requests.contains([&](const PackageRequest& request) {
    return request.matches(identity, version);
  });
}

static auto inspect_package(
    Environment::Toolchain& toolchain,
    Environment::Workspace& workspace,
    Ttx::Lexical::Errors& errors,
    Dynamic::Record<Package::Snapshots> snapshots,
    View::Bytes package_root,
    Allocator::Arena& transaction) -> Option<Package::Language::Monograph&> {
  Allocator::Arena acquisition;
  auto storage = Package::Storage::open(acquisition, package_root, snapshots);
  BAIL_IF(!storage);

  Option<Package::Content&> manifest;
  storage->read("package.ttx"_view)
      .visit(
          [&](Package::Content& content) { manifest = content; },
          [](const Package::Storage::Failure&) {});
  BAIL_IF(!manifest);

  View::Bytes source = transaction.proxy(manifest->get_contents());
  View::Bytes path = transaction.proxy(manifest->get_diagnostic_path());
  Ttx::Lexical::Tokenizer tokenizer(transaction, source, path);
  Ttx::Lexical::Associations associations(transaction);
  Ttx::Lexical::Cursor cursor(tokenizer, errors, associations);
  auto interpreted = Tetrodotoxin::Language::Dialect::interpret_source(
      toolchain.get_dialects(), cursor, workspace);
  return interpreted ? interpreted->select<Package::Language::Monograph>()
                     : Option<Package::Language::Monograph&>();
}

static auto import_package(
    Environment::Toolchain& toolchain,
    Environment::Workspace& workspace,
    Ttx::Lexical::Errors& errors,
    Dynamic::Record<Package::Snapshots> snapshots,
    View::Bytes packages_root,
    View::Bytes package_root,
    View::Bytes semantic_name,
    View::Bytes identity,
    Version version,
    Dynamic::Vector<PackageRequest>& loaded,
    Dynamic::Vector<PackageRequest>& active) -> Bool {
  if (contains_request(loaded, identity, version)) {
    return True;
  }

  BAIL_IF(contains_request(active, identity, version));
  active.emplace(PackageRequest(identity, version, package_root));

  Allocator::Arena inspection;
  auto manifest = inspect_package(
      toolchain, workspace, errors, snapshots, package_root, inspection);
  if (!manifest) {
    active.remove(active.get_size() - 1);
    return False;
  }

  Bool dependencies_loaded = True;
  for (const Package::Language::Dependency& dependency :
       manifest->get_dependencies()) {
    Dynamic::Bytes dependency_root =
        join_path(packages_root, dependency.get_package_name());
    dependencies_loaded &= import_package(
        toolchain, workspace, errors, snapshots, packages_root,
        dependency_root.get_view(), dependency.get_package_name(),
        dependency.get_package_name(), dependency.get_version(), loaded,
        active);
  }

  Bool imported = False;
  if (dependencies_loaded) {
    imported = Bool(workspace.import_package(
        errors, package_root, semantic_name, "package.ttx"_view, identity,
        version));
  }

  active.remove(active.get_size() - 1);
  if (imported) {
    loaded.emplace(PackageRequest(identity, version, package_root));
  }
  return imported;
}

Lsp::Documents::Documents(View::Bytes selected_packages_root)
    : snapshots(), packages_root(selected_packages_root) {
  auto package = toolchain.install<Package::Dialect>("Package"_view);
  auto library = toolchain.install<Library::Dialect>("Library"_view);
  auto app = toolchain.install<App::Dialect>("App"_view);
  if (!package || !library || !app) {
    return;
  }

  toolchain_ready =
      Bool(toolchain.install<Scene::Dialect>("Scene"_view, *library));
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
  document.standalone_errors = {};
  document.standalone_workspace = {};

  Dynamic::Bytes path = decode_file_uri(uri);
  if (document.package_root.is_empty() && !path.is_empty()) {
    auto package = find_package(toolchain, snapshots, path.get_view(), source);
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
      session = created;
    }
  }

  if (!session && !document.package_root.is_empty()) {
    document.package_root.clear();
    document.logical_route.clear();
  }

  if (session) {
    snapshots->overlay(
        document.package_root.get_view(), document.logical_route.get_view(),
        source);
    invalidate_package(document.package_root.get_view());
  }
}

auto Lsp::Documents::erase(View::Bytes uri) -> void {
  Count slot = find(uri);
  if (slot == Count(-1)) {
    return;
  }

  Document& document = records[slot];
  auto session = select_session(document);
  if (session) {
    snapshots->remove_overlay(
        document.package_root.get_view(), document.logical_route.get_view());
    invalidate_package(document.package_root.get_view());
  }

  Dynamic::Bytes root = document.package_root;
  document.active = False;
  document.uri.clear();
  document.text.clear();
  document.package_root.clear();
  document.logical_route.clear();
  document.standalone_errors = {};
  document.standalone_workspace = {};

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
      sessions[selected].errors = {};
      sessions[selected].workspace = {};
      sessions[selected].dependencies.clear();
    }
  }
}

auto Lsp::Documents::get_text(View::Bytes uri) const -> View::Bytes {
  Count slot = find(uri);
  return slot == Count(-1) ? View::Bytes() : records[slot].text.get_view();
}

auto Lsp::Documents::create_workspace(Document& document)
    -> Option<Environment::Workspace&> {
  BAIL_IF(!toolchain_ready);

  auto session = select_session(document);
  if (session) {
    session->errors = Dynamic::Record<Ttx::Lexical::Errors>();
    session->workspace =
        Dynamic::Record<Environment::Workspace>(toolchain, snapshots);
    Dynamic::Vector<PackageRequest> loaded;
    Dynamic::Vector<PackageRequest> active;
    import_package(
        toolchain, **session->workspace, **session->errors, snapshots,
        packages_root.get_view(), document.package_root.get_view(),
        "puffer.package"_view, document.package_root.get_view(), Version(1, 0),
        loaded, active);
    session->dependencies.clear();
    for (const PackageRequest& dependency : loaded.get_view()) {
      session->dependencies.insert(Dynamic::Bytes(dependency.get_root()));
    }
    return **session->workspace;
  }

  document.standalone_errors = Dynamic::Record<Ttx::Lexical::Errors>();
  document.standalone_workspace =
      Dynamic::Record<Environment::Workspace>(toolchain);
  (*document.standalone_workspace)
      ->interpret_source(
          **document.standalone_errors, "puffer.document"_view,
          document.uri.get_view(), document.text.get_view());
  return **document.standalone_workspace;
}

auto Lsp::Documents::get_workspace(Document& document)
    -> Option<Environment::Workspace&> {
  auto session = select_session(document);
  if (session) {
    return session->workspace
               ? Option<Environment::Workspace&>(**session->workspace)
               : create_workspace(document);
  }

  return document.standalone_workspace
             ? Option<Environment::Workspace&>(**document.standalone_workspace)
             : create_workspace(document);
}

auto Lsp::Documents::get_errors(Document& document)
    -> Option<const Ttx::Lexical::Errors&> {
  BAIL_IF(!get_workspace(document));

  auto session = select_session(document);
  if (session) {
    return session->errors
               ? Option<const Ttx::Lexical::Errors&>(**session->errors)
               : Option<const Ttx::Lexical::Errors&>();
  }

  return document.standalone_errors
             ? Option<const Ttx::Lexical::Errors&>(**document.standalone_errors)
             : Option<const Ttx::Lexical::Errors&>();
}

auto Lsp::Documents::get_diagnostics(View::Bytes uri) -> Option<Diagnostics> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));

  Document& document = records[slot];
  auto errors = get_errors(document);
  BAIL_IF(!errors);
  View::Bytes source_name = document.package_root.is_empty()
                                ? document.uri.get_view()
                                : document.logical_route.get_view();
  return Diagnostics(*errors, source_name);
}

auto Lsp::Documents::find_semantic(
    View::Bytes uri,
    const PositionEncoding::Position& position)
    -> Option<const Ttx::Concept::Abstract&> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));

  Document& document = records[slot];
  // The source text is available here, which makes this the natural place to
  // turn an editor coordinate back into TTX's authored byte offset. The lookup
  // that follows can then stay entirely within canonical lexical facts.
  auto offset =
      position_encoding.find_offset(document.text.get_view(), position);
  BAIL_IF(!offset);

  View::Bytes source_name = document.package_root.is_empty()
                                ? document.uri.get_view()
                                : document.logical_route.get_view();
  auto workspace = get_workspace(document);
  BAIL_IF(!workspace);
  auto associations = workspace->get_associations(source_name);
  auto semantic = associations ? associations->find_at(*offset)
                               : Option<const Ttx::Concept::Abstract&>();
  BAIL_IF(!semantic);

  auto identifier =
      semantic->select<Library::Language::Expressions::Identifier>();
  if (identifier && identifier->get_result().is<Ttx::Concept::Invalid>()) {
    const Ttx::Concept::Abstract& authored = identifier->resolve_authored();
    if (!authored.is<Ttx::Concept::Invalid>()) {
      return authored;
    }

    auto monograph = workspace->get_monograph(source_name);
    if (monograph) {
      const Ttx::Concept::Abstract& candidate =
          monograph->resolve_context(identifier->get_name());
      if (!candidate.is<Ttx::Concept::Invalid>()) {
        return candidate;
      }
    }
  }
  return semantic;
}

auto Lsp::Documents::set_position_encoding(PositionEncoding selected) -> void {
  position_encoding = selected;
}

auto Lsp::Documents::get_position_encoding() const -> const PositionEncoding& {
  return position_encoding;
}

auto Lsp::Documents::get_associations(View::Bytes uri)
    -> Option<const Ttx::Lexical::Associations&> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));

  Document& document = records[slot];
  auto workspace = get_workspace(document);
  BAIL_IF(!workspace);
  View::Bytes source_name = document.package_root.is_empty()
                                ? document.uri.get_view()
                                : document.logical_route.get_view();
  return workspace->get_associations(source_name);
}

auto Lsp::Documents::get_monograph(View::Bytes uri)
    -> Option<const Tetrodotoxin::Language::Monograph&> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));

  Document& document = records[slot];
  auto workspace = get_workspace(document);
  BAIL_IF(!workspace);
  View::Bytes source_name = document.package_root.is_empty()
                                ? document.uri.get_view()
                                : document.logical_route.get_view();
  return workspace->get_monograph(source_name);
}

auto Lsp::Documents::get_completed_monograph(View::Bytes uri)
    -> Option<const Tetrodotoxin::Language::Monograph&> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));

  Document& document = records[slot];
  auto workspace = get_workspace(document);
  BAIL_IF(!workspace);
  View::Bytes source_name = document.package_root.is_empty()
                                ? document.uri.get_view()
                                : document.logical_route.get_view();
  return workspace->get_completed_monograph(source_name);
}

auto Lsp::Documents::get_tokens(View::Bytes uri)
    -> View::Vector<Ttx::Lexical::Token> {
  Count slot = find(uri);
  BAIL_IF(slot == Count(-1));

  Document& document = records[slot];
  auto workspace = get_workspace(document);
  BAIL_IF(!workspace);
  View::Bytes source_name = document.package_root.is_empty()
                                ? document.uri.get_view()
                                : document.logical_route.get_view();
  return workspace->get_tokens(source_name);
}

auto Lsp::Documents::find_definition(
    View::Bytes source_uri,
    const Ttx::Concept::Abstract& semantic)
    -> Option<Environment::Workspace::AuthoredLocation> {
  Count slot = find(source_uri);
  BAIL_IF(slot == Count(-1));

  Document& source_document = records[slot];
  auto workspace = get_workspace(source_document);
  BAIL_IF(!workspace);
  return workspace->find_authored_location(semantic);
}

auto Lsp::Documents::resolve_uri(
    const Environment::Workspace::AuthoredLocation& authored) const
    -> Dynamic::Bytes {
  Dynamic::Bytes target_uri;
  View::Bytes package_root = authored.get_package_root();
  View::Bytes diagnostic_path = authored.get_diagnostic_path();
  for (const Document& document : records.get_view()) {
    if (!document.active) {
      continue;
    }
    Bool standalone = package_root.is_empty() &&
                      document.package_root.is_empty() &&
                      document.uri == diagnostic_path;
    Bool package_member = !package_root.is_empty() &&
                          document.package_root == package_root &&
                          document.logical_route == diagnostic_path;
    if (standalone || package_member) {
      target_uri = document.uri;
      break;
    }
  }

  if (target_uri.is_empty()) {
    constexpr View::Bytes file_prefix = "file://"_view;
    if (package_root.is_empty() &&
        diagnostic_path.get_size() >= file_prefix.get_size() &&
        diagnostic_path.slice(0, file_prefix.get_size()) == file_prefix) {
      target_uri = diagnostic_path;
    } else {
      Dynamic::Bytes path = package_root.is_empty()
                                ? Dynamic::Bytes(diagnostic_path)
                                : join_path(package_root, diagnostic_path);
      Path normalized(path.get_view());
      BAIL_IF(!normalized.is_rooted());
      target_uri = encode_file_uri(normalized.get_view());
    }
  }

  return target_uri;
}

auto Lsp::Documents::invalidate_package(View::Bytes root) -> void {
  for (Count index = 0; index < sessions.get_size(); index++) {
    Session& session = sessions[index];
    Bool affected =
        session.root == root || session.dependencies.get_view().contains(
                                    [&](const Dynamic::Bytes& dependency) {
                                      return dependency == root;
                                    });
    if (session.active && affected) {
      session.errors = {};
      session.workspace = {};
    }
  }
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

    auto contains_path = [&](View::Bytes root) {
      return path.get_size() > root.get_size() &&
             path.get_view().slice(0, root.get_size()) == root &&
             path[root.get_size()] == '/';
    };
    Bool affected = contains_path(session.root.get_view()) ||
                    session.dependencies.get_view().contains(
                        [&](const Dynamic::Bytes& dependency) {
                          return contains_path(dependency.get_view());
                        });
    if (affected) {
      session.errors = {};
      session.workspace = {};
    }
  }
}
