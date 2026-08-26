// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/package/archive/writer.hpp"

#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/writer/binary.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/vector.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;

using LittleWriter = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;

static constexpr U64 format_limit = U32(-1);
static constexpr U32 section_header_size = 8;
static constexpr U16 required_field = 1;
static constexpr U16 interface_profile = 1;
static constexpr U64 format_two_section_count =
    U8(Package::Archive::Archive::Sections::ArtifactMetadata);

// Holds the proven payload size for each canonical section and the complete
// envelope. These measurements belong to one write transaction and never
// become retained Archive facts.
struct FormatSizes {
  U32 identity = 0;
  U32 dependencies = 0;
  U32 members = 0;
  U32 artifact_ids = 0;
  U32 exports = 0;
  U32 artifact_metadata = 0;
  U32 resources = 0;
  U32 imports = 0;
  U32 body = 0;
  Count total = 0;
};

// Includes the unsigned 32 bit length prefix in one sized byte value.
static auto measure_sized_bytes(View::Bytes value) -> U64 {
  return 4 + U64(value.get_size());
}

// Record measurements exclude their outer size prefix. The containing list
// adds that prefix exactly once after each body has been measured.
static auto measure_dependency_record(
    const Package::Language::Dependency& dependency) -> U64 {
  return measure_sized_bytes(dependency.get_local_name()) +
         measure_sized_bytes(dependency.get_package_name()) + 4;
}

static auto measure_member_record(const Package::Archive::Member& member)
    -> U64 {
  return measure_sized_bytes(member.get_semantic_name()) +
         measure_sized_bytes(member.get_dialect_name()) +
         measure_sized_bytes(member.get_payload());
}

static auto measure_import_record(const Linker::Import& import) -> U64 {
  return 1 + measure_sized_bytes(import.get_abi()) +
         measure_sized_bytes(import.get_symbol()) +
         measure_sized_bytes(import.get_provider());
}

static auto measure_artifact_record(const Package::Archive::Artifact& artifact)
    -> U64 {
  U64 size = measure_sized_bytes(artifact.get_id()) +
             measure_sized_bytes(artifact.get_target()) + 8 + 4;
  for (const Linker::Import& import : artifact.get_imports()) {
    size += 4 + measure_import_record(import);
  }
  return size;
}

static auto measure_export_record(const Package::Archive::Export& entry)
    -> U64 {
  return measure_sized_bytes(entry.get_semantic_route()) +
         measure_sized_bytes(entry.get_artifact_id()) +
         measure_sized_bytes(entry.get_symbol_locator());
}

static auto measure_resource_record(const Package::Archive::Resource& resource)
    -> U64 {
  return measure_sized_bytes(resource.get_route()) +
         measure_sized_bytes(resource.get_value());
}

static auto measure_graph_import_record(
    const Package::Archive::GraphImport& import) -> U64 {
  return 1 + measure_sized_bytes(import.get_importer()) +
         measure_sized_bytes(import.get_local_name()) +
         measure_sized_bytes(import.get_target()) + 4;
}

// Measures all seven section payloads and the complete body with unsigned 64
// bit locals. Every nested value contributes a positive part of the body.
// Proving the body fits therefore proves every unsigned 32 bit section and
// record size fits before allocation begins.
static auto calculate_sizes(const Package::Archive::Archive& archive)
    -> Option<FormatSizes> {
  auto dependency_values = archive.get_dependencies();
  auto member_values = archive.get_members();
  auto artifacts_values = archive.get_artifacts();
  auto export_values = archive.get_exports();
  auto resource_values = archive.get_resources();
  auto import_values = archive.get_imports();
  if (dependency_values.get_size() > format_limit ||
      member_values.get_size() > format_limit ||
      artifacts_values.get_size() > format_limit ||
      export_values.get_size() > format_limit ||
      resource_values.get_size() > format_limit ||
      import_values.get_size() > format_limit) {
    return {};
  }

  U64 identity = measure_sized_bytes(archive.get_identity());
  U64 dependencies = 4;
  for (Count i = 0; i < dependency_values.get_size(); i++) {
    dependencies +=
        4 + measure_dependency_record(dependency_values.get_data()[i]);
  }

  U64 members = 4;
  for (Count i = 0; i < member_values.get_size(); i++) {
    members += 4 + measure_member_record(member_values.get_data()[i]);
  }

  U64 artifact_ids = 4;
  U64 artifact_metadata = 4;
  for (const Package::Archive::Artifact& artifact : artifacts_values) {
    if (artifact.get_imports().get_size() > format_limit) {
      return {};
    }
    artifact_ids += 4 + measure_sized_bytes(artifact.get_id());
    artifact_metadata += 4 + measure_artifact_record(artifact);
  }

  U64 exports = 4;
  for (Count i = 0; i < export_values.get_size(); i++) {
    exports += 4 + measure_export_record(export_values.get_data()[i]);
  }

  U64 resources = 4;
  for (const Package::Archive::Resource& resource : resource_values) {
    resources += 4 + measure_resource_record(resource);
  }

  U64 imports = 4;
  for (const Package::Archive::GraphImport& import : import_values) {
    imports += 4 + measure_graph_import_record(import);
  }

  Bool graph_format = !import_values.is_empty();
  U64 section_count = format_two_section_count +
                      (graph_format ? 2 : (resource_values.is_empty() ? 0 : 1));
  U64 body = section_header_size * section_count + identity + 4 + dependencies +
             members + artifact_ids + exports + artifact_metadata +
             (graph_format || !resource_values.is_empty() ? resources : 0) +
             (graph_format ? imports : 0);
  if (body > format_limit) {
    return {};
  }

  return FormatSizes{
    .identity = U32(identity),
    .dependencies = U32(dependencies),
    .members = U32(members),
    .artifact_ids = U32(artifact_ids),
    .exports = U32(exports),
    .artifact_metadata = U32(artifact_metadata),
    .resources = U32(resources),
    .imports = U32(imports),
    .body = U32(body),
    .total = Count(body) + Package::Archive::Archive::header_size,
  };
}

