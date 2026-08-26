// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/repository/repository.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"
#include "perimortem/serialization/stream/textual.hpp"

#include "tetrodotoxin/linker/manifest.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

static auto append_package_root(
    Dynamic::Bytes& path,
    View::Bytes root,
    View::Bytes identity,
    Version version) -> Bool {
  BAIL_IF(
      root.is_empty() || identity.is_empty() || version.is_null() ||
      identity.get_size() > Path::max_size);

  for (Count index = 0; index < identity.get_size(); index++) {
    U8 byte = identity[index];
    BAIL_IF(byte == '/' || byte == '\\' || byte == '\0');
  }

  path = root;
  if (path[path.get_size() - 1] != '/') {
    path.append('/');
  }

  path.concat(identity);
  path.append('/');
  Perimortem::Serialization::Stream::Textual<Dynamic::Bytes> output(path);
  output << version.get_major() << "."_view << version.get_minor();
  return path.get_size() <= Path::max_size;
}

static auto package_product_path(
    View::Bytes root,
    View::Bytes identity,
    Version version,
    View::Bytes product) -> Dynamic::Bytes {
  Dynamic::Bytes path;
  BAIL_IF(!append_package_root(path, root, identity, version));
  path.append('/');
  path.concat(product);
  BAIL_IF(path.get_size() > Path::max_size);
  return path;
}

// Repository logs describe declaration and selection facts that a textual
// source diagnostic cannot recover after this transaction returns.
static constexpr View::Bytes repository_create_operation =
    "Package::Repository declaration construction"_view;
static constexpr View::Bytes repository_select_operation =
    "Package::Repository exact input selection"_view;

template <Count capacity>
static auto write_version(
    Diagnostics::Log::Message<capacity>& message,
    Version version) -> void {
  message << version.get_major() << '.' << version.get_minor();
}

template <Count capacity>
static auto write_output(
    Diagnostics::Log::Message<capacity>& message,
    View::Bytes label,
    View::Bytes kind,
    const Package::Repository::Output& output) -> void {
  message << ' ' << label << "_kind="_view << kind << ' ' << label
          << "_identity="_view << output.get_identity() << ' ' << label
          << "_version="_view;
  write_version(message, output.get_version());
  message << ' ' << label << "_artifact="_view << output.get_artifact_id()
          << ' ' << label << "_route="_view << output.get_route();
}

static auto log_duplicate_output(
    View::Bytes first_kind,
    const Package::Repository::Output& first,
    View::Bytes second_kind,
    const Package::Repository::Output& second) -> void {
  Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
  message << repository_create_operation
          << " failed. reason=duplicate key"_view;
  write_output(message, "first"_view, first_kind, first);
  write_output(message, "second"_view, second_kind, second);
}

static auto log_invalid_output(
    View::Bytes kind,
    const Package::Repository::Output& output,
    View::Bytes reason) -> void {
  Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
  message << repository_create_operation << " failed. reason="_view << reason;
  write_output(message, "output"_view, kind, output);
}

static auto log_route_collision(
    View::Bytes first_kind,
    const Package::Repository::Output& first,
    View::Bytes second_kind,
    const Package::Repository::Output& second,
    View::Bytes normalized) -> void {
  Diagnostics::Log::Message<1024> message(Diagnostics::Log::Level::Info);
  message << repository_create_operation
          << " failed. reason=normalized route collision normalized_route="_view
          << normalized;
  write_output(message, "first"_view, first_kind, first);
  write_output(message, "second"_view, second_kind, second);
}

