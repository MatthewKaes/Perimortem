// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/archive/archive.hpp"

#include "validation/unit_test.hpp"
#include "validation/unit_tests/tetrodotoxin/package/fixture.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "tetrodotoxin/package/archive/export.hpp"
#include "tetrodotoxin/package/archive/member.hpp"
#include "tetrodotoxin/package/archive/reader.hpp"
#include "tetrodotoxin/package/archive/writer.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Validation;

// This literal is the independent Format 2 oracle. Keeping it separate from
// Writer and every Writer helper lets the test detect a shared encoding mistake
// instead of reproducing one.
static constexpr U8 format_two_golden[] = {
  0x54, 0x54, 0x58, 0x41, 0x02, 0x00, 0x00, 0x00, 0x7D, 0x01, 0x00, 0x00, 0x01,
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
  0x61, 0x73, 0x73, 0x65, 0x74, 0x07, 0x00, 0x01, 0x00, 0x78, 0x00, 0x00, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x4D, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x63,
  0x70, 0x75, 0x11, 0x00, 0x00, 0x00, 0x78, 0x38, 0x36, 0x5F, 0x36, 0x34, 0x2D,
  0x73, 0x79, 0x73, 0x76, 0x2D, 0x6C, 0x69, 0x6E, 0x75, 0x78, 0xEF, 0xCD, 0xAB,
  0x89, 0x67, 0x45, 0x23, 0x01, 0x01, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x00,
  0x00, 0x01, 0x00, 0x00, 0x00, 0x43, 0x0B, 0x00, 0x00, 0x00, 0x6E, 0x61, 0x74,
  0x69, 0x76, 0x65, 0x5F, 0x63, 0x61, 0x6C, 0x6C, 0x08, 0x00, 0x00, 0x00, 0x50,
  0x6B, 0x67, 0x2E, 0x48, 0x6F, 0x73, 0x74, 0x1F, 0x00, 0x00, 0x00, 0x03, 0x00,
  0x00, 0x00, 0x72, 0x65, 0x73, 0x08, 0x00, 0x00, 0x00, 0x76, 0x75, 0x6C, 0x6B,
  0x61, 0x6E, 0x2D, 0x31, 0x10, 0x32, 0x54, 0x76, 0x98, 0xBA, 0xDC, 0xFE, 0x00,
  0x00, 0x00, 0x00,
};

static constexpr Count identity_field = 12;
static constexpr Count version_field = 32;
static constexpr Count dependency_field = 44;
static constexpr Count member_field = 84;
static constexpr Count artifact_field = 153;
static constexpr Count export_field = 187;
static constexpr Count artifact_metadata_field = 265;

static auto golden() -> View::Bytes {
  return View::Bytes(format_two_golden);
}

static auto contains(View::Bytes text, View::Bytes fragment) -> Bool {
  if (fragment.is_empty()) {
    return True;
  }

  if (fragment.get_size() > text.get_size()) {
    return False;
  }

  for (Count start = 0; start + fragment.get_size() <= text.get_size();
       start++) {
    Bool matches = True;
    for (Count i = 0; i < fragment.get_size(); i++) {
      if (text[start + i] != fragment[i]) {
        matches = False;
        break;
      }
    }

    if (matches) {
      return True;
    }
  }

  return False;
}

static auto set_u16(Dynamic::Bytes& bytes, Count offset, U16 value) -> void {
  auto target = bytes.get_access();
  auto* data = target.get_data();
  data[offset] = U8(value);
  data[offset + 1] = U8(value >> 8);
}

static auto set_u32(Dynamic::Bytes& bytes, Count offset, U32 value) -> void {
  auto target = bytes.get_access();
  auto* data = target.get_data();
  data[offset] = U8(value);
  data[offset + 1] = U8(value >> 8);
  data[offset + 2] = U8(value >> 16);
  data[offset + 3] = U8(value >> 24);
}

static auto splice(
    View::Bytes source,
    Count offset,
    Count removed,
    View::Bytes inserted) -> Dynamic::Bytes {
  Dynamic::Bytes result;
  result.concat(source.slice(0, offset));
  result.concat(inserted);
  result.concat(source.slice(offset + removed));
  return result;
}

static auto body_splice(
    View::Bytes source,
    Count offset,
    Count removed,
    View::Bytes inserted) -> Dynamic::Bytes {
  Dynamic::Bytes result = splice(source, offset, removed, inserted);
  set_u32(result, 8, U32(result.get_size() - 12));
  return result;
}

static auto selected_archive(
    Result<Package::Archive::Archive, Package::Archive::Reader::Error>& result)
    -> Package::Archive::Archive* {
  return result.visit(
      [](Package::Archive::Archive& archive) { return &archive; },
      [](Package::Archive::Reader::Error) {
        return static_cast<Package::Archive::Archive*>(nullptr);
      });
}

