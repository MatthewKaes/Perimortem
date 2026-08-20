// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/object.hpp"

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/diagnostics/log.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem;

auto Core::Object::create(const Descriptor& descriptor) -> Object {
  static_assert(sizeof(Control) <= Bibliotheca::legal_underwrite_size);

  Bool valid =
      descriptor.get_size() != 0 && descriptor.get_alignment() != 0 &&
      (descriptor.get_alignment() & (descriptor.get_alignment() - 1)) == 0 &&
      descriptor.get_alignment() <= Bibliotheca::allocation_alignment &&
      descriptor.get_finalizer();
  if (!valid) {
    Diagnostics::Log::fatal(
        "Core Object received an invalid runtime descriptor."_view);
  }

  Bibliotheca::Allocation allocation =
      Bibliotheca::check_out(descriptor.get_size());
  auto control =
      Data::cast<Control>(allocation.ptr - sizeof(Core::Object::Control));
  new (control) Control(descriptor);
  return Object(allocation.ptr);
}

auto Core::Object::get_control() const -> Control& {
  if (!payload) {
    Diagnostics::Log::fatal("Core Object received an empty handle."_view);
  }

  return *Data::cast<Control>(payload - sizeof(Control));
}

auto Core::Object::retain() const -> void {
  get_control();
  Bibliotheca::reserve(payload);
}

auto Core::Object::release() const -> void {
  Control& control = get_control();
  if (Bibliotheca::reservation_count(payload) == 1) {
    control.get_descriptor().get_finalizer()(payload);
  }

  Bibliotheca::remit(payload);
}

auto Core::Object::get_descriptor() const -> const Descriptor& {
  return get_control().get_descriptor();
}