// Output keys already satisfy the caller Arena lifetime contract. Only routes
// need retention because Path derives a new spelling that cannot borrow the
// authored declaration.
static auto retain_outputs(
    Allocator::Arena& arena,
    View::Vector<Package::Repository::Output> outputs,
    View::Bytes output_kind,
    View::Vector<Package::Repository::Output> prior_outputs,
    View::Bytes prior_kind,
    Dynamic::Vector<View::Bytes>& normalized_routes)
    -> Option<View::Vector<Package::Repository::Output>> {
  Managed::Vector<Package::Repository::Output> retained(arena);
  const auto* prior_output_data = prior_outputs.get_data();

  for (Count i = 0; i < outputs.get_size(); i++) {
    const auto& output = outputs.get_data()[i];
    // Archive and native inventories use the same logical key space. Checking
    // both prevents product kind from becoming an accidental fourth key field.
    for (Count earlier = 0; earlier < i; earlier++) {
      if (outputs.get_data()[earlier] == output) {
        log_duplicate_output(
            output_kind, outputs.get_data()[earlier], output_kind, output);
        return {};
      }
    }

    for (Count earlier = 0; earlier < prior_outputs.get_size(); earlier++) {
      if (prior_output_data[earlier] == output) {
        log_duplicate_output(
            prior_kind, prior_output_data[earlier], output_kind, output);
        return {};
      }
    }

    // Product declarations use one host neutral spelling. Accepting backslashes
    // would make the authored contract platform dependent even though Path can
    // normalize them for general filesystem use.
    View::Bytes route = output.get_route();
    if (route.is_empty()) {
      log_invalid_output(
          output_kind, output, "the output route is empty."_view);
      return {};
    }

    if (route.get_size() > Path::max_size) {
      log_invalid_output(
          output_kind, output,
          "the output route exceeds the Path capacity."_view);
      return {};
    }

    if (route[0] == '/') {
      log_invalid_output(
          output_kind, output, "the output route is rooted."_view);
      return {};
    }

    for (Count byte = 0; byte < route.get_size(); byte++) {
      if (route[byte] == '\0') {
        log_invalid_output(
            output_kind, output, "the output route contains a NUL byte."_view);
        return {};
      }

      if (route[byte] == '\\') {
        log_invalid_output(
            output_kind, output, "the output route contains a backslash."_view);
        return {};
      }
    }

    // Ask Path to write directly into the final Arena storage. Constructing a
    // temporary Path here and proxying it would add a copy to every output.
    auto normalized = Path::normalize(arena, route);
    if (!normalized) {
      log_invalid_output(
          output_kind, output,
          "the output route escapes or names no destination after "
          "normalization."_view);
      return {};
    }

    // Different lexical routes can collapse to one physical destination.
    // Detecting collisions after normalization prevents one product output from
    // overwriting another through `.` or parent aliases.
    for (Count earlier = 0; earlier < normalized_routes.get_size(); earlier++) {
      if (normalized_routes[earlier] == *normalized) {
        const Bool belongs_to_prior = earlier < prior_outputs.get_size();
        const auto& first =
            belongs_to_prior
                ? prior_output_data[earlier]
                : outputs.get_data()[earlier - prior_outputs.get_size()];
        log_route_collision(
            belongs_to_prior ? prior_kind : output_kind, first, output_kind,
            output, *normalized);
        return {};
      }
    }

    // Retain only the normalized route needed by the resulting Output. The
    // transaction vector keeps collision state off the Arena so construction
    // does not leave a second owner inventory behind.
    normalized_routes.emplace(View::Bytes(*normalized));
    retained.emplace(
        Package::Repository::Output(
            output.get_identity(), output.get_version(),
            output.get_artifact_id(), *normalized));
  }

  return retained.get_view();
}

// A caller supplies a small bounded declaration list, so a linear lookup avoids
// a second index that would duplicate Package key ownership and lifetime state.
static auto find_input(
    View::Vector<Package::Repository::Input> inputs,
    View::Bytes identity,
    Version version) -> Option<const Package::Repository::Input&> {
  const auto* input_data = inputs.get_data();
  for (Count i = 0; i < inputs.get_size(); i++) {
    const auto& input = input_data[i];
    if (input.get_identity() == identity && input.get_version() == version) {
      return input;
    }
  }

  return {};
}

template <Count capacity>
static auto write_input_key(
    Diagnostics::Log::Message<capacity>& message,
    const Package::Repository::Input& input) -> void {
  message << " identity="_view << input.get_identity() << " version="_view;
  write_version(message, input.get_version());
  message << " archive_location="_view << input.get_archive_location();
}

template <Count capacity>
static auto write_requested_key(
    Diagnostics::Log::Message<capacity>& message,
    View::Bytes identity,
    Version version) -> void {
  message << " requested_identity="_view << identity
          << " requested_version="_view;
  write_version(message, version);
}

