// Perimortem Engine
// Copyright © Matt Kaes

#include "puffer/lsp/semantic_document.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/scene/dialect.hpp"

using namespace Perimortem::Core;
using namespace Puffer;
using namespace Tetrodotoxin;

Lsp::SemanticDocument::SemanticDocument(View::Bytes uri, View::Bytes source) {
  auto package = workspace.install_dialect<Package::Dialect>("Package"_view);
  auto library = workspace.install_dialect<Library::Dialect>("Library"_view);
  if (!package || !library) {
    return;
  }
  if (!workspace.install_dialect<Scene::Dialect>("Scene"_view, *library)) {
    return;
  }

  auto interpreted =
      workspace.interpret_source(errors, "puffer.document"_view, uri, source);
  if (!interpreted) {
    return;
  }
  associations = workspace.get_associations(*interpreted);
}

auto Lsp::SemanticDocument::find(Count byte_offset) const
    -> Option<const Ttx::Concept::Abstract&> {
  if (!associations) {
    return {};
  }

  return associations->find_at(byte_offset);
}
