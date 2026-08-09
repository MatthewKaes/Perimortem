// Perimortem Engine
// Copyright © Matt Kaes
//
// Shared lexical prototype for the Tetrodotoxin dialect grammars.
//
// The original omni grammar lived at tetrodotoxin/syntax/ttx.g4 through
// eb53a0a. The runtime lexer remains handwritten. This file separates exact
// grammar spellings for ANTLR readability and does not require the runtime Code
// enum to promote every contextual word into a global keyword.

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
CHILD     : 'child';
SIGNAL    : 'signal';
PREPARE   : 'prepare';
PAUSE     : 'pause';
RESUME    : 'resume';
UPDATE    : 'update';
RELEASE   : 'release';
STAGE     : 'stage';
RESOURCE  : 'resource';
SHADER    : 'shader';

COMMENT   : '//' ~[\r\n]*;
DISABLED  : '/>';
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
