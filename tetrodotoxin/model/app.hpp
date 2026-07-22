// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "tetrodotoxin/model/apps/binding.hpp"
#include "ttx/model/addressable.hpp"
#include "ttx/model/callable.hpp"
#include "ttx/model/types/managed.hpp"

namespace Tetrodotoxin::Model {

// App is the managed host-state Type and explicit lifecycle composition
// contract. Runtime follows these direct Callable, Render-root, and Shader
// binding edges. It never searches authored names for policy.
class App : public Ttx::Model::Types::Managed {
 public:
  using ContractOwner = App;
  static constexpr Perimortem::System::Uuid contract_id{
    0x87e6ed62b55c4eac,
    0xbf4b16ef5e09d836,
  };

  constexpr auto implements(Perimortem::System::Uuid requested) const
      -> Bool override {
    return requested == contract_id ||
           Ttx::Model::Types::Managed::implements(requested);
  }

  virtual constexpr auto get_start() const -> const Ttx::Model::Callable& = 0;
  virtual constexpr auto get_frame() const -> const Ttx::Model::Callable& = 0;
  virtual constexpr auto get_stop() const -> const Ttx::Model::Callable& = 0;
  virtual constexpr auto get_render_root_count() const -> Count = 0;
  virtual auto get_render_root(Count index) const
      -> const Ttx::Concept::Abstract& = 0;
  virtual constexpr auto get_binding_count() const -> Count = 0;
  virtual constexpr auto get_binding(Count index) const
      -> const Apps::Binding& = 0;
  virtual constexpr auto get_member_count() const -> Count = 0;
  virtual auto get_member(Count index) const
      -> const Ttx::Concept::Abstract& = 0;
};

}  // namespace Tetrodotoxin::Model
