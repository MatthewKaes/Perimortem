// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"

#include "ttx/type.hpp"

namespace Ttx {
class Layout {
 public:
  constexpr Layout() = default;
  explicit Layout(const Type& type);
  explicit constexpr Layout(
      Perimortem::Core::View::Vector<Type::Member> members)
      : members(members) {}

  constexpr auto is_empty() const -> Bool { return members.is_empty(); }
  constexpr auto has_named_members() const -> Bool {
    for (Count member_index = 0; member_index < members.get_size();
         member_index++) {
      if (members[member_index].is_named()) {
        return True;
      }
    }

    return False;
  }

  constexpr auto get_members() const
      -> Perimortem::Core::View::Vector<Type::Member> {
    return members;
  }
  constexpr auto get_member_count() const -> Count { return members.get_size(); }
  constexpr auto member_at(Count index) const -> Type::Member {
    return index < members.get_size() ? members[index] : Type::Member();
  }
  constexpr auto find_member(Perimortem::Core::View::Bytes name) const
      -> const Type::Member* {
    for (Count member_index = 0; member_index < members.get_size();
         member_index++) {
      const Type::Member& member = members[member_index];
      if (member.get_name() == name) {
        return &member;
      }
    }

    return nullptr;
  }

  auto equivalent_to(const Layout& other) const -> Bool;
  auto fits(const Layout& target) const -> Bool;

 private:
  auto count_required_members() const -> Count;

  Perimortem::Core::View::Vector<Type::Member> members;
};

}  // namespace Ttx
