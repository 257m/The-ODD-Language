#ifndef TOKENIZER_H
#define TOKENIZER_H

// The lexer returns tokens [0-255] if it is an unknown character, otherwise one
// of these for known things.
enum Token {
	tok_eof = -1,

	// commands
	tok_def = -2,
	tok_extern = -3,
	tok_return = -4,
	tok_goto = -5,

	// primary
	tok_identifier = -6,
	tok_number = -7,

	// control
	tok_if = -8,
	tok_else = -9,

	// multi character comparison operators
	tok_dequ = -10,
	tok_nequ = -11,
	tok_lte = -12,
	tok_gte = -13,
	tok_dand = -14,
	tok_dor = -15,

	// data types keywords
	tok_int8 = -18,
	tok_int16 = -19,
	tok_int32 = -20,
	tok_int64 = -21,
	tok_int128 = -22,
	tok_float = -23,
	tok_double = -24,
	tok_quad = -25,
	tok_bool = -26,
	tok_void = -27,
	tok_unsigned = -28,
};

#define IDENTIFIER_MAX 32
#define NUMBER_MAX 384

extern char IdentifierStr[IDENTIFIER_MAX]; // Filled in if tok_identifier
extern double NumVal;						// Filled in if tok_number

/* Custom String Functions */
bool string_compare(const char* str1, const char* str2);
char* CharToNull(char* string, char* output, char c);
void str_append(char* string, char c);
void SetStringToChar(char* string, char c);


int check_for_type(char* identifier);
bool isoperator(int op);

extern int CurTok;
extern uint32_t linenumber;
extern uint64_t linestart;

// gettok - Return the next token from standard input.
int gettok();

#define getNextToken() (CurTok = gettok())

#endif /* TOKENIZER_H */