static constexpr auto selection_error_name(
    Package::Repository::Repository::Error error) -> View::Bytes {
  switch (error) {
  case Package::Repository::Repository::Error::NotDeclared:
    return "NotDeclared"_view;
  case Package::Repository::Repository::Error::Unreadable:
    return "Unreadable"_view;
  case Package::Repository::Repository::Error::InvalidFormat:
    return "InvalidFormat"_view;
  case Package::Repository::Repository::Error::UnsupportedFormat:
    return "UnsupportedFormat"_view;
  case Package::Repository::Repository::Error::PackageKeyMismatch:
    return "PackageKeyMismatch"_view;
  case Package::Repository::Repository::Error::ArtifactMismatch:
    return "ArtifactMismatch"_view;
  case Package::Repository::Repository::Error::AbiMismatch:
    return "AbiMismatch"_view;
  case Package::Repository::Repository::Error::ArtifactNotDeclared:
    return "ArtifactNotDeclared"_view;
  default:
    return "Unknown"_view;
  }
}

template <Count capacity>
static auto write_selection_failure(
    Diagnostics::Log::Message<capacity>& message,
    Package::Repository::Repository::Error error,
    View::Bytes identity,
    Version version) -> void {
  message << repository_select_operation << " failed. selection_error="_view
          << selection_error_name(error);
  write_requested_key(message, identity, version);
}

static auto log_duplicate_input(
    const Package::Repository::Input& first,
    const Package::Repository::Input& second) -> void {
  Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
  message << repository_create_operation
          << " failed. reason=duplicate Package input key first"_view;
  write_input_key(message, first);
  message << " second"_view;
  write_input_key(message, second);
}

static auto log_selection_failure(
    const Package::Repository::Input& input,
    Package::Repository::Repository::Error error,
    View::Bytes identity,
    Version version,
    View::Bytes reason) -> void {
  Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
  write_selection_failure(message, error, identity, version);
  message << " reason="_view << reason;
  write_input_key(message, input);
}

// Archive order is durable semantic data while native declaration order is
// only caller input order. Compare the inventories as a set so order cannot
// create a false mismatch.
static auto validate_artifacts(
    const Package::Repository::Input& input,
    const Package::Archive::Archive& archive,
    View::Bytes requested_identity,
    Version requested_version,
    View::Bytes requested_artifact) -> Bool {
  auto expected = archive.get_artifacts();
  auto declared = input.get_artifacts();
  const auto* expected_data = expected.get_data();
  const auto* declared_data = declared.get_data();

  // The declaration owns physical locations while Archive owns the semantic
  // ID inventory. Report the exact side that introduced each disagreement
  // without building a second merged representation.
  for (Count i = 0; i < declared.get_size(); i++) {
    View::Bytes artifact_id = declared_data[i].get_id();
    for (Count earlier = 0; earlier < i; earlier++) {
      if (declared_data[earlier].get_id() == artifact_id) {
        Diagnostics::Log::Message<1024> message(Diagnostics::Log::Level::Info);
        write_selection_failure(
            message, Package::Repository::Repository::Error::ArtifactMismatch,
            requested_identity, requested_version);
        message << " requested_artifact="_view << requested_artifact
                << " reason=duplicate native artifact mapping"_view;
        write_input_key(message, input);
        message << " artifact_id="_view << artifact_id
                << " first_location="_view
                << declared_data[earlier].get_filesystem_location()
                << " second_location="_view
                << declared_data[i].get_filesystem_location();
        return False;
      }
    }

    Bool expected_id = False;
    for (const Package::Archive::Artifact& artifact : expected) {
      expected_id |= artifact.get_id() == artifact_id;
    }
    if (!expected_id) {
      Diagnostics::Log::Message<896> message(Diagnostics::Log::Level::Info);
      write_selection_failure(
          message, Package::Repository::Repository::Error::ArtifactMismatch,
          requested_identity, requested_version);
      message << " requested_artifact="_view << requested_artifact
              << " reason=unknown native artifact mapping"_view;
      write_input_key(message, input);
      message << " artifact_id="_view << artifact_id << " native_location="_view
              << declared_data[i].get_filesystem_location();
      return False;
    }
  }

  for (Count i = 0; i < expected.get_size(); i++) {
    View::Bytes artifact_id = expected_data[i].get_id();
    Bool found =
        declared.contains([&](const Package::Repository::Artifact& artifact) {
          return artifact.get_id() == artifact_id;
        });

    if (!found) {
      Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
      write_selection_failure(
          message, Package::Repository::Repository::Error::ArtifactMismatch,
          requested_identity, requested_version);
      message << " requested_artifact="_view << requested_artifact
              << " reason=missing native artifact mapping"_view;
      write_input_key(message, input);
      message << " artifact_id="_view << artifact_id;
      return False;
    }
  }

  return True;
}

