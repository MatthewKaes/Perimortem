// Perimortem Engine
// Copyright © Matt Kaes

// Validation utility for regenerating syntax-formatting golden files.

#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/view/bytes.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/bytes.hpp"

#include "perimortem/system/file.hpp"

#include "tetrodotoxin/format/source.hpp"
#include "ttx/lexical/tokenizer.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/ttx.hpp"

static auto format_ttx_fixture(
    Perimortem::Core::View::Bytes source_path,
    Perimortem::Core::View::Bytes golden_path) -> Bool {
  Perimortem::System::File source_file;
  if (!source_file.read(source_path)) {
    return False;
  }

  Perimortem::Memory::Allocator::Arena arena;
  ::Ttx::Lexical::Tokenizer tokenizer(arena);
  tokenizer.parse(source_file.get_view(), false);

  Tetrodotoxin::Syntax::Context ctx(tokenizer, source_path);
  Tetrodotoxin::Syntax::Ttx package =
      Tetrodotoxin::Syntax::Ttx::parse(ctx);
  if (!package.is_valid() || ctx.get_error_count() != 0) {
    return False;
  }

  Perimortem::Memory::Dynamic::Bytes formatted_source =
      Tetrodotoxin::Format::format(package);

  Perimortem::System::File golden_file;
  golden_file.update_contents(formatted_source.get_view());
  return golden_file.write(golden_path);
}

Signed_32 main(Signed_32 argument_count, Signed_8** arguments) {
  if (argument_count != 3) {
    return 1;
  }

  Perimortem::Core::View::Bytes source_path =
      Perimortem::Core::NullTerminated::to_view(arguments[1]);
  Perimortem::Core::View::Bytes golden_path =
      Perimortem::Core::NullTerminated::to_view(arguments[2]);

  return format_ttx_fixture(source_path, golden_path) ? 0 : 1;
}
