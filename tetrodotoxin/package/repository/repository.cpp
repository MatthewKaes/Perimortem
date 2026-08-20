// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/repository/repository.hpp"

#include "perimortem/core/diagnostics/log.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

#include "tetrodotoxin/package/archive/reader.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

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

    // Bazel declarations use one host neutral spelling. Accepting backslashes
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
    // Detecting collisions after normalization prevents one Bazel output from
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

// Bazel supplies a small bounded declaration list, so a linear lookup avoids a
// second index that would duplicate Package key ownership and lifetime state.
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
    Package::Repository::SelectionError error) -> View::Bytes {
  switch (error) {
  case Package::Repository::SelectionError::NotDeclared:
    return "NotDeclared"_view;
  case Package::Repository::SelectionError::Unreadable:
    return "Unreadable"_view;
  case Package::Repository::SelectionError::InvalidFormat:
    return "InvalidFormat"_view;
  case Package::Repository::SelectionError::UnsupportedFormat:
    return "UnsupportedFormat"_view;
  case Package::Repository::SelectionError::PackageKeyMismatch:
    return "PackageKeyMismatch"_view;
  case Package::Repository::SelectionError::ArtifactMismatch:
    return "ArtifactMismatch"_view;
  case Package::Repository::SelectionError::ArtifactNotDeclared:
    return "ArtifactNotDeclared"_view;
  default:
    return "Unknown"_view;
  }
}

template <Count capacity>
static auto write_selection_failure(
    Diagnostics::Log::Message<capacity>& message,
    Package::Repository::SelectionError error,
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
    Package::Repository::SelectionError error,
    View::Bytes identity,
    Version version,
    View::Bytes reason) -> void {
  Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
  write_selection_failure(message, error, identity, version);
  message << " reason="_view << reason;
  write_input_key(message, input);
}