static auto returns_read_error(
    const Result<Package::Archive::Archive, Package::Archive::Reader::Error>&
        result,
    Package::Archive::Reader::Error expected) -> Bool {
  return result.visit(
      [](const Package::Archive::Archive&) { return False; },
      [&](Package::Archive::Reader::Error error) {
        return error == expected ? True : False;
      });
}

static auto rejects(
    View::Bytes input,
    Package::Archive::Reader::Error expected =
        Package::Archive::Reader::Error::InvalidFormat) -> Bool {
  Allocator::Arena arena;

  auto rejected = Package::Archive::Reader::read(arena, input);
  if (!returns_read_error(rejected, expected) ||
      !Test::error_contains(
          "Package::Archive::Reader Format 2 read failed"_view,
          Diagnostics::Log::Level::Debug)) {
    return False;
  }

  auto accepted = Package::Archive::Reader::read(arena, golden());
  return selected_archive(accepted) != nullptr;
}

static Harness PackageArchive = {
  .name = "Tetrodotoxin::Package::Archive"_view,
  .setup =
      []() { Diagnostics::Log::set_level(Diagnostics::Log::Level::Debug); },
  .teardown =
      []() { Diagnostics::Log::set_level(Diagnostics::Log::Level::Info); },
};

PERIMORTEM_UNIT_TEST(PackageArchive, typed_read_outcomes) {
  Allocator::Arena arena;

  auto empty = Package::Archive::Reader::read(arena, View::Bytes());
  EXPECT(returns_read_error(
      empty, Package::Archive::Reader::Error::InvalidFormat));
  EXPECT(
      Test::error_contains(
          "Package::Archive::Reader Format 2 read failed. stage=header "
          "byte_offset=0 reason=the fixed header extends beyond the input "
          "bytes."_view,
          Diagnostics::Log::Level::Debug));

  Dynamic::Bytes future(golden());
  set_u16(future, 4, 3);
  auto unsupported = Package::Archive::Reader::read(arena, future);
  EXPECT(returns_read_error(
      unsupported, Package::Archive::Reader::Error::UnsupportedFormat));
  EXPECT(
      Test::error_contains(
          "Package::Archive::Reader Format 2 read failed. stage=header "
          "byte_offset=4 expected_format=2 actual_format=3"_view,
          Diagnostics::Log::Level::Debug));

  auto accepted = Package::Archive::Reader::read(arena, golden());
  EXPECT(selected_archive(accepted) != nullptr);
}

