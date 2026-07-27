// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/package/language/source.hpp"

#include "perimortem/utility/range.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::System;
using namespace Perimortem::Utility;
using namespace Ttx::Lexical;
using namespace Tetrodotoxin::Package;

// source [Type] from "Version";
auto Language::Source::parse(Cursor& cursor) -> Option<Source> {
  if (cursor.bail(
          "source"_view,
          "Expected `source` statement after `resolve` block."_view)) {
    return {};
  }

  Token binding_name = cursor.require(
      Code::Type::Type,
      "Source requires a Type name to bind the code to."_view);
  if (!binding_name) {
    return {};
  }

  if (cursor.bail(
          "from"_view,
          "Source statements require `from` followed by a source path."_view)) {
    return {};
  }

  // Expect a string and end statement.
  auto path = cursor.require(Code::Type::String);
  if (!path || !cursor.require(Code::Type::EndStatement)) {
    cursor.recover_to_statement();
    return {};
  }

  return Source(cursor.caculate_text(binding_name), cursor.caculate_text(path));
}