// Archive order is durable semantic data while native declaration order is
// only Bazel input order. Compare the inventories as a set so order cannot
// create a false mismatch.
static auto validate_artifacts(
    const Package::Repository::Input& input,
    const Package::Archive::Archive& archive,
    View::Bytes requested_identity,
    Version requested_version,
    View::Bytes requested_artifact) -> Bool {
  auto expected = archive.get_artifact_ids();
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
            message, Package::Repository::SelectionError::ArtifactMismatch,
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

    if (!expected.contains(artifact_id)) {
      Diagnostics::Log::Message<896> message(Diagnostics::Log::Level::Info);
      write_selection_failure(
          message, Package::Repository::SelectionError::ArtifactMismatch,
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
    View::Bytes artifact_id = expected_data[i];
    Bool found =
        declared.contains([&](const Package::Repository::Artifact& artifact) {
          return artifact.get_id() == artifact_id;
        });

    if (!found) {
      Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
      write_selection_failure(
          message, Package::Repository::SelectionError::ArtifactMismatch,
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

auto Package::Repository::Repository::create(
    Allocator::Arena& arena,
    View::Vector<Input> inputs,
    View::Vector<Output> archive_outputs,
    View::Vector<Output> native_outputs) -> Option<Repository> {
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

  // Construction is the transaction boundary. Returning only after both passes
  // keeps duplicate and collision failures from publishing partial state.
  return Repository(
      arena, inputs, *retained_archive_outputs, *retained_native_outputs);
}

auto Package::Repository::Repository::select_archive(
    View::Bytes identity,
    Version version)
    -> Result<const Archive::Archive&, Package::Repository::SelectionError> {
  using Selection = Result<const Archive::Archive&, SelectionError>;

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

  // A missing declaration is a stable dependency decision rather than a null
  // lookup. Repository records the requested key because no selected Input can
  // carry that evidence for a later textual diagnostic.
  auto selected = find_input(inputs, identity, version);
  if (!selected) {
    Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
    write_selection_failure(
        message, SelectionError::NotDeclared, identity, version);
    message << " reason=no Package input declaration matches the requested "
               "key."_view;
    return SelectionError::NotDeclared;
  }

  // Lazy reading keeps an invalid unused declaration inert and avoids touching
  // every filesystem input during Repository construction.
  auto bytes = File::read(arena, selected->get_archive_location());
  if (!bytes) {
    log_selection_failure(
        *selected, SelectionError::Unreadable, identity, version,
        "the Archive file could not be read."_view);
    return SelectionError::Unreadable;
  }

  // File keeps an empty read distinct from storage failure. Reader classifies
  // those bytes with every other invalid envelope, while Repository adds the
  // declaration key and location that Reader cannot know.
  auto read = Archive::Reader::read(arena, *bytes);
  return read.visit(
      [&](Archive::Archive& archive) -> Selection {
        // A valid Archive can still be attached to the wrong Bazel key. Reader
        // cannot check that external declaration, so Repository compares it
        // after decode.
        if (archive.get_identity() != selected->get_identity() ||
            archive.get_version() != selected->get_version()) {
          Diagnostics::Log::Message<1024> message(
              Diagnostics::Log::Level::Info);
          write_selection_failure(
              message, SelectionError::PackageKeyMismatch, identity, version);
          message << " reason=decoded Package key mismatch expected"_view;
          write_input_key(message, *selected);
          message << " actual_identity="_view << archive.get_identity()
                  << " actual_version="_view;
          write_version(message, archive.get_version());
          return SelectionError::PackageKeyMismatch;
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
              *selected, SelectionError::InvalidFormat, identity, version,
              "the Archive failed Format 1 validation."_view);
          return SelectionError::InvalidFormat;
        case Archive::Reader::Error::UnsupportedFormat:
          log_selection_failure(
              *selected, SelectionError::UnsupportedFormat, identity, version,
              "the Archive format revision is unsupported."_view);
          return SelectionError::UnsupportedFormat;
        default:
          log_selection_failure(
              *selected, SelectionError::Unknown, identity, version,
              "the Archive reader returned an unknown error."_view);
          return SelectionError::Unknown;
        }
      });
}

auto Package::Repository::Repository::select_native(
    View::Bytes identity,
    Version version,
    View::Bytes artifact_id)
    -> Result<View::Bytes, Package::Repository::SelectionError> {
  using Selection = Result<View::Bytes, SelectionError>;

  // Semantic failures already have one exact Repository record. Propagating
  // the selected category keeps native control flow typed without manufacturing
  // a second explanation for the same failed Archive.
  auto archive_selection = select_archive(identity, version);
  return archive_selection.visit(
      [&](const Package::Archive::Archive& archive) -> Selection {
        auto selected = find_input(inputs, identity, version);
        // Repository inputs are immutable, so Archive success proves this
        // declaration. Keep the proof local because native validation cannot
        // accept a missing owner.
        if (!selected) {
          return SelectionError::NotDeclared;
        }

        // Native declarations are one complete physical representation of the
        // Archive artifact inventory. Validation stays here so semantic cache
        // publication remains useful when that representation is absent or bad.
        Bool artifacts_match = validate_artifacts(
            *selected, archive, identity, version, artifact_id);
        if (!artifacts_match) {
          return SelectionError::ArtifactMismatch;
        }

        // Once the inventories agree, the requested ID can expose its borrowed
        // path without a native read or another retained representation.
        auto artifacts = selected->get_artifacts();
        const auto* artifact_data = artifacts.get_data();
        for (Count i = 0; i < artifacts.get_size(); i++) {
          if (artifact_data[i].get_id() == artifact_id) {
            return artifact_data[i].get_filesystem_location();
          }
        }

        Diagnostics::Log::Message<896> message(Diagnostics::Log::Level::Info);
        write_selection_failure(
            message, SelectionError::ArtifactNotDeclared, identity, version);
        message << " requested_artifact="_view << artifact_id
                << " reason=requested native artifact is not declared"_view;
        write_input_key(message, *selected);
        message << " artifact_id="_view << artifact_id;
        return SelectionError::ArtifactNotDeclared;
      },
      [](SelectionError error) -> Selection { return error; });
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
