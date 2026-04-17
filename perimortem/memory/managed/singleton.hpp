// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

namespace Perimortem::Memory::Managed {

// Used to ensure a class only has a single instance per thread.
template <typename T>
class Singleton {
 public:
  static auto instance() -> T& {
    // Class that can actually create the object.
    struct LockedAbstract final : T {
      void AbstractInjectionLock() const noexcept override {}
    };
    thread_local static LockAbstract instance;

    return instance;
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