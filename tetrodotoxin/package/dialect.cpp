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
// resolve System : Perimortem.System = "1.0";

// source Splash from "scenes/splash.ttx";
// source Title from "scenes/title.ttx";
// source Main from "main.ttx";

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
    bad_generation = bad_generation || !dependency;
    if (!bad_generation) {
      dependencies.insert(*dependency);
    }
  }

  // The rest of the file is just an ordered list of sources to include.
  Managed::Vector<Language::Source> sources(domain);
  while (cursor.current().get_code() != Code::Type::Terminal) {
    auto source = Language::Source::parse(cursor);
    bad_generation = bad_generation || !source;
    if (!bad_generation) {
      sources.insert(*source);
    }
  }

  // Now that we've parsed as much as we can we can now bail.
  if (sources.is_empty()) {
    cursor.create_error(
        "Package doesn't contain any source files. At least one `source` is "
        "required."_view);
    bad_generation = true;
  }

  // Now that we've parsed as much as we can we can now bail.
  if (!bad_generation) {
    return {};
  }

  return domain.construct<Language::Monograph>(
      domain, doc, *this, dependencies, sources);
}