static auto validate_abi_manifest(
    const Package::Repository::Input& input,
    const Package::Archive::Archive& archive,
    const Package::Repository::Artifact& declared,
    View::Bytes identity,
    Version version) -> Bool {
  auto bytes = File::read(declared.get_abi_manifest_location());
  if (!bytes) {
    Diagnostics::Log::Message<896> message(Diagnostics::Log::Level::Info);
    write_selection_failure(
        message, Package::Repository::Repository::Error::AbiMismatch, identity,
        version);
    message << " requested_artifact="_view << declared.get_id()
            << " reason=the native ABI Manifest is unreadable"_view;
    write_input_key(message, input);
    message << " manifest_location="_view
            << declared.get_abi_manifest_location();
    return False;
  }

  Allocator::Arena arena;
  auto decoded = Linker::Manifest::read(arena, *bytes);
  Option<Linker::Manifest> manifest;
  decoded.visit(
      [&](const Linker::Manifest& value) { manifest = value; },
      [](const Linker::Manifest::Error&) {});
  Bool matches = manifest && manifest->get_artifact() == declared.get_id() &&
                 archive.matches(*manifest);
  if (matches) {
    return True;
  }

  Diagnostics::Log::Message<1024> message(Diagnostics::Log::Level::Info);
  write_selection_failure(
      message, Package::Repository::Repository::Error::AbiMismatch, identity,
      version);
  message << " requested_artifact="_view << declared.get_id()
          << " reason=the native ABI Manifest disagrees with the Archive"_view;
  write_input_key(message, input);
  message << " manifest_location="_view << declared.get_abi_manifest_location();
  return False;
}

auto Package::Repository::Repository::create(
    Allocator::Arena& arena,
    View::Vector<Input> inputs,
    View::Vector<Output> archive_outputs,
    View::Vector<Output> native_outputs,
    View::Bytes installed_root) -> Option<Repository> {
  // Two locations for one Package key would make selection depend on
  // declaration order. Input equality intentionally ignores those locations so
  // the ambiguity is rejected here.
  const auto* input_data = inputs.get_data();
  for (Count i = 0; i < inputs.get_size(); i++) {
    for (Count earlier = 0; earlier < i; earlier++) {
      if (input_data[earlier] == input_data[i]) {
        log_duplicate_input(input_data[earlier], input_data[i]);
        return {};
      }
    }
  }

  // Archive routes are checked first so cross kind failures can name the
  // original declarations without introducing a second product registry.
  Dynamic::Vector<View::Bytes> normalized_routes;
  auto retained_archive_outputs = retain_outputs(
      arena, archive_outputs, "Archive"_view, View::Vector<Output>(),
      View::Bytes(), normalized_routes);
  if (!retained_archive_outputs) {
    return {};
  }

  // Reusing normalized_routes gives both product kinds one physical collision
  // domain without introducing a separate publication registry.
  auto retained_native_outputs = retain_outputs(
      arena, native_outputs, "native"_view, archive_outputs, "Archive"_view,
      normalized_routes);
  if (!retained_native_outputs) {
    return {};
  }

  View::Bytes retained_root;
  if (!installed_root.is_empty()) {
    Path root(installed_root);
    if (!root.is_rooted()) {
      Diagnostics::Log::Message<384> message(Diagnostics::Log::Level::Info);
      message << repository_create_operation
              << " failed. reason=the installed root is not rooted root="_view
              << installed_root;
      return {};
    }

    auto normalized = Path::normalize(arena, installed_root);
    if (!normalized) {
      Diagnostics::Log::Message<384> message(Diagnostics::Log::Level::Info);
      message << repository_create_operation
              << " failed. reason=the installed root is invalid root="_view
              << installed_root;
      return {};
    }

    retained_root = *normalized;
  }

  // Construction is the transaction boundary. The source root joins the two
  // product inventories only after every declaration has one stable meaning.
  return Repository(
      arena, inputs, *retained_archive_outputs, *retained_native_outputs,
      retained_root);
}

