// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/language/visibility.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/type.hpp"

namespace Tetrodotoxin::Render::Language {

// Structure is one authored Render contract context. Its nested declarations
// remain separate semantic identities, while its instance Layout contains only
// ordinary GPU values that participate in value flow.
class Structure : public Ttx::Model::Type {
 public:
  TTX_CONTRACT(Structure, Ttx::Model::Type);

  static auto create(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Structure&;

  auto retain_addressable(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_callable(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_type(
      Ttx::Concept::Abstract& declaration,
      Tetrodotoxin::Language::Visibility visibility) -> Bool;

  auto retain_instance(Ttx::Model::Addressable& value) -> void;

  auto link(Ttx::Lexical::Cursor& cursor) -> Bool;

  auto resolve() const -> const Ttx::Concept::Abstract& override;

  auto resolve_context(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_local_context(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract&;

  auto resolve_access(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  auto resolve_call(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;

  auto get_layout() const -> const Ttx::Concept::Layout& override;

  TTX_NAME(definition.get_name());
  TTX_DOCUMENTATION(definition.get_documentation());

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_addressables() const { return addressables.get_view(); }

  constexpr auto get_callables() const { return callables.get_view(); }

  constexpr auto get_types() const { return types.get_view(); }

 private:
  class InstanceLayout : public Ttx::Concept::Layout {
   public:
    constexpr InstanceLayout(const Structure& owner) : owner(owner) {}

    auto get_size() const -> Count override;
    auto get_abstract(Count index) const
        -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override;
    auto get_name(Count index) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;
    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override;
    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override;
    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const
        -> Perimortem::Utility::Result<
            const Ttx::Concept::Abstract&,
            Ttx::Concept::Layout::Errors> override;

   private:
    const Structure& owner;
  };

  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : definition(definition),
        addressables(domain),
        published_addressables(domain),
        callables(domain),
        published_callables(domain),
        types(domain),
        published_types(domain),
        instances(domain),
        layout(*this) {}

  Tetrodotoxin::Language::Definition& definition;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      addressables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      published_addressables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      callables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      published_callables;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      types;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Concept::Abstract>>
      published_types;
  Perimortem::Memory::Managed::Vector<
      Ttx::Concept::Reference<Ttx::Model::Addressable>>
      instances;
  InstanceLayout layout;
  Bool linked = False;
};

}  // namespace Tetrodotoxin::Render::Language
