// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/repository/repository.hpp"

#include "validation/unit_test.hpp"

#include <stdio.h>
#include <stdlib.h>

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"
#include "perimortem/system/path.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Validation;

static_assert(
    static_cast<Unsigned_8>(Package::Repository::SelectionError::Unknown) ==
    Unsigned_8(-1));
static_assert(
    static_cast<Unsigned_8>(Package::Repository::SelectionError::NotDeclared) ==
    Unsigned_8(0));

template <typename value_type>
static auto returns_selection_error(
    const Result<value_type, Package::Repository::SelectionError>& result,
    Package::Repository::SelectionError expected) -> Bool {
  return result.visit(
      [](const auto&) { return False; },
      [&](Package::Repository::SelectionError error) {
        return error == expected ? True : False;
      });
}

static auto selected_archive(
    const Result<
        const Package::Archive::Archive&,
        Package::Repository::SelectionError>& result)
    -> const Package::Archive::Archive* {
  return result.visit(
      [](const Package::Archive::Archive& archive) { return &archive; },
      [](Package::Repository::SelectionError) {
        return static_cast<const Package::Archive::Archive*>(nullptr);
      });
}

static auto selected_native(
    const Result<View::Bytes, Package::Repository::SelectionError>& result)
    -> const View::Bytes* {
  return result.visit(
      [](const View::Bytes& path) { return &path; },
      [](Package::Repository::SelectionError) {
        return static_cast<const View::Bytes*>(nullptr);
      });
}

// This manually encoded Format 1 value is independent of Archive Writer. The
// Repository tests write only this literal or direct byte mutations of it, so
// the selected file oracle cannot reproduce a Writer defect.
static constexpr Unsigned_8 format_one_archive[] = {
  0x54, 0x54, 0x58, 0x41, 0x01, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x00, 0x00, 0x01,
  0x00, 0x01, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x50, 0x6B,
  0x67, 0x2E, 0x43, 0x6F, 0x72, 0x65, 0x02, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x43,
  0x6F, 0x72, 0x65, 0x08, 0x00, 0x00, 0x00, 0x50, 0x6B, 0x67, 0x2E, 0x42, 0x61,
  0x73, 0x65, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01, 0x00, 0x3D, 0x00, 0x00,
  0x00, 0x02, 0x00, 0x00, 0x00, 0x13, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
  0x4D, 0x61, 0x69, 0x6E, 0x03, 0x00, 0x00, 0x00, 0x4C, 0x69, 0x62, 0x00, 0x00,
  0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x53, 0x63, 0x65,
  0x6E, 0x65, 0x3A, 0x3A, 0x4F, 0x6E, 0x65, 0x05, 0x00, 0x00, 0x00, 0x53, 0x63,
  0x65, 0x6E, 0x65, 0x03, 0x00, 0x00, 0x00, 0xAA, 0x00, 0x55, 0x05, 0x00, 0x01,
  0x00, 0x1A, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x07, 0x00, 0x00, 0x00,
  0x03, 0x00, 0x00, 0x00, 0x63, 0x70, 0x75, 0x07, 0x00, 0x00, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x72, 0x65, 0x73, 0x06, 0x00, 0x01, 0x00, 0x46, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x4D,
  0x61, 0x69, 0x6E, 0x3A, 0x3A, 0x52, 0x75, 0x6E, 0x03, 0x00, 0x00, 0x00, 0x63,
  0x70, 0x75, 0x04, 0x00, 0x00, 0x00, 0x6D, 0x61, 0x69, 0x6E, 0x1E, 0x00, 0x00,
  0x00, 0x0A, 0x00, 0x00, 0x00, 0x53, 0x63, 0x65, 0x6E, 0x65, 0x3A, 0x3A, 0x4F,
  0x6E, 0x65, 0x03, 0x00, 0x00, 0x00, 0x72, 0x65, 0x73, 0x05, 0x00, 0x00, 0x00,
  0x61, 0x73, 0x73, 0x65, 0x74,
};

static constexpr Count archive_identity_offset = 24;
static constexpr Count archive_version_offset = 40;
static constexpr Count temporary_path_capacity = 192;

// The unit harness retains only the newest log message. Selection rejection
// emits Reader detail before Repository context, so this sink observes both
// owners while still forwarding the newest record to the ordinary assertions.
static View::Bytes expected_selection_info;
static View::Bytes expected_selection_debug;
static Bool selection_info_seen;
static Bool selection_debug_seen;
static Count selection_log_count;

static auto capture_selection_logs(
    Diagnostics::Log::Level level,
    View::Bytes message,
    const Diagnostics::Source& location) -> void {
  Test::capture_sink(level, message, location);
  selection_log_count++;

  if (level == Diagnostics::Log::Level::Info &&
      Test::error_contains(expected_selection_info, level)) {
    selection_info_seen = True;
  } else if (
      level == Diagnostics::Log::Level::Debug &&
      Test::error_contains(expected_selection_debug, level)) {
    selection_debug_seen = True;
  }
}

static auto literal_archive() -> View::Bytes {
  return View::Bytes(format_one_archive);
}