auto Package::Repository::Repository::select_source(
    View::Bytes identity,
    Version version) -> Result<View::Bytes, Error> {
  for (const SourceSelection& cached : source_cache.get_view()) {
    if (cached.identity == identity && cached.version == version) {
      return cached.root;
    }
  }

  auto input = find_input(inputs, identity, version);
  View::Bytes declared = input ? input->get_source_location() : View::Bytes();
  Dynamic::Bytes installed;
  if (declared.is_empty() && !installed_root.is_empty()) {
    if (!append_package_root(installed, installed_root, identity, version)) {
      return Error::Unreadable;
    }

    declared = installed.get_view();
  }

  if (declared.is_empty()) {
    Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
    write_selection_failure(message, Error::NotDeclared, identity, version);
    message
        << " reason=no local source or installed Package matches the key"_view;
    return Error::NotDeclared;
  }

  Path normalized(declared);
  Dynamic::Bytes manifest(normalized.get_view());
  if (!normalized.is_rooted() || normalized.get_view().is_empty()) {
    Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
    write_selection_failure(message, Error::Unreadable, identity, version);
    message << " reason=the selected source root is invalid root="_view
            << declared;
    return Error::Unreadable;
  }

  manifest.concat("/package.ttx"_view);
  if (!File::exists(manifest.get_view())) {
    Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
    write_selection_failure(message, Error::Unreadable, identity, version);
    message << " reason=the selected source root has no package.ttx root="_view
            << normalized.get_view();
    return Error::Unreadable;
  }

  View::Bytes retained = arena.proxy(normalized.get_view());
  source_cache.insert(SourceSelection(identity, version, retained));
  return retained;
}

auto Package::Repository::Repository::select_archive(
    View::Bytes identity,
    Version version)
    -> Result<const Archive::Archive&, Package::Repository::Repository::Error> {
  using Selection = Result<const Archive::Archive&, Error>;

  // Archive byte views borrow the same Arena as Repository. Reusing the
  // retained value avoids another file read and keeps later file replacement
  // or removal from changing already selected facts.
  auto cached = archive_cache.get_view();
  auto cached_data = cached.get_data();
  for (Count i = 0; i < cached.get_size(); i++) {
    if (cached_data[i].get_identity() == identity &&
        cached_data[i].get_version() == version) {
      return cached_data[i];
    }
  }

  auto selected = find_input(inputs, identity, version);
  Dynamic::Bytes installed_archive;
  View::Bytes archive_location =
      selected ? selected->get_archive_location() : View::Bytes();
  if (archive_location.is_empty() && !installed_root.is_empty()) {
    installed_archive = package_product_path(
        installed_root, identity, version, "contract.txa"_view);
    archive_location = installed_archive.get_view();
  }

  if (archive_location.is_empty()) {
    Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
    write_selection_failure(message, Error::NotDeclared, identity, version);
    if (!selected && installed_root.is_empty()) {
      message << " reason=no Package input declaration matches the requested "
                 "key."_view;
    } else {
      message
          << " reason=no declared or installed Contract matches the key"_view;
    }

    return Error::NotDeclared;
  }

  Input declaration(
      identity, version, archive_location,
      selected ? selected->get_artifacts() : View::Vector<Artifact>(),
      selected ? selected->get_source_location() : View::Bytes());

  // Lazy reading leaves unused installed coordinates inert. A selected source
  // Package can therefore remain useful to the editor even when no Contract
  // product has been installed beside it.
  auto bytes = File::read(arena, declaration.get_archive_location());
  if (!bytes) {
    log_selection_failure(
        declaration, Error::Unreadable, identity, version,
        "the Archive file could not be read."_view);
    return Error::Unreadable;
  }

  // File keeps an empty read distinct from storage failure. Reader classifies
  // those bytes with every other invalid envelope, while Repository adds the
  // declaration key and location that Reader cannot know.
  auto read = Archive::Reader::read(arena, *bytes);
  return read.visit(
      [&](Archive::Archive& archive) -> Selection {
        // A valid Archive can still be attached to the wrong caller key. Reader
        // cannot check that external declaration, so Repository compares it
        // after decode.
        if (archive.get_identity() != declaration.get_identity() ||
            archive.get_version() != declaration.get_version()) {
          Diagnostics::Log::Message<1024> message(
              Diagnostics::Log::Level::Info);
          write_selection_failure(
              message, Error::PackageKeyMismatch, identity, version);
          message << " reason=decoded Package key mismatch expected"_view;
          write_input_key(message, declaration);
          message << " actual_identity="_view << archive.get_identity()
                  << " actual_version="_view;
          write_version(message, archive.get_version());
          return Error::PackageKeyMismatch;
        }

        // The semantic cache has no native declaration dependency. Workspace
        // can therefore restore a valid Archive without native inputs, and a
        // later native mismatch cannot poison these retained facts.
        const Archive::Archive& retained =
            archive_cache.emplace(Archive::Archive(archive));
        return retained;
      },
      [&](Archive::Reader::Error read_error) -> Selection {
        switch (read_error) {
        case Archive::Reader::Error::InvalidFormat:
          log_selection_failure(
              declaration, Error::InvalidFormat, identity, version,
              "the Archive failed validation."_view);
          return Error::InvalidFormat;
        case Archive::Reader::Error::UnsupportedFormat:
          log_selection_failure(
              declaration, Error::UnsupportedFormat, identity, version,
              "the Archive format revision is unsupported."_view);
          return Error::UnsupportedFormat;
        default:
          log_selection_failure(
              declaration, Error::Unknown, identity, version,
              "the Archive reader returned an unknown error."_view);
          return Error::Unknown;
        }
      });
}

