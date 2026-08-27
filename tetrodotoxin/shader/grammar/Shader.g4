// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
// Canonical Shader source shape. Shader owns contracts, storage roles, and
// bridges while executable syntax comes directly from Library.

parser grammar Shader;

options {
  tokenVocab = TTXLexer;
}

import Library;

shaderSource
    : documentation DIALECT DEFINE SHADER_DIALECT END_STATEMENT
      (sourceImport | packageImport)* shaderImplementation
      documentedShaderDeclaration* EOF
    ;

shaderImplementation
    : ADDRESSABLE SOURCE PACKING_START STRING PACKING_END END_STATEMENT
    ;

documentedShaderDeclaration
    : documentation? shaderStageDeclaration
    | definition shaderDeclaration
    ;

shaderDeclaration
    : shaderStageDeclaration
    | shaderUniformDeclaration
    | shaderBridgeDeclaration
    | shaderStorageDeclaration
    ;

shaderBridgeDeclaration
    : addressableName typeReference CALL typeReference END_STATEMENT
    ;

// Pipeline owns the required Stage signature. Shader repeats that expected
// Layout explicitly and supplies the executable Library body.
shaderStageDeclaration
    : SHADER_DIALECT ADDRESSABLE functionSignature block
    ;

shaderUniformDeclaration
    : ADDRESSABLE typeReference (ASSIGN declarationInitializer)? END_STATEMENT
    ;

shaderStorageDeclaration
    : PUSH typeReference (ASSIGN declarationInitializer)? END_STATEMENT
    | RESOURCE ADDRESSABLE+ typeReference CALL typeReference END_STATEMENT
    ;
