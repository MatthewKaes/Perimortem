// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/interpreter/dialects/package.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/interpreter/definition.hpp"
#include "tetrodotoxin/interpreter/definitions.hpp"
#include "tetrodotoxin/interpreter/dialects/alias.hpp"
#include "tetrodotoxin/interpreter/dialects/group.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/sources.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

auto Tetrodotoxin::Interpreter::Dialects::Package::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Model::Dialect::implements(requested);
}

auto Tetrodotoxin::Interpreter::Dialects::Package::evaluate(
    Cursor& cursor,
    const Abstract& context) const -> const Abstract& {
  using PackageDefinitions = Definitions<
      Definition<Dialects::Alias, Code::Type::Public>,
      Definition<Dialects::Group, Code::Type::Public>>;

  if (!context.is<Model::Source>()) {
    cursor.create_error("Package requires a Source evaluation context."_view);
    return Invalid::get_invalid();
  }

  // Package owns one closed public Namespace. The Source remains an outer
  // visible context so definitions can reach Environment bindings without
  // copying them into the Package surface.
  const Model::Source& source = context.assume<Model::Source>();
  Model::Namespace& exports = cursor.get_arena().construct<Model::Namespace>(
      cursor.get_arena(), ""_view);
  const Static::Vector<Reference<Abstract>, 1> visible = {{source}};
  while (!cursor.matches(Code::Type::Terminal)) {
    const Abstract& exported =
        PackageDefinitions::evaluate(cursor, exports, visible);
    if (exported.is<Invalid>()) {
      return Invalid::get_invalid();
    }
  }

  // Source publication happens in Source::evaluate after this complete Package
  // has been produced. The internal Namespace is never substituted as a root.
  const Static::Vector<Reference<Model::Source>, 1> members = {{source}};
  return Model::Packages::Sources::construct(
      cursor.get_arena(), members, exports);
}
