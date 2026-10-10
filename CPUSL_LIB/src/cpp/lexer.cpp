#include "../h/lexer.h"
#include <charconv>

#include "common.h"

// Helpers
bool is_digit(char _char)
{
	return _char >= '0' && _char <= '9';
}

bool is_letter(char _char)
{
	return _char >= 'a' && _char <= 'z' || _char >= 'A' && _char <= 'Z' || _char == '_'|| _char == '-';
}

bool is_alphanum(char _char)
{
	return _char >= '0' && _char <= '9' || _char >= 'a' && _char <= 'z' || _char >= 'A' && _char <= 'Z' || _char == '_' || _char == '-';
}

bool Lexer::check_keyword_tail(int _start, int _length, std::string_view _kw) {
	if (current - start == _start + _length && 
		_kw.substr(_start, _length).compare(source.substr(start + _start, current - start - _start)) == 0)
	{
		return true;
	}

	return false;
}

std::string token_type_to_string(TokenType _type)
{
	switch (_type)
	{
	case TOK_IDENTIFIER: return "Identifier";
	case TOK_IMMEDIATE: return "Immediate";
	case TOK_CONST_CAT: return "Constant Category";
	case TOK_ENTRY_CAT: return "Entry Category";
	case TOK_PS_CAT: return "PS Category";
	case TOK_ADD: return "Add";
	case TOK_SUB: return "Subtract";
	case TOK_MUL: return "Multiply";
	case TOK_DIV: return "Divide";
	case TOK_ASS: return "Assign";
	case TOK_LINE_END: return "Line end";
	case TOK_L_PAR: return "Left Parenthesis";
	case TOK_R_PAR: return "Right Parenthesis";
	case TOK_CATEGORY_START: return "Category Start";
	case TOK_DOT: return "Access";
	case TOK_COMMA: return "Comma";
	case TOK_DOT_PROD: return "Dot product";
	case TOK_CROSS_PROD: return "Cross product";
	case TOK_MIN: return "Min";
	case TOK_MAX: return "Max";
	case TOK_LERP: return "Lerp";
	case TOK_CLAMP: return "Clamp";
	case TOK_SQRT: return "Square root";
	case TOK_PI: return "Pi";
	case TOK_COS: return "Cos";
	case TOK_SIN: return "Sin";
	case TOK_TAN: return "Tan";
	case TOK_SIZE: return "Size";
	case TOK_SAMPLE: return "Sample";
	case TOK_SET: return "Set";
	case TOK_LENGTH: return "Length";
	case TOK_LENGTHSQR: return "Length Squared";
	case TOK_NORMALIZE: return "Normalize";
	case TOK_FLOAT: return "Float";
	case TOK_FLOAT2: return "Float 2";
	case TOK_FLOAT3: return "Float 3";
	case TOK_FLOAT4: return "Float 4";
	case TOK_TEXTURE1D: return "Texture 1D";
	case TOK_TEXTURE2D: return "Texture 2D";
	case TOK_EOF: return "End of file";
	default: return "Unknown Token";
	}
}

void Lexer::report_errors()
{
	for (int i = 0; i < error_tokens.size(); i++)
	{
		std::cout << "Lexing error l." << error_tokens[i].line << " c." << error_tokens[i].column << ": Unknown token " << error_tokens[i].lexeme << std::endl;
	}
}

void Lexer::scan_source()
{
	while(is_at_end() == false)
	{
		next();
	} 

	start = current;
	add_token(TOK_EOF);

	report_errors();
}

void Lexer::set_source(std::string_view _source)
{
	line = 0;
	column = 0;

	start = 0;
	current = start;

	source = _source;
	source_size = source.size();

	tokens.clear();
	error_tokens.clear();
}

std::vector<Token> Lexer::get_tokens()
{
	return tokens;
}

void Lexer::next()
{
	// Clear whitespace
	if(ignore_whitespace()) return;
	// Ignore Comments?

	start = current;

	// Check one char tokens
	if (check_operators()) return;
	if (check_symbols()) return;

	// Check immediates
	if (check_immediate()) return;

	// Check types/identifiers
	if (get_entire_word()) return; // Breaks if token is invalid

	if (check_keyword()) return;

	add_token(TOK_IDENTIFIER);
}

bool Lexer::ignore_whitespace()
{
	for (;;)
	{
		switch (peek(0))
		{
		case '\n':	// line break
			line++;
			column = 0;
			[[fallthrough]];
		case ' ':	// space
		case '	':	// tab
			advance();
			if (is_at_end()) return true;
			break;

		default:

			return false;
		}
	}
	
}

char Lexer::advance()
{
	column++;
	return source.at(current++);
}

char Lexer::peek(int _offset = 0)
{
	return source.at(current + _offset);
}

