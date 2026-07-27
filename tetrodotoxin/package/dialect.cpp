// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/dialect.hpp"

#include "tetrodotoxin/package/language/dependency.hpp"
#include "tetrodotoxin/package/language/monograph.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Language;
using namespace Tetrodotoxin::Package;

// resolve Math : Perimortem.Math = "1.0";
// resolve Graphics : Perimortem.Graphics = "1.0";
// resolve Runtime : Perimortem.Runtime = "1.0";

// source "scenes/splash.ttx";
// source "scenes/title.ttx";
// source "main.ttx";

auto Tetrodotoxin::Package::Dialect::interpret(
    Allocator::Arena& domain,
    Ttx::Lexical::Cursor& cursor,
    const Ttx::Concept::Documentation& doc,
    Abstract& registry) -> Option<Dialect::Monograph&> {
  // Delay early exit so we can gather multiple errors in a single pass.
  Bool bad_generation = false;

  Managed::Vector<Language::Dependency> dependencies(domain);
  while (cursor.get_code() == Code::Type::Addressable &&
         cursor.get_text() == "resolve"_view) {
    auto dependency = Language::Dependency::parse(cursor);
    if (!dependency) {
      bad_generation = true;
    }

    if (!bad_generation) {
      dependencies.insert(*dependency);
    }
  }

  // The rest of the file is just an ordered list of sources to include.
  Managed::Vector<View::Bytes> sources(domain);
  while (cursor.current().get_code() != Code::Type::Terminal) {
    if (cursor.get_code() != Code::Type::Addressable ||
        cursor.get_text() != "source"_view) {
      cursor.create_token_error(
          "Expected `source` followed by a source path."_view,
          "After the `resolve` block package expects only a list of source "
          "includes."_view);
    }

    // Expect a string and end statement.
    auto path = cursor.require(Code::Type::String);
    if (!path || !cursor.require(Code::Type::EndStatement)) {
      bad_generation = true;
      cursor.recover_to_statement();
    }

    if (!bad_generation) {
      sources.insert(path.caculate_text(cursor.get_source_text()));
    }
  }

  // Now that we've parsed as much as we can we can now bail.

  if (!bad_generation) {
    return {};
  }

  return domain.construct<Language::Monograph>(
      domain, doc, *this, dependencies, sources);
}