auto Package::Repository::Repository::select_manifest(
    View::Bytes identity,
    Version version,
    View::Bytes artifact_id) -> Result<const Linker::Manifest&, Error> {
  for (const Linker::Manifest& cached : manifest_cache.get_view()) {
    if (cached.get_identity() == identity && cached.get_version() == version &&
        cached.get_artifact() == artifact_id) {
      return cached;
    }
  }

  Option<const Archive::Archive&> archive;
  Option<Error> archive_error;
  select_archive(identity, version)
      .visit(
          [&](const Archive::Archive& selected) { archive = selected; },
          [&](Error error) { archive_error = error; });
  if (archive_error || !archive) {
    return archive_error ? *archive_error : Error::Unknown;
  }

  auto input = find_input(inputs, identity, version);
  Option<const Artifact&> declared;
  if (input) {
    for (const Artifact& candidate : input->get_artifacts()) {
      if (candidate.get_id() == artifact_id) {
        declared = candidate;
        break;
      }
    }
  }

  Dynamic::Bytes installed_manifest;
  View::Bytes manifest_location =
      declared ? declared->get_abi_manifest_location() : View::Bytes();
  if (manifest_location.is_empty() && !installed_root.is_empty()) {
    installed_manifest = package_product_path(
        installed_root, identity, version, "abi.manifest"_view);
    manifest_location = installed_manifest.get_view();
  }

  if (manifest_location.is_empty()) {
    Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
    write_selection_failure(
        message, Error::ArtifactNotDeclared, identity, version);
    message << " requested_artifact="_view << artifact_id
            << " reason=no ABI Manifest matches the artifact"_view;
    return Error::ArtifactNotDeclared;
  }

  auto bytes = File::read(arena, manifest_location);
  if (!bytes) {
    Diagnostics::Log::Message<640> message(Diagnostics::Log::Level::Info);
    write_selection_failure(message, Error::AbiMismatch, identity, version);
    message << " requested_artifact="_view << artifact_id
            << " reason=the ABI Manifest is unreadable location="_view
            << manifest_location;
    return Error::AbiMismatch;
  }

  Option<Linker::Manifest> manifest;
  Linker::Manifest::read(arena, *bytes)
      .visit(
          [&](const Linker::Manifest& selected) { manifest = selected; },
          [](Linker::Manifest::Error) {});
  if (!manifest || manifest->get_identity() != identity ||
      manifest->get_version() != version ||
      manifest->get_artifact() != artifact_id || !archive->matches(*manifest)) {
    Diagnostics::Log::Message<640> message(Diagnostics::Log::Level::Info);
    write_selection_failure(message, Error::AbiMismatch, identity, version);
    message << " requested_artifact="_view << artifact_id
            << " reason=the ABI Manifest disagrees with the Contract"_view;
    return Error::AbiMismatch;
  }

  manifest_cache.insert(*manifest);
  const Linker::Manifest& retained =
      manifest_cache[manifest_cache.get_size() - 1];
  return retained;
}