PERIMORTEM_UNIT_TEST(PackageArchive, literal_format_two) {
  using Sections = Package::Archive::Archive::Sections;

  EXPECT_EQ(Package::Archive::Archive::header_size, Count(12));
  EXPECT_EQ(U16(Sections::Identity), U16(1));
  EXPECT_EQ(U16(Sections::Version), U16(2));
  EXPECT_EQ(U16(Sections::Dependencies), U16(3));
  EXPECT_EQ(U16(Sections::Members), U16(4));
  EXPECT_EQ(U16(Sections::ArtifactIds), U16(5));
  EXPECT_EQ(U16(Sections::Exports), U16(6));
  EXPECT_EQ(U16(Sections::ArtifactMetadata), U16(7));

  Allocator::Arena arena;
  auto decoded = Package::Archive::Reader::read(arena, golden());

  auto archive = selected_archive(decoded);
  ASSERT(archive);
  EXPECT(archive->get_profile() == Language::Persistence::Profile::Complete);
  EXPECT_TEXT(archive->get_identity(), "Pkg.Core"_view);
  EXPECT_EQ(archive->get_version().get_major(), U16(1));
  EXPECT_EQ(archive->get_version().get_minor(), U16(2));

  auto dependencies = archive->get_dependencies();
  ASSERT_EQ(dependencies.get_size(), Count(1));
  EXPECT_TEXT(dependencies.get_data()[0].get_local_name(), "Core"_view);
  EXPECT_TEXT(dependencies.get_data()[0].get_package_name(), "Pkg.Base"_view);
  EXPECT_EQ(dependencies.get_data()[0].get_version().get_major(), U16(3));
  EXPECT_EQ(dependencies.get_data()[0].get_version().get_minor(), U16(4));

  auto members = archive->get_members();
  ASSERT_EQ(members.get_size(), Count(2));
  EXPECT_TEXT(members.get_data()[0].get_semantic_name(), "Main"_view);
  EXPECT_TEXT(members.get_data()[0].get_dialect_name(), "Lib"_view);
  EXPECT(members.get_data()[0].get_payload().is_empty());
  EXPECT_TEXT(members.get_data()[1].get_semantic_name(), "Scene::One"_view);
  EXPECT_TEXT(members.get_data()[1].get_dialect_name(), "Scene"_view);
  ASSERT_EQ(members.get_data()[1].get_payload().get_size(), Count(3));
  EXPECT_EQ(members.get_data()[1].get_payload()[0], U8(0xAA));
  EXPECT_EQ(members.get_data()[1].get_payload()[1], U8(0x00));
  EXPECT_EQ(members.get_data()[1].get_payload()[2], U8(0x55));

  auto artifacts = archive->get_artifacts();
  ASSERT_EQ(artifacts.get_size(), Count(2));
  EXPECT_TEXT(artifacts.get_data()[0].get_id(), "cpu"_view);
  EXPECT_TEXT(artifacts.get_data()[0].get_target(), "x86_64-sysv-linux"_view);
  EXPECT_EQ(
      artifacts.get_data()[0].get_fingerprint().get_value(),
      U64(0x0123456789ABCDEF));
  ASSERT_EQ(artifacts.get_data()[0].get_imports().get_size(), Count(1));
  EXPECT_TEXT(
      artifacts.get_data()[0].get_imports().get_data()[0].get_symbol(),
      "native_call"_view);
  EXPECT_TEXT(
      artifacts.get_data()[0].get_imports().get_data()[0].get_provider(),
      "Pkg.Host"_view);
  EXPECT_TEXT(artifacts.get_data()[1].get_id(), "res"_view);
  EXPECT_TEXT(artifacts.get_data()[1].get_target(), "vulkan-1"_view);

  auto exports = archive->get_exports();
  ASSERT_EQ(exports.get_size(), Count(2));
  EXPECT_TEXT(exports.get_data()[0].get_semantic_route(), "Main::Run"_view);
  EXPECT_TEXT(exports.get_data()[0].get_artifact_id(), "cpu"_view);
  EXPECT_TEXT(exports.get_data()[0].get_symbol_locator(), "main"_view);
  EXPECT_TEXT(exports.get_data()[1].get_semantic_route(), "Scene::One"_view);
  EXPECT_TEXT(exports.get_data()[1].get_artifact_id(), "res"_view);
  EXPECT_TEXT(exports.get_data()[1].get_symbol_locator(), "asset"_view);

  auto encoded_once = Package::Archive::Writer::write(*archive);
  auto encoded_twice = Package::Archive::Writer::write(*archive);
  ASSERT(encoded_once);
  ASSERT(encoded_twice);
  EXPECT(encoded_once->get_view() == golden());
  EXPECT(encoded_twice->get_view() == golden());

  Allocator::Arena second_arena;
  auto round_trip = Package::Archive::Reader::read(second_arena, *encoded_once);
  auto round_trip_archive = selected_archive(round_trip);
  ASSERT(round_trip_archive);
  auto encoded_round_trip =
      Package::Archive::Writer::write(*round_trip_archive);
  ASSERT(encoded_round_trip);
  EXPECT(encoded_round_trip->get_view() == golden());
}

