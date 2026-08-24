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
      documentedShaderDefinition+ EOF
    ;

documentedShaderDefinition
    : definition shaderDefinition
    ;

shaderDefinition
    : SHADER typeReference SCOPE_START
      documentedShaderDeclaration* SCOPE_END
    ;

documentedShaderDeclaration
    : definition shaderDeclaration
    ;

shaderDeclaration
    : shaderStageDeclaration
    | shaderValueDeclaration
    | shaderBridgeDeclaration
    | shaderTypeDeclaration
    ;

shaderBridgeDeclaration
    : addressableName typeReference CALL typeReference END_STATEMENT
    ;

// Definition supplies the shared declaration envelope while Library owns the
// complete Function, Signature, and Block forms.
shaderStageDeclaration
    : functionDefinition
    ;

shaderValueDeclaration
    : fieldDefinition
    | (PUSH | RESOURCE) typeReference
      (ASSIGN declarationInitializer)? END_STATEMENT
    ;

// Shader admits Library Alias and Structure declarations without copying their
// parsers or semantic Types.
shaderTypeDeclaration
    : ALIAS ASSIGN typeReference END_STATEMENT
    | STRUCT structureBody
    ;
