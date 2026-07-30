// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/archive/reader.hpp"

#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/reader/binary.hpp"

#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/serialization/stream/textual.hpp"

#include "ttx/lexical/lexicon.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

using LittleReader = Perimortem::Core::Reader::Binary<Data::ByteOrder::Little>;

// Names the complete read transaction once so every rejection is attributed to
// the same operation. The required flag and section bounds stay local because
// only Reader applies those framing rules.
static constexpr View::Bytes archive_read_operation =
    "Package::Archive::Reader Format 1 read"_view;
static constexpr Unsigned_16 required_field = 1;
static constexpr Unsigned_8 first_section =
    Unsigned_8(Package::Archive::Archive::Sections::Identity);
static constexpr Unsigned_8 last_section =
    Unsigned_8(Package::Archive::Archive::Sections::Exports);

// Archive qualification rules name their lexical separator Codes once and
// leave every segment and separator spelling check with the shared Lexicon.
static constexpr Static::Vector<Code::Type, 1> semantic_name_separators = {{
  Code::Type::TypeAccessOp,
}};
static constexpr Static::Vector<Code::Type, 1> package_identity_separators = {{
  Code::Type::AddressOp,
}};

// Opaque Archive identifiers have no Package grammar beyond presence and NUL
// freedom. Their later semantic or native owners decide whether each spelling
// is meaningful.
static auto is_opaque_identifier(View::Bytes value) -> Bool {
  if (value.is_empty()) {
    return False;
  }

  for (Count i = 0; i < value.get_size(); i++) {
    if (value[i] == '\0') {
      return False;
    }
  }

  return True;
}

// Proves the relationships between the complete decoded sections before any
// record inventory enters the caller Arena.
static auto validate(
    View::Bytes identity,
    Version version,
    View::Vector<Package::Language::Dependency> dependencies,
    View::Vector<Package::Archive::Member> members,
    View::Vector<View::Bytes> artifact_ids,
    View::Vector<Package::Archive::Export> exports) -> Bool {
  if (!Lexicon::validate(
          Code::Type::Type, identity, package_identity_separators) ||
      version.is_null() || members.is_empty()) {
    return False;
  }

  // Dependencies retain authored order but require unique local aliases and
  // exact Package identities and pinned versions.
  for (Count i = 0; i < dependencies.get_size(); i++) {
    const auto& dependency = dependencies[i];
    Bool duplicate_alias = dependencies.slice(0, i).contains(
        [&dependency](const Package::Language::Dependency& existing) {
          return existing.get_local_name() == dependency.get_local_name();
        });
    if (!Lexicon::validate(
            Code::Type::Type, dependency.get_local_name(),
            semantic_name_separators) ||
        !Lexicon::validate(
            Code::Type::Type, dependency.get_package_name(),
            package_identity_separators) ||
        dependency.get_version().is_null() || duplicate_alias) {
      return False;
    }
  }

  // Members require unique semantic names and one concrete Dialect name.
  // Payload contents remain opaque, including engaged empty bytes.
  for (Count i = 0; i < members.get_size(); i++) {
    const auto& member = members[i];
    Bool duplicate_name = members.slice(0, i).contains(
        [&member](const Package::Archive::Member& existing) {
          return existing.get_semantic_name() == member.get_semantic_name();
        });
    if (!Lexicon::validate(
            Code::Type::Type, member.get_semantic_name(),
            semantic_name_separators) ||
        !Lexicon::validate(Code::Type::Type, member.get_dialect_name()) ||
        duplicate_name) {
      return False;
    }
  }

  // Artifact IDs remain direct opaque byte values. Package proves only their
  // presence and uniqueness before an Export can refer to one.
  for (Count i = 0; i < artifact_ids.get_size(); i++) {
    View::Bytes id = artifact_ids[i];
    if (!is_opaque_identifier(id) || artifact_ids.slice(0, i).contains(id)) {
      return False;
    }
  }

  // Export spellings stay opaque here. Package proves only their presence,
  // uniqueness, NUL freedom, and reference to a declared artifact.
  for (Count i = 0; i < exports.get_size(); i++) {
    const auto& entry = exports[i];
    Bool duplicate_route = exports.slice(0, i).contains(
        [&entry](const Package::Archive::Export& existing) {
          return existing.get_semantic_route() == entry.get_semantic_route();
        });
    Bool known_artifact = artifact_ids.contains(entry.get_artifact_id());
    if (!is_opaque_identifier(entry.get_semantic_route()) ||
        !is_opaque_identifier(entry.get_artifact_id()) ||
        !is_opaque_identifier(entry.get_symbol_locator()) || duplicate_route ||
        !known_artifact) {
      return False;
    }
  }

  return True;
}