PERIMORTEM_UNIT_TEST(PackageArchive, omits_provenance) {
  static constexpr View::Bytes source =
      "// Authored Package\n"
      "dialect : Package;\n"
      "resolve Core : Pkg.Base = \"3.4\";\n"
      "source Main from \"main.ttx\";"_view;
  Package::Dialect authored_dialect;
  Allocator::Arena authored_arena;
  Ttx::Lexical::Errors errors;

  // The direct Dialect fixture supplies real authored provenance without
  // asking Workspace to publish an incomplete Package. A synthetic Dependency
  // alone could not prove spans were excluded.
  auto interpreted = interpret_package(
      authored_arena, authored_dialect, errors, source, "package.ttx"_view);
  ASSERT(interpreted);
  const Package::Language::Monograph& authored = *interpreted;
  ASSERT_EQ(authored.get_dependencies().get_size(), Count(1));
  ASSERT_EQ(authored.get_sources().get_size(), Count(1));
  EXPECT(authored.get_dependencies().get_data()[0].get_span());
  EXPECT(authored.get_sources().get_data()[0].get_span());
  EXPECT(errors.is_empty());

  Allocator::Arena arena;
  Package::Dialect synthetic_dialect;
  Package::Language::Dependency source_free_dependencies[] = {
    Package::Language::Dependency("Core"_view, "Pkg.Base"_view, Version(3, 4)),
  };
  auto& source_free = Package::Language::Monograph::create_synthetic(
      arena, synthetic_dialect, synthetic_dialect, source_free_dependencies);
  EXPECT_NOT(source_free.get_dependencies().get_data()[0].get_span());
  EXPECT(source_free.get_sources().is_empty());
  Package::Archive::Member members[] = {
    Package::Archive::Member("Main"_view, "Lib"_view, View::Bytes()),
  };
  Package::Archive::Archive authored_archive(
      "Pkg.Core"_view, Version(1, 2), authored.get_dependencies(), members,
      View::Vector<Package::Archive::Artifact>(),
      View::Vector<Package::Archive::Export>());
  Package::Archive::Archive source_free_archive(
      "Pkg.Core"_view, Version(1, 2), source_free.get_dependencies(), members,
      View::Vector<Package::Archive::Artifact>(),
      View::Vector<Package::Archive::Export>());

  // Archive receives the same durable Dependency facts from both construction
  // paths. Equal output proves the extra authored coordinates are not input.
  auto authored_bytes = Package::Archive::Writer::write(authored_archive);
  auto source_free_bytes = Package::Archive::Writer::write(source_free_archive);
  ASSERT(authored_bytes);
  ASSERT(source_free_bytes);
  EXPECT(authored_bytes->get_view() == source_free_bytes->get_view());

  // Reader rebuilds only the three Dependency identity fields. Encoding that
  // source free result must reproduce the authored Archive bytes exactly.
  Allocator::Arena restored_arena;
  auto restored_result =
      Package::Archive::Reader::read(restored_arena, *authored_bytes);
  auto restored = selected_archive(restored_result);
  ASSERT(restored);
  ASSERT_EQ(restored->get_dependencies().get_size(), Count(1));
  EXPECT_TEXT(
      restored->get_dependencies().get_data()[0].get_local_name(), "Core"_view);
  EXPECT_TEXT(
      restored->get_dependencies().get_data()[0].get_package_name(),
      "Pkg.Base"_view);
  EXPECT(
      restored->get_dependencies().get_data()[0].get_version() ==
      Version(3, 4));
  EXPECT_NOT(restored->get_dependencies().get_data()[0].get_span());

  auto restored_bytes = Package::Archive::Writer::write(*restored);
  ASSERT(restored_bytes);
  EXPECT(restored_bytes->get_view() == authored_bytes->get_view());
}

PERIMORTEM_UNIT_TEST(PackageArchive, empty_inventories) {
  Package::Archive::Member members[] = {
    Package::Archive::Member("Main"_view, "Lib"_view, View::Bytes()),
  };
  Package::Archive::Archive archive(
      "Pkg"_view, Version(1, 0), View::Vector<Package::Language::Dependency>(),
      members, View::Vector<Package::Archive::Artifact>(),
      View::Vector<Package::Archive::Export>());

  EXPECT(archive.get_dependencies().is_empty());
  EXPECT(archive.get_artifacts().is_empty());
  EXPECT(archive.get_exports().is_empty());
  auto encoded = Package::Archive::Writer::write(archive);
  ASSERT(encoded);

  Allocator::Arena decoded_arena;
  auto decoded = Package::Archive::Reader::read(decoded_arena, *encoded);
  auto decoded_archive = selected_archive(decoded);
  ASSERT(decoded_archive);
  EXPECT(decoded_archive->get_members().get_data()[0].get_payload().is_empty());
}

PERIMORTEM_UNIT_TEST(PackageArchive, equal_payloads) {
  const U8 payload_bytes[] = {0x10, 0x20};
  View::Bytes payload(payload_bytes);
  Package::Archive::Member members[] = {
    Package::Archive::Member("First"_view, "Lib"_view, payload),
    Package::Archive::Member("Second"_view, "Lib"_view, payload),
  };

  Package::Archive::Archive archive(
      "Pkg"_view, Version(1, 0), View::Vector<Package::Language::Dependency>(),
      members, View::Vector<Package::Archive::Artifact>(),
      View::Vector<Package::Archive::Export>());

  auto encoded = Package::Archive::Writer::write(archive);
  ASSERT(encoded);

  Allocator::Arena arena;
  auto decoded = Package::Archive::Reader::read(arena, *encoded);
  auto decoded_archive = selected_archive(decoded);
  ASSERT(decoded_archive);
  auto retained = decoded_archive->get_members();
  ASSERT_EQ(retained.get_size(), Count(2));
  EXPECT(retained.get_data()[0].get_payload() == payload);
  EXPECT(retained.get_data()[1].get_payload() == payload);
}