static auto set_u16(Dynamic::Bytes& bytes, Count offset, Unsigned_16 value)
    -> void {
  auto target = bytes.get_access();
  auto* data = target.get_data();
  data[offset] = Unsigned_8(value);
  data[offset + 1] = Unsigned_8(value >> 8);
}

static auto archive_with(View::Bytes identity, Version version)
    -> Dynamic::Bytes {
  Dynamic::Bytes bytes(literal_archive());
  if (identity.get_size() != Count(8)) {
    return {};
  }

  auto target = bytes.get_access();
  auto* data = target.get_data();
  for (Count i = 0; i < identity.get_size(); i++) {
    data[archive_identity_offset + i] = identity[i];
  }

  set_u16(bytes, archive_version_offset, version.get_major());
  set_u16(bytes, archive_version_offset + 2, version.get_minor());
  return bytes;
}

static auto join_path(View::Bytes root, View::Bytes member) -> Dynamic::Bytes {
  Dynamic::Bytes path(root);
  path.append('/');
  path.concat(member);
  return path;
}

// A private root lets tests distinguish missing, empty, corrupt, and replaced
// files without depending on the process working directory or repository data.
class TemporaryRepositoryFiles {
 public:
  TemporaryRepositoryFiles() {
    Signed_32 written = snprintf(
        Data::cast<char>(root.get_data()), root.get_size(),
        "/tmp/tetrodotoxin_repository_XXXXXX");
    if (written <= 0 || Count(written) >= root.get_size()) {
      return;
    }

    valid = mkdtemp(Data::cast<char>(root.get_data())) != nullptr;
  }

  TemporaryRepositoryFiles(const TemporaryRepositoryFiles&) = delete;
  auto operator=(const TemporaryRepositoryFiles&)
      -> TemporaryRepositoryFiles& = delete;

  ~TemporaryRepositoryFiles() {
    static constexpr Static::Vector<View::Bytes, 13> members = {{
      "core-12.ttxa"_view,
      "core-20.ttxa"_view,
      "other.ttxa"_view,
      "decoy.ttxa"_view,
      "corrupt.ttxa"_view,
      "empty.ttxa"_view,
      "truncated.ttxa"_view,
      "identity.ttxa"_view,
      "version.ttxa"_view,
      "cache.ttxa"_view,
      "mapping.ttxa"_view,
      "replacement.ttxa"_view,
      "absent.ttxa"_view,
    }};

    if (!valid) {
      return;
    }

    for (Count i = 0; i < members.get_size(); i++) {
      Dynamic::Bytes location = get_path(members[i]);
      if (File::exists(location)) {
        File::remove(location);
      }
    }

    File::remove(get_root());
  }

  operator bool() const { return bool(valid); }

  auto get_root() const -> View::Bytes {
    return NullTerminated::to_view(Data::cast<const char>(root.get_data()));
  }

  auto get_path(View::Bytes member) const -> Dynamic::Bytes {
    return join_path(get_root(), member);
  }

  auto write(View::Bytes member, View::Bytes bytes) const -> Bool {
    Dynamic::Bytes location = get_path(member);
    return File::write(bytes, location);
  }

  auto remove(View::Bytes member) const -> Bool {
    Dynamic::Bytes location = get_path(member);
    return File::remove(location);
  }

 private:
  Static::Bytes<temporary_path_capacity> root;
  Bool valid = False;
};

