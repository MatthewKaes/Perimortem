// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/model/dialects/package.hpp"

#include "perimortem/core/static/vector.hpp"

#include "tetrodotoxin/model/definition.hpp"
#include "tetrodotoxin/model/definitions.hpp"
#include "tetrodotoxin/model/dialects/alias.hpp"
#include "tetrodotoxin/model/dialects/group.hpp"
#include "tetrodotoxin/model/namespace.hpp"
#include "tetrodotoxin/model/packages/sources.hpp"
#include "tetrodotoxin/model/source.hpp"
#include "ttx/concept/invalid.hpp"

using namespace Perimortem::Core;
using namespace Ttx::Concept;
using namespace Ttx::Lexical;

auto Tetrodotoxin::Model::Dialects::Package::implements(
    Perimortem::System::Uuid requested) const -> Bool {
  return requested == contract_id || Dialect::implements(requested);
}

auto Tetrodotoxin::Model::Dialects::Package::evaluate(
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
  // visible context so definitions can reach imported Alias bindings without
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
  return cursor.get_arena().construct<Model::Packages::Sources>(
      cursor.get_arena(), source, exports);
}