PERIMORTEM_UNIT_TEST(PackageArchive, opaque_locators) {
  Package::Archive::Member members[] = {
    Package::Archive::Member("Main"_view, "Lib"_view, View::Bytes()),
  };
  Package::Archive::Artifact artifacts[] = {
    Package::Archive::Artifact(
        "cpu-v1"_view, "x86_64-sysv-linux"_view,
        Linker::Fingerprint(0x0123456789ABCDEF)),
  };
  Package::Archive::Export exports[] = {
    Package::Archive::Export(
        "lower.case#route"_view, "cpu-v1"_view, "symbol@v1"_view),
  };

  Package::Archive::Archive archive(
      "Pkg"_view, Version(1, 0), View::Vector<Package::Language::Dependency>(),
      members, artifacts, exports);

  auto encoded = Package::Archive::Writer::write(archive);
  ASSERT(encoded);
  Allocator::Arena decoded_arena;
  auto decoded = Package::Archive::Reader::read(decoded_arena, *encoded);
  auto decoded_archive = selected_archive(decoded);
  ASSERT(decoded_archive);
  EXPECT_TEXT(
      decoded_archive->get_exports().get_data()[0].get_semantic_route(),
      "lower.case#route"_view);
  EXPECT_TEXT(
      decoded_archive->get_exports().get_data()[0].get_symbol_locator(),
      "symbol@v1"_view);
}

PERIMORTEM_UNIT_TEST(PackageArchive, format_size_limits) {
  const U8 byte = 'A';
  const Count oversized_size = Count(U32(-1)) + 1;
  View::Bytes oversized(&byte, oversized_size);
  Package::Archive::Member valid_member("Main"_view, "Lib"_view, View::Bytes());

  Package::Archive::Archive oversized_identity(
      oversized, Version(1, 0), View::Vector<Package::Language::Dependency>(),
      View::Vector<Package::Archive::Member>(&valid_member, 1),
      View::Vector<Package::Archive::Artifact>(),
      View::Vector<Package::Archive::Export>());
  EXPECT_NOT(Package::Archive::Writer::write(oversized_identity));

  Package::Archive::Member oversized_payload(
      "Main"_view, "Lib"_view, oversized);
  Package::Archive::Archive rejected_payload(
      "Pkg"_view, Version(1, 0), View::Vector<Package::Language::Dependency>(),
      View::Vector<Package::Archive::Member>(&oversized_payload, 1),
      View::Vector<Package::Archive::Artifact>(),
      View::Vector<Package::Archive::Export>());
  EXPECT_NOT(Package::Archive::Writer::write(rejected_payload));

  Package::Archive::Archive oversized_count(
      "Pkg"_view, Version(1, 0), View::Vector<Package::Language::Dependency>(),
      View::Vector<Package::Archive::Member>(&valid_member, oversized_size),
      View::Vector<Package::Archive::Artifact>(),
      View::Vector<Package::Archive::Export>());
  EXPECT_NOT(Package::Archive::Writer::write(oversized_count));
  EXPECT(contains(
      Test::captured_message(),
      "Package::Archive::Writer exceeded the Format 2 body limit."_view));
}

PERIMORTEM_UNIT_TEST(PackageArchive, borrowed_input) {
  Dynamic::Bytes input(golden());
  Allocator::Arena arena;
  auto read = Package::Archive::Reader::read(arena, input);
  auto archive = selected_archive(read);
  ASSERT(archive);

  EXPECT_EQ(
      archive->get_identity().get_data(),
      input.get_view().slice(identity_field + 12).get_data());
  EXPECT_EQ(
      archive->get_dependencies().get_data()[0].get_local_name().get_data(),
      input.get_view().slice(dependency_field + 20).get_data());

  auto encoded = Package::Archive::Writer::write(*archive);
  ASSERT(encoded);
  EXPECT(encoded->get_view() == golden());

  Dynamic::Bytes temporary_input(golden());
  Allocator::Arena temporary_arena;
  View::Bytes stable_input = temporary_arena.proxy(temporary_input);
  auto temporary =
      Package::Archive::Reader::read(temporary_arena, stable_input);
  auto temporary_archive = selected_archive(temporary);
  ASSERT(temporary_archive);
  temporary_input.set(0xFF);
  auto temporary_bytes = Package::Archive::Writer::write(*temporary_archive);
  ASSERT(temporary_bytes);
  EXPECT(temporary_bytes->get_view() == golden());

  Allocator::Arena later_arena;
  auto later = Package::Archive::Reader::read(later_arena, golden());
  EXPECT(selected_archive(later) != nullptr);
}

PERIMORTEM_UNIT_TEST(PackageArchive, truncation_bounds) {
  for (Count size = 0; size < golden().get_size(); size++) {
    EXPECT(rejects(golden().slice(0, size)));
  }
}

