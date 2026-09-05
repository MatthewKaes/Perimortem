// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include <memory>
#include <vector>

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/null_terminated.hpp"

#include "ttx/ffi/cpp/domain.hpp"
#include "ttx/reference/model/layouts/fluid.hpp"

namespace Puffer {

// Invocation is the one immutable host authority raised for a Puffer command.
// The root source receives it through Workspace instead of reading argv or the
// process environment, which leaves the selected Dialect responsible for the
// meaning of every argument after the source name.
class Invocation final : public Ttx::Concept::Abstract {
 public:
  Invocation(
      Perimortem::Core::View::Bytes working_directory,
      Perimortem::Core::View::Bytes sdk,
      const std::vector<Perimortem::Core::View::Bytes>& arguments);

  TTX_NAME("Puffer invocation"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

 private:
  class Input final : public Ttx::Model::Domain {
   public:
    Input(
        Perimortem::Core::View::Bytes name,
        Perimortem::Core::View::Bytes value);

    TTX_NAME(name);
    TTX_EMPTY_DOCUMENTATION();

    void bytes(ttx_abstract self, ttx_bytes_result result) const override;

   protected:
    auto negotiate(ttx_abstract requirement) const
        -> ttx_interface_relation override;

   private:
    struct Binding {
      ttx_bytes_ops operations;
    };

    static auto select(ttx_bytes self) -> const Input&;
    static auto TTX_CALL candidate(ttx_bytes self) -> ttx_abstract;
    static auto TTX_CALL size(ttx_bytes self) -> uint64_t;
    static void TTX_CALL visit(ttx_bytes self, ttx_bytes_sink sink);

    std::vector<uint8_t> name_storage;
    std::vector<uint8_t> value_storage;
    Perimortem::Core::View::Bytes name;
    Perimortem::Core::View::Bytes value;
    Binding binding;
  };

  class Inputs final : public Ttx::Model::Domain {
   public:
    explicit Inputs(const std::vector<const Ttx::Concept::Abstract*>& entries);

    TTX_NAME("Puffer arguments"_view);
    TTX_EMPTY_DOCUMENTATION();

    auto get_layout() const -> const Ttx::Concept::Layout& override;

   private:
    Ttx::Model::Layouts::Fluid layout;
  };

  static auto copy(Perimortem::Core::View::Bytes value) -> std::vector<uint8_t>;

  std::vector<std::unique_ptr<Input>> argument_values;
  std::vector<const Ttx::Concept::Abstract*> argument_entries;
  Input working_directory;
  Input sdk;
  std::unique_ptr<Inputs> arguments;
};

}  // namespace Puffer
