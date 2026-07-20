// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/package_name.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Package::PackageName::evaluate(
    Cursor& cursor,
    Ttx::Documentation documentation) -> Package::PackageName {
  switch (cursor.current().get_code().get_type()) {
  case Code::Type::Type: {
    const Token* first_segment = cursor.require(
        Code::Type::Type,
        "Expected package name to start with a Type name."_view);
    if (first_segment == nullptr) {
      return Package::PackageName();
    }

    const Token* last_segment = first_segment;
    while (cursor.matches(Code::Type::AddressOp)) {
      cursor.consume();
      last_segment = cursor.require(
          Code::Type::Type,
          "Package name segments should all be Type names."_view);
      if (last_segment == nullptr) {
        return Package::PackageName();
      }
    }

    View::Bytes start = first_segment->get_text();
    View::Bytes end = last_segment->get_text();
    return Package::PackageName(
        View::Bytes(
            start.get_data(),
            end.get_data() - start.get_data() + end.get_size()),
        documentation);
  }

  default:
    cursor.token_error("Expected package name."_view);
    return Package::PackageName();
  }
}