bool Lexer::check_operators()
{
	switch (peek(0))
	{
	case '+':
		advance();
		add_token(TOK_ADD);
		return true;
	case '-':
		advance();
		add_token(TOK_SUB);
		return true;
	case '*':
		advance();
		add_token(TOK_MUL);
		return true;
	case '/':
		advance();
		add_token(TOK_DIV);
		return true;
	case '=':
		advance();
		add_token(TOK_ASS);
		return true;

	default:
		return false;
	}
}

bool Lexer::check_symbols()
{
	switch (peek())
	{
	case ';':
		advance();
		add_token(TOK_LINE_END);
		return true;
	case ':':
		advance();
		add_token(TOK_CATEGORY_START);
		return true;
	case '(':
		advance();
		add_token(TOK_L_PAR);
		return true;
	case ')':
		advance();
		add_token(TOK_R_PAR);
		return true;
	case '.':
		advance();
		add_token(TOK_DOT);
		return true;
	case ',':
		advance();
		add_token(TOK_COMMA);
		return true;

	default:
		return false;
	}
}

bool Lexer::check_immediate()
{
	if (is_digit(peek(0)))
	{
		advance();
		if (is_at_end())
		{
			float val;
			std::from_chars(source.data() + start, source.data() + current, val);
			add_token(TOK_IMMEDIATE, val);
			return true;
		}

		while (is_digit(peek(0)))
		{
			advance();
			if (is_at_end())
			{
				float val;
				std::from_chars(source.data() + start, source.data() + current, val);
				add_token(TOK_IMMEDIATE, val);
				return true;
			}
		}

		if (peek(0) == '.')
		{
			advance();
			if (is_at_end())
			{
				float val;
				std::from_chars(source.data() + start, source.data() + current, val);
				add_token(TOK_IMMEDIATE, val);
				return true;
			}
			while (is_digit(peek(0)))
			{
				advance();
				if (is_at_end())
				{
					float val;
					std::from_chars(source.data() + start, source.data() + current, val);
					add_token(TOK_IMMEDIATE, val);
					return true;
				}
			}
		}

		float val;
		std::from_chars(source.data() + start, source.data() + current, val);
		add_token(TOK_IMMEDIATE, val);
		return true;
	}
	
	return false;
}

bool Lexer::get_entire_word()
{
	if (is_letter(peek(0)))
	{
		advance();

		if (is_at_end()) return false;

		while (is_alphanum(peek()))
		{
			advance();
			if (is_at_end()) return false;
		}

		return false; // VALID
	}

	advance();
	Token err_tok = add_error_token();
	return true; // INVALID
}

