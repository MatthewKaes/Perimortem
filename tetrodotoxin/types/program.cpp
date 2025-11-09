// Perimortem Engine
// Copyright © Matt Kaes

#include "types/program.hpp"

#include "types/std/byt.hpp"
#include "types/std/dec.hpp"
#include "types/std/int.hpp"
#include "types/std/num.hpp"

using namespace Tetrodotoxin::Language::Parser::Types;

Program::Program(bool include_std_lib) {
  if (!include_std_lib)
    return;

  external_abstracts[Byt::instance().get_name()] = &Byt::instance();
  external_abstracts[Dec::instance().get_name()] = &Dec::instance();
  external_abstracts[Int::instance().get_name()] = &Int::instance();
  external_abstracts[Num::instance().get_name()] = &Num::instance();
}