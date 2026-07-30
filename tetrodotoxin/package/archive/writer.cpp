// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/archive/writer.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/writer/binary.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

using LittleWriter = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;

static constexpr Unsigned_64 format_limit = Unsigned_32(-1);
static constexpr Unsigned_32 section_header_size = 8;
static constexpr Unsigned_16 required_field = 1;
static constexpr Unsigned_64 section_count =
    Unsigned_8(Package::Archive::Archive::Sections::Exports);

// Holds the proven payload size for each canonical section and the complete
// envelope. These measurements belong to one write transaction and never
// become retained Archive facts.
struct FormatSizes {
  Unsigned_32 identity = 0;
  Unsigned_32 dependencies = 0;
  Unsigned_32 members = 0;
  Unsigned_32 artifact_ids = 0;
  Unsigned_32 exports = 0;
  Unsigned_32 body = 0;
  Count total = 0;
};

// Includes the unsigned 32 bit length prefix in one sized byte value.
static auto measure_sized_bytes(View::Bytes value) -> Unsigned_64 {
  return 4 + Unsigned_64(value.get_size());
}

// Record measurements exclude their outer size prefix. The containing list
// adds that prefix exactly once after each body has been measured.
static auto measure_dependency_record(
    const Package::Language::Dependency& dependency) -> Unsigned_64 {
  return measure_sized_bytes(dependency.get_local_name()) +
         measure_sized_bytes(dependency.get_package_name()) + 4;
}

static auto measure_member_record(const Package::Archive::Member& member)
    -> Unsigned_64 {
  return measure_sized_bytes(member.get_semantic_name()) +
         measure_sized_bytes(member.get_dialect_name()) +
         measure_sized_bytes(member.get_payload());
}

static auto measure_export_record(const Package::Archive::Export& entry)
    -> Unsigned_64 {
  return measure_sized_bytes(entry.get_semantic_route()) +
         measure_sized_bytes(entry.get_artifact_id()) +
         measure_sized_bytes(entry.get_symbol_locator());
}

// Measures all six section payloads and the complete body with unsigned 64 bit
// locals. Every nested value contributes a positive part of the body. Proving
// the body fits therefore proves every unsigned 32 bit section and record size
// fits before allocation begins.
static auto calculate_sizes(const Package::Archive::Archive& archive)
    -> Option<FormatSizes> {
  auto dependency_values = archive.get_dependencies();
  auto member_values = archive.get_members();
  auto artifact_ids = archive.get_artifact_ids();
  auto export_values = archive.get_exports();
  if (dependency_values.get_size() > format_limit ||
      member_values.get_size() > format_limit ||
      artifact_ids.get_size() > format_limit ||
      export_values.get_size() > format_limit) {
    return {};
  }

  Unsigned_64 identity = measure_sized_bytes(archive.get_identity());
  Unsigned_64 dependencies = 4;
  for (Count i = 0; i < dependency_values.get_size(); i++) {
    dependencies += 4 + measure_dependency_record(dependency_values[i]);
  }

  Unsigned_64 members = 4;
  for (Count i = 0; i < member_values.get_size(); i++) {
    members += 4 + measure_member_record(member_values[i]);
  }

  Unsigned_64 artifact_ids_size = 4;
  for (Count i = 0; i < artifact_ids.get_size(); i++) {
    artifact_ids_size += 4 + measure_sized_bytes(artifact_ids[i]);
  }

  Unsigned_64 exports = 4;
  for (Count i = 0; i < export_values.get_size(); i++) {
    exports += 4 + measure_export_record(export_values[i]);
  }

  Unsigned_64 body = section_header_size * section_count + identity + 4 +
                     dependencies + members + artifact_ids_size + exports;
  if (body > format_limit) {
    return {};
  }

  return FormatSizes{
    .identity = Unsigned_32(identity),
    .dependencies = Unsigned_32(dependencies),
    .members = Unsigned_32(members),
    .artifact_ids = Unsigned_32(artifact_ids_size),
    .exports = Unsigned_32(exports),
    .body = Unsigned_32(body),
    .total = Count(body) + Package::Archive::Archive::header_size,
  };
}

// Writes one required section header from the Archive vocabulary. Reader may
// accept bounded optional extensions, but canonical output contains only these
// six known sections.
static auto write_section_header(
    LittleWriter& writer,
    Package::Archive::Archive::Sections section,
    Unsigned_32 payload_size) -> void {
  writer << Unsigned_16(section);
  writer << required_field;
  writer << payload_size;
}

