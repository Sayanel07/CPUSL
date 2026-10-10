#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

#include <vector>
#include <string>
#include <unordered_map>

enum ExprType {
    DECLARATION,
    IDENTIFIER,
    LITERAL,
    ACCESSOR,
    UNARY,
    BINARY,
    FUNCTION,
};

enum VarType {
    T_INT,
    T_FLOAT,

    T_INT2,
    T_INT3,
    T_INT4,

    T_FLOAT2,
    T_FLOAT3,
    T_FLOAT4,

    T_TEXTURE1D,
    T_TEXTURE2D,

    T_NIL
};

enum CategoryType
{
    CAT_CONST,
    CAT_ENTRY,
    CAT_PS,
};

struct Expr {
    Token tok;
    VarType variable_type = T_NIL;
    ExprType type;
    Expr(ExprType _type, Token _tk) : type(_type), tok(_tk) {}
};

struct IdentifierExpr : Expr {
    CategoryType decl_category;
    int variable_index;
    IdentifierExpr(Token _tk) : Expr(IDENTIFIER, _tk) {}
};

struct DeclarationExpr : Expr {
    CategoryType decl_category;
    int variable_index;
    DeclarationExpr(Token _tk) : Expr(DECLARATION, _tk) {}
};

struct AccessorExpr : Expr {
    CategoryType decl_category;
    std::string name;
    uint8_t component_index; // 0 <=> x, 1 <=> y, etc.
    AccessorExpr(Token _tk) : Expr(ACCESSOR, _tk) {}
};

struct LiteralExpr : Expr {
    LiteralExpr(Token _tk) : Expr(LITERAL, _tk) {}
};

struct UnaryExpr : Expr {
    std::shared_ptr<Expr> expr;
    UnaryExpr(Token _op, std::shared_ptr<Expr> _expr) : Expr(UNARY, _op), expr(_expr) {}
};

struct BinaryExpr : Expr {
    std::shared_ptr<Expr> lhs;
    std::shared_ptr<Expr> rhs;
    BinaryExpr(std::shared_ptr<Expr> _lhs, const Token& _op, std::shared_ptr<Expr> _rhs) : Expr(BINARY, _op), lhs(_lhs), rhs(_rhs) {}
};

// TODO : Explicit name
struct FunctionCallExpr : Expr {
    std::vector<std::shared_ptr<Expr>> args;
    FunctionCallExpr(Token _name,std::vector<std::shared_ptr<Expr>> _args) : Expr(FUNCTION, _name),args(_args) {}
};

struct Category
{
    std::vector<std::shared_ptr<Expr>> statements;
};

class Parser
{
    std::vector<Token> tokens;
    int current = 0;
    Category* current_cat;
    
public:
    std::unordered_map<std::string, Category> exprs;
    Parser(std::vector<Token> _tokens);
    
    void parse();
    
    std::shared_ptr<Expr> expression(int min_bp = 1);
    
private :
    void consume(TokenType op, std::string msg);
    
    Token advance();
    
    bool is_at_file_end();
    
    Token peek();
    
    Token previous();
    
    bool is_right_associative(TokenType type);

    int binding_power(TokenType type);

    std::shared_ptr<Expr> prefix(Token token);

    std::shared_ptr<Expr> make_func(Token func_token);
    
    std::shared_ptr<Expr> make_accessor(std::string _varName);

    std::shared_ptr<Expr> init_variable(Token token, TokenType _type);
    
    bool category_check();
};

#endif