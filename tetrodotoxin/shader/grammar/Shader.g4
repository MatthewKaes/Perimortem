// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
// Canonical Shader source shape. Its expressions and bodies are spelled out
// because similar syntax does not route GPU facts through Library's CPU model.

parser grammar Shader;

options {
  tokenVocab = TTXLexer;
}

import Tetrodotoxin;

shaderSource
    : documentation DIALECT DEFINE SHADER_DIALECT END_STATEMENT
      documentedShaderDefinition+ EOF
    ;

documentedShaderDefinition
    : documentation? attribute* shaderDefinition
    ;

shaderDefinition
    : SHADER typeName DEFINE typeReference SCOPE_START
      documentedShaderDeclaration* SCOPE_END
    ;

documentedShaderDeclaration
    : documentation? attribute* shaderDeclaration
    ;

shaderDeclaration
    : shaderStageDeclaration
    | shaderValueDeclaration
    | shaderAliasDeclaration
    | shaderStructureDeclaration
    ;

shaderStageDeclaration
    : FUNC typeName shaderStageSignature shaderBlock
    ;

shaderStageSignature
    : shaderParameterLayout CALL shaderResultLayout
    ;

shaderParameterLayout
    : typeReference
    | BRACKET_START shaderParameterEntries? PACK? BRACKET_END
    ;

shaderParameterEntries
    : shaderNamedLayoutSlot (PACK shaderNamedLayoutSlot)*
    | typeReference (PACK typeReference)*
    ;

shaderResultLayout
    : typeReference
    | BRACKET_START shaderResultEntries? PACK? BRACKET_END
    ;

shaderResultEntries
    : shaderNamedLayoutSlot (PACK shaderNamedLayoutSlot)*
    | typeReference (PACK typeReference)*
    ;

shaderNamedLayoutSlot
    : attribute* ADDRESS addressableName DEFINE typeReference
    ;

shaderValueDeclaration
    : visibility? (CONST | PUSH | RESOURCE | STATE) addressableName
      DEFINE typeReference (ASSIGN shaderExpression)? END_STATEMENT
    ;

shaderAliasDeclaration
    : visibility typeName DEFINE ALIAS ASSIGN typeReference END_STATEMENT
    ;

shaderStructureDeclaration
    : visibility typeName DEFINE STRUCT SCOPE_START
      documentedShaderDeclaration* SCOPE_END
    ;

shaderBlock
    : SCOPE_START shaderStatement* SCOPE_END
    ;

shaderStatement
    : documentation
    | shaderConditional
    | shaderReturn
    | shaderLocalDeclaration
    | shaderAssignment
    | shaderExpression END_STATEMENT
    ;

shaderConditional
    : IF PACKING_START shaderExpression PACKING_END shaderBlock
      (ELSE (shaderConditional | shaderBlock))?
    ;

shaderReturn
    : RETURN shaderExpression? END_STATEMENT
    ;

shaderLocalDeclaration
    : (CONST | STATE) addressableName DEFINE typeReference
      (ASSIGN shaderExpression)? END_STATEMENT
    ;

shaderAssignment
    : shaderAssignmentTarget (ASSIGN | ADD_ASSIGN | SUB_ASSIGN)
      shaderExpression END_STATEMENT
    ;

shaderAssignmentTarget
    : (addressableName | SELF) shaderAssignmentSuffix*
    ;

shaderAssignmentSuffix
    : ADDRESS addressableName
    ;

shaderExpression
    : shaderOrExpression
    ;

shaderOrExpression
    : shaderAndExpression (OR shaderAndExpression)*
    ;

shaderAndExpression
    : shaderEqualityExpression (AND shaderEqualityExpression)*
    ;

shaderEqualityExpression
    : shaderComparisonExpression
      ((EQUAL | NOT_EQUAL) shaderComparisonExpression)*
    ;

shaderComparisonExpression
    : shaderAdditiveExpression
      ((LESS | GREATER | LESS_EQUAL | GREATER_EQUAL) shaderAdditiveExpression)*
    ;

shaderAdditiveExpression
    : shaderMultiplicativeExpression
      ((ADD | SUBTRACT) shaderMultiplicativeExpression)*
    ;

shaderMultiplicativeExpression
    : shaderUnaryExpression
      ((MULTIPLY | DIVIDE | MODULO) shaderUnaryExpression)*
    ;

shaderUnaryExpression
    : (NOT | SUBTRACT) shaderUnaryExpression
    | shaderPostfixExpression
    ;

shaderPostfixExpression
    : shaderStaticInvocation shaderPostfixSuffix*
    | shaderPrimaryExpression shaderPostfixSuffix*
    ;

shaderStaticInvocation
    : typeReference CALL addressableName shaderParenthesizedPack
    ;

shaderPostfixSuffix
    : ADDRESS addressableName
    | CALL addressableName shaderParenthesizedPack
    | SWIZZLE shaderSwizzleSelection? BRACKET_END
    ;

shaderSwizzleSelection
    : addressableName (PACK addressableName)* PACK?
    ;

shaderPrimaryExpression
    : literal
    | addressableName
    | SELF
    | shaderConstructionExpression
    | shaderParenthesizedPack
    ;

shaderConstructionExpression
    : typeReference shaderParenthesizedPack
    ;

// Shader retains its own expression grammar while following the shared Pack
// distinction: parentheses supply values, and a named value uses `=`.
shaderParenthesizedPack
    : PACKING_START
      (shaderNamedPackEntries | shaderPositionalPackEntries)? PACK? PACKING_END
    ;

shaderNamedPackEntries
    : shaderNamedPackEntry (PACK shaderNamedPackEntry)*
    ;

shaderNamedPackEntry
    : ADDRESS addressableName ASSIGN shaderExpression
    ;

shaderPositionalPackEntries
    : shaderExpression (PACK shaderExpression)*
    ;
