// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

namespace Tetrodotoxin::Package::Archive {

// Names the stable caller decision for a rejected Package Archive. Reader
// keeps framing and semantic details in its Debug record because those facts
// are useful for diagnosis but do not change the caller's recovery path.
enum class ReadError {
  InvalidFormat,
  UnsupportedFormat,
};

}  // namespace Tetrodotoxin::Package::Archive
