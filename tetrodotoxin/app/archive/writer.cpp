// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/app/archive/writer.hpp"

#include "perimortem/core/writer/binary.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

using LittleWriter = Perimortem::Core::Writer::Binary<Data::ByteOrder::Little>;

auto App::Archive::Writer::write(
    const App::Language::Monograph& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile) const
    -> Option<Dynamic::Bytes> {
  View::Bytes route = monograph.get_program().get_route();
  View::Bytes callable = monograph.get_program().get_callable_name();
  Count size = 16 + route.get_size() + callable.get_size();
  if (route.get_size() > Unsigned_32(-1) ||
      callable.get_size() > Unsigned_32(-1)) {
    return {};
  }

  Dynamic::Bytes output;
  output.forgetful_resize(size);
  LittleWriter writer(output.get_access());
  writer << "TTAP"_view;
  writer << Unsigned_16(1);
  writer << Unsigned_8(profile);
  writer << Unsigned_8(1);
  writer << Unsigned_32(route.get_size());
  writer << route;
  writer << Unsigned_32(callable.get_size());
  writer << callable;
  return writer.is_valid() && writer.get_location() == size
             ? Option<Dynamic::Bytes>(Data::take(output))
             : Option<Dynamic::Bytes>();
}