PERIMORTEM_UNIT_TEST(PackageArchive, envelope_boundaries) {
  Dynamic::Bytes bad_magic(golden());
  bad_magic.get_access().get_data()[0] = 'X';
  EXPECT(rejects(bad_magic));
  EXPECT(
      Test::error_contains(
          "stage=header byte_offset=0 expected_magic=TTXA "
          "actual_magic=XTXA"_view,
          Diagnostics::Log::Level::Debug));

  Dynamic::Bytes bad_format(golden());
  set_u16(bad_format, 4, 3);
  EXPECT(
      rejects(bad_format, Package::Archive::Reader::Error::UnsupportedFormat));

  Dynamic::Bytes header_flags(golden());
  set_u16(header_flags, 6, 1);
  Allocator::Arena contract_arena;
  auto contract = Package::Archive::Reader::read(contract_arena, header_flags);
  auto contract_archive = selected_archive(contract);
  ASSERT(contract_archive);
  EXPECT(
      contract_archive->get_profile() ==
      Language::Persistence::Profile::Contract);

  Dynamic::Bytes unknown_header_flags(golden());
  set_u16(unknown_header_flags, 6, 2);
  EXPECT(rejects(unknown_header_flags));

  Dynamic::Bytes short_body(golden());
  set_u32(short_body, 8, U32(golden().get_size() - 13));
  EXPECT(rejects(short_body));

  Dynamic::Bytes long_body(golden());
  set_u32(long_body, 8, U32(golden().get_size() - 11));
  EXPECT(rejects(long_body));

  Dynamic::Bytes trailing(golden());
  trailing.append(0);
  EXPECT(rejects(trailing));

  Dynamic::Bytes known_optional(golden());
  set_u16(known_optional, identity_field + 2, 0);
  EXPECT(rejects(known_optional));

  Dynamic::Bytes reserved_field_flag(golden());
  set_u16(reserved_field_flag, identity_field + 2, 2);
  EXPECT(rejects(reserved_field_flag));

  Dynamic::Bytes unknown_required(golden());
  set_u16(unknown_required, identity_field, 8);
  EXPECT(rejects(unknown_required));
  EXPECT(
      Test::error_contains(
          "stage=field tag byte_offset=12 unknown_required_tag=8"_view,
          Diagnostics::Log::Level::Debug));

  const U8 malformed_optional[] = {
    0x08, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0xAA,
  };
  Dynamic::Bytes bad_optional = body_splice(
      golden(), golden().get_size(), 0, View::Bytes(malformed_optional));
  EXPECT(rejects(bad_optional));

  const U8 optional[] = {
    0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xAA,
  };
  Dynamic::Bytes accepted_optional =
      body_splice(golden(), version_field, 0, View::Bytes(optional));
  Allocator::Arena optional_arena;
  auto decoded =
      Package::Archive::Reader::read(optional_arena, accepted_optional);
  auto decoded_archive = selected_archive(decoded);
  ASSERT(decoded_archive);
  auto canonical = Package::Archive::Writer::write(*decoded_archive);
  ASSERT(canonical);
  EXPECT(canonical->get_view() == golden());
}

PERIMORTEM_UNIT_TEST(PackageArchive, singleton_fields) {
  Dynamic::Bytes missing = body_splice(
      golden(), dependency_field, member_field - dependency_field,
      View::Bytes());
  EXPECT(rejects(missing));

  View::Bytes first_field =
      golden().slice(identity_field, version_field - identity_field);
  Dynamic::Bytes repeated =
      body_splice(golden(), version_field, 0, first_field);
  EXPECT(rejects(repeated));

  Dynamic::Bytes reordered;
  reordered.concat(golden().slice(0, identity_field));
  reordered.concat(
      golden().slice(version_field, dependency_field - version_field));
  reordered.concat(
      golden().slice(identity_field, version_field - identity_field));
  reordered.concat(golden().slice(dependency_field));
  EXPECT(rejects(reordered));

  Dynamic::Bytes field_escape(golden());
  set_u32(field_escape, artifact_metadata_field + 4, 121);
  EXPECT(rejects(field_escape));
  EXPECT(
      Test::error_contains(
          "stage=field framing byte_offset=265 section_tag=7 "
          "payload_size=121 reason=the field extends beyond the declared "
          "body."_view,
          Diagnostics::Log::Level::Debug));

  const U8 extra_payload[] = {0xAA};
  Dynamic::Bytes unconsumed_field =
      body_splice(golden(), version_field, 0, View::Bytes(extra_payload));
  set_u32(unconsumed_field, identity_field + 4, 13);
  EXPECT(rejects(unconsumed_field));

  Dynamic::Bytes zero_tag(golden());
  set_u16(zero_tag, identity_field, 0);
  EXPECT(rejects(zero_tag));
}