static auto rejects_selected_input(
    TemporaryRepositoryFiles& files,
    View::Bytes member,
    View::Bytes bytes,
    Bool write_input,
    View::Bytes identity,
    Version version,
    Package::Repository::SelectionError expected_error,
    View::Bytes expected_info,
    View::Bytes expected_debug = View::Bytes()) -> Bool {
  if (write_input) {
    Bool written = files.write(member, bytes);
    if (!written) {
      return False;
    }
  }

  Dynamic::Bytes archive_location = files.get_path(member);
  Static::Vector<Package::Repository::Artifact, 2> artifacts = {{
    Package::Repository::Artifact("cpu"_view, "missing/cpu.a"_view),
    Package::Repository::Artifact("res"_view, "missing/res.bin"_view),
  }};
  Package::Repository::Input input(
      identity, version, archive_location, artifacts);
  Allocator::Arena arena;

  auto repository = Package::Repository::Repository::create(
      arena, View::Vector<Package::Repository::Input>(&input, 1),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  if (!repository) {
    return False;
  }

  expected_selection_info = expected_info;
  expected_selection_debug = expected_debug;
  selection_info_seen = False;
  selection_debug_seen = expected_debug.is_empty();
  selection_log_count = 0;
  Diagnostics::Log::set_sink(capture_selection_logs);

  auto selected = repository->select_archive(identity, version);

  Diagnostics::Log::set_sink(Test::capture_sink);
  return returns_selection_error(selected, expected_error) &&
         selection_info_seen && selection_debug_seen;
}

static auto rejects_mapping(
    View::Bytes archive_location,
    View::Vector<Package::Repository::Artifact> artifacts,
    View::Bytes expected_log) -> Bool {
  Package::Repository::Input input(
      "Pkg.Core"_view, Version(1, 2), archive_location, artifacts);
  Allocator::Arena arena;
  auto repository = Package::Repository::Repository::create(
      arena, View::Vector<Package::Repository::Input>(&input, 1),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  if (!repository) {
    return False;
  }

  auto selected =
      repository->select_native("Pkg.Core"_view, Version(1, 2), "cpu"_view);
  return returns_selection_error(
             selected, Package::Repository::SelectionError::ArtifactMismatch) &&
         Test::error_contains(
             "selection_error=ArtifactMismatch "
             "requested_identity=Pkg.Core requested_version=1.2 "
             "requested_artifact=cpu"_view,
             Diagnostics::Log::Level::Info) &&
         Test::error_contains(expected_log, Diagnostics::Log::Level::Info);
}

static auto rejects_archive_route(View::Bytes route, View::Bytes expected_log)
    -> Bool {
  Package::Repository::Output output(
      "Pkg.Core"_view, Version(1, 2), "archive"_view, route);
  Allocator::Arena arena;
  auto repository = Package::Repository::Repository::create(
      arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(&output, 1),
      View::Vector<Package::Repository::Output>());
  return !repository &&
         Test::error_contains(expected_log, Diagnostics::Log::Level::Info);
}

static auto rejects_outputs(
    View::Vector<Package::Repository::Output> archive_outputs,
    View::Vector<Package::Repository::Output> native_outputs) -> Bool {
  Allocator::Arena arena;
  auto repository = Package::Repository::Repository::create(
      arena, View::Vector<Package::Repository::Input>(), archive_outputs,
      native_outputs);
  return !repository;
}

static Harness PackageRepository = {
  .name = "Tetrodotoxin::Package::Repository"_view,
  .setup =
      []() { Diagnostics::Log::set_level(Diagnostics::Log::Level::Debug); },
  .teardown =
      []() { Diagnostics::Log::set_level(Diagnostics::Log::Level::Info); },
};

PERIMORTEM_UNIT_TEST(PackageRepository, exact_lazy_selection) {
  TemporaryRepositoryFiles files;
  ASSERT(files);

  Dynamic::Bytes core_20 = archive_with("Pkg.Core"_view, Version(2, 0));
  Dynamic::Bytes other = archive_with("Pkg.More"_view, Version(1, 0));
  Dynamic::Bytes decoy = archive_with("Pkg.Core"_view, Version(9, 9));
  Dynamic::Bytes corrupt(literal_archive());
  corrupt.get_access().get_data()[0] = 'X';
  Bool core_12_written = files.write("core-12.ttxa"_view, literal_archive());
  Bool core_20_written = files.write("core-20.ttxa"_view, core_20);
  Bool other_written = files.write("other.ttxa"_view, other);
  Bool decoy_written = files.write("decoy.ttxa"_view, decoy);
  Bool corrupt_written = files.write("corrupt.ttxa"_view, corrupt);
  ASSERT(core_12_written);
  ASSERT(core_20_written);
  ASSERT(other_written);
  ASSERT(decoy_written);
  ASSERT(corrupt_written);

  Dynamic::Bytes core_12_path = files.get_path("core-12.ttxa"_view);
  Dynamic::Bytes core_20_path = files.get_path("core-20.ttxa"_view);
  Dynamic::Bytes other_path = files.get_path("other.ttxa"_view);
  Dynamic::Bytes corrupt_path = files.get_path("corrupt.ttxa"_view);
  Dynamic::Bytes absent_path = files.get_path("absent.ttxa"_view);
  Static::Vector<Package::Repository::Artifact, 2> valid_artifacts = {{
    Package::Repository::Artifact("cpu"_view, "missing/native-cpu.a"_view),
    Package::Repository::Artifact("res"_view, "missing/native-res.bin"_view),
  }};
  Static::Vector<Package::Repository::Artifact, 2> invalid_artifacts = {{
    Package::Repository::Artifact("cpu"_view, "missing/first.a"_view),
    Package::Repository::Artifact("cpu"_view, "missing/second.a"_view),
  }};
  Static::Vector<Package::Repository::Input, 5> inputs = {{
    Package::Repository::Input(
        "Pkg.Core"_view, Version(1, 2), core_12_path, valid_artifacts),
    Package::Repository::Input(
        "Pkg.Core"_view, Version(2, 0), core_20_path, valid_artifacts),
    Package::Repository::Input(
        "Pkg.More"_view, Version(1, 0), other_path, valid_artifacts),
    Package::Repository::Input(
        "Pkg.Bad"_view, Version(1, 0), corrupt_path, invalid_artifacts),
    Package::Repository::Input(
        "Pkg.Gone"_view, Version(1, 0), absent_path, invalid_artifacts),
  }};

  Allocator::Arena arena;
  auto repository = Package::Repository::Repository::create(
      arena, inputs, View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);

  // Several exact keys share one transaction while malformed unselected
  // declarations remain inert. Successful results must select Archive rather
  // than merely avoiding an error alternative.
  auto core_12 = repository->select_archive("Pkg.Core"_view, Version(1, 2));
  auto core_20_selected =
      repository->select_archive("Pkg.Core"_view, Version(2, 0));
  auto other_selected =
      repository->select_archive("Pkg.More"_view, Version(1, 0));
  auto core_12_archive = selected_archive(core_12);
  auto core_20_archive = selected_archive(core_20_selected);
  auto other_archive = selected_archive(other_selected);
  ASSERT(core_12_archive);
  ASSERT(core_20_archive);
  ASSERT(other_archive);
  EXPECT(core_12_archive->get_version() == Version(1, 2));
  EXPECT(core_20_archive->get_version() == Version(2, 0));
  EXPECT_TEXT(other_archive->get_identity(), "Pkg.More"_view);

  // Identity and Version are one exact key. Each kind of absent match chooses
  // the same stable category but records the complete request that missed.
  auto missing_identity =
      repository->select_archive("Pkg.None"_view, Version(1, 2));
  EXPECT(returns_selection_error(
      missing_identity, Package::Repository::SelectionError::NotDeclared));
  EXPECT(
      Test::error_contains(
          "selection_error=NotDeclared requested_identity=Pkg.None "
          "requested_version=1.2 reason=no Package input declaration matches "
          "the requested key."_view,
          Diagnostics::Log::Level::Info));

  auto missing_version =
      repository->select_archive("Pkg.Core"_view, Version(9, 9));
  EXPECT(returns_selection_error(
      missing_version, Package::Repository::SelectionError::NotDeclared));
  EXPECT(
      Test::error_contains(
          "selection_error=NotDeclared requested_identity=Pkg.Core "
          "requested_version=9.9 reason=no Package input declaration matches "
          "the requested key."_view,
          Diagnostics::Log::Level::Info));

  // Native selection must return the semantic category without adding its own
  // conflicting record. Counting the custom sink makes that absence
  // observable instead of relying on the last message alone.
  expected_selection_info =
      "selection_error=NotDeclared requested_identity=Pkg.None "
      "requested_version=1.2"_view;
  expected_selection_debug = View::Bytes();
  selection_info_seen = False;
  selection_debug_seen = True;
  selection_log_count = 0;
  Diagnostics::Log::set_sink(capture_selection_logs);

  auto propagated =
      repository->select_native("Pkg.None"_view, Version(1, 2), "cpu"_view);

  Diagnostics::Log::set_sink(Test::capture_sink);
  EXPECT(returns_selection_error(
      propagated, Package::Repository::SelectionError::NotDeclared));
  EXPECT(selection_info_seen);
  EXPECT_EQ(selection_log_count, Count(1));
  EXPECT_TEXT(
      Test::captured_message(),
      "Package::Repository exact input selection failed. "
      "selection_error=NotDeclared requested_identity=Pkg.None "
      "requested_version=1.2 reason=no Package input declaration matches the "
      "requested key."_view);

  // A complete native inventory exposes only its borrowed path. Once that
  // inventory agrees, a different requested ID is an artifact lookup failure
  // rather than an inventory mismatch.
  auto native =
      repository->select_native("Pkg.Core"_view, Version(1, 2), "cpu"_view);
  auto missing_native =
      repository->select_native("Pkg.Core"_view, Version(1, 2), "unknown"_view);
  auto native_path = selected_native(native);
  ASSERT(native_path);
  EXPECT_TEXT(*native_path, "missing/native-cpu.a"_view);
  EXPECT(returns_selection_error(
      missing_native,
      Package::Repository::SelectionError::ArtifactNotDeclared));
  EXPECT(
      Test::error_contains(
          "selection_error=ArtifactNotDeclared "
          "requested_identity=Pkg.Core requested_version=1.2 "
          "requested_artifact=unknown"_view,
          Diagnostics::Log::Level::Info));
  EXPECT(
      Test::error_contains(
          "reason=requested native artifact is not declared "
          "identity=Pkg.Core version=1.2"_view,
          Diagnostics::Log::Level::Info));
  EXPECT(
      Test::error_contains(
          "artifact_id=unknown"_view, Diagnostics::Log::Level::Info));
}

PERIMORTEM_UNIT_TEST(PackageRepository, selected_failures) {
  TemporaryRepositoryFiles files;
  ASSERT(files);

  Dynamic::Bytes truncated(literal_archive());
  truncated.resize(truncated.get_size() - 1);
  Dynamic::Bytes corrupt(literal_archive());
  corrupt.get_access().get_data()[0] = 'X';
  Dynamic::Bytes future(literal_archive());
  set_u16(future, 4, 2);

  // Reader and Repository emit consecutive records with different owner
  // detail. The helper observes both before checking the typed caller category.
  EXPECT(rejects_selected_input(
      files, "absent.ttxa"_view, View::Bytes(), False, "Pkg.Core"_view,
      Version(1, 2), Package::Repository::SelectionError::Unreadable,
      "selection_error=Unreadable requested_identity=Pkg.Core "
      "requested_version=1.2"_view));
  EXPECT(
      Test::error_contains(
          "reason=the Archive file could not be read. identity=Pkg.Core "
          "version=1.2"_view,
          Diagnostics::Log::Level::Info));
  EXPECT(rejects_selected_input(
      files, "empty.ttxa"_view, View::Bytes(), True, "Pkg.Core"_view,
      Version(1, 2), Package::Repository::SelectionError::InvalidFormat,
      "selection_error=InvalidFormat requested_identity=Pkg.Core "
      "requested_version=1.2"_view,
      "Package::Archive::Reader Format 1 read failed. stage=header "
      "byte_offset=0 reason=the fixed header extends beyond the input "
      "bytes."_view));
  EXPECT(
      Test::error_contains(
          "reason=the Archive failed Format 1 validation. identity=Pkg.Core "
          "version=1.2"_view,
          Diagnostics::Log::Level::Info));
  EXPECT(rejects_selected_input(
      files, "truncated.ttxa"_view, truncated, True, "Pkg.Core"_view,
      Version(1, 2), Package::Repository::SelectionError::InvalidFormat,
      "selection_error=InvalidFormat requested_identity=Pkg.Core "
      "requested_version=1.2"_view,
      "Package::Archive::Reader Format 1 read failed"_view));
  EXPECT(
      Test::error_contains(
          "reason=the Archive failed Format 1 validation. identity=Pkg.Core "
          "version=1.2"_view,
          Diagnostics::Log::Level::Info));
  EXPECT(rejects_selected_input(
      files, "corrupt.ttxa"_view, corrupt, True, "Pkg.Core"_view, Version(1, 2),
      Package::Repository::SelectionError::InvalidFormat,
      "selection_error=InvalidFormat requested_identity=Pkg.Core "
      "requested_version=1.2"_view,
      "stage=header byte_offset=0 expected_magic=TTXA "
      "actual_magic=XTXA"_view));
  EXPECT(
      Test::error_contains(
          "reason=the Archive failed Format 1 validation. identity=Pkg.Core "
          "version=1.2"_view,
          Diagnostics::Log::Level::Info));
  EXPECT(rejects_selected_input(
      files, "replacement.ttxa"_view, future, True, "Pkg.Core"_view,
      Version(1, 2), Package::Repository::SelectionError::UnsupportedFormat,
      "selection_error=UnsupportedFormat requested_identity=Pkg.Core "
      "requested_version=1.2"_view,
      "stage=header byte_offset=4 expected_format=1 actual_format=2"_view));
  EXPECT(
      Test::error_contains(
          "reason=the Archive format revision is unsupported. "
          "identity=Pkg.Core version=1.2"_view,
          Diagnostics::Log::Level::Info));
  EXPECT(rejects_selected_input(
      files, "identity.ttxa"_view, literal_archive(), True, "Pkg.More"_view,
      Version(1, 2), Package::Repository::SelectionError::PackageKeyMismatch,
      "selection_error=PackageKeyMismatch requested_identity=Pkg.More "
      "requested_version=1.2"_view));
  EXPECT(
      Test::error_contains(
          "reason=decoded Package key mismatch expected identity=Pkg.More "
          "version=1.2"_view,
          Diagnostics::Log::Level::Info));
  EXPECT(rejects_selected_input(
      files, "version.ttxa"_view, literal_archive(), True, "Pkg.Core"_view,
      Version(2, 0), Package::Repository::SelectionError::PackageKeyMismatch,
      "selection_error=PackageKeyMismatch requested_identity=Pkg.Core "
      "requested_version=2.0"_view));
  EXPECT(
      Test::error_contains(
          "reason=decoded Package key mismatch expected identity=Pkg.Core "
          "version=2.0"_view,
          Diagnostics::Log::Level::Info));
}

PERIMORTEM_UNIT_TEST(PackageRepository, caller_arena_and_retained_cache) {
  TemporaryRepositoryFiles files;
  ASSERT(files);
  Bool archive_written = files.write("cache.ttxa"_view, literal_archive());
  ASSERT(archive_written);

  Allocator::Arena arena;
  Dynamic::Bytes archive_location = files.get_path("cache.ttxa"_view);
  Dynamic::Bytes native_location = files.get_path("native-cache.a"_view);
  View::Bytes retained_identity = arena.proxy("Pkg.Core"_view);
  View::Bytes retained_archive_location =
      arena.proxy(archive_location.get_view());
  View::Bytes retained_artifact_id = arena.proxy("cpu"_view);
  View::Bytes retained_native_location =
      arena.proxy(native_location.get_view());
  Static::Vector<Package::Repository::Artifact, 2> artifacts = {{
    Package::Repository::Artifact(
        retained_artifact_id, retained_native_location),
    Package::Repository::Artifact("res"_view, "missing/res.bin"_view),
  }};
  Package::Repository::Input input(
      retained_identity, Version(1, 2), retained_archive_location, artifacts);
  auto repository = Package::Repository::Repository::create(
      arena, View::Vector<Package::Repository::Input>(&input, 1),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);

  // The caller supplied views already satisfy the Arena lifetime contract.
  // Destroying their mutable path builders must not change Repository inputs.
  archive_location.set('x');
  native_location.set('x');

  auto first = repository->select_archive("Pkg.Core"_view, Version(1, 2));
  auto first_archive = selected_archive(first);
  ASSERT(first_archive);
  auto native =
      repository->select_native("Pkg.Core"_view, Version(1, 2), "cpu"_view);
  auto native_path = selected_native(native);
  ASSERT(native_path);
  EXPECT_TEXT(*native_path, retained_native_location);

  // Replacement and removal happen after the first semantic selection. Both
  // later calls must return the exact retained Archive rather than touching the
  // backing file again.
  Dynamic::Bytes replacement(literal_archive());
  replacement.get_access().get_data()[0] = 'X';
  Bool replacement_written = files.write("cache.ttxa"_view, replacement);
  ASSERT(replacement_written);

  auto after_replacement =
      repository->select_archive("Pkg.Core"_view, Version(1, 2));
  auto replacement_archive = selected_archive(after_replacement);
  ASSERT(replacement_archive);
  EXPECT(first_archive == replacement_archive);

  Bool archive_removed = files.remove("cache.ttxa"_view);
  ASSERT(archive_removed);

  auto after_removal =
      repository->select_archive("Pkg.Core"_view, Version(1, 2));
  auto removal_archive = selected_archive(after_removal);
  ASSERT(removal_archive);
  EXPECT(first_archive == removal_archive);
  EXPECT_TEXT(removal_archive->get_identity(), "Pkg.Core"_view);
  ASSERT_EQ(removal_archive->get_artifact_ids().get_size(), Count(2));
  EXPECT_TEXT(removal_archive->get_artifact_ids().get_data()[0], "cpu"_view);
}

PERIMORTEM_UNIT_TEST(PackageRepository, semantic_cache_without_native_inputs) {
  TemporaryRepositoryFiles files;
  ASSERT(files);
  Bool archive_written = files.write("mapping.ttxa"_view, literal_archive());
  ASSERT(archive_written);

  Dynamic::Bytes archive_location = files.get_path("mapping.ttxa"_view);
  Package::Repository::Input input(
      "Pkg.Core"_view, Version(1, 2), archive_location,
      View::Vector<Package::Repository::Artifact>());
  Allocator::Arena arena;
  auto repository = Package::Repository::Repository::create(
      arena, View::Vector<Package::Repository::Input>(&input, 1),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  ASSERT(repository);

  auto first = repository->select_archive("Pkg.Core"_view, Version(1, 2));
  auto first_archive = selected_archive(first);
  ASSERT(first_archive);

  // Semantic cache publication precedes native inventory policy. The absent
  // mapping therefore rejects only native selection and cannot remove or
  // replace the already retained Archive.
  auto native =
      repository->select_native("Pkg.Core"_view, Version(1, 2), "cpu"_view);
  EXPECT(returns_selection_error(
      native, Package::Repository::SelectionError::ArtifactMismatch));
  EXPECT(
      Test::error_contains(
          "selection_error=ArtifactMismatch "
          "requested_identity=Pkg.Core requested_version=1.2 "
          "requested_artifact=cpu reason=missing native artifact mapping "
          "identity=Pkg.Core version=1.2"_view,
          Diagnostics::Log::Level::Info));

  auto after_failure =
      repository->select_archive("Pkg.Core"_view, Version(1, 2));
  auto retained = selected_archive(after_failure);
  ASSERT(retained);
  EXPECT(first_archive == retained);
}

PERIMORTEM_UNIT_TEST(PackageRepository, exact_artifact_inventory) {
  TemporaryRepositoryFiles files;
  ASSERT(files);
  Bool archive_written = files.write("mapping.ttxa"_view, literal_archive());
  ASSERT(archive_written);
  Dynamic::Bytes archive_location = files.get_path("mapping.ttxa"_view);

  Static::Vector<Package::Repository::Artifact, 1> missing = {{
    Package::Repository::Artifact("cpu"_view, "missing/cpu.a"_view),
  }};
  Static::Vector<Package::Repository::Artifact, 2> duplicate = {{
    Package::Repository::Artifact("cpu"_view, "missing/first.a"_view),
    Package::Repository::Artifact("cpu"_view, "missing/second.a"_view),
  }};
  Static::Vector<Package::Repository::Artifact, 2> unknown = {{
    Package::Repository::Artifact("cpu"_view, "missing/cpu.a"_view),
    Package::Repository::Artifact("bad"_view, "missing/bad.a"_view),
  }};

  EXPECT(rejects_mapping(
      archive_location, missing,
      "reason=missing native artifact mapping identity=Pkg.Core version=1.2 "
      "archive_location="_view));
  EXPECT(rejects_mapping(
      archive_location, duplicate,
      "reason=duplicate native artifact mapping identity=Pkg.Core "
      "version=1.2"_view));
  EXPECT(rejects_mapping(
      archive_location, unknown,
      "reason=unknown native artifact mapping identity=Pkg.Core version=1.2 "
      "archive_location="_view));
}

PERIMORTEM_UNIT_TEST(PackageRepository, declared_publication_paths) {
  static constexpr View::Bytes output_root =
      "r00_repository_publication_side_effect_probe_7349"_view;
  EXPECT_NOT(File::exists(output_root));

  Dynamic::Bytes archive_route(
      "r00_repository_publication_side_effect_probe_7349/alpha/../"
      "archive.ttxa"_view);
  Dynamic::Bytes native_route(
      "r00_repository_publication_side_effect_probe_7349//native/./cpu.a"_view);
  Static::Vector<Package::Repository::Output, 2> archive_outputs = {{
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "archive"_view, archive_route),
    Package::Repository::Output(
        "Pkg.More"_view, Version(1, 0), "archive"_view,
        "products/other.ttxa"_view),
  }};
  Static::Vector<Package::Repository::Output, 2> native_outputs = {{
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "cpu"_view, native_route),
    Package::Repository::Output(
        "Pkg.More"_view, Version(1, 0), "res"_view,
        "products/native/res.bin"_view),
  }};
  Allocator::Arena first_arena;
  auto first = Package::Repository::Repository::create(
      first_arena, View::Vector<Package::Repository::Input>(), archive_outputs,
      native_outputs);
  ASSERT(first);

  archive_route.set('x');
  native_route.set('x');

  Static::Vector<Package::Repository::Output, 2> reversed_archives = {{
    Package::Repository::Output(
        "Pkg.More"_view, Version(1, 0), "archive"_view,
        "products/other.ttxa"_view),
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "archive"_view,
        "r00_repository_publication_side_effect_probe_7349/archive.ttxa"_view),
  }};
  Static::Vector<Package::Repository::Output, 2> reversed_natives = {{
    Package::Repository::Output(
        "Pkg.More"_view, Version(1, 0), "res"_view,
        "products/native/res.bin"_view),
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "cpu"_view,
        "r00_repository_publication_side_effect_probe_7349/native/cpu.a"_view),
  }};
  Allocator::Arena second_arena;
  auto second = Package::Repository::Repository::create(
      second_arena, View::Vector<Package::Repository::Input>(),
      reversed_archives, reversed_natives);
  ASSERT(second);

  auto first_archive = first->get_archive_output_path(
      "Pkg.Core"_view, Version(1, 2), "archive"_view);
  auto second_archive = second->get_archive_output_path(
      "Pkg.Core"_view, Version(1, 2), "archive"_view);
  auto first_native =
      first->get_native_output_path("Pkg.Core"_view, Version(1, 2), "cpu"_view);
  auto second_native = second->get_native_output_path(
      "Pkg.Core"_view, Version(1, 2), "cpu"_view);
  ASSERT(first_archive);
  ASSERT(second_archive);
  ASSERT(first_native);
  ASSERT(second_native);
  EXPECT_TEXT(
      *first_archive,
      "r00_repository_publication_side_effect_probe_7349/archive.ttxa"_view);
  EXPECT(*first_archive == *second_archive);
  EXPECT_TEXT(
      *first_native,
      "r00_repository_publication_side_effect_probe_7349/native/cpu.a"_view);
  EXPECT(*first_native == *second_native);

  EXPECT_NOT(first->get_archive_output_path(
      "Pkg.Core"_view, Version(9, 9), "archive"_view));
  EXPECT_NOT(first->get_archive_output_path(
      "Pkg.Core"_view, Version(1, 2), "unknown"_view));
  EXPECT_NOT(first->get_native_output_path(
      "Pkg.Core"_view, Version(1, 2), "res"_view));
  EXPECT_NOT(File::exists(output_root));
}

