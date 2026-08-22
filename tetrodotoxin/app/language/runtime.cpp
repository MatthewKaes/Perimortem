// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/language/runtime.hpp"

#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::App;

static auto require_text(
    Cursor& cursor,
    Code::Type code,
    View::Bytes text,
    View::Bytes message) -> Token {
  if (!cursor.matches(code) || cursor.get_text() != text) {
    cursor.create_token_error(cursor.current(), message);
    return {};
  }

  return cursor.consume();
}

auto Language::Runtime::parse(
    Cursor& cursor,
    const Documentation& documentation) -> Option<Runtime&> {
  Token opening = require_text(
      cursor, Code::Type::Addressable, "runtime"_view,
      "App runtime declaration requires `runtime`."_view);
  BAIL_IF(!opening);
  BAIL_IF(!cursor.require(
      Code::Type::Assign,
      "App runtime declaration requires `=` before its profile."_view));
  BAIL_IF(!require_text(
      cursor, Code::Type::Type, "Terminal"_view,
      "This App slice accepts only the Terminal runtime profile."_view));
  BAIL_IF(!cursor.require(
      Code::Type::ScopeStart,
      "Terminal runtime profile requires an empty `{}` body."_view));
  Token closing = cursor.require(
      Code::Type::ScopeEnd,
      "Terminal runtime profile does not accept settings."_view);
  BAIL_IF(!closing);

  Runtime& runtime = cursor.get_arena().construct_from<Runtime>([&]() {
    return Runtime(
        documentation, Anchor::create(opening, Span(opening, closing)));
  });
  cursor.get_associations().create(runtime.get_anchor(), runtime);
  return runtime;
}

auto Language::Runtime::create_synthetic(
    Perimortem::Memory::Allocator::Arena& arena,
    const Documentation& documentation) -> Runtime& {
  return arena.construct_from<Runtime>(
      [&]() { return Runtime(documentation, Anchor::create({})); });
}

auto Language::Runtime::resolve_context(View::Bytes) const -> const Abstract& {
  return Invalid::get_invalid();
}
