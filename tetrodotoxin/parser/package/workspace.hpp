// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/union.hpp"

#include "perimortem/memory/dynamic/bytes.hpp"

namespace Tetrodotoxin::Parser::Package {

// Workspace pins one opened package-root directory for a complete package
// construction transaction. Every selected object is resolved beneath that
// root, classified, and read through the same opened file descriptor.
class Workspace {
 private:
  class Construction {};

 public:
  enum class Failure {
    Missing,
    NonFile,
    Unreadable,
    OutsideRoot,
  };

  // Open is the closed root-acquisition result. Success owns the Workspace for
  // the result's lifetime; failure exposes only its exact host classification.
  // Its Union reference is owning: Open releases the selected Workspace and
  // resets a moved-from result to failure so there is always exactly one owner.
  class Open {
   public:
    Open(Construction, Workspace& workspace) : result(workspace) {}
    Open(Construction, Failure failure) : result(failure) {}
    Open(const Open&) = delete;
    Open(Open&& source);
    auto operator=(const Open&) -> Open& = delete;
    auto operator=(Open&& source) -> Open&;
    ~Open();

    template <typename SuccessVisitor, typename FailureVisitor>
    auto visit(SuccessVisitor success_visitor, FailureVisitor failure_visitor)
        const -> decltype(auto) {
      return result.visit(
          [&]() -> decltype(failure_visitor(Failure::Unreadable)) {
            __builtin_unreachable();
          },
          static_cast<SuccessVisitor&&>(success_visitor),
          static_cast<FailureVisitor&&>(failure_visitor));
    }

   private:
    auto release() -> void;

    Perimortem::Core::Static::Union<const Workspace&, Failure> result;
  };

  // Read owns either one complete byte snapshot or one failure. A zero-byte
  // snapshot is a selected success, and a failure never retains partial bytes.
  // Its Union reference is owning so the selected snapshot is also the sole
  // success state rather than backing storage paired with a second tag.
  class Read {
   public:
    Read(Construction, Perimortem::Memory::Dynamic::Bytes&& bytes);
    Read(Construction, Failure failure) : result(failure) {}
    Read(const Read&) = delete;
    Read(Read&& source);
    auto operator=(const Read&) -> Read& = delete;
    auto operator=(Read&& source) -> Read&;
    ~Read();

    template <typename SuccessVisitor, typename FailureVisitor>
    auto visit(SuccessVisitor success_visitor, FailureVisitor failure_visitor)
        -> decltype(auto) {
      return result.visit(
          [&]() -> decltype(failure_visitor(Failure::Unreadable)) {
            __builtin_unreachable();
          },
          static_cast<SuccessVisitor&&>(success_visitor),
          static_cast<FailureVisitor&&>(failure_visitor));
    }

   private:
    auto release() -> void;

    Perimortem::Core::Static::
        Union<Perimortem::Memory::Dynamic::Bytes&, Failure>
            result;
  };

  static auto open(Perimortem::Core::View::Bytes package_root) -> Open;

  Workspace(const Workspace&) = delete;
  Workspace(Workspace&&) = delete;
  auto operator=(const Workspace&) -> Workspace& = delete;
  auto operator=(Workspace&&) -> Workspace& = delete;
  ~Workspace();

  auto read(Perimortem::Core::View::Bytes normalized_route) const -> Read;

 private:
  explicit Workspace(int root) : root(root) {}

  int root;
};

}  // namespace Tetrodotoxin::Parser::Package
