// Perimortem Engine
// Copyright © Matt Kaes
//
// Shader dialect grammar prototype. Its expression and body rules are spelled
// here deliberately: similar surface syntax does not route GPU facts through
// Library's CPU Expression model.

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
    : documentation? DISABLED? attribute* shaderDefinition
    ;

shaderDefinition
    : SHADER typeName DEFINE typeReference SCOPE_START
      documentedShaderDeclaration* SCOPE_END
    ;

documentedShaderDeclaration
    : documentation? DISABLED? attribute* shaderDeclaration
    ;

shaderDeclaration
    : shaderStageDeclaration
    | shaderValueDeclaration
    | shaderAliasDeclaration
    | shaderStructureDeclaration
    ;

shaderStageDeclaration
    : FUNC typeName functionSignature shaderBlock
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
    : IF shaderArgumentPack shaderBlock
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
    : ADDRESS (addressableName | typeName)
    | BRACKET_START shaderExpression BRACKET_END
    | SWIZZLE shaderExpressionList? PACK? BRACKET_END
    ;

shaderExpression
    : shaderRangeExpression
    ;

shaderRangeExpression
    : shaderOrExpression (RANGE shaderOrExpression)?
    ;

shaderOrExpression
    : shaderAndExpression ((OR | OR_OP) shaderAndExpression)*
    ;

shaderAndExpression
    : shaderEqualityExpression ((AND | AND_OP) shaderEqualityExpression)*
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
    : shaderPrimaryExpression shaderPostfixSuffix*
    ;

shaderPostfixSuffix
    : ADDRESS (addressableName | typeName)
    | CALL (addressableName | typeName) shaderArgumentPack
    | BRACKET_START shaderExpression BRACKET_END
    | SWIZZLE shaderExpressionList? PACK? BRACKET_END
    | VALUE_ACCESS shaderExpression (PACK shaderExpression)? BRACKET_END
    ;

shaderPrimaryExpression
    : literal
    | addressableName
    | SELF
    | DISCARD
    | typeReference shaderArgumentPack
    | typeReference
    | shaderArgumentPack
    ;

shaderArgumentPack
    : PACKING_START
      (shaderNamedArguments | shaderExpressionList)? PACK? PACKING_END
    ;

shaderNamedArguments
    : shaderNamedArgument (PACK shaderNamedArgument)*
    ;

shaderNamedArgument
    : ADDRESS addressableName ASSIGN shaderExpression
    ;

shaderExpressionList
    : shaderExpression (PACK shaderExpression)*
    ;
