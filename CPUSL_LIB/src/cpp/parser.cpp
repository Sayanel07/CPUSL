#include "../h/parser.h"


Parser::Parser(std::vector<Token> _tokens)
{
    tokens = _tokens;
    exprs["const"] = Category();
    exprs["entry"] = Category(); // TODO : Add default entry
    exprs["ps"] = Category();
}

void Parser::parse()
{
    if (category_check() == false)
        throw std::runtime_error(" "); // TODO : error file not starting with a category
    
    while (peek().type != TOK_EOF)
    {
        if (category_check())
            continue;
        std::shared_ptr<Expr> expr = expression();
        current_cat->statements.push_back(expr);
        consume(TOK_LINE_END, "Expression must end with \";\".");
    }
}

std::shared_ptr<Expr> Parser::expression(int _min_bp)
{
    Token left = advance();
    std::shared_ptr<Expr> lhs = prefix(left);
    for (;;)
    {
        Token op = peek();
        int bp = binding_power(op.type);
        if (bp < _min_bp || bp == 0)
            break;

        advance();
        int next_min_bp = is_right_associative(op.type) ? bp : bp + 1;

        std::shared_ptr<Expr> rhs = expression(next_min_bp);
        lhs = std::make_shared<BinaryExpr>(lhs, op, rhs);
    }
    return lhs;
}

void Parser::consume(TokenType _op, std::string _msg) {
    if (peek().type != _op)
        throw std::runtime_error(_msg); // TODO : improve error msg
    advance();
}

Token Parser::advance() {
    if (!is_at_file_end()) current++;
    return previous();
}

bool Parser::is_at_file_end() {
    return peek().type == TOK_EOF;
}

Token Parser::peek() {
    return tokens.at(current);
}

Token Parser::previous() {
    return tokens.at(current - 1);
}

bool Parser::is_right_associative(TokenType _type) {
    switch (_type)
    {
    case TOK_ASS:
        return true;
    case TOK_ADD:
    case TOK_SUB:
    case TOK_MUL:
    case TOK_DIV:
    case TOK_DOT:
        return false;
    default:
        throw std::runtime_error(" "); // TODO : make an error msg
    }
}

int Parser::binding_power(TokenType _type)
{
    switch(_type) {
        case TOK_ASS:
            return 1;
        case TOK_ADD:
        case TOK_SUB:
            return 2;
        case TOK_MUL:
        case TOK_DIV:
            return 3;
        case TOK_DOT:
            return 4;
        default:
            return 0;
    }
}

std::shared_ptr<Expr> Parser::prefix(Token _token)
{
    // type
    if (_token.type & TOK_TYPE)
    {

        switch (peek().type) {
        case TOK_IDENTIFIER:
        {
            std::shared_ptr<Expr> expr = init_variable(advance(), _token.type);
            if (peek().type != TOK_ASS && peek().type != TOK_LINE_END)
                throw std::runtime_error(" ");
            return expr;
        }
        case TOK_L_PAR:
            return make_func(_token);
        default:
            throw std::runtime_error(" "); // TODO : add error msg
        }
    }

    switch (_token.type)
    {
        case TOK_IMMEDIATE:
        case TOK_PI:
            return std::make_shared<LiteralExpr>(_token);
        
        case TOK_PIXEL_COLOR:
        case TOK_PIXEL_COORD:
        case TOK_UV_COORD:
        case TOK_SOURCE:
        case TOK_IDENTIFIER: {
            switch (peek().type) {
                case TOK_DOT:
                {
                    return make_accessor(std::string(_token.lexeme));
                }
            default:
                    return std::make_shared<IdentifierExpr>(_token);
            }
        }

        case TOK_DOT_PROD:     case TOK_CROSS_PROD: case TOK_MIN:  case TOK_MAX:
        case TOK_LERP:    case TOK_CLAMP: case TOK_SQRT: case TOK_COS:
        case TOK_SIN:     case TOK_TAN:   case TOK_SIZE: case TOK_SAMPLE:
        case TOK_SET:     case TOK_LENGTH:case TOK_LENGTHSQR: case TOK_NORMALIZE:
            return make_func(_token);

        case TOK_L_PAR:
        {
            std::shared_ptr<Expr> expr = expression(1);
            consume(TOK_R_PAR, " ");
            return expr;
        }
        case TOK_ADD:
        case TOK_SUB:
        {
            std::shared_ptr<Expr> expr = expression(6);
            return std::make_shared<UnaryExpr>(_token, expr);
        }
        default:
            throw std::runtime_error(" "); // TODO : add error msg
    }
}

std::shared_ptr<Expr> Parser::make_func(Token _func_token) {
    consume(TOK_L_PAR, "");
    std::vector<std::shared_ptr<Expr>> arguments;

    if (peek().type != TOK_R_PAR) {
        for (;;) {
            arguments.push_back(expression(1));
            if (peek().type == TOK_COMMA)
                advance();
            else
                break;
        }
    }
    consume(TOK_R_PAR, "");
    return std::make_shared<FunctionCallExpr>(_func_token, arguments);
}

std::shared_ptr<Expr> Parser::make_accessor(std::string _var_name)
{
    advance();
    consume(TOK_IDENTIFIER, "Expected component after \".\"");
    Token component_tok = previous();
    if (component_tok.length != 1) throw std::runtime_error("Expected x, y, z or w."); // Error
    int component_index = -1;
    switch (component_tok.lexeme[0])
    {
    case 'x': case 'r': component_index = 0; break;
    case 'y': case 'g': component_index = 1; break;
    case 'z': case 'b': component_index = 2; break;
    case 'w': case 'a': component_index = 3; break;
    default:
        throw std::runtime_error("Expected x, y, z or w."); // Error
    }

    std::shared_ptr<AccessorExpr> accessor = std::make_shared<AccessorExpr>(component_tok);
    accessor->name = _var_name;
    accessor->component_index = component_index;
    return accessor;
}

std::shared_ptr<Expr> Parser::init_variable(Token _token, TokenType _type)
{
    std::shared_ptr<DeclarationExpr> var = std::make_shared<DeclarationExpr>(_token);

    switch (_type)
    {
    case TOK_INT: var->variable_type = T_INT; break;
    case TOK_INT2: var->variable_type = T_INT2; break;
    case TOK_INT3: var->variable_type = T_INT3; break;
    case TOK_INT4: var->variable_type = T_INT4; break;
    case TOK_FLOAT: var->variable_type = T_FLOAT; break;
    case TOK_FLOAT2: var->variable_type = T_FLOAT2; break;
    case TOK_FLOAT3: var->variable_type = T_FLOAT3; break;
    case TOK_FLOAT4: var->variable_type = T_FLOAT4; break;
    case TOK_TEXTURE1D: var->variable_type = T_TEXTURE1D; break;
    case TOK_TEXTURE2D: var->variable_type = T_TEXTURE2D; break;
    default: throw std::runtime_error("[PARSER ERROR] Unknown variable type."); // Error: [PARSER ERROR] Unknown variable type. (should never happen)
    }
    
    return var;
}

bool Parser::category_check()
{
    switch (peek().type)
    {
    case TOK_CONST_CAT:
        current_cat = &exprs["const"];
        break;
    case TOK_ENTRY_CAT:
        current_cat = &exprs["entry"];
        break;
    case TOK_PS_CAT:
        current_cat = &exprs["ps"];
        break;
    default:
        return false;
    }
    
    advance();
    consume(TOK_CATEGORY_START, " ");
    return true;
}