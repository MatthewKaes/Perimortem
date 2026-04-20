// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/bibliotheca.hpp"

namespace Perimortem::Memory::Managed {

// Used to create a global that uses any memory from the Biblotheca and needs to
// be guaranteed destruction after the Biblotheca.
//
// Access to Singleton's is not speedy and they should be used sparingly for
// bulky thread level objects that need dynamic memory management.
template <typename T>
class Singleton {
 public:
  static auto instance() -> T& {
    // Class that can actually create the object.
    struct LockedAbstract final : T {
      void AbstractInjectionLock() const noexcept override {}
    };

    // Class that manages the lifetime of the thread object.
    struct LifetimeManager {
      LifetimeManager(LockedAbstract* singleton) : singleton(singleton) {}
      ~LifetimeManager() { singleton->~T(); }

      LockedAbstract* singleton;
    };

    // Create a thread_local container while forcing the Bibliotheca to be
    // guaranteed instantiated before this object.
    thread_local static LifetimeManager object(
        new (Allocator::Bibliotheca::preface_to_corpus(
            Allocator::Bibliotheca::check_out(sizeof(LockedAbstract))))
            LockedAbstract());

    return *object.singleton;
  }

 protected:
  Singleton() = default;
  Singleton(const Singleton&) = delete;
  Singleton(Singleton&&) = delete;
  Singleton& operator=(const Singleton&) = delete;
  Singleton& operator=(Singleton&&) = delete;
  virtual ~Singleton() = default;

 private:
  // Force the top instance to be virtual.
  virtual void AbstractInjectionLock() const noexcept = 0;
};

}  // namespace Perimortem::Memory::Managed