bool Lexer::check_keyword()
{
	char c = source.at(start);

	switch (c)
	{
	case 'c':
	{
		if (current <= start + 1) return false;
		c = source.at(start + 1);
		switch (c)
		{
		case 'l': if (check_keyword_tail(2, 3, "clamp")) { add_token(TOK_CLAMP);  return true; }; break;
		case 'o': if (check_keyword_tail(2, 1, "cos")) { add_token(TOK_COS);  return true; }; break;
		case 'r': if (check_keyword_tail(2, 11, "cross_product")) { add_token(TOK_CROSS_PROD);  return true; }; break;
		}
		break;
	}
	case 'd':
	{
		if (check_keyword_tail(1, 10, "dot_product")) { add_token(TOK_DOT_PROD);  return true; };
		break;
	}
	case 'f':
	{
		if (current - start == 6)
		{
			if (source.substr(start, 5) == "float") // floatX
			{
				switch (source.at(start + 5))
				{
				case '2': { add_token(TOK_FLOAT2);  return true; }
				case '3': { add_token(TOK_FLOAT3);  return true; }
				case '4': { add_token(TOK_FLOAT4);  return true; }
				default: break;
				}
			}
		}
		else
		{
			if (check_keyword_tail(2, 3, "float")) { add_token(TOK_FLOAT);  return true; }; break; // float tout court
		}
		break;
	}
	case 'i':
	{
		switch (current - start)
		{
		case 3: if (check_keyword_tail(1, 2, "int")) { add_token(TOK_INT); return true; }; break; // int tout court
		case 4:
			if (source.substr(start, 3) == "int") // intX
			{
				switch (source.at(start + 3))
				{
				case '2': { add_token(TOK_INT2);  return true; }
				case '3': { add_token(TOK_INT3);  return true; }
				case '4': { add_token(TOK_INT4);  return true; }
				default: break;
				}
			}
			break;
		}
		break;
	}
	case 'l':
	{
		if (current <= start + 1) return false;
		c = source.at(start + 1);
		switch (c)
		{
		case 'e':
		{
			if (current <= start + 2) return false;
			c = source.at(start + 2);
			switch (c)
			{
			case 'r': if (check_keyword_tail(3, 1, "lerp")) { add_token(TOK_LERP);  return true; }; break;
			case 'n':
			{
				if (check_keyword_tail(3, 7, "length_sqr")) { add_token(TOK_LENGTHSQR);  return true; }
				else if (check_keyword_tail(3, 3, "length")) { add_token(TOK_LENGTH);  return true; }
				break;
			}
			}
			break;
		}
		}
		break;
	}
	case 'm':
	{
		if (current <= start + 1) return false;
		c = source.at(start + 1);
		switch (c)
		{
		case 'a': if (check_keyword_tail(2, 1, "max")) { add_token(TOK_MAX);  return true; }; break;
		case 'i': if (check_keyword_tail(2, 1, "min")) { add_token(TOK_MIN);  return true; }; break;
		}
		break;
	}
	case 'n':
	{
		if (check_keyword_tail(1, 9, "normalize")) { add_token(TOK_NORMALIZE);  return true; };
		break;
	}
	case 'p':
	{
		if (current <= start + 1) return false;
		c = source.at(start + 1);

		if (c == 'i')
		{
			if (start + 2 == current) { add_token(TOK_PI); tokens.back().value = M_PI; return true; };
			
			if (check_keyword_tail(2, 9, "pixel_color")) { add_token(TOK_PIXEL_COLOR);  return true; };
			if (check_keyword_tail(2, 9, "pixel_coord")) { add_token(TOK_PIXEL_COORD);  return true; };
		}
		
		break;
	}
	case 's':
	{
		if (current <= start + 1) return false;
		c = source.at(start + 1);
		switch (c)
		{
		case 'a': if (check_keyword_tail(2, 4, "sample")) { add_token(TOK_SAMPLE);  return true; }; break;
		case 'e': if (check_keyword_tail(2, 1, "set")) { add_token(TOK_SET);  return true; }; break;
		case 'i':
		{
			if (current <= start + 2) return false;
			c = source.at(start + 2);
			switch (c)
			{
			case 'n': if (check_keyword_tail(3, 0, "sin")) { add_token(TOK_SIN);  return true; }; break;
			case 'z': if (check_keyword_tail(3, 1, "size")) { add_token(TOK_SIZE);  return true; }; break;
			}
			break;
		}
		case 'o': if (check_keyword_tail(2, 4, "source")) { add_token(TOK_SOURCE);  return true; }; break;
		case 'q': if (check_keyword_tail(2, 2, "sqrt")) { add_token(TOK_SQRT);  return true; }; break;
		}
		break;
	}
	case 't':
	{
		if (current <= start + 1) return false;
		c = source.at(start + 1);
		switch (c)
		{
			case 'a': if (check_keyword_tail(2, 1, "tan")) { add_token(TOK_TAN);  return true; }; break;
			case 'e':
			{
				if (source.substr(start, 7) == "texture") // textureX
				{
					if (source.at(start + 8) != 'd') break; // does not end with d
					switch (source.at(start + 7))
					{
					case '1':  { add_token(TOK_TEXTURE1D);  return true; }
					case '2':  { add_token(TOK_TEXTURE2D);  return true; }
					default: break;
					}
				}
			}

		}

		
		break;
	}
	case 'u':
	{
		if (check_keyword_tail(1, 7, "uv_coord")) { add_token(TOK_UV_COORD);  return true; }; break;
	}

	case 'C':
	{
		if (check_keyword_tail(1, 4, "CONST")) { add_token(TOK_CONST_CAT);  return true; }; break;
		break;
	}
	case 'E':
	{
		if (check_keyword_tail(1, 4, "ENTRY")) { add_token(TOK_ENTRY_CAT);  return true; }; break;
		break;
	}
	case 'P':
	{
		if (check_keyword_tail(1, 1, "PS")) { add_token(TOK_PS_CAT);  return true; }; break;
		break;
	}
	default:
		return false;
	}

	return false;
}

Token Lexer::add_token(TokenType _tok)
{
	Token tok;

	tok.lexeme = source.substr(start, current - start);
	tok.length = current - start;

	tok.line = line;
	tok.column = column - tok.length;

	tok.type = _tok;

	tokens.push_back(tok);

	return tok;
}

Token Lexer::add_token(TokenType _tok, float _value)
{
	Token tok;

	tok.lexeme = source.substr(start, current - start);
	tok.length = current - start;

	tok.line = line;
	tok.column = column - tok.length;

	tok.type = _tok;
	tok.value = _value;

	tokens.push_back(tok);

	return tok;
}

Token Lexer::add_error_token()
{
	Token tok;

	tok.lexeme = source.substr(start, current - start);
	tok.length = current - start;

	tok.line = line;
	tok.column = column - tok.length;

	tok.type = TOK_ERR;

	tokens.push_back(tok);
	error_tokens.push_back(tok);

	return tok;
}

bool Lexer::is_at_end()
{
	return current >= source_size;
}

std::ostream& operator<<(std::ostream& os, const Token& dt)
{
	os << "\"" << dt.lexeme << "\" " << token_type_to_string(dt.type) << " At l." << dt.line << " c." << dt.column;
	return os;
}
