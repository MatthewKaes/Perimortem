// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/vulkan/programs/sprite.hpp"

#include "perimortem/core/data.hpp"
#include "perimortem/core/null_terminated.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Vulkan;
using namespace Perimortem;

// These modules are the optimized output of the neighboring GLSL reference.
// Keeping the words aligned and embedded gives the native application the same
// process lifetime product shape that a TTX Package will eventually publish.
alignas(U32) static constexpr U32 vertex_module[] = {
  119734787u,  65536u,      851979u,     88u,         0u,          131089u,
  1u,          393227u,     1u,          1280527431u, 1685353262u, 808793134u,
  0u,          196622u,     0u,          1u,          524303u,     0u,
  4u,          1852399981u, 0u,          22u,         79u,         87u,
  196611u,     2u,          450u,        655364u,     1197427783u, 1279741775u,
  1885560645u, 1953718128u, 1600482425u, 1701734764u, 1919509599u, 1769235301u,
  25974u,      524292u,     1197427783u, 1279741775u, 1852399429u, 1685417059u,
  1768185701u, 1952671090u, 6649449u,    262149u,     4u,          1852399981u,
  0u,          393221u,     22u,         1449094247u, 1702130277u, 1684949368u,
  30821u,      327685u,     25u,         1701080681u, 1818386808u, 101u,
  393221u,     30u,         1769107539u, 1850303860u, 1937012080u, 0u,
  393222u,     30u,         0u,          1851880052u, 1919903347u, 7888749u,
  393222u,     30u,         1u,          1851880052u, 1919903347u, 7954285u,
  327686u,     30u,         2u,          1701736308u, 0u,          262149u,
  32u,         1769107571u, 25972u,      393221u,     77u,         1348430951u,
  1700164197u, 2019914866u, 0u,          393222u,     77u,         0u,
  1348430951u, 1953067887u, 7237481u,    458758u,     77u,         1u,
  1348430951u, 1953393007u, 1702521171u, 0u,          458758u,     77u,
  2u,          1130327143u, 1148217708u, 1635021673u, 6644590u,    458758u,
  77u,         3u,          1130327143u, 1147956341u, 1635021673u, 6644590u,
  196613u,     79u,         0u,          327685u,     87u,         1954047348u,
  1600483957u, 30325u,      262215u,     22u,         11u,         42u,
  196679u,     30u,         2u,          327752u,     30u,         0u,
  35u,         0u,          327752u,     30u,         1u,          35u,
  16u,         327752u,     30u,         2u,          35u,         32u,
  196679u,     77u,         2u,          327752u,     77u,         0u,
  11u,         0u,          327752u,     77u,         1u,          11u,
  1u,          327752u,     77u,         2u,          11u,         3u,
  327752u,     77u,         3u,          11u,         4u,          262215u,
  87u,         30u,         0u,          131091u,     2u,          196641u,
  3u,          2u,          196630u,     6u,          32u,         262167u,
  7u,          6u,          2u,          262176u,     8u,          7u,
  7u,          262165u,     10u,         32u,         0u,          262187u,
  10u,         11u,         6u,          262172u,     12u,         7u,
  11u,         262187u,     6u,          13u,         0u,          327724u,
  7u,          14u,         13u,         13u,         262187u,     6u,
  15u,         1065353216u, 327724u,     7u,          16u,         15u,
  13u,         327724u,     7u,          17u,         15u,         15u,
  327724u,     7u,          18u,         13u,         15u,         589868u,
  12u,         19u,         14u,         16u,         17u,         14u,
  17u,         18u,         262165u,     20u,         32u,         1u,
  262176u,     21u,         1u,          20u,         262203u,     21u,
  22u,         1u,          262176u,     24u,         7u,          12u,
  262167u,     29u,         6u,          4u,          327710u,     30u,
  29u,         29u,         29u,         262176u,     31u,         9u,
  30u,         262203u,     31u,         32u,         9u,          262187u,
  20u,         33u,         0u,          262176u,     34u,         9u,
  29u,         262187u,     10u,         40u,         2u,          262176u,
  41u,         9u,          6u,          262187u,     20u,         45u,
  1u,          262187u,     10u,         60u,         3u,          262187u,
  6u,          64u,         1073741824u, 262187u,     10u,         67u,
  1u,          262172u,     76u,         6u,          67u,         393246u,
  77u,         29u,         6u,          76u,         76u,         262176u,
  78u,         3u,          77u,         262203u,     78u,         79u,
  3u,          262176u,     84u,         3u,          29u,         262176u,
  86u,         3u,          7u,          262203u,     86u,         87u,
  3u,          327734u,     2u,          4u,          0u,          3u,
  131320u,     5u,          262203u,     24u,         25u,         7u,
  262205u,     20u,         23u,         22u,         196670u,     25u,
  19u,         327745u,     8u,          26u,         25u,         23u,
  262205u,     7u,          27u,         26u,         327745u,     34u,
  35u,         32u,         33u,         262205u,     29u,         36u,
  35u,         458831u,     7u,          37u,         36u,         36u,
  0u,          1u,          327828u,     6u,          39u,         37u,
  27u,         393281u,     41u,         42u,         32u,         33u,
  40u,         262205u,     6u,          43u,         42u,         327809u,
  6u,          44u,         39u,         43u,         327745u,     34u,
  46u,         32u,         45u,         262205u,     29u,         47u,
  46u,         458831u,     7u,          48u,         47u,         47u,
  0u,          1u,          327828u,     6u,          50u,         48u,
  27u,         393281u,     41u,         51u,         32u,         45u,
  40u,         262205u,     6u,          52u,         51u,         327809u,
  6u,          53u,         50u,         52u,         393281u,     41u,
  61u,         32u,         33u,         60u,         262205u,     6u,
  62u,         61u,         327816u,     6u,          63u,         44u,
  62u,         327813u,     6u,          65u,         63u,         64u,
  327811u,     6u,          66u,         65u,         15u,         393281u,
  41u,         70u,         32u,         45u,         60u,         262205u,
  6u,          71u,         70u,         327816u,     6u,          72u,
  53u,         71u,         327813u,     6u,          73u,         72u,
  64u,         327811u,     6u,          74u,         15u,         73u,
  458832u,     29u,         83u,         66u,         74u,         13u,
  15u,         327745u,     84u,         85u,         79u,         33u,
  196670u,     85u,         83u,         196670u,     87u,         27u,
  65789u,      65592u,
};