PERIMORTEM_UNIT_TEST(PackageRepository, invalid_publication_routes) {
  Static::Bytes<3> embedded_nul = {{'a', '\0', 'b'}};
  Static::Bytes<Path::max_size + 1> oversized;
  for (Count i = 0; i < oversized.get_size(); i++) {
    oversized[i] = 'a';
  }

  EXPECT(rejects_archive_route(
      View::Bytes(), "reason=the output route is empty."_view));
  EXPECT(rejects_archive_route(
      "/rooted/archive.ttxa"_view, "reason=the output route is rooted."_view));
  EXPECT(rejects_archive_route(
      "../escaped/archive.ttxa"_view,
      "reason=the output route escapes or names no destination after "
      "normalization."_view));
  EXPECT(rejects_archive_route(
      "parent/../../escaped.ttxa"_view,
      "reason=the output route escapes or names no destination after "
      "normalization."_view));
  EXPECT(rejects_archive_route(
      "windows\\archive.ttxa"_view,
      "reason=the output route contains a backslash."_view));
  EXPECT(rejects_archive_route(
      embedded_nul, "reason=the output route contains a NUL byte."_view));
  EXPECT(rejects_archive_route(
      oversized, "reason=the output route exceeds the Path capacity."_view));
}

PERIMORTEM_UNIT_TEST(PackageRepository, duplicate_keys_and_collisions) {
  Static::Vector<Package::Repository::Output, 2> duplicate_archives = {{
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "archive"_view, "first.ttxa"_view),
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "archive"_view, "second.ttxa"_view),
  }};
  EXPECT(rejects_outputs(
      duplicate_archives, View::Vector<Package::Repository::Output>()));
  EXPECT(
      Test::error_contains(
          "reason=duplicate key first_kind=Archive first_identity=Pkg.Core "
          "first_version=1.2 first_artifact=archive first_route=first.ttxa "
          "second_kind=Archive second_identity=Pkg.Core second_version=1.2 "
          "second_artifact=archive second_route=second.ttxa"_view,
          Diagnostics::Log::Level::Info));

  Static::Vector<Package::Repository::Output, 2> duplicate_natives = {{
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "cpu"_view, "first.a"_view),
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "cpu"_view, "second.a"_view),
  }};
  EXPECT(rejects_outputs(
      View::Vector<Package::Repository::Output>(), duplicate_natives));
  EXPECT(
      Test::error_contains(
          "reason=duplicate key first_kind=native first_identity=Pkg.Core "
          "first_version=1.2 first_artifact=cpu first_route=first.a "
          "second_kind=native second_identity=Pkg.Core second_version=1.2 "
          "second_artifact=cpu second_route=second.a"_view,
          Diagnostics::Log::Level::Info));

  Static::Vector<Package::Repository::Output, 2> native_collision = {{
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "cpu"_view, "same/path"_view),
    Package::Repository::Output(
        "Pkg.More"_view, Version(1, 0), "res"_view, "same/path"_view),
  }};
  EXPECT(rejects_outputs(
      View::Vector<Package::Repository::Output>(), native_collision));
  EXPECT(
      Test::error_contains(
          "reason=normalized route collision normalized_route=same/path "
          "first_kind=native first_identity=Pkg.Core first_version=1.2 "
          "first_artifact=cpu first_route=same/path second_kind=native "
          "second_identity=Pkg.More second_version=1.0 second_artifact=res "
          "second_route=same/path"_view,
          Diagnostics::Log::Level::Info));

  Static::Vector<Package::Repository::Output, 2> alias_collision = {{
    Package::Repository::Output(
        "Pkg.Core"_view, Version(1, 2), "cpu"_view, "a/../b"_view),
    Package::Repository::Output(
        "Pkg.More"_view, Version(1, 0), "res"_view, "b"_view),
  }};
  EXPECT(rejects_outputs(
      View::Vector<Package::Repository::Output>(), alias_collision));
  EXPECT(
      Test::error_contains(
          "reason=normalized route collision normalized_route=b "
          "first_kind=native first_identity=Pkg.Core first_version=1.2 "
          "first_artifact=cpu first_route=a/../b second_kind=native "
          "second_identity=Pkg.More second_version=1.0 second_artifact=res "
          "second_route=b"_view,
          Diagnostics::Log::Level::Info));

  Package::Repository::Output archive_output(
      "Pkg.Core"_view, Version(1, 2), "archive"_view, "cross/./product"_view);
  Package::Repository::Output native_output(
      "Pkg.More"_view, Version(1, 0), "cpu"_view, "cross/product"_view);
  EXPECT(rejects_outputs(
      View::Vector<Package::Repository::Output>(&archive_output, 1),
      View::Vector<Package::Repository::Output>(&native_output, 1)));
  EXPECT(
      Test::error_contains(
          "reason=normalized route collision normalized_route=cross/product "
          "first_kind=Archive first_identity=Pkg.Core first_version=1.2 "
          "first_artifact=archive first_route=cross/./product "
          "second_kind=native second_identity=Pkg.More second_version=1.0 "
          "second_artifact=cpu second_route=cross/product"_view,
          Diagnostics::Log::Level::Info));

  Package::Repository::Output repeated_archive(
      "Pkg.Core"_view, Version(1, 2), "shared"_view, "archive/shared"_view);
  Package::Repository::Output repeated_native(
      "Pkg.Core"_view, Version(1, 2), "shared"_view, "native/shared"_view);
  EXPECT(rejects_outputs(
      View::Vector<Package::Repository::Output>(&repeated_archive, 1),
      View::Vector<Package::Repository::Output>(&repeated_native, 1)));
  EXPECT(
      Test::error_contains(
          "reason=duplicate key first_kind=Archive first_identity=Pkg.Core "
          "first_version=1.2 first_artifact=shared first_route=archive/shared "
          "second_kind=native second_identity=Pkg.Core second_version=1.2 "
          "second_artifact=shared second_route=native/shared"_view,
          Diagnostics::Log::Level::Info));
}

