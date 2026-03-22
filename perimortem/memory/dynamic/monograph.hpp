// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/bibliotheca.hpp"

namespace Perimortem::Memory::Dynamic {

// A typed unique pointer that enforces sole ownership.
// Monographs always point to a valid object and can only be moved.
template <typename T>
class Monograph {
 public:
  template <typename... Vargs>
  Monograph(Vargs... args) {
    reserved_block = Bibliotheca::check_out(sizeof(T));
    new (access()) T(args...);
  }

  Monograph(Monograph&& rhs) : {
    Bibliotheca::remit(reserved_block);

    reserved_block = rhs.reserved_block;
    rhs.reserved_block = nullptr;
  }

  ~Monograph() {
    // Support 
    if (reserved_block)
    // Propagate the destructor to the object.
    reinterpret_cast<T*>(access())->~T();
    Bibliotheca::remit(reserved_block);
    reserved_block = nullptr;
  }

  auto operator=(Monograph&& rhs) -> Monograph& : {
    Bibliotheca::remit(reserved_block);

    reserved_block = rhs.reserved_block;
    rhs.reserved_block = nullptr;
    return *this;
  }

  auto operator=(const T& rhs) -> T& : { return (access() = rhs); }

  auto operator=(T&& rhs) -> T& : { return (access() = rhs); }

  auto operator*() -> T& { return access(); }

  auto operator->() -> T& { return access(); }

  auto access() -> T& {
    return *reinterpret_cast<T*>::preface_to_corpus(reserved_block);
  }

 private:
  Bibliotheca::Preface* reserved_block = nullptr;
};

}  // namespace Perimortem::Memory::Dynamic