// Binary moves its cursor to the maximum Count value when a requested value
// escapes the input and logs the boundary failure at Debug. Reader recognizes
// that state and publishes one scoped Report containing the caller identity and
// complete input.
static auto is_valid(const LittleReader& reader) -> Bool {
  return reader.get_location() != Count(-1);
}

// Consumes one unsigned 32 bit length and the exact byte slice it declares.
// Binary performs both boundary checks and leaves its terminal location on
// failure.
static auto read_sized_bytes(LittleReader& reader, View::Bytes& value) -> Bool {
  Unsigned_32 size = reader.read_unsigned_32();
  if (!is_valid(reader)) {
    return False;
  }

  value = reader.read_bytes(size);
  return is_valid(reader);
}

// Rejects impossible list counts before Dynamic storage is allocated. The
// minimum is the complete smallest valid record including its own size frame,
// so adversarial counts cannot reserve more records than the payload can hold.
template <typename value_type>
static auto can_allocate_records(
    Unsigned_32 count,
    View::Bytes payload,
    Count offset,
    Count minimum_record_size) -> Bool {
  if (offset > payload.get_size() || minimum_record_size == 0) {
    return False;
  }

  Count remaining = payload.get_size() - offset;
  return Count(count) <= remaining / minimum_record_size &&
         Count(count) <= Count(-1) / sizeof(value_type);
}

// A singleton section is valid only when its one value consumes the complete
// payload rather than a valid prefix.
static auto parse_identity(View::Bytes payload, View::Bytes& identity) -> Bool {
  LittleReader reader(payload);
  Bool read = read_sized_bytes(reader, identity);
  return read && reader.get_location() == reader.get_size();
}

// Version is the only fixed section payload. Both unsigned 16 bit values must
// consume its four bytes exactly.
static auto parse_version(View::Bytes payload, Version& version) -> Bool {
  LittleReader reader(payload);
  Unsigned_16 major = reader.read_unsigned_16();
  Unsigned_16 minor = reader.read_unsigned_16();
  if (!is_valid(reader) || reader.get_location() != reader.get_size()) {
    return False;
  }

  version = Version(major, minor);
  return True;
}

// Decodes ordered Dependency records into transaction storage that still
// borrows the input. Reader validates their relationships and retains them only
// after every section succeeds.
static auto parse_dependencies(
    View::Bytes payload,
    Dynamic::Vector<Package::Language::Dependency>& dependencies) -> Bool {
  LittleReader reader(payload);
  Unsigned_32 count = reader.read_unsigned_32();
  if (!is_valid(reader) || !can_allocate_records<Package::Language::Dependency>(
                               count, payload, reader.get_location(), 18)) {
    return False;
  }

  dependencies = Dynamic::Vector<Package::Language::Dependency>(count);
  for (Unsigned_32 i = 0; i < count; i++) {
    // Isolate one Dependency with its outer frame. The two sized names and
    // fixed version must consume that record exactly before it is retained.
    Unsigned_32 record_size = reader.read_unsigned_32();
    View::Bytes record = reader.read_bytes(record_size);
    if (!is_valid(reader)) {
      return False;
    }

    LittleReader record_reader(record);
    View::Bytes local_name;
    View::Bytes package_name;
    Bool local_name_read = read_sized_bytes(record_reader, local_name);
    Bool package_name_read = read_sized_bytes(record_reader, package_name);
    Unsigned_16 major = record_reader.read_unsigned_16();
    Unsigned_16 minor = record_reader.read_unsigned_16();
    if (!local_name_read || !package_name_read || !is_valid(record_reader) ||
        record_reader.get_location() != record_reader.get_size()) {
      return False;
    }

    dependencies.emplace(
        Package::Language::Dependency(
            local_name, package_name, Version(major, minor)));
  }

  return reader.get_location() == reader.get_size();
}

