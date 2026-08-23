// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/types/structure.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Object is the managed nonnull reference specialization of Structure. It
// retains the mandatory authored Definition through Structure while Composite
// owns every member, lookup, Layout, and completion rule.
class Object : public Structure {
 private:
  Object(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Bool provides_initialization = True);

 public:
  TTX_CONTRACT(Object, Structure);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Object&>;

  static auto restore(
      Archive::Reader& reader,
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Concept::Abstract& host,
      Tetrodotoxin::Language::Persistence::Profile profile)
      -> Perimortem::Core::Option<Object&>;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto create_supplied(
      Ttx::Lexical::Cursor& cursor,
      Model::Pack& arguments,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto create_supplied_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& arguments,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope)
      const -> Perimortem::Core::Option<Model::Pack&> override;

  auto persist(Archive::Writer& writer) const -> Bool override;

 protected:
};

}  // namespace Tetrodotoxin::Library::Language::Types
