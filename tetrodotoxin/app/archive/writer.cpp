// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "tetrodotoxin/app/archive/writer.hpp"

#include "perimortem/core/writer/binary.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin;

auto App::Archive::Writer::write(
    const App::Language::Monograph& monograph,
    Tetrodotoxin::Language::Persistence::Profile profile) const
    -> Option<Dynamic::Bytes> {
  // Format 1 predates startup Resource closure. Declining other profiles keeps
  // an App restored without source from losing policy that Package cannot
  // reconstruct.
  if (monograph.get_runtime().get_profile() !=
      App::Language::Runtime::Profile::Terminal) {
    return {};
  }

  View::Bytes route = monograph.get_program().get_route();
  View::Bytes callable = monograph.get_program().get_callable_name();
  Count size = 16 + route.get_size() + callable.get_size();
  if (route.get_size() > U32(-1) || callable.get_size() > U32(-1)) {
    return {};
  }

  Dynamic::Bytes output;
  output.forgetful_resize(size);
  Perimortem::Core::Writer::Binary<Data::ByteOrder::Little> writer(
      output.get_access());
  writer << "TTAP"_view;
  writer << U16(1);
  writer << U8(profile);
  writer << U8(App::Language::Runtime::Profile::Terminal);
  writer << U32(route.get_size());
  writer << route;
  writer << U32(callable.get_size());
  writer << callable;
  return writer.is_valid() && writer.get_location() == size
             ? Option<Dynamic::Bytes>(Data::take(output))
             : Option<Dynamic::Bytes>();
}