// Decodes each Member identity, Dialect identity, and payload without
// interpreting bytes owned by the concrete Dialect.
static auto parse_members(
    View::Bytes payload,
    Dynamic::Vector<Package::Archive::Member>& members) -> Bool {
  LittleReader reader(payload);
  Unsigned_32 count = reader.read_unsigned_32();
  if (!is_valid(reader) || !can_allocate_records<Package::Archive::Member>(
                               count, payload, reader.get_location(), 18)) {
    return False;
  }

  members = Dynamic::Vector<Package::Archive::Member>(count);
  for (Unsigned_32 i = 0; i < count; i++) {
    // Isolate each Member before reading its three sized values. An invalid
    // payload size therefore cannot consume the next record.
    Unsigned_32 record_size = reader.read_unsigned_32();
    View::Bytes record = reader.read_bytes(record_size);
    if (!is_valid(reader)) {
      return False;
    }

    LittleReader record_reader(record);
    View::Bytes semantic_name;
    View::Bytes dialect_name;
    View::Bytes member_payload;
    Bool semantic_name_read = read_sized_bytes(record_reader, semantic_name);
    Bool dialect_name_read = read_sized_bytes(record_reader, dialect_name);
    Bool payload_read = read_sized_bytes(record_reader, member_payload);
    if (!semantic_name_read || !dialect_name_read || !payload_read ||
        record_reader.get_location() != record_reader.get_size()) {
      return False;
    }

    members.emplace(
        Package::Archive::Member(semantic_name, dialect_name, member_payload));
  }

  return reader.get_location() == reader.get_size();
}

// Decodes each artifact record to the one logical ID retained by Archive.
static auto parse_artifact_ids(
    View::Bytes payload,
    Dynamic::Vector<View::Bytes>& artifact_ids) -> Bool {
  LittleReader reader(payload);
  Unsigned_32 count = reader.read_unsigned_32();
  if (!is_valid(reader) || !can_allocate_records<View::Bytes>(
                               count, payload, reader.get_location(), 9)) {
    return False;
  }

  artifact_ids = Dynamic::Vector<View::Bytes>(count);
  for (Unsigned_32 i = 0; i < count; i++) {
    Unsigned_32 record_size = reader.read_unsigned_32();
    View::Bytes record = reader.read_bytes(record_size);
    if (!is_valid(reader)) {
      return False;
    }

    LittleReader record_reader(record);
    View::Bytes id;
    Bool id_read = read_sized_bytes(record_reader, id);
    if (!id_read || record_reader.get_location() != record_reader.get_size()) {
      return False;
    }

    artifact_ids.emplace(View::Bytes(id));
  }

  return reader.get_location() == reader.get_size();
}

