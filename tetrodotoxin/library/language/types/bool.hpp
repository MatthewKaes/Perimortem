// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/flag.hpp"
#include "ttx/bootstrap/model/documentations/comment.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Boolean is the standard one bit Flag Type with byte addressable storage.
class Boolean : public Model::Types::Flag {
 public:
  TTX_NAME("Bool"_view);

  TTX_DOCUMENTATION(documentation);

  auto get_validity(const Model::Pack& value) const
      -> Perimortem::Core::Option<Bool> override;
  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  constexpr auto get_propagated_type() const
      -> Perimortem::Core::Option<const Model::Type&> override {
    return *this;
  }

  constexpr auto get_width() const -> Count override { return 1; }
  constexpr auto get_size() const -> Count override { return sizeof(::Bool); }
  constexpr auto get_alignment() const -> Count override {
    return alignof(::Bool);
  }

 private:
  static constexpr Ttx::Model::Documentations::Comment documentation{
    "Bool is stored as a 1 byte logical value."_view,
  };
};

}  // namespace Tetrodotoxin::Library::Language::Types
