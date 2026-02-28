// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/virtual_machine/engine/program.hpp"

#include "tetrodotoxin/virtual_machine/types/std/byt.hpp"
#include "tetrodotoxin/virtual_machine/types/std/dec.hpp"
#include "tetrodotoxin/virtual_machine/types/std/int.hpp"
#include "tetrodotoxin/virtual_machine/types/std/num.hpp"

using namespace Tetrodotoxin::Types;

Program::Program(bool include_std_lib) {
  if (!include_std_lib)
    return;

  external_abstracts[Byt::instance().get_name()] = &Byt::instance();
  external_abstracts[Dec::instance().get_name()] = &Dec::instance();
  external_abstracts[Int::instance().get_name()] = &Int::instance();
  external_abstracts[Num::instance().get_name()] = &Num::instance();
}