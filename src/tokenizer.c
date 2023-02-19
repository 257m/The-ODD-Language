#include "../include/common.h"
#include "../include/tokenizer.h"

char IdentifierStr[IDENTIFIER_MAX]; // Filled in if tok_identifier
double NumVal;						// Filled in if tok_number

/* Custom String Functions */
bool string_compare(const char* str1, const char* str2)
{
	int charnum = 0;

	while (str1[charnum] == str2[charnum]) {
		if (str1[charnum] == '\0' || str2[charnum] == '\0')
			break;
		charnum++;
	}

	if (str1[charnum] == '\0' && str2[charnum] == '\0')
		return 1;

	return 0;
}

char* CharToNull(char* string, char* output, char c)
{
	unsigned int charnum = 0;
	while (string[charnum] != '\0') {
		if (string[charnum] != c) {
			output[charnum] = string[charnum];
			charnum++;
		}
		else {
			output[charnum] = '\0';
			return output;
		}
	}
	return output;
}

void str_append(char* string, char c)
{
	unsigned int charnum = 0;

	while (string[charnum] != '\0')
		charnum++;

	string[charnum] = c;
}

void SetStringToChar(char* string, char c)
{
	unsigned int charnum = 0;
	string[charnum++] = c;
	while (string[charnum] != '\0')
		string[charnum++] = '\0';
}

int check_for_type(char* identifier)
{
	if (identifier[0] == 'i') {
		if (string_compare(identifier + 1, "nt8"))
			return tok_int8;
		if (string_compare(identifier + 1, "nt16"))
			return tok_int16;
		if (string_compare(identifier + 1, "nt32"))
			return tok_int32;
		if (string_compare(identifier + 1, "nt64"))
			return tok_int64;
		if (string_compare(identifier + 1, "nt128"))
			return tok_int128;
		return 0;
	}
	if (identifier[0] == 'f')
		if (string_compare(identifier + 1, "loat"))
			return tok_float;
	if (identifier[0] == 'd')
		if (string_compare(identifier + 1, "ouble"))
			return tok_double;
	if (identifier[0] == 'q')
		if (string_compare(identifier + 1, "uad"))
			return tok_quad;
	if (identifier[0] == 'b')
		if (string_compare(identifier + 1, "ool"))
			return tok_bool;
	if (identifier[0] == 'v')
		if (string_compare(identifier + 1, "oid"))
			return tok_void;
	if (identifier[0] == 'u')
		if (string_compare(identifier + 1, "nsigned"))
			return tok_unsigned;
	return 0;
}

bool isoperator(int op)
{
	return (op == '+') || (op == '-') || (op == '*') || (op == '/') ||
		(op == '=') || (op == tok_nequ) || (op == tok_lte) || (op == tok_gte) ||
		(op == tok_dand) || (op == tok_dor);
}

int CurTok = 0;
uint32_t linenumber = 1;
uint64_t linestart = 0;
// gettok - Return the next token from standard input.
int gettok()
{
	static int LastChar = ' ';

	// Skip any whitespace.
	while (isspace(LastChar)) {
		if (LastChar == '\n') {
			linestart = ftell(input_file);
			linenumber++;
		}
		LastChar = fgetc(input_file);
	}

	if (isalpha(LastChar)) { // identifier: [a-zA-Z][a-zA-Z0-9]*
		SetStringToChar(IdentifierStr, LastChar);
		while (isalnum((LastChar = fgetc(input_file))))
			str_append(IdentifierStr, LastChar);

		if (string_compare(IdentifierStr, "def"))
			return tok_def;
		if (string_compare(IdentifierStr, "extern"))
			return tok_extern;
		if (string_compare(IdentifierStr, "if"))
			return tok_if;
		if (string_compare(IdentifierStr, "else"))
			return tok_else;
		if (string_compare(IdentifierStr, "return"))
			return tok_return;
		if (string_compare(IdentifierStr, "goto"))
			return tok_goto;
		int data_type = check_for_type(IdentifierStr);
		if (data_type != 0)
			return data_type;
		return tok_identifier;
	}

	if (isdigit(LastChar) || LastChar == '.' ||
		(LastChar == '-' && CurTok != tok_number &&
			CurTok != tok_identifier)) { // Number: [0-9.]+
		char NumStr[NUMBER_MAX] = "";

		do {
			str_append(NumStr, LastChar);
			LastChar = fgetc(input_file);
		} while (isdigit(LastChar) || LastChar == '.');

		NumVal = strtod(NumStr, 0);
		return tok_number;
	}

	// Double Operators //
	if (LastChar == '=') {
		LastChar = fgetc(input_file);
		if (LastChar != '=') // checks if it a single operator
			return '=';		 // if so just return the character
		LastChar = fgetc(input_file);
		return tok_dequ; // elsewise we know this is a double operator
	}

	if (LastChar == '!') {
		LastChar = fgetc(input_file);
		if (LastChar != '=')
			return '!';
		LastChar = fgetc(input_file);
		return tok_nequ;
	}

	if (LastChar == '<') {
		LastChar = fgetc(input_file);
		if (LastChar != '=')
			return '<';
		LastChar = fgetc(input_file);
		return tok_lte;
	}

	if (LastChar == '>') {
		LastChar = fgetc(input_file);
		if (LastChar != '=')
			return '>';
		LastChar = fgetc(input_file);
		return tok_gte;
	}

	if (LastChar == '&') {
		LastChar = fgetc(input_file);
		if (LastChar != '&')
			return '&';
		LastChar = fgetc(input_file);
		return tok_dand;
	}

	if (LastChar == '|') {
		LastChar = fgetc(input_file);
		if (LastChar != '|')
			return '|';
		LastChar = fgetc(input_file);
		return tok_dor;
	}

	if (LastChar == '#') {
		// Comment until end of line.
		do
			LastChar = fgetc(input_file);
		while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

		if (LastChar != EOF)
			return gettok(input_file);
	}

	// Check for end of file.  Don't eat the EOF.
	if (LastChar == EOF)
		return tok_eof;

	// character_return:
	// Otherwise, just return the character as its ascii value.
	int ThisChar = LastChar;
	LastChar = fgetc(input_file);
	return ThisChar;
}