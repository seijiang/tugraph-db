
// Generated from src/cypher/grammar/Lcypher.g4 by ANTLR 4.12.0

#pragma once


#include "antlr4-runtime.h"


namespace parser {


class  LcypherLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    T__7 = 8, T__8 = 9, T__9 = 10, T__10 = 11, T__11 = 12, T__12 = 13, T__13 = 14, 
    T__14 = 15, T__15 = 16, T__16 = 17, T__17 = 18, T__18 = 19, T__19 = 20, 
    T__20 = 21, T__21 = 22, T__22 = 23, T__23 = 24, T__24 = 25, T__25 = 26, 
    T__26 = 27, T__27 = 28, T__28 = 29, T__29 = 30, T__30 = 31, T__31 = 32, 
    T__32 = 33, T__33 = 34, T__34 = 35, T__35 = 36, T__36 = 37, T__37 = 38, 
    T__38 = 39, T__39 = 40, T__40 = 41, T__41 = 42, T__42 = 43, T__43 = 44, 
    T__44 = 45, EXPLAIN = 46, PROFILE = 47, OPTIMIZE = 48, UNION = 49, ALL = 50, 
    OPTIONAL_ = 51, MATCH = 52, UNWIND = 53, AS = 54, MERGE = 55, ON = 56, 
    CREATE = 57, SET = 58, DETACH = 59, DELETE_ = 60, REMOVE = 61, CALL = 62, 
    YIELD = 63, WITH = 64, DISTINCT = 65, RETURN = 66, ORDER = 67, BY = 68, 
    L_SKIP = 69, LIMIT = 70, ASCENDING = 71, ASC = 72, DESCENDING = 73, 
    DESC = 74, USING = 75, JOIN = 76, START = 77, WHERE = 78, OR = 79, XOR = 80, 
    AND = 81, NOT = 82, IN = 83, STARTS = 84, ENDS = 85, CONTAINS = 86, 
    REGEXP = 87, IS = 88, NULL_ = 89, COUNT = 90, ANY = 91, NONE = 92, SINGLE = 93, 
    TRUE_ = 94, FALSE_ = 95, EXISTS = 96, CASE = 97, ELSE = 98, END = 99, 
    WHEN = 100, THEN = 101, StringLiteral = 102, EscapedChar = 103, HexInteger = 104, 
    DecimalInteger = 105, OctalInteger = 106, HexLetter = 107, HexDigit = 108, 
    Digit = 109, NonZeroDigit = 110, NonZeroOctDigit = 111, OctDigit = 112, 
    ZeroDigit = 113, ExponentDecimalReal = 114, RegularDecimalReal = 115, 
    FILTER = 116, EXTRACT = 117, UnescapedSymbolicName = 118, CONSTRAINT = 119, 
    DO = 120, FOR = 121, REQUIRE = 122, UNIQUE = 123, MANDATORY = 124, SCALAR = 125, 
    OF = 126, ADD = 127, DROP = 128, IdentifierStart = 129, IdentifierPart = 130, 
    EscapedSymbolicName = 131, SP = 132, WHITESPACE = 133, Comment = 134
  };

  explicit LcypherLexer(antlr4::CharStream *input);

  ~LcypherLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

}  // namespace parser