// Decodes Export routing without applying later Library or native symbol
// legality. Reader proves only the opaque byte constraints and the reference
// to a declared artifact.
static auto parse_exports(
    View::Bytes payload,
    Dynamic::Vector<Package::Archive::Export>& exports) -> Bool {
  LittleReader reader(payload);
  Unsigned_32 count = reader.read_unsigned_32();
  if (!is_valid(reader) || !can_allocate_records<Package::Archive::Export>(
                               count, payload, reader.get_location(), 19)) {
    return False;
  }

  exports = Dynamic::Vector<Package::Archive::Export>(count);
  for (Unsigned_32 i = 0; i < count; i++) {
    // Keep the semantic route, artifact ID, and symbol locator inside one
    // record so each Export is either complete or rejected.
    Unsigned_32 record_size = reader.read_unsigned_32();
    View::Bytes record = reader.read_bytes(record_size);
    if (!is_valid(reader)) {
      return False;
    }

    LittleReader record_reader(record);
    View::Bytes semantic_route;
    View::Bytes artifact_id;
    View::Bytes symbol_locator;
    Bool semantic_route_read = read_sized_bytes(record_reader, semantic_route);
    Bool artifact_id_read = read_sized_bytes(record_reader, artifact_id);
    Bool symbol_locator_read = read_sized_bytes(record_reader, symbol_locator);
    if (!semantic_route_read || !artifact_id_read || !symbol_locator_read ||
        record_reader.get_location() != record_reader.get_size()) {
      return False;
    }

    exports.emplace(
        Package::Archive::Export(semantic_route, artifact_id, symbol_locator));
  }

  return reader.get_location() == reader.get_size();
}

// Publishes the sole diagnostic for one rejected read. The scoped Report
// receives the caller identity and complete input directly so diagnostics
// retain the exact source context.
static auto reject_archive(
    Errors& errors,
    View::Bytes diagnostic_identity,
    View::Bytes input,
    View::Bytes reason) -> Option<Package::Archive::Archive> {
  Errors::Report report(errors, diagnostic_identity, input);
  report << archive_read_operation << " rejected the input: "_view << reason;
  return {};
}

// Copies one decoded inventory into Arena storage without copying any nested
// byte views from the accepted input.
template <typename value_type>
static auto retain_records(
    Allocator::Arena& arena,
    View::Vector<value_type> values) -> View::Vector<value_type> {
  auto retained = arena.reserve<value_type>(values.get_size());
  for (Count i = 0; i < values.get_size(); i++) {
    retained[i] = values[i];
  }

  return retained.get_view();
}

// Retains the typed record inventories while every decoded byte view continues
// to borrow the accepted input under the Reader lifetime contract.
static auto retain_archive(
    Allocator::Arena& arena,
    View::Bytes identity,
    Version version,
    View::Vector<Package::Language::Dependency> dependencies,
    View::Vector<Package::Archive::Member> members,
    View::Vector<View::Bytes> artifact_ids,
    View::Vector<Package::Archive::Export> exports)
    -> Package::Archive::Archive {
  auto retained_dependencies = retain_records(arena, dependencies);
  auto retained_members = retain_records(arena, members);
  auto retained_artifact_ids = retain_records(arena, artifact_ids);
  auto retained_exports = retain_records(arena, exports);

  return Package::Archive::Archive(
      identity, version, retained_dependencies, retained_members,
      retained_artifact_ids, retained_exports);
}

