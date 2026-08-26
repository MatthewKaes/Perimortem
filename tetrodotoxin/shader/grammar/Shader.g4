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
    | shaderUniformDeclaration
    | shaderBridgeDeclaration
    ;

shaderBridgeDeclaration
    : addressableName typeReference CALL typeReference END_STATEMENT
    ;

// Render owns each Stage signature. Shader supplies only the executable
// Library body selected by the matching Stage name.
shaderStageDeclaration
    : FUNC block
    ;

shaderUniformDeclaration
    : ADDRESSABLE typeReference (ASSIGN declarationInitializer)? END_STATEMENT
    ;