PERIMORTEM_UNIT_TEST(PackageArchive, framing_boundaries) {
  Dynamic::Bytes null_version(golden());
  set_u16(null_version, version_field + 8, 0);
  set_u16(null_version, version_field + 10, 0);
  EXPECT(rejects(null_version));

  Dynamic::Bytes null_dependency_version(golden());
  set_u16(null_dependency_version, dependency_field + 36, 0);
  set_u16(null_dependency_version, dependency_field + 38, 0);
  EXPECT(rejects(null_dependency_version));

  Dynamic::Bytes count_overflow(golden());
  set_u32(count_overflow, member_field + 8, U32(-1));
  EXPECT(rejects(count_overflow));

  Dynamic::Bytes length_overflow(golden());
  set_u32(length_overflow, identity_field + 8, U32(-1));
  EXPECT(rejects(length_overflow));

  Dynamic::Bytes record_escape(golden());
  set_u32(record_escape, dependency_field + 12, U32(-1));
  EXPECT(rejects(record_escape));

  const U8 extra_record[] = {0xAA};
  Dynamic::Bytes unconsumed_record =
      body_splice(golden(), member_field, 0, View::Bytes(extra_record));
  set_u32(unconsumed_record, dependency_field + 4, 33);
  set_u32(unconsumed_record, dependency_field + 12, 25);
  EXPECT(rejects(unconsumed_record));

  Dynamic::Bytes payload_escape(golden());
  set_u32(payload_escape, member_field + 31, 1);
  EXPECT(rejects(payload_escape));

  Dynamic::Bytes no_members =
      body_splice(golden(), member_field + 12, 57, View::Bytes());
  set_u32(no_members, member_field + 4, 4);
  set_u32(no_members, member_field + 8, 0);
  EXPECT(rejects(no_members));
}

PERIMORTEM_UNIT_TEST(PackageArchive, malformed_fields) {
  Dynamic::Bytes package_name(golden());
  package_name.get_access().get_data()[identity_field + 12] = 'p';
  EXPECT(rejects(package_name));
  EXPECT(
      Test::error_contains(
          "inventory=Package identity value=pkg.Core size=8 "
          "reason=the value is not a qualified Package Type name."_view,
          Diagnostics::Log::Level::Debug));

  Dynamic::Bytes dependency_alias(golden());
  dependency_alias.get_access().get_data()[dependency_field + 20] = 'c';
  EXPECT(rejects(dependency_alias));

  Dynamic::Bytes dependency_identity(golden());
  dependency_identity.get_access().get_data()[dependency_field + 28] = 'p';
  EXPECT(rejects(dependency_identity));

  Dynamic::Bytes member_name(golden());
  member_name.get_access().get_data()[member_field + 20] = 'm';
  EXPECT(rejects(member_name));

  Dynamic::Bytes member_route(golden());
  member_route.get_access().get_data()[member_field + 48] = '.';
  EXPECT(rejects(member_route));

  Dynamic::Bytes dialect_name(golden());
  dialect_name.get_access().get_data()[member_field + 28] = 'l';
  EXPECT(rejects(dialect_name));

  Dynamic::Bytes export_route_nul(golden());
  export_route_nul.get_access().get_data()[export_field + 20] = 0;
  EXPECT(rejects(export_route_nul));

  Dynamic::Bytes artifact_nul(golden());
  artifact_nul.get_access().get_data()[artifact_field + 20] = 0;
  EXPECT(rejects(artifact_nul));

  Dynamic::Bytes export_artifact_nul(golden());
  export_artifact_nul.get_access().get_data()[export_field + 33] = 0;
  EXPECT(rejects(export_artifact_nul));

  Dynamic::Bytes symbol_nul(golden());
  symbol_nul.get_access().get_data()[export_field + 40] = 0;
  EXPECT(rejects(symbol_nul));

  Dynamic::Bytes metadata_id(golden());
  metadata_id.get_access().get_data()[artifact_metadata_field + 20] = 'b';
  metadata_id.get_access().get_data()[artifact_metadata_field + 21] = 'a';
  metadata_id.get_access().get_data()[artifact_metadata_field + 22] = 'd';
  EXPECT(rejects(metadata_id));

  Dynamic::Bytes import_kind(golden());
  import_kind.get_access().get_data()[artifact_metadata_field + 60] = 3;
  EXPECT(rejects(import_kind));

  Dynamic::Bytes empty_export_route =
      body_splice(golden(), export_field + 20, 9, View::Bytes());
  set_u32(empty_export_route, export_field + 4, 61);
  set_u32(empty_export_route, export_field + 12, 19);
  set_u32(empty_export_route, export_field + 16, 0);
  EXPECT(rejects(empty_export_route));

  Dynamic::Bytes empty_artifact =
      body_splice(golden(), artifact_field + 20, 3, View::Bytes());
  set_u32(empty_artifact, artifact_field + 4, 23);
  set_u32(empty_artifact, artifact_field + 12, 4);
  set_u32(empty_artifact, artifact_field + 16, 0);
  EXPECT(rejects(empty_artifact));

  Dynamic::Bytes empty_export_artifact =
      body_splice(golden(), export_field + 33, 3, View::Bytes());
  set_u32(empty_export_artifact, export_field + 4, 67);
  set_u32(empty_export_artifact, export_field + 12, 25);
  set_u32(empty_export_artifact, export_field + 29, 0);
  EXPECT(rejects(empty_export_artifact));

  Dynamic::Bytes empty_symbol =
      body_splice(golden(), export_field + 40, 4, View::Bytes());
  set_u32(empty_symbol, export_field + 12, 24);
  set_u32(empty_symbol, export_field + 4, 66);
  set_u32(empty_symbol, export_field + 36, 0);
  EXPECT(rejects(empty_symbol));
}

