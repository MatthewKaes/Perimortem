// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/dialect.hpp"

#include "tetrodotoxin/app/archive/reader.hpp"
#include "tetrodotoxin/app/archive/writer.hpp"
#include "tetrodotoxin/app/language/monograph.hpp"
#include "tetrodotoxin/app/language/program.hpp"
#include "tetrodotoxin/app/language/runtime.hpp"
#include "tetrodotoxin/language/parser/comment.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin;

auto App::Dialect::interpret(
    Cursor& cursor,
    const Documentation& documentation,
    const Anchor&,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  Option<App::Language::Runtime&> runtime;
  Option<App::Language::Program&> program;
  Bool failed = False;

  while (!cursor.matches(Code::Type::Terminal)) {
    const Documentation& declaration_documentation =
        Tetrodotoxin::Language::Parser::Comment::parse(cursor);
    View::Bytes declaration = cursor.get_text();
    if (cursor.matches(Code::Type::Addressable) &&
        declaration == "runtime"_view) {
      if (runtime) {
        cursor.create_token_error(
            cursor.current(),
            "App accepts exactly one runtime declaration."_view);
        cursor.recover_to_statement();
        failed = True;
        continue;
      }

      auto parsed =
          App::Language::Runtime::parse(cursor, declaration_documentation);
      if (!parsed) {
        cursor.recover_to_statement();
        failed = True;
        continue;
      }
      runtime = *parsed;
      continue;
    }

    if (cursor.matches(Code::Type::Addressable) &&
        declaration == "lifecycle"_view) {
      if (program) {
        cursor.create_token_error(
            cursor.current(),
            "App accepts exactly one lifecycle declaration."_view);
        cursor.recover_to_statement();
        failed = True;
        continue;
      }

      auto parsed =
          App::Language::Program::parse(cursor, declaration_documentation);
      if (!parsed) {
        cursor.recover_to_statement();
        failed = True;
        continue;
      }
      program = *parsed;
      continue;
    }

    cursor.create_token_error(
        cursor.current(),
        "App accepts only `runtime` and `lifecycle` declarations."_view);
    cursor.recover_to_statement();
    failed = True;
  }

  if (!runtime) {
    cursor.create_error("App requires one Terminal runtime declaration."_view);
    failed = True;
  }
  if (!program) {
    cursor.create_error("App requires one Program lifecycle declaration."_view);
    failed = True;
  }
  if (failed) {
    return {};
  }

  App::Language::Monograph& monograph = App::Language::Monograph::create(
      cursor.get_arena(), *this, documentation, context, *runtime, *program);
  return monograph;
}

auto App::Dialect::encode(
    const Abstract& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile) const
    -> Option<Perimortem::Memory::Dynamic::Bytes> {
  auto app = monograph.select<App::Language::Monograph>();
  return app ? App::Archive::Writer().write(*app, profile)
             : Option<Perimortem::Memory::Dynamic::Bytes>();
}

auto App::Dialect::restore(
    Perimortem::Memory::Allocator::Arena& arena,
    View::Bytes payload,
    Tetrodotoxin::Language::Persistence::Profile profile,
    const Documentation& documentation,
    Abstract& context) -> Option<Tetrodotoxin::Language::Monograph&> {
  auto restored = App::Archive::Reader().read(
      arena, payload, profile, *this, documentation, context);
  return restored ? Option<Tetrodotoxin::Language::Monograph&>(*restored)
                  : Option<Tetrodotoxin::Language::Monograph&>();
}
