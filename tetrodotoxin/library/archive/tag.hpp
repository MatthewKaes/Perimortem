// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

namespace Tetrodotoxin::Library::Archive {

// Tag is the frozen Library Archive Format 1 vocabulary. Records retain their
// own byte extent, allowing a future reader to skip an unknown optional record
// without interpreting any of its contents.
enum class Tag : U16 {
  Source = 1,
  Import = 2,
  Foreign = 3,
  ForeignState = 4,
  ForeignFunction = 5,
  Alias = 6,
  Structure = 7,
  Object = 8,
  Enumeration = 9,
  EnumerationCase = 10,
  Field = 11,
  Function = 12,
  Signature = 13,
  TypeReference = 14,
  PackGroup = 15,
  ConstantFalse = 16,
  ConstantTrue = 17,
  ConstantUnsigned = 18,
  ConstantSigned = 19,
  ConstantReal = 20,
  ConstantBytes = 21,
  ConstantEnumeration = 22,
  ConstantRange = 23,
  ConstantOption = 24,
  ConstantResult = 25,
  Layout = 26,
  FieldSlot = 27,
  ConstantObject = 28,
  ConstantResourceBytes = 29,
};

}  // namespace Tetrodotoxin::Library::Archive
