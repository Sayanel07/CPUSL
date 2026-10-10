#ifndef LEXER_H
#define LEXER_H

#include <vector>
#include <iostream>
#include <string>

enum TokenType
{
	// Identifiers
	TOK_IDENTIFIER,

	// Imm
	TOK_IMMEDIATE,

	// Categories
	TOK_CONST_CAT,
	TOK_ENTRY_CAT,
	TOK_PS_CAT,

	// Operators
	TOK_ADD,
	TOK_SUB,
	TOK_MUL,
	TOK_DIV,
	TOK_ASS,

	// Symbols
	TOK_LINE_END,
	TOK_L_PAR,
	TOK_R_PAR,
	TOK_CATEGORY_START,
	TOK_DOT,
	TOK_COMMA,
	TOK_EOF,

	TOK_PIXEL_COLOR,
	TOK_PIXEL_COORD,
	TOK_UV_COORD,
	TOK_SOURCE,

	// Functions
	TOK_DOT_PROD,
	TOK_CROSS_PROD,
	TOK_MIN,
	TOK_MAX,
	TOK_LERP,
	TOK_CLAMP,
	TOK_SQRT,
	TOK_PI,
	TOK_COS,
	TOK_SIN,
	TOK_TAN,
	TOK_SIZE,
	TOK_SAMPLE,
	TOK_SET,
	TOK_LENGTH,
	TOK_LENGTHSQR,
	TOK_NORMALIZE,

	// Types
	TOK_TYPE = 0b0100'0000,
	TOK_INT = 1			| TOK_TYPE,
	TOK_INT2 = 2		| TOK_TYPE,
	TOK_INT3 = 3		| TOK_TYPE,
	TOK_INT4 = 4		| TOK_TYPE,
	TOK_FLOAT = 5		| TOK_TYPE,
	TOK_FLOAT2 = 6		| TOK_TYPE,
	TOK_FLOAT3 = 7		| TOK_TYPE,
	TOK_FLOAT4 = 8		| TOK_TYPE,
	TOK_TEXTURE1D = 9	| TOK_TYPE,
	TOK_TEXTURE2D = 10	| TOK_TYPE,

	TOK_ERR = 0b111111111
};

struct Token
{
	std::string_view lexeme;
	TokenType type;

	int line;
	int column;

	int length;

	float value;

	friend std::ostream& operator<<(std::ostream& os, const Token& dt);
	
};

class Lexer
{
	std::string_view source;
	int source_size;
	std::vector<Token> tokens;
	std::vector<Token> error_tokens;

	int start;
	int current;

	int line;
	int column;

	bool check_keyword_tail(int _start, int _length, std::string_view _kw);

	void next();
	bool ignore_whitespace();
	char advance();
	char peek(int _offset);
	bool check_operators();
	bool check_symbols();
	bool check_immediate();

	bool get_entire_word();
	bool check_keyword();
	Token add_token(TokenType _tok);
	Token add_token(TokenType _tok, float _value);
	Token add_error_token();

	bool is_at_end();

public:
	void report_errors();

	void scan_source();
	void set_source(std::string_view _source);
	std::vector<Token> get_tokens();
};

#endif
