// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/foreign.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Source is one Library Monograph's synthetic root Composite. It owns synthetic
// Definition policy, source grammar, Static publication, and exactly one
// Foreign context. Repeated Foreign blocks merge into that identity while
// Composite keeps the shared inventories, lookup, and lifecycle.
class Source : public Composite {
 private:
  Source(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : Composite(domain, definition),
        foreign(domain, *this),
        import_routes(domain),
        imports(domain) {}

 protected:
  auto retain_binding(
      Ttx::Concept::Abstract& binding,
      Tetrodotoxin::Language::Definition& definition,
      Category category,
      Ttx::Lexical::Cursor& cursor) -> Bool override;

 public:
  TTX_CONTRACT(Source, Composite);

  static auto create_synthetic(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& host,
      const Ttx::Lexical::Anchor& source_anchor) -> Source&;

  auto restore(
      Archive::Reader& contents,
      Tetrodotoxin::Language::Persistence::Profile profile) -> Bool;

  Source(const Source&) = delete;
  Source(Source&&) = delete;
  auto operator=(const Source&) -> Source& = delete;
  auto operator=(Source&&) -> Source& = delete;

  auto retain_authored_import(Import import) -> Bool;

  auto link(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Concept::Abstract& interpretation_context) -> Bool;

  auto link_restored(Ttx::Concept::Abstract& interpretation_context) -> Bool;

  auto finalize_restored() -> Bool override;

  auto finalize(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

  constexpr auto get_foreign() -> Foreign& { return foreign; }

  constexpr auto get_foreign() const -> const Foreign& { return foreign; }

  auto bind_static(Ttx::Concept::Abstract& binding, Category category) -> Bool;

  auto can_bind_static(const Ttx::Concept::Abstract& binding, Category category)
      const -> Bool;

  auto resolve_imports(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract&;

  constexpr auto resolve() const -> const Ttx::Concept::Abstract& override {
    return *this;
  }

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_type_access(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route,
      Model::Type::Access access) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_type_call(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route,
      Model::Type::Access access) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_local(
      Perimortem::Core::View::Bytes route,
      Tetrodotoxin::Language::Visibility visibility =
          Tetrodotoxin::Language::Visibility::Public) const
      -> const Ttx::Concept::Abstract&;

  constexpr auto get_imports() const { return import_routes.get_view(); }

 private:
  auto retain_import(const Ttx::Concept::Abstract& context) -> Bool;
  auto link_imports(
      Ttx::Lexical::Cursor& cursor,
      Ttx::Concept::Abstract& interpretation_context) -> Bool;
  auto link_types(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto link_fields(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto link_initializers(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto link_callable_signatures(Ttx::Lexical::Cursor& cursor) -> Bool override;
  auto link_callable_bodies(Ttx::Lexical::Cursor& cursor) -> Bool override;

  Foreign foreign;
  Perimortem::Memory::Managed::Vector<Import> import_routes;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<const Ttx::Concept::Abstract>>
      imports;
  Bool imports_linked = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