// Writes one required section header from the Archive vocabulary. Reader may
// accept bounded optional extensions, while canonical output contains only the
// sections selected by the current revision.
static auto write_section_header(
    LittleWriter& writer,
    Package::Archive::Archive::Sections section,
    U32 payload_size) -> void {
  writer << U16(section);
  writer << required_field;
  writer << payload_size;
}

// Writes one sized byte value whose complete framing was proven by the
// measurement pass.
static auto write_sized_bytes(LittleWriter& writer, View::Bytes value) -> void {
  writer << U32(value.get_size());
  writer << value;
}

auto Package::Archive::Writer::write(const Archive& archive)
    -> Option<Dynamic::Bytes> {
  // Prove the complete envelope fits its bounded format before allocating
  // any output.
  auto measured = calculate_sizes(archive);
  if (!measured) {
    Diagnostics::Log::warning(
        "Package::Archive::Writer exceeded the body limit."_view);
    return {};
  }

  const FormatSizes& sizes = *measured;

  // Allocate the exact header and body size so successful emission requires no
  // growth and cannot leave unused capacity inside the result.
  Dynamic::Bytes output;
  output.forgetful_resize(sizes.total);
  LittleWriter writer(output.get_access());

  // Resource free values preserve Format 2 exactly. Format 3 adds one final
  // required Package Resource section while leaving the first seven section
  // identities unchanged.
  writer << "TTXA"_view;
  writer << U16(
      !archive.get_imports().is_empty()
          ? 4
          : (archive.get_resources().is_empty() ? 2 : 3));
  writer << U16(
      archive.get_profile() ==
              Tetrodotoxin::Language::Persistence::Profile::Contract
          ? interface_profile
          : 0);
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
  writer << U32(dependencies.get_size());
  for (Count i = 0; i < dependencies.get_size(); i++) {
    const auto& dependency = dependencies.get_data()[i];
    U32 record_size = U32(measure_dependency_record(dependency));
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
  writer << U32(members.get_size());
  for (Count i = 0; i < members.get_size(); i++) {
    const auto& member = members.get_data()[i];
    U32 record_size = U32(measure_member_record(member));
    writer << record_size;
    write_sized_bytes(writer, member.get_semantic_name());
    write_sized_bytes(writer, member.get_dialect_name());
    write_sized_bytes(writer, member.get_payload());
  }

  // Artifact identity remains the direct Export reference section. Metadata
  // repeats the ID later so Reader can reject a missing or duplicate agreement.
  write_section_header(
      writer, Archive::Sections::ArtifactIds, sizes.artifact_ids);
  auto artifacts = archive.get_artifacts();
  writer << U32(artifacts.get_size());
  for (const Package::Archive::Artifact& artifact : artifacts) {
    U32 record_size = U32(measure_sized_bytes(artifact.get_id()));
    writer << record_size;
    write_sized_bytes(writer, artifact.get_id());
  }

  // Preserve Export order and keep each semantic route, artifact reference,
  // and symbol locator inside one record.
  write_section_header(writer, Archive::Sections::Exports, sizes.exports);
  auto exports = archive.get_exports();
  writer << U32(exports.get_size());
  for (Count i = 0; i < exports.get_size(); i++) {
    const auto& entry = exports.get_data()[i];
    U32 record_size = U32(measure_export_record(entry));
    writer << record_size;
    write_sized_bytes(writer, entry.get_semantic_route());
    write_sized_bytes(writer, entry.get_artifact_id());
    write_sized_bytes(writer, entry.get_symbol_locator());
  }

  // Target ABI metadata follows routing so Format 2 keeps the original six
  // section offsets stable and adds one independently framed agreement.
  write_section_header(
      writer, Archive::Sections::ArtifactMetadata, sizes.artifact_metadata);
  writer << U32(artifacts.get_size());
  for (const Package::Archive::Artifact& artifact : artifacts) {
    U32 record_size = U32(measure_artifact_record(artifact));
    writer << record_size;
    write_sized_bytes(writer, artifact.get_id());
    write_sized_bytes(writer, artifact.get_target());
    writer << artifact.get_fingerprint().get_value();
    writer << U32(artifact.get_imports().get_size());
    for (const Linker::Import& import : artifact.get_imports()) {
      writer << U32(measure_import_record(import));
      writer << U8(import.get_kind());
      write_sized_bytes(writer, import.get_abi());
      write_sized_bytes(writer, import.get_symbol());
      write_sized_bytes(writer, import.get_provider());
    }
  }

  auto resources = archive.get_resources();
  if (!resources.is_empty() || !archive.get_imports().is_empty()) {
    write_section_header(writer, Archive::Sections::Resources, sizes.resources);
    writer << U32(resources.get_size());
    for (const Package::Archive::Resource& resource : resources) {
      writer << U32(measure_resource_record(resource));
      write_sized_bytes(writer, resource.get_route());
      write_sized_bytes(writer, resource.get_value());
    }
  }

  auto graph_imports = archive.get_imports();
  if (!graph_imports.is_empty()) {
    write_section_header(writer, Archive::Sections::Imports, sizes.imports);
    writer << U32(graph_imports.get_size());
    for (const Package::Archive::GraphImport& import : graph_imports) {
      writer << U32(measure_graph_import_record(import));
      writer << U8(import.get_kind());
      write_sized_bytes(writer, import.get_importer());
      write_sized_bytes(writer, import.get_local_name());
      write_sized_bytes(writer, import.get_target());
      writer << import.get_version().get_major();
      writer << import.get_version().get_minor();
    }
  }

  // Require emission to finish at the measured boundary. A mismatch means the
  // measurement and canonical encoding no longer describe the same format.
  if (!writer.is_valid() || writer.get_location() != output.get_size()) {
    Diagnostics::Log::error(
        "Package::Archive::Writer did not emit the measured size."_view);
    return {};
  }

  // Transfer ownership only after the complete canonical envelope is proven.
  return Option<Dynamic::Bytes>(static_cast<Dynamic::Bytes&&>(output));
}

auto Package::Archive::Writer::write(
    const Package::Language::Monograph& package,
    View::Bytes identity,
    Perimortem::System::Version version,
    Tetrodotoxin::Language::Persistence::Profile profile,
    View::Vector<GraphMember> graph,
    View::Vector<GraphImport> imports,
    View::Vector<Artifact> artifacts,
    View::Vector<Export> exports) -> Option<Dynamic::Bytes> {
  BAIL_IF(identity.is_empty());

  Allocator::Arena arena;
  Dynamic::Vector<Dynamic::Bytes> payloads(graph.get_size() + 1);
  Managed::Vector<Member> members(arena);
  auto package_library = package.get_library()
                             .get_language()
                             .select<Tetrodotoxin::Language::Dialect>();
  BAIL_IF(!package_library);
  auto package_payload =
      package_library->encode(package.get_library(), profile);
  BAIL_IF(!package_payload);
  payloads.emplace(static_cast<Dynamic::Bytes&&>(*package_payload));
  members.insert(Member(
      "PackageSurface"_view, package_library->get_name(),
      payloads[payloads.get_size() - 1].get_view()));

  for (const GraphMember& selected : graph) {
    const Tetrodotoxin::Language::Monograph& member = selected.get_monograph();
    auto dialect =
        member.get_language().select<Tetrodotoxin::Language::Dialect>();
    BAIL_IF(!dialect);

    auto payload = dialect->encode(member, profile);
    if (!payload) {
      Diagnostics::Log::Message<256> message(
          Diagnostics::Log::Level::Error, Diagnostics::Source());
      message << "Package Archive could not encode `"_view
              << selected.get_name() << "` with the "_view
              << dialect->get_name() << " Dialect."_view;
      return {};
    }
    payloads.emplace(static_cast<Dynamic::Bytes&&>(*payload));
    members.insert(Member(
        selected.get_name(), dialect->get_name(),
        payloads[payloads.get_size() - 1].get_view()));
  }

  Managed::Vector<Package::Archive::Resource> resources(arena);
  for (const Ttx::Concept::Reference<Package::Resource>& retained :
       package.get_resources().get_values()) {
    const Package::Resource& resource = retained.get();
    resources.insert(
        Package::Archive::Resource(resource.get_route(), resource.get_value()));
  }

  Managed::Vector<Package::Language::Dependency> dependencies(arena);
  Archive archive(
      identity, version, dependencies.get_view(), members.get_view(), artifacts,
      exports, profile, resources.get_view(), imports);
  return write(archive);
}