auto Package::Repository::Repository::select_native(
    View::Bytes identity,
    Version version,
    View::Bytes artifact_id)
    -> Result<View::Bytes, Package::Repository::Repository::Error> {
  using Selection = Result<View::Bytes, Error>;

  for (const NativeSelection& cached : native_cache.get_view()) {
    if (cached.identity == identity && cached.version == version &&
        cached.artifact == artifact_id) {
      return cached.path;
    }
  }

  // Semantic failures already have one exact Repository record. Propagating
  // the selected category keeps native control flow typed without manufacturing
  // a second explanation for the same failed Archive.
  auto archive_selection = select_archive(identity, version);
  return archive_selection.visit(
      [&](const Package::Archive::Archive& archive) -> Selection {
        auto selected = find_input(inputs, identity, version);
        if ((!selected || selected->get_artifacts().is_empty()) &&
            !installed_root.is_empty()) {
          Bool declared = archive.get_artifacts().contains(
              [&](const Package::Archive::Artifact& artifact) {
                return artifact.get_id() == artifact_id;
              });
          if (!declared) {
            Diagnostics::Log::Message<768> message(
                Diagnostics::Log::Level::Info);
            write_selection_failure(
                message, Error::ArtifactNotDeclared, identity, version);
            message << " requested_artifact="_view << artifact_id
                    << " reason=requested native artifact is not declared"_view;
            return Error::ArtifactNotDeclared;
          }

          for (Count index = 0; index < artifact_id.get_size(); index++) {
            U8 byte = artifact_id[index];
            if (byte == '/' || byte == '\\' || byte == '\0') {
              return Error::ArtifactNotDeclared;
            }
          }

          Dynamic::Bytes package_root;
          if (!append_package_root(
                  package_root, installed_root, identity, version)) {
            return Error::ArtifactNotDeclared;
          }

          Dynamic::Bytes installed_native(package_root.get_view());
          installed_native.concat("/native/"_view);
          installed_native.concat(artifact_id);
          installed_native.concat("/package.a"_view);
          if (installed_native.get_size() > Path::max_size) {
            return Error::ArtifactNotDeclared;
          }

          Bool manifest_matches =
              select_manifest(identity, version, artifact_id)
                  .visit(
                      [](const Linker::Manifest&) { return True; },
                      [](Error) { return False; });
          if (!manifest_matches) {
            return Error::AbiMismatch;
          }

          View::Bytes retained_path = arena.proxy(installed_native.get_view());
          native_cache.insert(
              NativeSelection(identity, version, artifact_id, retained_path));
          return retained_path;
        }

        if (!selected) {
          return Error::NotDeclared;
        }

        // Native declarations are one complete physical representation of the
        // Archive artifact inventory. Validation stays here so semantic cache
        // publication remains useful when that representation is absent or bad.
        Bool artifacts_match = validate_artifacts(
            *selected, archive, identity, version, artifact_id);
        if (!artifacts_match) {
          return Error::ArtifactMismatch;
        }

        // Once the inventories agree, the requested ID can expose its borrowed
        // path without a native read or another retained representation.
        auto artifacts = selected->get_artifacts();
        const auto* artifact_data = artifacts.get_data();
        for (Count i = 0; i < artifacts.get_size(); i++) {
          if (artifact_data[i].get_id() == artifact_id) {
            if (!validate_abi_manifest(
                    *selected, archive, artifact_data[i], identity, version)) {
              return Error::AbiMismatch;
            }
            return artifact_data[i].get_filesystem_location();
          }
        }

        Diagnostics::Log::Message<896> message(Diagnostics::Log::Level::Info);
        write_selection_failure(
            message, Error::ArtifactNotDeclared, identity, version);
        message << " requested_artifact="_view << artifact_id
                << " reason=requested native artifact is not declared"_view;
        write_input_key(message, *selected);
        message << " artifact_id="_view << artifact_id;
        return Error::ArtifactNotDeclared;
      },
      [](Error error) -> Selection { return error; });
}

auto Package::Repository::Repository::get_archive_output_path(
    View::Bytes identity,
    Version version,
    View::Bytes artifact_id) const -> Option<View::Bytes> {
  const auto* output_data = archive_outputs.get_data();
  for (Count i = 0; i < archive_outputs.get_size(); i++) {
    const auto& output = output_data[i];
    if (output.get_identity() == identity && output.get_version() == version &&
        output.get_artifact_id() == artifact_id) {
      return output.get_route();
    }
  }

  return {};
}

auto Package::Repository::Repository::get_native_output_path(
    View::Bytes identity,
    Version version,
    View::Bytes artifact_id) const -> Option<View::Bytes> {
  const auto* output_data = native_outputs.get_data();
  for (Count i = 0; i < native_outputs.get_size(); i++) {
    const auto& output = output_data[i];
    if (output.get_identity() == identity && output.get_version() == version &&
        output.get_artifact_id() == artifact_id) {
      return output.get_route();
    }
  }

  return {};
}
