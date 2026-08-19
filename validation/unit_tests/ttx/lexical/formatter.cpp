// Perimortem Engine
// Copyright © Matt Kaes

#include "ttx/lexical/formatter.hpp"

#include "validation/unit_test.hpp"

#include "perimortem/memory/allocator/arena.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Ttx::Lexical;
using namespace Validation;

static Harness TtxFormatter = {
  .name = "TTX::Formatter"_view,
};

PERIMORTEM_UNIT_TEST(TtxFormatter, canonical_declaration_order) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "private Hidden:struct{public state value:Unsigned_8;} "
      "private hidden:func=[self]->Unsigned_8{return 1;} "
      "@abi(\"C\") @symbol(\"first\") "
      "public first:func=[]->Unsigned_8{return 2;} "
      "using Core from dependency; dialect:Library; "
      "public state value:Unsigned_8=0; "
      "public named:func=[self]->Unsigned_8{return value;} "
      "private local:func=[]->Unsigned_8{return 3;} "
      "public CountAlias:alias=Unsigned_8; "
      "private const limit:Unsigned_8=4; "
      "foreign \"C\"{public func imported_z[]->Unsigned_8; "
      "public func imported_a[]->Unsigned_8;} "
      "public Visible:enum[Unsigned_8]{first=0;second=1;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "//\n"
      "// Place holder source documentation.\n"
      "//\n"
      "\n"
      "dialect : Library;\n"
      "\n"
      "using Core from dependency;\n"
      "\n"
      "foreign \"C\" {\n"
      "  public func imported_a[] -> Unsigned_8;\n"
      "  public func imported_z[] -> Unsigned_8;\n"
      "}\n"
      "\n"
      "public CountAlias : alias = Unsigned_8;\n"
      "\n"
      "private const limit : Unsigned_8 = 4;\n"
      "\n"
      "public state value : Unsigned_8 = 0;\n"
      "\n"
      "@abi(\"C\") @symbol(\"first\")\n"
      "public first : func = [] -> Unsigned_8 : return 2;\n"
      "\n"
      "public named : func = [self] -> Unsigned_8 : return value;\n"
      "\n"
      "private local : func = [] -> Unsigned_8 : return 3;\n"
      "\n"
      "private hidden : func = [self] -> Unsigned_8 : return 1;\n"
      "\n"
      "public Visible : enum[Unsigned_8] {\n"
      "  first = 0;\n"
      "  second = 1;\n"
      "}\n"
      "\n"
      "private Hidden : struct {\n"
      "  public state value : Unsigned_8;\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, inferred_declaration_stays_compact) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public create:func=[]->Unsigned_8{state value:=-1; "
      "if !false {value+=1;} for [.entry:Unsigned_8] in 0...1 {value+=entry;} "
      "return value!+2;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public create : func = [] -> Unsigned_8 {\n"
      "  state value := -1;\n"
      "  if !false : value += 1;\n"
      "  for [.entry : Unsigned_8] in 0...1 : value += entry;\n"
      "\n"
      "  return value! + 2;\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, composite_declaration_order) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public Container:struct{private Nested:struct{} "
      "private self_call:func=[self]->[]{return;} "
      "public state first:Unsigned_8; private const beta:Unsigned_8=2; "
      "public static_call:func=[]->[]{return;} "
      "private state second:Unsigned_8; public const alpha:Unsigned_8=1; "
      "public self_call:func=[self]->[]{return;} "
      "public Nested:enum[Unsigned_8]{first=0;} "
      "private static_call:func=[]->[]{return;}}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public Container : struct {\n"
      "  public const alpha : Unsigned_8 = 1;\n"
      "  private const beta : Unsigned_8 = 2;\n"
      "\n"
      "  public state first   : Unsigned_8;\n"
      "  private state second : Unsigned_8;\n"
      "\n"
      "  public static_call : func = [] -> [] : return;\n"
      "\n"
      "  public self_call : func = [self] -> [] : return;\n"
      "\n"
      "  private static_call : func = [] -> [] : return;\n"
      "\n"
      "  private self_call : func = [self] -> [] : return;\n"
      "\n"
      "  public Nested : enum[Unsigned_8] {\n"
      "    first = 0;\n"
      "  }\n"
      "\n"
      "  private Nested : struct {}\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, alphabetical_declaration_blocks) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; private ZebraAlias:alias=Unsigned_8; "
      "public ZebraAlias:alias=Unsigned_8; "
      "private AlphaAlias:alias=Unsigned_8; "
      "public AlphaAlias:alias=Unsigned_8; "
      "private const zebra:Unsigned_8=2; "
      "public const alpha:Unsigned_8=1; "
      "public zebra:func=[]->[]{return;} "
      "public alpha:func=[]->[]{return;} "
      "private Zebra:struct{} public Zebra:struct{} "
      "private Alpha:struct{} public Alpha:struct{}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public AlphaAlias  : alias = Unsigned_8;\n"
      "public ZebraAlias  : alias = Unsigned_8;\n"
      "private AlphaAlias : alias = Unsigned_8;\n"
      "private ZebraAlias : alias = Unsigned_8;\n"
      "\n"
      "public const alpha  : Unsigned_8 = 1;\n"
      "private const zebra : Unsigned_8 = 2;\n"
      "\n"
      "public alpha : func = [] -> [] : return;\n"
      "public zebra : func = [] -> [] : return;\n"
      "\n"
      "public Alpha : struct {}\n"
      "\n"
      "public Zebra : struct {}\n"
      "\n"
      "private Alpha : struct {}\n"
      "\n"
      "private Zebra : struct {}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, documentation_starts_paragraph) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "//Source.\n"
      "dialect:Library; @abi(\"C\") // Function documentation.\n"
      "@symbol(\"run\") public run:func=[]->[]{state first:Unsigned_8=0; "
      "first=1; //Observed statement.\n"
      "first; //\n"
      "return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "// Function documentation.\n"
      "@abi(\"C\") @symbol(\"run\")\n"
      "public run : func = [] -> [] {\n"
      "  state first : Unsigned_8 = 0;\n"
      "  first = 1;\n"
      "\n"
      "  // Observed statement.\n"
      "  first;\n"
      "  //\n"
      "  return;\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, canonical_hexadecimal_literals) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "//Source.\n"
      "dialect:Library; private const small:=0xa; "
      "private const medium:=0xabc; private const large:=0xabcde; "
      "private const bytes:=0x[0a   bC\t00];"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "private const bytes  := 0x[0A BC 00];\n"
      "private const large  := 0x000ABCDE;\n"
      "private const medium := 0x0ABC;\n"
      "private const small  := 0x0A;\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, bounded_alignment_islands) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; private const a:Unsigned_64=1; "
      "private const extraordinarily_long_constant_name:Unsigned_64=2; "
      "public Table:struct{public state count:Unsigned_64=0; "
      "private state selected:Bool=false;} "
      "public assign:func=[]->[]{first=1;longer_name=2;return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "private const a : Unsigned_64 = 1;\n"
      "private const extraordinarily_long_constant_name : Unsigned_64 = 2;\n"
      "\n"
      "public assign : func = [] -> [] {\n"
      "  first       = 1;\n"
      "  longer_name = 2;\n"
      "}\n"
      "\n"
      "public Table : struct {\n"
      "  public state count     : Unsigned_64 = 0;\n"
      "  private state selected : Bool        = false;\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, canonical_pack_width) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; private const compact:=(.right=2,.left=1,); "
      "private const expanded:=(.first=11111111111111111111,"
      ".second=22222222222222222222,.third=33333333333333333333);"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "private const compact  := (.right = 2, .left = 1);\n"
      "private const expanded := (\n"
      "  .first = 11111111111111111111,\n"
      "  .second = 22222222222222222222,\n"
      "  .third = 33333333333333333333,\n"
      ");\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, empty_result_fallthrough) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public empty:func=[]->[]{} "
      "public effect:func=[]->[]{value=1;return;} "
      "public many:func=[]->[]{first=1;second=2;return;} "
      "public explicit:func=[]->[]:return; "
      "public malformed:func=[]->[]{return;value=1;} "
      "public repeated:func=[]->[]{return;return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public effect : func = [] -> [] : value = 1;\n"
      "public empty : func = [] -> [] : return;\n"
      "public explicit : func = [] -> [] : return;\n"
      "public malformed : func = [] -> [] {\n"
      "  return;\n"
      "  value = 1;\n"
      "}\n"
      "\n"
      "public many : func = [] -> [] {\n"
      "  first  = 1;\n"
      "  second = 2;\n"
      "}\n"
      "\n"
      "public repeated : func = [] -> [] : return;\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, executable_paragraph_state) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public run:func=[]->[]{state first:Unsigned_8=0; "
      "first=1; const second:Unsigned_8=2; second; if true {first=2;} "
      "while false {break;} return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public run : func = [] -> [] {\n"
      "  state first : Unsigned_8 = 0;\n"
      "  first = 1;\n"
      "\n"
      "  const second : Unsigned_8 = 2;\n"
      "  second;\n"
      "  if true : first = 2;\n"
      "  while false : break;\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, compressed_block_paragraph_state) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public run:func=[]->[]{state alpha:Unsigned_64=0;"
      "alpha=0;if first:alpha=1;if second:alpha=2;alpha=3;alpha=4;"
      "while third{alpha=5;alpha=6;}alpha=7;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public run : func = [] -> [] {\n"
      "  state alpha : Unsigned_64 = 0;\n"
      "  alpha = 0;\n"
      "  if first : alpha = 1;\n"
      "  if second : alpha = 2;\n"
      "\n"
      "  alpha = 3;\n"
      "  alpha = 4;\n"
      "  while third {\n"
      "    alpha = 5;\n"
      "    alpha = 6;\n"
      "  }\n"
      "\n"
      "  alpha = 7;\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, compact_single_statement_blocks) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; public run:func=[.first:Bool,.second:Bool]->[]{"
      "if first {if second {first;}} else {second;} match first {"
      "case true {first;} case _ {first;second;}} return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public run : func = [.first : Bool, .second : Bool] -> [] {\n"
      "  if first {\n"
      "    if second : first;\n"
      "  } else : second;\n"
      "\n"
      "  match first {\n"
      "    case true : first;\n"
      "    case _ {\n"
      "      first;\n"
      "      second;\n"
      "    }\n"
      "  }\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, dialect_callable_preserves_body_order) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Scene; Scene update[self]->Void{self.value=1; "
      "const observed:Unsigned_8=2; return;} "
      "Scene release[self]->Void{return;}"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Scene;\n"
      "\n"
      "Scene release[self] -> Void : return;\n"
      "Scene update[self] -> Void {\n"
      "  self.value = 1;\n"
      "\n"
      "  const observed : Unsigned_8 = 2;\n"
      "  return;\n"
      "}\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, malformed_source_is_stable) {
  Allocator::Arena first_arena;
  Tokenizer first(
      first_arena,
      "// Source.\n"
      "dialect:Library; public state value:Unsigned_8 = ^ "_view,
      "Test.ttx"_view);
  Dynamic::Bytes first_output = Formatter(first).format();

  Allocator::Arena second_arena;
  Tokenizer second(second_arena, first_output.get_view(), "Test.ttx"_view);
  Dynamic::Bytes second_output = Formatter(second).format();

  EXPECT_TEXT(first_output, second_output.get_view());
  EXPECT_TEXT(
      first_output,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "public state value : Unsigned_8 = ^\n"_view);
}

PERIMORTEM_UNIT_TEST(TtxFormatter, wraps_long_postfix_expression) {
  Allocator::Arena arena;
  Tokenizer tokenizer(
      arena,
      "// Source.\n"
      "dialect:Library; private value:Fixed[Unsigned_8,2]="
      "0x[00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F]"
      ":[Parameters.offset+12,Parameters.size];"_view,
      "Test.ttx"_view);

  Dynamic::Bytes formatted = Formatter(tokenizer).format();
  EXPECT_TEXT(
      formatted,
      "// Source.\n"
      "dialect : Library;\n"
      "\n"
      "private value : Fixed[Unsigned_8, 2] = "
      "0x[00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F]"
      ":[Parameters.offset +\n"
      "    12, Parameters.size];\n"_view);
}
