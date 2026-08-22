// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors
//
// Canonical token reference for the Tetrodotoxin G4 language descriptions.
// Words promoted by the runtime Lexicon are reserved from generic Addressable
// dispatch. Other contextual grammar words need not have distinct runtime Codes.

lexer grammar TTXLexer;

PACKAGE_DIALECT : 'Package';
LIBRARY_DIALECT : 'Library';
APP_DIALECT     : 'App';
SCENE_DIALECT   : 'Scene';
RENDER_DIALECT  : 'Render';
SHADER_DIALECT  : 'Shader';

WINDOWED_PROFILE : 'Windowed';
TERMINAL_PROFILE : 'Terminal';
HEADLESS_PROFILE : 'Headless';
PROGRAM_LIFETIME : 'Program';

AND      : 'and';
OR       : 'or';
IF       : 'if';
IN       : 'in';
FOR      : 'for';
WHILE    : 'while';
CASE     : 'case';
MATCH    : 'match';
BREAK    : 'break';
CONTINUE : 'continue';
ELSE     : 'else';
FUNC     : 'func';
SELF     : 'self';
TRUE     : 'true';
FALSE    : 'false';
RETURN   : 'return';
NEW      : 'new';
RESOLVE  : 'resolve';
SOURCE   : 'source';
DIALECT  : 'dialect';
ALIAS    : 'alias';

PUBLIC  : 'public';
PRIVATE : 'private';
EXPOSE  : 'expose';
STATE   : 'state';
CONST   : 'const';

USING     : 'using';
ENUM      : 'enum';
STRUCT    : 'struct';
OBJECT    : 'object';
FOREIGN   : 'foreign';
FROM      : 'from';
RUNTIME   : 'runtime';
LIFECYCLE : 'lifecycle';
START     : 'start';
INITIAL   : 'initial';
ON        : 'on';
REPLACE   : 'replace';
PUSH      : 'push';
POP       : 'pop';
EXIT      : 'exit';
SIGNAL    : 'signal';
EMIT      : 'emit';
PREPARE   : 'prepare';
PAUSE     : 'pause';
RESUME    : 'resume';
UPDATE    : 'update';
RELEASE   : 'release';
STAGE     : 'stage';
RESOURCE  : 'resource';
SHADER    : 'shader';

COMMENT   : '//' ~[\r\n]*;
ATTRIBUTE : '@' [a-zA-Z_] [a-zA-Z0-9_]*;

SWIZZLE     : '.[';
VALUE_ACCESS: ':[';
TYPE_ACCESS : '::';
RANGE       : '...';
ADD_ASSIGN  : '+=';
SUB_ASSIGN  : '-=';
LESS_EQUAL  : '<=';
GREATER_EQUAL: '>=';
EQUAL       : '==';
NOT_EQUAL   : '!=';
CALL        : '->';

SCOPE_START   : '{';
SCOPE_END     : '}';
PACKING_START : '(';
PACKING_END   : ')';
BRACKET_START : '[';
BRACKET_END   : ']';
ASSIGN        : '=';
DEFINE        : ':';
END_STATEMENT : ';';

ADD        : '+';
SUBTRACT   : '-';
DIVIDE     : '/';
MULTIPLY   : '*';
MODULO     : '%';
LESS       : '<';
GREATER    : '>';
ADDRESS    : '.';
PACK       : ',';
NOT        : '!';
QUESTION   : '?';
AND_OP     : '&';
OR_OP      : '|';

BYTES    : '0x[' (HEX_DIGIT | [ \t\r\n])* ']';
EMBEDDED : '$[' ~[\]\r\n]* ']';
FLOAT    : DEC_DIGIT+ '.' DEC_DIGIT+;
HEX      : '0x' HEX_DIGIT+;
NUMERIC  : DEC_DIGIT+;
STRING   : '"' (ESCAPE | ~["\\\r\n])* '"';
DISCARD  : '_';

ADDRESSABLE : [a-z] [a-zA-Z0-9_]*;
TYPE        : [A-Z] [a-zA-Z0-9_]*;

fragment ESCAPE    : '\\' .;
fragment HEX_DIGIT : [0-9a-fA-F];
fragment DEC_DIGIT : [0-9];

WHITESPACE : [ \t\r\n]+ -> skip;
UNKNOWN    : .;
