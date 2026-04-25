// Perimortem Engine
// Copyright © Matt Kaes

// This header implements the standard library glue that requires compiler
// extensions or very specific implementations.
//
// There are cases where it makes sense to ally parts of the engine to use the
// standard library, prototyping being one of the bigger ones.

/* Explicity don't protect the include to catch multiple includes */
// #pragma once

#ifndef _NEW
void* operator new(__SIZE_TYPE__ size, void* ptr) noexcept {
  return ptr;
}

#endif
