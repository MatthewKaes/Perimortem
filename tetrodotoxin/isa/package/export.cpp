// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/package/export.hpp"

#include "tetrodotoxin/isa/base/expression/type.hpp"
#include "tetrodotoxin/isa/package/group.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Package::Export::evaluate(
    Cursor& cursor,
    Base::Context& context,
    Ttx::Documentation documentation) -> Package::Export {
  Base::Declaration definition = Base::Declaration::evaluate(
      cursor, documentation, {{Code::Type::Expose}}, {{Code::Type::Type}},
      {{Code::Type::Alias, Code::Type::Type, Code::Type::Addressable}});
  if (definition.is_empty()) {
    return Package::Export();
  }

  if (definition.get_kind() == "group"_view) {
    Perimortem::Memory::Managed::Vector<Package::Export> exports(
        context.get_arena());
    Bool evaluated = Package::Group::evaluate(cursor, context, exports);
    if (!evaluated) {
      return Package::Export();
    }

    return Package::Export(definition, exports.get_view());
  }

  if (definition.get_kind() != "alias"_view) {
    cursor.token_error(
        "Expected package definition kind `alias` or `group`."_view);
    return Package::Export();
  }

  Bool has_assignment = cursor.require(
      Code::Type::Assign, "Expected `=` before package export target."_view);
  if (!has_assignment) {
    return Package::Export();
  }

  const Count error_count = cursor.get_errors().get_size();
  const Ttx::Type* target = Base::Expression::Type::evaluate(cursor, context);
  if (cursor.get_errors().get_size() != error_count) {
    return Package::Export();
  }

  if (target == nullptr) {
    cursor.token_error("Package export target could not be resolved."_view);
    return Package::Export();
  }

  Bool has_statement_end = cursor.require(
      Code::Type::EndStatement, "Expected `;` after package export."_view);
  if (!has_statement_end) {
    return Package::Export();
  }

  return Package::Export(definition, target);
}