alignas(U32) static constexpr U32 fragment_module[] = {
  119734787u,  65536u,      851979u,     29u,         0u,          131089u,
  1u,          393227u,     1u,          1280527431u, 1685353262u, 808793134u,
  0u,          196622u,     0u,          1u,          458767u,     4u,
  4u,          1852399981u, 0u,          9u,          17u,         196624u,
  4u,          7u,          196611u,     2u,          450u,        655364u,
  1197427783u, 1279741775u, 1885560645u, 1953718128u, 1600482425u, 1701734764u,
  1919509599u, 1769235301u, 25974u,      524292u,     1197427783u, 1279741775u,
  1852399429u, 1685417059u, 1768185701u, 1952671090u, 6649449u,    262149u,
  4u,          1852399981u, 0u,          393221u,     9u,          1886680431u,
  1667200117u, 1919904879u, 0u,          262149u,     13u,         1734438249u,
  101u,        327685u,     17u,         1954047348u, 1600483957u, 30325u,
  393221u,     20u,         1769107539u, 1850303860u, 1937012080u, 0u,
  393222u,     20u,         0u,          1851880052u, 1919903347u, 7888749u,
  393222u,     20u,         1u,          1851880052u, 1919903347u, 7954285u,
  327686u,     20u,         2u,          1701736308u, 0u,          262149u,
  22u,         1769107571u, 25972u,      262215u,     9u,          30u,
  0u,          262215u,     13u,         33u,         0u,          262215u,
  13u,         34u,         0u,          262215u,     17u,         30u,
  0u,          196679u,     20u,         2u,          327752u,     20u,
  0u,          35u,         0u,          327752u,     20u,         1u,
  35u,         16u,         327752u,     20u,         2u,          35u,
  32u,         131091u,     2u,          196641u,     3u,          2u,
  196630u,     6u,          32u,         262167u,     7u,          6u,
  4u,          262176u,     8u,          3u,          7u,          262203u,
  8u,          9u,          3u,          589849u,     10u,         6u,
  1u,          0u,          0u,          0u,          1u,          0u,
  196635u,     11u,         10u,         262176u,     12u,         0u,
  11u,         262203u,     12u,         13u,         0u,          262167u,
  15u,         6u,          2u,          262176u,     16u,         1u,
  15u,         262203u,     16u,         17u,         1u,          327710u,
  20u,         7u,          7u,          7u,          262176u,     21u,
  9u,          20u,         262203u,     21u,         22u,         9u,
  262165u,     23u,         32u,         1u,          262187u,     23u,
  24u,         2u,          262176u,     25u,         9u,          7u,
  327734u,     2u,          4u,          0u,          3u,          131320u,
  5u,          262205u,     11u,         14u,         13u,         262205u,
  15u,         18u,         17u,         327767u,     7u,          19u,
  14u,         18u,         327745u,     25u,         26u,         22u,
  24u,         262205u,     7u,          27u,         26u,         327813u,
  7u,          28u,         19u,         27u,         196670u,     9u,
  28u,         65789u,      65592u,
};

static constexpr Description::Stage input_stages[] = {
  Description::Stage::Vertex,
  Description::Stage::Pixel,
};

static const Description::Module modules[] = {
  {
    Description::Stage::Vertex,
    View::Vector<U32>(vertex_module, sizeof(vertex_module) / sizeof(U32)),
    {},
  },
  {
    Description::Stage::Pixel,
    View::Vector<U32>(fragment_module, sizeof(fragment_module) / sizeof(U32)),
    {},
  },
};

static const Description::HostInputRange host_input_ranges[] = {
  {
    0,
    sizeof(R32) * 12,
    View::Vector<Description::Stage>(
        input_stages,
        sizeof(input_stages) / sizeof(Description::Stage)),
  },
};

static const Description::DescriptorBinding descriptors[] = {
  {"image"_view, 0, 0},
};

static const Description::HostField host_fields[] = {
  {"transform_x"_view, 0, sizeof(R32) * 4},
  {"transform_y"_view, sizeof(R32) * 4, sizeof(R32) * 4},
  {"tone"_view, sizeof(R32) * 8, sizeof(R32) * 4},
};

auto Programs::Sprite::get_locator() -> Perimortem::Graphics::Frame::Program {
  return Perimortem::Graphics::Frame::Program(
      Data::cast<const U8>(vertex_module));
}

auto Programs::Sprite::get_description() -> Description::Program {
  return {
    View::Vector<Description::Module>(
        modules, sizeof(modules) / sizeof(Description::Module)),
    View::Vector<Description::HostInputRange>(
        host_input_ranges,
        sizeof(host_input_ranges) / sizeof(Description::HostInputRange)),
    View::Vector<Description::DescriptorBinding>(
        descriptors,
        sizeof(descriptors) / sizeof(Description::DescriptorBinding)),
    View::Vector<Description::HostField>(
        host_fields, sizeof(host_fields) / sizeof(Description::HostField)),
  };
}
