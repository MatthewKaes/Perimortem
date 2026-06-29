// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"
#include "perimortem/core/writer/textual.hpp"

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/format/syntax.hpp"
#include "tetrodotoxin/syntax/ast/comment.hpp"
#include "tetrodotoxin/syntax/ast/import.hpp"
#include "tetrodotoxin/syntax/attribute/validation.hpp"
#include "tetrodotoxin/syntax/context.hpp"
#include "tetrodotoxin/syntax/dialect/dialect.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Syntax;
using namespace Tetrodotoxin::Lexical;

auto Tetrodotoxin::Format::format(Formatter& ctx, const Package& package)
    -> void {
  // If the package is missing a documentation comment then add one.
  if (!package.get_documentation().is_empty()) {
    Format::format(ctx, package.get_documentation());
  } else {
    ctx << "//\n// Place holder TTX documentation\n//\n"_view;
  }

  // Output the actual package definition.
  ctx << Class::Type::Package << " "_view << Class::Type::Define << " "_view;
  ctx << package.get_kind().get_source_text();
  ctx << Class::Type::EndStatement;
  ctx.emit_newline();

  // Format the import block
  View::Vector<Ast::Import> imports = package.get_imports();
  if (!imports.is_empty()) {
    ctx.ensure_blank();

    Count max_import_name = 0;

    for (Count i = 0; i < imports.get_size(); i++) {
      Count n = imports[i].get_local_name().get_size();

      if (n > max_import_name) {
        max_import_name = n;
      }
    }

    for (Count i = 0; i < imports.get_size(); i++) {
      Format::format(ctx, imports[i], max_import_name);
    }
  }

  // Format the member block
  if (!package.get_members().is_empty() ||
      !package.get_functions().is_empty() || !package.get_types().is_empty()) {
    ctx.ensure_blank();
    Format::format(ctx, package.get_body(), BodyOrder::Package);
  }
}
