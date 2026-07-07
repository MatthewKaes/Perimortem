// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/puffer/terminal/plan.hpp"

#include "perimortem/utility/table.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin;
using namespace Tetrodotoxin::Puffer;
using namespace Tetrodotoxin::Puffer::Resolution;

auto Terminal::Plan::add_record(Record& record) -> void {
  if (!record_set.insert(&record)) {
    return;
  }

  records.insert(&record);
  append_terminal(record);
}

auto Terminal::Plan::add_package(Resolver& resolver, Record& root) -> void {
  resolver.visit_reachable(
      root, [this](Record& record) -> void { add_record(record); });
}

auto Terminal::Plan::lower() -> Bool {
  // Render records only register contract facts with the Shader compiler today,
  // but that must happen before any Shader record tries to emit stage modules.
  return lower_kind(RecordKind::Library) &&
         lower_kind(RecordKind::Render) &&
         lower_kind(RecordKind::Shader);
}

auto Terminal::Plan::lower_kind(RecordKind kind) -> Bool {
  Bool valid = True;
  for (Count i = 0; i < records.get_size(); i++) {
    if (classify(*records[i]) == kind && !lower_record(*records[i], kind)) {
      valid = False;
    }
  }

  return valid;
}

auto Terminal::Plan::lower_record(Record& record, RecordKind kind) -> Bool {
  const Ttx::Type* type = record.get_type();
  if (type == nullptr) {
    return True;
  }

  View::Bytes module = build_module_name(record.get_source_path());
  Ttx::Lexical::Source source(record.get_source_path(), record.get_content());
  switch (kind) {
  case RecordKind::Library:
    return library_compiler.lower(
        arena, compiler_errors, source, module, *type);

  case RecordKind::Render:
  case RecordKind::Shader:
    return shader_compiler.lower(
        arena, compiler_errors, source, module, *type);

  case RecordKind::Package:
  case RecordKind::Other:
    return True;
  }
}

auto Terminal::Plan::build_module_name(View::Bytes source_path) -> View::Bytes {
  Count start = 0;
  for (Count i = 0; i < source_path.get_size(); i++) {
    if (source_path[i] == '/' || source_path[i] == '\\') {
      start = i + 1;
    }
  }

  Count end = source_path.get_size();
  for (Count i = start; i < source_path.get_size(); i++) {
    if (source_path[i] == '.') {
      end = i;
    }
  }

  return source_path.slice(start, end - start);
}

auto Terminal::Plan::append_terminal(Record& record) -> void {
  append_field("source"_view, record.get_source_path());
  append_field("import"_view, record.get_import_name());
  append_field("isa"_view, record.get_boot().get_isa());
  if (record.get_type() != nullptr) {
    append_type_facts(*record.get_type(), record.get_type()->get_name());
  }
}

auto Terminal::Plan::append_field(View::Bytes name, View::Bytes value) -> void {
  puffer_buffer.concat(name);
  puffer_buffer.append(':');
  puffer_buffer.append(' ');
  puffer_buffer.concat(value);
  puffer_buffer.append('\n');
}

auto Terminal::Plan::append_type_facts(
    const Ttx::Type& type,
    View::Bytes path) -> void {
  append_field("type"_view, path);

  View::Vector<Ttx::Type::Member> members = type.get_members();
  for (Count i = 0; i < members.get_size(); i++) {
    append_member_fact(path, members[i]);
  }

  View::Vector<Ttx::Type::Function> functions = type.get_functions();
  for (Count i = 0; i < functions.get_size(); i++) {
    append_function_fact(path, functions[i]);
  }

  View::Vector<const Ttx::Type*> types = type.get_types();
  for (Count i = 0; i < types.get_size(); i++) {
    if (types[i] == nullptr) {
      continue;
    }

    Dynamic::Bytes nested_path;
    nested_path.concat(path);
    nested_path.concat("::"_view);
    nested_path.concat(types[i]->get_name());
    append_type_facts(*types[i], nested_path);
  }
}

auto Terminal::Plan::append_member_fact(
    View::Bytes owner,
    const Ttx::Type::Member& member) -> void {
  puffer_buffer.concat("member: "_view);
  puffer_buffer.concat(owner);
  puffer_buffer.concat("."_view);
  puffer_buffer.concat(
      member.get_name().is_empty() ? "_"_view : member.get_name());
  puffer_buffer.concat(" "_view);
  puffer_buffer.concat(type_name(member.get_type()));
  puffer_buffer.append('\n');
}

auto Terminal::Plan::append_function_fact(
    View::Bytes owner,
    const Ttx::Type::Function& function) -> void {
  puffer_buffer.concat("function: "_view);
  puffer_buffer.concat(owner);
  puffer_buffer.concat("->"_view);
  puffer_buffer.concat(function.get_name());
  puffer_buffer.concat(" params="_view);
  append_decimal(puffer_buffer, function.get_parameters().get_size());
  puffer_buffer.concat(" result="_view);
  append_decimal(puffer_buffer, function.get_result().get_size());
  puffer_buffer.concat(" blocks="_view);
  append_decimal(puffer_buffer, function.get_blocks().get_size());
  puffer_buffer.append('\n');
}

auto Terminal::Plan::classify(const Record& record) -> RecordKind {
  using RecordKinds = Table<RecordKind, record_kinds>;
  return RecordKinds::find_or_default(
      record.get_boot().get_isa(), RecordKind::Other);
}

auto Terminal::Plan::type_name(const Ttx::Type* type) -> View::Bytes {
  return type == nullptr ? "<unresolved>"_view : type->get_name();
}

auto Terminal::Plan::append_decimal(Dynamic::Bytes& output, Count value)
    -> void {
  Bits_8 digits[32];
  Count digit_count = 0;
  do {
    digits[digit_count++] = Bits_8('0' + value % 10);
    value /= 10;
  } while (value != 0);

  for (Count i = digit_count; i > 0; i--) {
    output.append(digits[i - 1]);
  }
}