// Writes one sized byte value whose complete framing was proven by the
// measurement pass.
static auto write_sized_bytes(LittleWriter& writer, View::Bytes value) -> void {
  writer << Unsigned_32(value.get_size());
  writer << value;
}

auto Package::Archive::Writer::write(const Archive& archive)
    -> Option<Dynamic::Bytes> {
  // Prove the complete envelope fits Format 1 before allocating or emitting
  // any output.
  auto measured = calculate_sizes(archive);
  if (!measured) {
    Diagnostics::Log::warning(
        "Package::Archive::Writer exceeded the Format 1 body limit."_view);
    return {};
  }

  const FormatSizes& sizes = *measured;

  // Allocate the exact header and body size so successful emission requires no
  // growth and cannot leave unused capacity inside the result.
  Dynamic::Bytes output;
  output.forgetful_resize(sizes.total);
  LittleWriter writer(output.get_access());

  // Establish the fixed Format 1 header before emitting any section payload.
  writer << "TTXA"_view;
  writer << Unsigned_16(1);
  writer << Unsigned_16(0);
  writer << sizes.body;

  // Encode Package identity as the first required sized byte value.
  write_section_header(writer, Archive::Sections::Identity, sizes.identity);
  write_sized_bytes(writer, archive.get_identity());

  // Encode the pinned Package version in its fixed four byte section.
  write_section_header(writer, Archive::Sections::Version, 4);
  writer << archive.get_version().get_major();
  writer << archive.get_version().get_minor();

  // Preserve Dependency request order and frame every record independently.
  write_section_header(
      writer, Archive::Sections::Dependencies, sizes.dependencies);
  auto dependencies = archive.get_dependencies();
  writer << Unsigned_32(dependencies.get_size());
  for (Count i = 0; i < dependencies.get_size(); i++) {
    const auto& dependency = dependencies[i];
    Unsigned_32 record_size =
        Unsigned_32(measure_dependency_record(dependency));
    writer << record_size;
    write_sized_bytes(writer, dependency.get_local_name());
    write_sized_bytes(writer, dependency.get_package_name());
    writer << dependency.get_version().get_major();
    writer << dependency.get_version().get_minor();
  }

  // Preserve Member order while keeping every semantic name, Dialect name, and
  // opaque payload inside its own record.
  write_section_header(writer, Archive::Sections::Members, sizes.members);
  auto members = archive.get_members();
  writer << Unsigned_32(members.get_size());
  for (Count i = 0; i < members.get_size(); i++) {
    const auto& member = members[i];
    Unsigned_32 record_size = Unsigned_32(measure_member_record(member));
    writer << record_size;
    write_sized_bytes(writer, member.get_semantic_name());
    write_sized_bytes(writer, member.get_dialect_name());
    write_sized_bytes(writer, member.get_payload());
  }

  // Encode each artifact ID directly because Archive retains no wrapper around
  // the logical byte value.
  write_section_header(
      writer, Archive::Sections::ArtifactIds, sizes.artifact_ids);
  auto artifact_ids = archive.get_artifact_ids();
  writer << Unsigned_32(artifact_ids.get_size());
  for (Count i = 0; i < artifact_ids.get_size(); i++) {
    View::Bytes artifact_id = artifact_ids[i];
    Unsigned_32 record_size = Unsigned_32(measure_sized_bytes(artifact_id));
    writer << record_size;
    write_sized_bytes(writer, artifact_id);
  }

  // Preserve Export order and keep each semantic route, artifact reference,
  // and symbol locator inside one record.
  write_section_header(writer, Archive::Sections::Exports, sizes.exports);
  auto exports = archive.get_exports();
  writer << Unsigned_32(exports.get_size());
  for (Count i = 0; i < exports.get_size(); i++) {
    const auto& entry = exports[i];
    Unsigned_32 record_size = Unsigned_32(measure_export_record(entry));
    writer << record_size;
    write_sized_bytes(writer, entry.get_semantic_route());
    write_sized_bytes(writer, entry.get_artifact_id());
    write_sized_bytes(writer, entry.get_symbol_locator());
  }

  // Require emission to finish at the measured boundary. A mismatch means the
  // measurement and canonical encoding no longer describe the same format.
  if (!writer.is_valid() || writer.get_location() != output.get_size()) {
    Diagnostics::Log::error(
        "Package::Archive::Writer did not emit the measured Format 1 "
        "size."_view);
    return {};
  }

  // Transfer ownership only after the complete canonical envelope is proven.
  return Option<Dynamic::Bytes>(static_cast<Dynamic::Bytes&&>(output));
}
