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

  for (Count i = 0; i < outputs.get_size(); i++) {
    const auto& output = outputs[i];
    // Archive and native inventories use the same logical key space. Checking
    // both prevents product kind from becoming an accidental fourth key field.
    for (Count earlier = 0; earlier < i; earlier++) {
      if (outputs[earlier] == output) {
        log_duplicate_output(
            output_kind, outputs[earlier], output_kind, output);
        return {};
      }
    }

    for (Count earlier = 0; earlier < prior_outputs.get_size(); earlier++) {
      if (prior_outputs[earlier] == output) {
        log_duplicate_output(
            prior_kind, prior_outputs[earlier], output_kind, output);
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
        const auto& first = belongs_to_prior
                                ? prior_outputs[earlier]
                                : outputs[earlier - prior_outputs.get_size()];
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
  for (Count i = 0; i < inputs.get_size(); i++) {
    const auto& input = inputs[i];
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
    View::Bytes reason) -> void {
  Diagnostics::Log::Message<640> message(Diagnostics::Log::Level::Info);
  message << repository_select_operation << " failed. reason="_view << reason;
  write_input_key(message, input);
}

// Archive order is durable semantic data while native declaration order is
// only Bazel input order. Compare the inventories as a set so order cannot
// create a false mismatch.
static auto validate_artifacts(
    const Package::Repository::Input& input,
    const Package::Archive::Archive& archive) -> Bool {
  auto expected = archive.get_artifact_ids();
  auto declared = input.get_artifacts();

  // The declaration owns physical locations while Archive owns the semantic
  // ID inventory. Report the exact side that introduced each disagreement
  // without building a second merged representation.
  for (Count i = 0; i < declared.get_size(); i++) {
    View::Bytes artifact_id = declared[i].get_id();
    for (Count earlier = 0; earlier < i; earlier++) {
      if (declared[earlier].get_id() == artifact_id) {
        Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
        message << repository_select_operation
                << " failed. reason=duplicate native artifact mapping"_view;
        write_input_key(message, input);
        message << " artifact_id="_view << artifact_id
                << " first_location="_view
                << declared[earlier].get_filesystem_location()
                << " second_location="_view
                << declared[i].get_filesystem_location();
        return False;
      }
    }

    if (!expected.contains(artifact_id)) {
      Diagnostics::Log::Message<640> message(Diagnostics::Log::Level::Info);
      message << repository_select_operation
              << " failed. reason=unknown native artifact mapping"_view;
      write_input_key(message, input);
      message << " artifact_id="_view << artifact_id << " native_location="_view
              << declared[i].get_filesystem_location();
      return False;
    }
  }

  for (Count i = 0; i < expected.get_size(); i++) {
    View::Bytes artifact_id = expected[i];
    Bool found = False;
    for (Count declared_index = 0; declared_index < declared.get_size();
         declared_index++) {
      if (declared[declared_index].get_id() == artifact_id) {
        found = True;
        break;
      }
    }

    if (!found) {
      Diagnostics::Log::Message<512> message(Diagnostics::Log::Level::Info);
      message << repository_select_operation
              << " failed. reason=missing native artifact mapping"_view;
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
  for (Count i = 0; i < inputs.get_size(); i++) {
    for (Count earlier = 0; earlier < i; earlier++) {
      if (inputs[earlier] == inputs[i]) {
        log_duplicate_input(inputs[earlier], inputs[i]);
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
    Version version) -> Option<const Archive::Archive&> {
  // Archive byte views borrow the same Arena as Repository. Reusing the
  // retained value avoids another file read and keeps later file replacement
  // or removal from changing already selected facts.
  View::Vector<Archive::Archive> cached = archive_cache.get_view();
  for (Count i = 0; i < cached.get_size(); i++) {
    if (cached[i].get_identity() == identity &&
        cached[i].get_version() == version) {
      return cached[i];
    }
  }

  // Missing keys are expected during optional lookup. Only a selected
  // declaration produces a low level validation trace.
  auto selected = find_input(inputs, identity, version);
  if (!selected) {
    return {};
  }

  // Lazy reading keeps an invalid unused declaration inert and avoids touching
  // every filesystem input during Repository construction.
  auto bytes = File::read(arena, (*selected).get_archive_location());
  if (!bytes) {
    log_selection_failure(
        *selected, "the Archive file could not be read."_view);
    return {};
  }

  // File correctly reports an empty regular file as successful bytes.
  // Repository distinguishes that configured product failure from an unreadable
  // path so the Reader never has to infer filesystem state from an empty view.
  if ((*bytes).is_empty()) {
    log_selection_failure(*selected, "the Archive file is empty."_view);
    return {};
  }

  // Reader logs its exact Format 1 rejection before absence reaches this
  // selection boundary. Repository adds the declaration key and location that
  // Reader cannot know.
  auto archive = Archive::Reader::read(arena, *bytes);
  if (!archive) {
    log_selection_failure(
        *selected, "the Archive failed Format 1 validation."_view);
    return {};
  }

  // A valid Archive can still be attached to the wrong Bazel key. Reader cannot
  // check that external declaration, so Repository compares it after decode.
  if ((*archive).get_identity() != (*selected).get_identity() ||
      (*archive).get_version() != (*selected).get_version()) {
    Diagnostics::Log::Message<768> message(Diagnostics::Log::Level::Info);
    message << repository_select_operation
            << " failed. reason=decoded Package key mismatch expected"_view;
    write_input_key(message, *selected);
    message << " actual_identity="_view << (*archive).get_identity()
            << " actual_version="_view;
    write_version(message, (*archive).get_version());
    return {};
  }

  // Returning native paths not named by the Archive would let physical inputs
  // disagree with restored semantic data. Require the exact decoded inventory
  // before either product becomes observable.
  if (!validate_artifacts(*selected, *archive)) {
    return {};
  }

  // Caching before declaration checks would make a failed first selection pass
  // on retry. Publish the retained value only after the whole transaction.
  const Archive::Archive& retained =
      archive_cache.emplace(Archive::Archive(*archive));
  return retained;
}

auto Package::Repository::Repository::select_native(
    View::Bytes identity,
    Version version,
    View::Bytes artifact_id) -> Option<View::Bytes> {
  // Native paths are meaningful only in the artifact inventory of a valid
  // Archive. Reusing semantic selection also shares its cache and validation
  // trace.
  auto archive = select_archive(identity, version);
  if (!archive) {
    return {};
  }

  // The declaration list is already proven to match the Archive, so this lookup
  // can return the borrowed path without reading or copying native bytes.
  auto selected = find_input(inputs, identity, version);
  if (!selected) {
    return {};
  }

  auto artifacts = (*selected).get_artifacts();
  for (Count i = 0; i < artifacts.get_size(); i++) {
    if (artifacts[i].get_id() == artifact_id) {
      return artifacts[i].get_filesystem_location();
    }
  }

  Diagnostics::Log::Message<640> message(Diagnostics::Log::Level::Info);
  message << repository_select_operation
          << " failed. reason=requested native artifact is not declared"_view;
  write_input_key(message, *selected);
  message << " artifact_id="_view << artifact_id;
  return {};
}

auto Package::Repository::Repository::get_archive_output_path(
    View::Bytes identity,
    Version version,
    View::Bytes artifact_id) const -> Option<View::Bytes> {
  for (Count i = 0; i < archive_outputs.get_size(); i++) {
    const auto& output = archive_outputs[i];
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
  for (Count i = 0; i < native_outputs.get_size(); i++) {
    const auto& output = native_outputs[i];
    if (output.get_identity() == identity && output.get_version() == version &&
        output.get_artifact_id() == artifact_id) {
      return output.get_route();
    }
  }

  return {};
}
