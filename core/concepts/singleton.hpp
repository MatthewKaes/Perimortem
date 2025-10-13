// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <memory>

namespace Perimortem::Concepts {

template <typename T>
class Singleton {
 public:
  static T& instance() noexcept(
      std::is_nothrow_default_constructible<T>::value) {
    // Inject a class that locks down the parent class.
    struct LockAbstract final : T {
      void AbstractInjectionLock() const noexcept override {}
    };

    static LockAbstract instance;
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
  // Create a fake distructor that only classes under singleton override.
  virtual void AbstractInjectionLock() const noexcept = 0;
};

}  // namespace Perimortem::Concepts