PERIMORTEM_UNIT_TEST(PackageArchive, unique_references) {
  Dynamic::Bytes scope_collision(golden());
  auto scope_bytes = scope_collision.get_access();
  auto* scope_data = scope_bytes.get_data();
  scope_data[dependency_field + 20] = 'M';
  scope_data[dependency_field + 21] = 'a';
  scope_data[dependency_field + 22] = 'i';
  scope_data[dependency_field + 23] = 'n';
  EXPECT(rejects(scope_collision));
  EXPECT(
      Test::error_contains(
          "duplicate_inventory=Package scope names value=Main "
          "dependency_index=0 member_index=0"_view,
          Diagnostics::Log::Level::Debug));

  View::Bytes dependency_record = golden().slice(dependency_field + 12, 28);
  Dynamic::Bytes duplicate_dependency =
      body_splice(golden(), member_field, 0, dependency_record);
  set_u32(duplicate_dependency, dependency_field + 4, 60);
  set_u32(duplicate_dependency, dependency_field + 8, 2);
  EXPECT(rejects(duplicate_dependency));
  EXPECT(
      Test::error_contains(
          "duplicate_inventory=Dependency local names value=Core "
          "first_index=0 second_index=1"_view,
          Diagnostics::Log::Level::Debug));

  View::Bytes member_record = golden().slice(member_field + 12, 23);
  Dynamic::Bytes duplicate_member =
      body_splice(golden(), member_field + 35, 0, member_record);
  set_u32(duplicate_member, member_field + 4, 84);
  set_u32(duplicate_member, member_field + 8, 3);
  EXPECT(rejects(duplicate_member));
  EXPECT(
      Test::error_contains(
          "duplicate_inventory=Member semantic names value=Main "
          "first_index=0 second_index=1"_view,
          Diagnostics::Log::Level::Debug));

  Dynamic::Bytes duplicate_artifact(golden());
  duplicate_artifact.get_access().get_data()[artifact_field + 31] = 'c';
  duplicate_artifact.get_access().get_data()[artifact_field + 32] = 'p';
  duplicate_artifact.get_access().get_data()[artifact_field + 33] = 'u';
  EXPECT(rejects(duplicate_artifact));
  EXPECT(
      Test::error_contains(
          "duplicate_inventory=Artifact IDs value=cpu first_index=0 "
          "second_index=1"_view,
          Diagnostics::Log::Level::Debug));

  View::Bytes export_record = golden().slice(export_field + 12, 32);
  Dynamic::Bytes duplicate_export =
      body_splice(golden(), export_field + 44, 0, export_record);
  set_u32(duplicate_export, export_field + 4, 102);
  set_u32(duplicate_export, export_field + 8, 3);
  EXPECT(rejects(duplicate_export));
  EXPECT(
      Test::error_contains(
          "duplicate_inventory=Export semantic routes value=Main::Run "
          "first_index=0 second_index=1"_view,
          Diagnostics::Log::Level::Debug));

  Dynamic::Bytes undeclared_artifact(golden());
  undeclared_artifact.get_access().get_data()[export_field + 33] = 'b';
  undeclared_artifact.get_access().get_data()[export_field + 34] = 'a';
  undeclared_artifact.get_access().get_data()[export_field + 35] = 'd';
  EXPECT(rejects(undeclared_artifact));
  EXPECT(
      Test::error_contains(
          "export_index=0 semantic_route=Main::Run unknown_artifact_id=bad"_view,
          Diagnostics::Log::Level::Debug));
}
