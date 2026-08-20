// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/vector.hpp"
#include "perimortem/core/option.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/dynamic/vector.hpp"

#include "puffer/lsp/document.hpp"
#include "tetrodotoxin/environment/toolchain.hpp"
#include "tetrodotoxin/environment/workspace.hpp"
#include "tetrodotoxin/package/snapshots.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/errors.hpp"

namespace Puffer::Lsp {

// Documents owns open files and their bounded Package sessions. Protocol
// updates replace complete text overlays, invalidate one shared Package graph,
// and keep semantic work lazy until diagnostics or hover need the snapshot.
class Documents {
 public:
  class Diagnostics {
   public:
    constexpr Diagnostics(
        const Ttx::Lexical::Errors& errors,
        Perimortem::Core::View::Bytes source_name)
        : errors(errors), source_name(source_name) {}

    constexpr auto get_errors() const -> const Ttx::Lexical::Errors& {
      return errors;
    }

    constexpr auto get_source_name() const -> Perimortem::Core::View::Bytes {
      return source_name;
    }

   private:
    const Ttx::Lexical::Errors& errors;
    Perimortem::Core::View::Bytes source_name;
  };

  Documents(Perimortem::Core::View::Bytes packages_root = {});

  auto upsert(
      Perimortem::Core::View::Bytes uri,
      Perimortem::Core::View::Bytes source) -> void;
  auto erase(Perimortem::Core::View::Bytes uri) -> void;
  auto get_text(Perimortem::Core::View::Bytes uri) const
      -> Perimortem::Memory::Dynamic::Bytes;
  auto find_semantic(
      Perimortem::Core::View::Bytes uri,
      Count line,
      Count utf_16_character)
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;
  auto get_diagnostics(Perimortem::Core::View::Bytes uri)
      -> Perimortem::Core::Option<Diagnostics>;
  auto invalidate(Perimortem::Core::View::Bytes uri) -> void;

 private:
  struct Session {
    Bool active = False;
    Perimortem::Memory::Dynamic::Bytes root;
    Perimortem::Core::Option<
        Perimortem::Memory::Dynamic::Record<Ttx::Lexical::Errors>>
        errors;
    Perimortem::Core::Option<Perimortem::Memory::Dynamic::Record<
        Tetrodotoxin::Environment::Workspace>>
        workspace;
    Perimortem::Memory::Dynamic::Vector<Perimortem::Memory::Dynamic::Bytes>
        dependencies;
  };

  auto find(Perimortem::Core::View::Bytes uri) const -> Count;
  auto find_session(Perimortem::Core::View::Bytes root) const -> Count;
  auto create_workspace(Document& document)
      -> Perimortem::Core::Option<Tetrodotoxin::Environment::Workspace&>;
  auto get_workspace(Document& document)
      -> Perimortem::Core::Option<Tetrodotoxin::Environment::Workspace&>;
  auto get_errors(Document& document)
      -> Perimortem::Core::Option<const Ttx::Lexical::Errors&>;
  auto invalidate_package(Perimortem::Core::View::Bytes root) -> void;
  auto select_session(Document& document) -> Perimortem::Core::Option<Session&>;

  Tetrodotoxin::Environment::Toolchain toolchain;
  Perimortem::Memory::Dynamic::Record<Tetrodotoxin::Package::Snapshots>
      snapshots;
  Perimortem::Core::Static::Vector<Document, 64> records;
  Perimortem::Core::Static::Vector<Session, 16> sessions;
  Perimortem::Memory::Dynamic::Bytes packages_root;
  Bool toolchain_ready = False;
};

}  // namespace Puffer::Lsp
