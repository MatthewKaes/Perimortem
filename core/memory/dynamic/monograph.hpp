// // Perimortem Engine
// // Copyright © Matt Kaes

// #pragma once

// #include "core/memory/bibliotheca.hpp"

// namespace Perimortem::Memory::Dynamic {

// // A Monograph turns a Record into a typed object for C++
// template <typename T>
// class Monograph {
//  public:
// 	template <typename... Vargs>
//   Monograph(Vargs... args) {
//     auto reserved_block = Bibliotheca::check_out(sizeof(T));
//     new (access()) T(args...);
//   }

//   Monograph(const Monograph&& rhs) : {}

//   ~Monograph() {
//     // Propagate the destructor to the object.
//     reinterpret_cast<T*>(access())->~T();
//   }

//   Record(const Record& record) {
//     Bibliotheca::exchange(reserved_block, record.reserved_block);
//   }

//   ~Record() {
//     Bibliotheca::remit(reserved_block);
//     reserved_block = nullptr;
//   }

//  private:
//   T& object;
// };

// }  // namespace Perimortem::Memory::Dynamic
