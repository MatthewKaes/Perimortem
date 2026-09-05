// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/system/version.hpp"

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/language/monograph.hpp"
#include "tetrodotoxin/library/language/monograph.hpp"
#include "tetrodotoxin/package/dialect.hpp"
#include "tetrodotoxin/package/resources.hpp"
#include "ttx/lexical/span.hpp"

namespace Tetrodotoxin::Package::Language {

// Package is a restricted Library source that gives one source graph a public
// name and version. Its common Import Types are ordinary source-local names;
// Workspace owns every imported Monograph and Package owns no parallel member
// or dependency inventory.
class Monograph : public Tetrodotoxin::Language::Monograph {
 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Ttx::Concept::Abstract& context,
      const Ttx::Concept::Abstract& library_language,
      Perimortem::Memory::Managed::Vector<RequiredDialect> requirements,
      Perimortem::Core::View::Vector<Tetrodotoxin::Package::Resource*>
          resources,
      Bool resources_sealed);

 public:

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Ttx::Concept::Abstract& context,
      const Ttx::Concept::Abstract& library_language,
      Perimortem::Memory::Managed::Vector<RequiredDialect> requirements)
      -> Monograph&;

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      Perimortem::Core::View::Bytes identity,
      Perimortem::System::Version version,
      Ttx::Concept::Abstract& context,
      const Ttx::Concept::Abstract& library_language,
      Perimortem::Core::View::Vector<Tetrodotoxin::Package::Resource*>
          resources = {}) -> Monograph&;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  auto resolve_lexical_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_root() const -> const Ttx::Concept::Abstract& override {
    return *this;
  }

  auto retain_import(
      const Tetrodotoxin::Language::Import::Description& description,
      Perimortem::Core::Option<Ttx::Lexical::Associations&> associations = {})
      -> Bool override;

  constexpr auto get_imports() const -> Perimortem::Core::View::Vector<
      Tetrodotoxin::Language::Import*> override {
    return library.get_imports();
  }

  auto get_layer(const Ttx::Concept::Abstract& requested) const
      -> Perimortem::Core::Option<
          const Tetrodotoxin::Language::Monograph&> override;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto link_restored() -> Bool override;
  auto finalize_restored() -> Bool override;

  constexpr auto edit_library() -> Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  constexpr auto get_library() const
      -> const Tetrodotoxin::Library::Language::Monograph& {
    return library;
  }

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  constexpr auto get_version() const -> Perimortem::System::Version {
    return version;
  }

  constexpr auto get_required_dialects() const
      -> Perimortem::Core::View::Vector<RequiredDialect> {
    return requirements;
  }

  auto get_resources() -> Tetrodotoxin::Package::Resources&;
  auto get_resources() const -> const Tetrodotoxin::Package::Resources&;

 private:
  mutable Tetrodotoxin::Package::Resources resources;
  Perimortem::Core::View::Bytes identity;
  Perimortem::System::Version version;
  Perimortem::Memory::Managed::Vector<RequiredDialect> requirements;
  Tetrodotoxin::Library::Language::Monograph& library;
};

}  // namespace Tetrodotoxin::Package::Language