auto Package::Archive::Reader::read(
    Allocator::Arena& arena,
    Errors& errors,
    View::Bytes diagnostic_identity,
    View::Bytes input) -> Option<Archive> {
  // Decode the complete fixed header first. Accepted input must carry the
  // Format 1 magic and version while leaving every reserved flag clear.
  LittleReader reader(input);
  View::Bytes magic = reader.read_bytes(4);
  Unsigned_16 format = reader.read_unsigned_16();
  Unsigned_16 header_flags = reader.read_unsigned_16();
  Unsigned_32 body_size = reader.read_unsigned_32();
  if (!is_valid(reader) || magic != "TTXA"_view || format != 1 ||
      header_flags != 0) {
    return reject_archive(
        errors, diagnostic_identity, input, "the header is invalid."_view);
  }

  // Require the declared body to consume every remaining input byte. Every
  // section read is then bounded by both the body declaration and the input.
  if (reader.get_location() != Archive::header_size ||
      Count(body_size) != reader.get_size() - reader.get_location()) {
    return reject_archive(
        errors, diagnostic_identity, input,
        "the declared body does not exactly match the input."_view);
  }

  // Hold decoded views in transaction storage until all six sections and their
  // semantic relationships pass. A malformed envelope therefore cannot retain
  // partial Archive state in the caller Arena.
  View::Bytes identity;
  Version version;
  Dynamic::Vector<Language::Dependency> dependencies;
  Dynamic::Vector<Member> members;
  Dynamic::Vector<View::Bytes> artifact_ids;
  Dynamic::Vector<Export> exports;
  Unsigned_8 expected_section = first_section;

  while (reader.has_content()) {
    // Isolate one section payload before interpreting its tag. A malformed
    // payload size reaches Binary's terminal state instead of escaping the
    // declared body.
    Unsigned_16 section_tag = reader.read_unsigned_16();
    Unsigned_16 flags = reader.read_unsigned_16();
    Unsigned_32 payload_size = reader.read_unsigned_32();
    View::Bytes payload = reader.read_bytes(payload_size);
    if (!is_valid(reader)) {
      return reject_archive(
          errors, diagnostic_identity, input,
          "a field escapes the declared body."_view);
    }

    // Bit zero is the only Format 1 section flag. Any other bit would assign
    // semantics that this Reader cannot prove.
    if ((flags & ~required_field) != 0) {
      return reject_archive(
          errors, diagnostic_identity, input,
          "a reserved field flag is set."_view);
    }

    const Bool known =
        section_tag >= first_section && section_tag <= last_section;
    if (!known) {
      // Skip an optional extension only after Binary has bounded its complete
      // payload. A required extension is rejected because this Reader cannot
      // establish the missing semantics.
      if ((flags & required_field) != 0) {
        return reject_archive(
            errors, diagnostic_identity, input,
            "an unknown required field is present."_view);
      }

      continue;
    }

    // Require each known section exactly once in canonical ascending order.
    if (flags != required_field || section_tag != expected_section) {
      return reject_archive(
          errors, diagnostic_identity, input,
          "known fields are missing, repeated, reordered, or optional."_view);
    }

    // Dispatch the current section through the shared public vocabulary. Each
    // parser requires exact payload and nested record consumption.
    Bool parsed = False;
    switch (Archive::Sections(Unsigned_8(section_tag))) {
    case Archive::Sections::Identity:
      parsed = parse_identity(payload, identity);
      break;
    case Archive::Sections::Version:
      parsed = parse_version(payload, version);
      break;
    case Archive::Sections::Dependencies:
      parsed = parse_dependencies(payload, dependencies);
      break;
    case Archive::Sections::Members:
      parsed = parse_members(payload, members);
      break;
    case Archive::Sections::ArtifactIds:
      parsed = parse_artifact_ids(payload, artifact_ids);
      break;
    case Archive::Sections::Exports:
      parsed = parse_exports(payload, exports);
      break;
    default:
      parsed = False;
      break;
    }

    if (!parsed) {
      return reject_archive(
          errors, diagnostic_identity, input,
          "a known field payload is malformed."_view);
    }

    expected_section++;
  }

  // Reaching the body boundary is not sufficient when a required section was
  // omitted. The expected value advances only after a section parses.
  if (expected_section != last_section + 1) {
    return reject_archive(
        errors, diagnostic_identity, input,
        "one or more required fields are missing."_view);
  }

  // Validate Package names, versions, uniqueness, and Export references after
  // every section is structurally complete.
  if (!validate(
          identity, version, dependencies, members, artifact_ids, exports)) {
    return reject_archive(
        errors, diagnostic_identity, input,
        "the decoded Package facts are invalid."_view);
  }

  // Retain the typed record ranges in the caller Arena without copying input
  // bytes. The returned Archive itself remains an ordinary value.
  return retain_archive(
      arena, identity, version, dependencies, members, artifact_ids, exports);
}
