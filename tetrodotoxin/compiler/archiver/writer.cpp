// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/archiver/writer.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Perimortem::Utility;
using namespace Tetrodotoxin::Archiver;

auto Writer::write(
    Allocator::Arena&,
    const Manifest&,
    const Tetrodotoxin::Ttx::Model::Package::Source&,
    View::Vector<Tetrodotoxin::Ttx::Model::Package::Terminal>)
    -> Option<View::Bytes> {
  // TODO: Encode after Package exposes a finalized durable graph.
  return none;
}