PERIMORTEM_UNIT_TEST(PackageRepository, rejected_transaction_recovery) {
  Package::Repository::Input first(
      "Pkg.Core"_view, Version(1, 2), "missing/first.ttxa"_view,
      View::Vector<Package::Repository::Artifact>());
  Package::Repository::Input second(
      "Pkg.Core"_view, Version(1, 2), "missing/second.ttxa"_view,
      View::Vector<Package::Repository::Artifact>());
  Static::Vector<Package::Repository::Input, 2> duplicates = {{
    first,
    second,
  }};

  Allocator::Arena rejected_arena;
  auto rejected = Package::Repository::Repository::create(
      rejected_arena, duplicates, View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  EXPECT_NOT(rejected);
  EXPECT(
      Test::error_contains(
          "reason=duplicate Package input key first identity=Pkg.Core "
          "version=1.2 archive_location=missing/first.ttxa second "
          "identity=Pkg.Core version=1.2 "
          "archive_location=missing/second.ttxa"_view,
          Diagnostics::Log::Level::Info));

  Allocator::Arena valid_arena;
  auto valid = Package::Repository::Repository::create(
      valid_arena, View::Vector<Package::Repository::Input>(),
      View::Vector<Package::Repository::Output>(),
      View::Vector<Package::Repository::Output>());
  EXPECT(valid);
}
