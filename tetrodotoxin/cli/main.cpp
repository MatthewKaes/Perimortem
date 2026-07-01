// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/math.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "perimortem/system/file.hpp"

#include "ttx/lexical/tokenizer.hpp"
#include "ttx/parse/source.hpp"

static auto extract_source_line(
    Perimortem::Core::View::Bytes source,
    const Ttx::Lexical::Token& token) -> Perimortem::Core::View::Bytes {
  if (source.is_empty() || token.get_text().is_empty()) {
    return Perimortem::Core::View::Bytes();
  }

  const Bits_8* base = source.get_data();
  const Bits_8* start = token.get_text().get_data();
  if (start < base || start >= base + source.get_size()) {
    return Perimortem::Core::View::Bytes();
  }

  Count offset = Count(start - base);
  Count line_start = offset;
  while (line_start > 0 && source[line_start - 1] != '\n') {
    line_start--;
  }

  Count line_end = offset;
  while (line_end < source.get_size() && source[line_end] != '\n') {
    line_end++;
  }

  return source.slice(line_start, line_end - line_start);
}

static auto write_gutter(
    Perimortem::Core::Writer::Textual& writer,
    Signed_32 line) -> void {
  if (line < 0) {
    writer << "     | "_view;
    return;
  }

  if (line < 10) {
    writer << "   "_view;
  } else if (line < 100) {
    writer << "  "_view;
  } else if (line < 1000) {
    writer << " "_view;
  }

  writer << line << " | "_view;
}

static auto render_error(
    Perimortem::Core::View::Bytes path,
    Perimortem::Core::View::Bytes source,
    const Ttx::Parse::Error& error) -> Perimortem::Memory::Dynamic::Bytes {
  Perimortem::Core::Static::Bytes<4096> storage;
  Perimortem::Core::Writer::Textual writer(storage.get_access());
  const Ttx::Lexical::Token& token = error.get_token();

  writer << "error: "_view;
  if (!path.is_empty()) {
    writer << path << ":"_view << token.get_line() << ":"_view
           << token.get_column() << ": "_view;
  }
  writer << error.get_message() << "\n"_view;

  Perimortem::Core::View::Bytes line = extract_source_line(source, token);
  if (!line.is_empty()) {
    write_gutter(writer, token.get_line());
    writer << line << "\n"_view;

    write_gutter(writer, -1);
    for (Bits_32 column = 1; column < token.get_column(); column++) {
      writer << " "_view;
    }

    Count token_width =
        Perimortem::Core::Math::max(token.get_text().get_size(), Count(1));
    writer << "^"_view;
    for (Count index = 1; index < token_width; index++) {
      writer << "~"_view;
    }
    writer << "\n"_view;
  }

  if (!error.get_hint().is_empty()) {
    writer << "note: "_view << error.get_hint() << "\n"_view;
  }

  writer << "\n"_view;
  return Perimortem::Memory::Dynamic::Bytes(
      storage.slice(0, writer.get_location()));
}

static auto write_file(
    Perimortem::Core::View::Bytes path,
    Perimortem::Core::View::Bytes data) -> Bool {
  Perimortem::System::File file;
  file.update_contents(data);
  return file.write(path);
}

static auto parse_source(Perimortem::Core::View::Bytes source_path) -> Bool {
  Perimortem::System::File source_file;
  if (!source_file.read(source_path)) {
    Perimortem::Core::Diagnostics::Log::error(
        "Tetrodotoxin failed to read source."_view);
    return False;
  }

  Perimortem::Memory::Allocator::Arena arena;
  Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(source_file.get_view(), False);

  Ttx::Parse::Source source = Ttx::Parse::Source::parse(tokenizer, source_path);
  if (!source.is_valid()) {
    Perimortem::Core::View::Vector<Ttx::Parse::Error> errors =
        source.get_errors();
    for (Count error_index = 0; error_index < errors.get_size();
         error_index++) {
      Perimortem::Core::Diagnostics::Log::error(
          render_error(
              source.get_source_path(), source.get_source(),
              errors[error_index])
              .get_view());
    }
    return False;
  }

  return True;
}

Signed_32 main(Signed_32 argc, Signed_8** argv) {
  Perimortem::Core::View::Bytes output_path;
  Perimortem::Core::View::Bytes header_path;
  Perimortem::Memory::Dynamic::Vector<Perimortem::Core::View::Bytes>
      source_paths;
  Perimortem::Memory::Dynamic::Vector<Perimortem::Core::View::Bytes>
      dependency_paths;

  for (Count argument_index = 1; argument_index < argc; argument_index++) {
    const Perimortem::Core::View::Bytes argument =
        Perimortem::Core::NullTerminated::to_view(argv[argument_index]);
    if (argument == "--output"_view && argument_index + 1 < argc) {
      output_path =
          Perimortem::Core::NullTerminated::to_view(argv[++argument_index]);
      continue;
    }

    if (argument == "--header"_view && argument_index + 1 < argc) {
      header_path =
          Perimortem::Core::NullTerminated::to_view(argv[++argument_index]);
      continue;
    }

    if (argument == "--dep"_view && argument_index + 1 < argc) {
      dependency_paths.insert(
          Perimortem::Core::NullTerminated::to_view(argv[++argument_index]));
      continue;
    }

    source_paths.insert(argument);
  }

  if (output_path.is_empty()) {
    return 1;
  }

  for (Count dependency_index = 0;
       dependency_index < dependency_paths.get_size(); dependency_index++) {
    if (!parse_source(dependency_paths[dependency_index])) {
      return 1;
    }
  }

  for (Count source_index = 0; source_index < source_paths.get_size();
       source_index++) {
    if (!parse_source(source_paths[source_index])) {
      return 1;
    }
  }

  if (!write_file(output_path, "!<arch>\n"_view)) {
    return 1;
  }

  if (!header_path.is_empty() &&
      !write_file(header_path, "// Generated by Tetrodotoxin\n"_view)) {
    return 1;
  }

  return 0;
}
