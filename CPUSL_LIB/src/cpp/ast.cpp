#include "ast.h"

#include "common.h"
#include <algorithm>

void AST::check_semantics()
{
	Category const_category = exprs["const"];
	Category entry_category = exprs["entry"];
	Category ps_category = exprs["ps"];

    check_category(const_category, CAT_CONST);
    check_category(entry_category, CAT_ENTRY);
    check_category(ps_category, CAT_PS);
    display_errors();
}

void AST::register_error(std::string _error_text, int _line, int _column)
{
    std::string error_text_final = "[Semantic Error] (l." + std::to_string(_line) + ", c." + std::to_string(_column) + "): ";
    error_text_final += _error_text;
    errors.push_back(error_text_final);
}

void AST::display_errors()
{
    int size = errors.size();
    for (int i = 0; i < size; i++)
    {
        std::cout << errors[i] << std::endl;
    }
}

bool AST::has_errors()
{
    return errors.size() > 0;
}

void AST::check_category(const Category& _category, CategoryType _type)
{
    int size = _category.statements.size();
    for (int i = 0; i < size; i++)
    {
        std::shared_ptr<Expr> statement = _category.statements[i];

        check_expression(statement, _type);
    }
}

bool AST::check_expression(std::shared_ptr<Expr> _statement, CategoryType _type)
{
    std::vector<VariableInfo>* category_variables = nullptr;
    switch (_type)
    {
    case CAT_CONST: category_variables = &const_variables; break;
    case CAT_ENTRY: 
        category_variables = &entry_variables; 
        if (_statement->type != DECLARATION) { register_error("Entry category is for declarations only.", _statement->tok.line, _statement->tok.column); return false; }
        break;
    case CAT_PS: category_variables = &variables; break;
    default:
        return false;
    }

    VarType var_type;
    CategoryType cat_type;

    switch (_statement->type)
    {
    case DECLARATION:
    {
        const auto declaration_expr = std::static_pointer_cast<DeclarationExpr>(_statement);

        if (does_variable_exist(std::string(declaration_expr->tok.lexeme), _type, var_type, cat_type) == true)
        {
            register_error("Variable \"" + std::string(declaration_expr->tok.lexeme) + "\" already declared.", declaration_expr->tok.line, declaration_expr->tok.column);
            return false;
        }

        if (declaration_expr->variable_type == T_TEXTURE1D || declaration_expr->variable_type == T_TEXTURE2D)
        {
            if (_type != CAT_ENTRY)
            {
                register_error("Cannot declare a texture out of ENTRY.", declaration_expr->tok.line, declaration_expr->tok.column);
            }

            entry_textures.push_back({ std::string(declaration_expr->tok.lexeme), declaration_expr->variable_type });
        }
        else
        {
            category_variables->push_back({ std::string(declaration_expr->tok.lexeme), declaration_expr->variable_type });
        }
        declaration_expr->decl_category = _type;
        

        break;
    }

    case LITERAL:
    {
        const auto literal_expr = std::static_pointer_cast<LiteralExpr>(_statement);
        literal_expr->variable_type = T_FLOAT;
        if(_type == CAT_PS) place_constant(literal_expr->tok.value);
        break;
    }

    case IDENTIFIER:
    {
        const auto identifier_expr = std::static_pointer_cast<IdentifierExpr>(_statement);
        
        if (does_variable_exist(std::string(identifier_expr->tok.lexeme), _type, var_type, cat_type) == false)
        {
            register_error("Unknown identifier \"" + std::string(identifier_expr->tok.lexeme) + "\".", identifier_expr->tok.line, identifier_expr->tok.column);
            return false;
        }

        identifier_expr->variable_type = var_type;
        identifier_expr->decl_category = cat_type;
        break;
    }

    case ACCESSOR:
    {
        const auto accessor_expr = std::static_pointer_cast<AccessorExpr>(_statement);
        if (does_variable_exist(accessor_expr->name, _type, var_type, cat_type) == false)
        {
            register_error("Unknown identifier \"" + std::string(accessor_expr->tok.lexeme) + "\".", accessor_expr->tok.line, accessor_expr->tok.column);
            return false;
        }
        switch (var_type)
        {
        case T_FLOAT2:
        case T_FLOAT3:
        case T_FLOAT4:
            accessor_expr->variable_type = T_FLOAT;
            break;
        case T_INT2:
        case T_INT3:
        case T_INT4:
            accessor_expr->variable_type = T_INT;
            break;
        }

        accessor_expr->decl_category = cat_type;
        break;
    }

    case UNARY:
    {
        const auto unary_expr = std::static_pointer_cast<UnaryExpr>(_statement);
        if (check_expression(unary_expr->expr, _type) == false) return false;
        unary_expr->variable_type = unary_expr->expr->variable_type;
        break;
    }

    case BINARY:
    {
        const auto binary_expr = std::static_pointer_cast<BinaryExpr>(_statement);

        bool valid = true;
        if (check_expression(binary_expr->lhs, _type) == false) valid = false;
        if (check_expression(binary_expr->rhs, _type) == false) valid = false;
        if (valid == false) return false;

        switch (binary_expr->tok.type)
        {
        case TOK_ASS:
        {
            if (binary_expr->lhs->type == DECLARATION)
            {
                const auto left_expr = std::static_pointer_cast<DeclarationExpr>(binary_expr->lhs);
                if (left_expr->decl_category == CAT_ENTRY)
                {
                    register_error("Cannot modify " + std::string(binary_expr->lhs->tok.lexeme) + ".", binary_expr->lhs->tok.line, binary_expr->lhs->tok.column);
                    return false;
                };
            }
            else if (binary_expr->lhs->type == IDENTIFIER)
            {
                const auto left_expr = std::static_pointer_cast<IdentifierExpr>(binary_expr->lhs);
                if (left_expr->decl_category != CAT_PS)
                {
                    register_error("Cannot modify " + std::string(binary_expr->lhs->tok.lexeme) + ".", binary_expr->lhs->tok.line, binary_expr->lhs->tok.column);
                    return false;
                };
            }
            else if (binary_expr->lhs->type == ACCESSOR)
            {
                const auto left_expr = std::static_pointer_cast<AccessorExpr>(binary_expr->lhs);
                if (left_expr->decl_category != CAT_PS)
                {
                    register_error("Cannot modify " + std::string(binary_expr->lhs->tok.lexeme) + ".", binary_expr->lhs->tok.line, binary_expr->lhs->tok.column);
                    return false;
                };
            }
            else
            {
                register_error("Cannot modify " + std::string(binary_expr->lhs->tok.lexeme) + ".", binary_expr->lhs->tok.line, binary_expr->lhs->tok.column);
                return false;
            }

            if (check_binary_exclusive(binary_expr) == false) return false;

            if (_type == CAT_CONST)
            {
                VariableInfo* const_info = get_id_by_name(std::string(binary_expr->lhs->tok.lexeme));
                const_info->precomputed_value = precompute_const_statement(binary_expr->rhs);
                place_constant(const_info->precomputed_value);
            }

            break;
        }
        case TOK_ADD:
        {
            if (check_binary_exclusive(binary_expr) == false) return false;
            break;
        }
        case TOK_SUB:
        {
            if (check_binary_exclusive(binary_expr) == false) return false;
            break;
        }
        case TOK_MUL:
        {
            if (check_binary_inclusive(binary_expr) == false) return false;
            break;
        }
        case TOK_DIV:
        {
            if (check_binary_inclusive(binary_expr) == false) return false;
            break;
        }

        }

        break;
    }

    case FUNCTION:
    {
        const auto function_expr = std::static_pointer_cast<FunctionCallExpr>(_statement);

        // Check argument validity
        bool valid = true;
        int arg_count = function_expr->args.size();
        for (int i = 0; i < arg_count; i++)
        {
            if (check_expression(function_expr->args[i], _type) == false) valid = false;
        }
        if (valid == false) return false;

        if (check_function_expectations(function_expr) == false)
            return false;

        break;
    }
    }

    return true;
}

Float4 AST::precompute_const_statement(std::shared_ptr<Expr> _statement)
{
    Float4 res;

    switch (_statement->type)
    {
    case LITERAL:
    {
        res = _statement->tok.value;
        break;
    }
    case BINARY:
    {
        const auto binary_expr = std::static_pointer_cast<BinaryExpr>(_statement);
        switch (_statement->tok.type)
        {
        case TOK_ADD:
        {
            Float4 left;
            Float4 right;

            left = precompute_const_statement(binary_expr->lhs);
            right = precompute_const_statement(binary_expr->rhs);

            res = left + right;
            break;
        }
        case TOK_SUB:
        {
            Float4 left;
            Float4 right;

            left = precompute_const_statement(binary_expr->lhs);
            right = precompute_const_statement(binary_expr->rhs);

            res = left - right;
            break;
        }
        case TOK_MUL:
        {
            Float4 left;
            Float4 right;

            left = precompute_const_statement(binary_expr->lhs);
            right = precompute_const_statement(binary_expr->rhs);

            res = left * right;
            break;
        }
        case TOK_DIV:
        {
            Float4 left;
            Float4 right;

            left = precompute_const_statement(binary_expr->lhs);
            right = precompute_const_statement(binary_expr->rhs);

            res = left / right;
            break;
        }

        }
        break;
    }
    case UNARY:
    {
        const auto unary_expr = std::static_pointer_cast<UnaryExpr>(_statement);
        switch (_statement->tok.type)
        {
        case TOK_ADD:
        {
            res = precompute_const_statement(unary_expr->expr);
            break;
        }
        case TOK_SUB:
        {
            res = precompute_const_statement(unary_expr->expr);
            res.x = -res.x;
            break;
        }
        }
        break;
    }
    case IDENTIFIER:
    {
        const auto identifier_expr = std::static_pointer_cast<IdentifierExpr>(_statement);
        switch (identifier_expr->decl_category)
        {
        case CAT_CONST:

            res = get_id_by_name(std::string(identifier_expr->tok.lexeme))->precomputed_value;
            break;
        }
        break;
    }

    case FUNCTION:
    {
        const auto function_expr = std::static_pointer_cast<FunctionCallExpr>(_statement);

        switch (function_expr->tok.type)
        {
        case TOK_COS:
        {
            res = cos(precompute_const_statement(function_expr->args[0]).x);
            break;
        }
        case TOK_SIN:
        {
            res = sin(precompute_const_statement(function_expr->args[0]).x);
            break;
        }
        case TOK_TAN:
        {
            res = tan(precompute_const_statement(function_expr->args[0]).x);
            break;
        }
        case TOK_CLAMP:
        {
            res = std::clamp(precompute_const_statement(function_expr->args[0]).x, precompute_const_statement(function_expr->args[1]).x, precompute_const_statement(function_expr->args[2]).x);
            break;
        }
        case TOK_DOT_PROD:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            Float4 arg2 = precompute_const_statement(function_expr->args[1]);
            
            res = (arg1.x * arg2.x) + (arg1.y * arg2.y) + (arg1.z * arg2.z) + (arg1.w * arg2.w);
            break;
        }
        case TOK_CROSS_PROD:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            Float4 arg2 = precompute_const_statement(function_expr->args[1]);
            
            res.x = (arg1.y * arg2.z) - (arg1.z * arg2.y);
            res.y = (arg1.z * arg2.x) - (arg1.x * arg2.z);
            res.z = (arg1.x * arg2.y) - (arg1.y * arg2.x);
            break;
        }
        case TOK_PI:
        {
            res.x = res.y = res.z = res.w = M_PI;
            break;
        }
        case TOK_FLOAT2:
        case TOK_INT2:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            Float4 arg2 = precompute_const_statement(function_expr->args[1]);

            res.x = arg1.x;
            res.y = arg2.x;
            break;
        }
        case TOK_FLOAT3:
        case TOK_INT3:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            Float4 arg2 = precompute_const_statement(function_expr->args[1]);
            Float4 arg3 = precompute_const_statement(function_expr->args[2]);

            res.x = arg1.x;
            res.y = arg2.x;
            res.z = arg3.x;
            break;
        }
        case TOK_FLOAT4:
        case TOK_INT4:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            Float4 arg2 = precompute_const_statement(function_expr->args[1]);
            Float4 arg3 = precompute_const_statement(function_expr->args[2]);
            Float4 arg4 = precompute_const_statement(function_expr->args[3]);

            res.x = arg1.x;
            res.y = arg2.x;
            res.z = arg3.x;
            res.w = arg4.x;
            break;
        }

        case TOK_SQRT:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            res = sqrt(arg1.x);
            break;
        }
        case TOK_LENGTH:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            res = sqrt((arg1.x * arg1.x) + (arg1.y * arg1.y) + (arg1.z * arg1.z) + (arg1.w * arg1.w));
            break;
        }
        case TOK_LENGTHSQR:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            res = (arg1.x * arg1.x) + (arg1.y * arg1.y) + (arg1.z * arg1.z) + (arg1.w * arg1.w);
            break;
        }
        case TOK_LERP:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            Float4 arg2 = precompute_const_statement(function_expr->args[1]);
            Float4 arg3 = precompute_const_statement(function_expr->args[2]);
            res = (arg1.x * (1 - arg3.x) + arg2.x * arg3.x);
            break;
        }
        case TOK_MIN:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            Float4 arg2 = precompute_const_statement(function_expr->args[1]);
            res = std::min(arg1.x, arg2.x);
            break;
        }
        case TOK_MAX:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            Float4 arg2 = precompute_const_statement(function_expr->args[1]);
            res = std::max(arg1.x, arg2.x);
            break;
        }
        case TOK_NORMALIZE:
        {
            Float4 arg1 = precompute_const_statement(function_expr->args[0]);
            float norm = sqrt((arg1.x * arg1.x) + (arg1.y * arg1.y) + (arg1.z * arg1.z) + (arg1.w * arg1.w));

            res = res / norm;
            break;
        }
        }

        break;
    }
    }
    return res;
}

void AST::place_constant(Float4 _const)
{
    int size = const_variables.size();
    for (int i = 0; i < size; i++)
    {
        if (const_variables[i].precomputed_value == _const && const_variables[i].type != T_FLOAT && const_variables[i].type != T_INT) return; //already in table
    }

    const_variables.push_back({" ", T_FLOAT4, _const});
}

void AST::place_constant(float _const)
{
    int size = const_variables.size();
    for (int i = 0; i < size; i++)
    {
        if (const_variables[i].precomputed_value == _const && (const_variables[i].type == T_FLOAT || const_variables[i].type == T_INT)) return; //already in table
    }

    const_variables.push_back({ " ", T_FLOAT, _const });
}

uint16_t AST::get_constant_offset(Float4 _const)
{
    bool found = false;
    size_t offset = 0;
    int size = const_variables.size();
    for (int i = 0; i < size; i++)
    {
        if (const_variables[i].precomputed_value == _const && const_variables[i].type != T_FLOAT && const_variables[i].type != T_INT)
        {
            found = true;
            break;
        }
        if (const_variables[i].is_scalar()) offset += 4;
        else offset += 16;
    }

    if (found == false)
    {
        place_constant(_const);
    }

    return offset;
}

uint16_t AST::get_constant_offset(float _const)
{
    bool found = false;
    size_t offset = 0;
    int size = const_variables.size();
    for (int i = 0; i < size; i++)
    {
        if (const_variables[i].precomputed_value == _const && (const_variables[i].type == T_FLOAT || const_variables[i].type == T_INT))
        {
            found = true;
            break;
        }
        if (const_variables[i].is_scalar()) offset += 4;
        else offset += 16;
    }

    if (found == false)
    {
        place_constant(_const);
    }

    return offset;
}

bool AST::check_function_expectations(std::shared_ptr<FunctionCallExpr> _statement)
{
    int argCount = function_expectations[std::string(_statement->tok.lexeme)][0].arguments.size();
    if (_statement->args.size() != argCount)
    {
        register_error("Function takes " + std::to_string(argCount) + " arguments.", _statement->tok.line, _statement->tok.column);
        return false;
    }

    int expectationSize = function_expectations[std::string(_statement->tok.lexeme)].size();

    for (int j = 0; j < expectationSize; j++)
    {
        FunctionExpectation exp = function_expectations[std::string(_statement->tok.lexeme)][j];
        
        bool valid = true;
        for (int i = 0; i < argCount; i++)
        {
            if (compare_types(exp.arguments[i], _statement->args[i]->variable_type) == false)
            {
                valid = false;
            }
        }
        if (valid == true)
        {
            _statement->variable_type = exp.return_type;
            return true;
        }
    }

    register_error("Unexpected type for arguments.", _statement->tok.line, _statement->tok.column);
    return false;
}

VariableInfo* AST::get_id_by_name(std::string _name)
{
    int size = const_variables.size();
    for (int i = 0; i < size; i++)
    {
        if (const_variables[i].name == _name)
        {
            return &const_variables[i];
        }
    }

    size = entry_variables.size();
    for (int i = 0; i < size; i++)
    {
        if (entry_variables[i].name == _name)
        {
            return &entry_variables[i];
        }
    }

    size = variables.size();
    for (int i = 0; i < size; i++)
    {
        if (variables[i].name == _name)
        {
            return &variables[i];
        }
    }

    return nullptr;
}

uint16_t AST::get_entry_size()
{
    uint16_t size = 0;
    for (int i = 0; i < entry_variables.size(); i++)
    {
        if (entry_variables[i].is_scalar()) size += 4;
        else size += 16;
    }

    return size;
}


// If in const: Only check const variables
// If in entry: Check const AND entry variables
// If in ps: Check const AND ps AND entry variables
bool AST::does_variable_exist(std::string _var_name, CategoryType _type, VarType& _var_type, CategoryType& _decl_type)
{
    _var_type = T_NIL;
    switch (_type)
    {
        case CAT_CONST: 
        {
            for (int i = 0; i < const_variables.size(); i++)
            {
                if (const_variables[i].name == _var_name)
                {
                    _var_type = const_variables[i].type;
                    _decl_type = CAT_CONST;
                    return true;
                }
            }

            return false;
        }
        case CAT_ENTRY:
        {
            for (int i = 0; i < const_variables.size(); i++)
            {
                if (const_variables[i].name == _var_name)
                {
                    _var_type = const_variables[i].type;
                    _decl_type = CAT_CONST;
                    return true;
                }
            }
            for (int i = 0; i < entry_variables.size(); i++)
            {
                if (entry_variables[i].name == _var_name)
                {
                    _var_type = entry_variables[i].type;
                    _decl_type = CAT_ENTRY;
                    return true;
                }
            }
            for (int i = 0; i < entry_textures.size(); i++)
            {
                if (entry_textures[i].name == _var_name)
                {
                    _var_type = entry_textures[i].type;
                    _decl_type = CAT_ENTRY;
                    return true;
                }
            }

            return false;
        }
        case CAT_PS: 
        {
            if (_var_name == "pixel_color")
            {
                _var_type = T_FLOAT4;
                _decl_type = CAT_PS;
                return true;
            }
            if (_var_name == "pixel_coord")
            {
                _var_type = T_FLOAT2;
                _decl_type = CAT_PS;
                return true;
            }
            if (_var_name == "uv_coord")
            {
                _var_type = T_FLOAT2;
                _decl_type = CAT_PS;
                return true;
            }
            if (_var_name == "source")
            {
                _var_type = T_TEXTURE2D;
                _decl_type = CAT_PS;
                return true;
            }

            for (int i = 0; i < const_variables.size(); i++)
            {
                if (const_variables[i].name == _var_name)
                {
                    _var_type = const_variables[i].type;
                    _decl_type = CAT_CONST;
                    return true;
                }
            }
            for (int i = 0; i < entry_variables.size(); i++)
            {
                if (entry_variables[i].name == _var_name)
                {
                    _var_type = entry_variables[i].type;
                    _decl_type = CAT_ENTRY;
                    return true;
                }
            }
            for (int i = 0; i < entry_textures.size(); i++)
            {
                if (entry_textures[i].name == _var_name)
                {
                    _var_type = entry_textures[i].type;
                    _decl_type = CAT_ENTRY;
                    return true;
                }
            }
            for (int i = 0; i < variables.size(); i++)
            {
                if (variables[i].name == _var_name)
                {
                    _var_type = variables[i].type;
                    _decl_type = CAT_PS;
                    return true;
                }
            }

            return false;
        }
    }

    return false;
}

// Call this only once tree has been walked
uint16_t AST::get_variable_offset(std::string _var_name)
{
    if (_var_name == "pixel_color") return 0;
    if (_var_name == "pixel_coord") return 16;
    if (_var_name == "uv_coord") return 32;

    uint16_t offset = 48;
    for (int i = 0; i < entry_variables.size(); i++)
    {
        if (entry_variables[i].name == _var_name)
        {
            return offset;
        }

        if(entry_variables[i].is_scalar())offset += 4;
        else offset += 16;
    }

    for (int i = 0; i < variables.size(); i++)
    {
        if (variables[i].name == _var_name)
        {
            return offset;
        }

        if (variables[i].is_scalar())offset += 4;
        else offset += 16;
    }

    throw std::runtime_error("Couldn't find variable (This shouldn't happen)");
    return -1;
}

uint8_t AST::get_texture_index(std::string _var_name)
{
    if (_var_name == "source") return 0;

    uint16_t index = 1;

    for (int i = 0; i < entry_textures.size(); i++)
    {
        if (entry_textures[i].name == _var_name)
        {
            return index;
        }
        index++;
    }

    throw std::runtime_error("Couldn't find variable (This shouldn't happen)");
    return -1;
}

uint16_t AST::get_constant_offset_by_name(std::string _var_name)
{
    uint16_t offset = 0;
    for (int i = 0; i < const_variables.size(); i++)
    {
        if (const_variables[i].name == _var_name)
        {
            return offset;
        }
        if (const_variables[i].is_scalar())offset += 4;
        else offset += 16;
    }

    throw std::runtime_error("Couldn't find variable (This shouldn't happen)");
    return -1;
}

CategoryType AST::get_category_by_name(std::string _var_name)
{
    for (int i = 0; i < const_variables.size(); i++)
    {
        if (const_variables[i].name == _var_name)
        {
            return CAT_CONST;
        }
    }
    for (int i = 0; i < entry_variables.size(); i++)
    {
        if (entry_variables[i].name == _var_name)
        {
            return CAT_ENTRY;
        }
    }
    for (int i = 0; i < entry_textures.size(); i++)
    {
        if (entry_textures[i].name == _var_name)
        {
            return CAT_ENTRY;
        }
    }
    for (int i = 0; i < variables.size(); i++)
    {
        if (variables[i].name == _var_name)
        {
            return CAT_PS;
        }
    }

    return CategoryType();
}

bool AST::compare_types(VarType _prio, VarType _second)
{
    switch (_prio)
    {
        case T_INT:
        case T_FLOAT:
        {
            if (_second != T_FLOAT && _second != T_INT) return false;
            return true;
        }
        case T_INT2:
        case T_FLOAT2:
        {
            if (_second != T_FLOAT2 && _second != T_INT2) return false;
            return true;
        }
        case T_INT3:
        case T_FLOAT3:
        {
            if (_second != T_FLOAT3 && _second != T_INT3) return false;
            return true;
        }
        case T_INT4:
        case T_FLOAT4:
        {
            if (_second != T_FLOAT4 && _second != T_INT4) return false;
            return true;
        }

        case T_TEXTURE1D: return _second == T_TEXTURE1D;
        case T_TEXTURE2D: return _second == T_TEXTURE2D;
        case T_NIL: return _second == T_NIL;

        default :
        {
            return false;
        }
    }
}

bool AST::check_binary_exclusive(std::shared_ptr<BinaryExpr> _expr)
{
    VarType l_type = _expr->lhs->variable_type;
    VarType r_type = _expr->rhs->variable_type;
    switch (l_type)
    {
    case T_FLOAT:
    case T_INT:
    {
        if (r_type != T_INT && r_type != T_FLOAT)
        {
            // ERROR: Cannot assign [r_type] / [l_type].
            register_error("Types ... and ... incompatible.", _expr->lhs->tok.line, _expr->lhs->tok.column);
            return false;
        }

        _expr->variable_type = T_FLOAT;
        break;
    }
    case T_FLOAT2:
    case T_INT2:
    {
        if (r_type != T_INT2 && r_type != T_FLOAT2)
        {
            // ERROR: Cannot assign [r_type] / [l_type].
            register_error("Types ... and ... incompatible.", _expr->lhs->tok.line, _expr->lhs->tok.column);
            return false;
        }
        _expr->variable_type = T_FLOAT2;
        break;
    }
    case T_FLOAT3:
    case T_INT3:
    {
        if (r_type != T_INT3 && r_type != T_FLOAT3)
        {
            // ERROR: Cannot assign [r_type] / [l_type].
            register_error("Types ... and ... incompatible.", _expr->lhs->tok.line, _expr->lhs->tok.column);
            return false;
        }
        _expr->variable_type = T_FLOAT3;
        break;
    }
    case T_FLOAT4:
    case T_INT4:
    {
        if (r_type != T_INT4 && r_type != T_FLOAT4)
        {
            // ERROR: Cannot assign [r_type] / [l_type].
            register_error("Types ... and ... incompatible.", _expr->lhs->tok.line, _expr->lhs->tok.column);
            return false;
        }
        _expr->variable_type = T_FLOAT4;
        break;
    }

    default:
        // ERROR: Cannot operate on [l_type].
        register_error("Cannot operate on ....", _expr->lhs->tok.line, _expr->lhs->tok.column);
        return false;
    }

    return true;
}

bool AST::check_binary_inclusive(std::shared_ptr<BinaryExpr> _expr)
{
    VarType l_type = _expr->lhs->variable_type;
    VarType r_type = _expr->rhs->variable_type;
    switch (l_type)
    {
    case T_FLOAT:
    case T_INT:
    {
        if (r_type != T_INT && r_type != T_FLOAT)
        {
            // ERROR: Cannot assign [r_type] / [l_type].
            register_error("Types ... and ... incompatible.", _expr->lhs->tok.line, _expr->lhs->tok.column);
            return false;
        }
        _expr->variable_type = T_FLOAT;
        break;
    }

    case T_FLOAT2:
    case T_INT2:
    {
        if (r_type != T_INT2 && r_type != T_FLOAT2 && r_type != T_INT && r_type != T_FLOAT)
        {
            // ERROR: Cannot assign [r_type] / [l_type].
            register_error("Types ... and ... incompatible.", _expr->lhs->tok.line, _expr->lhs->tok.column);
            return false;
        }

        _expr->variable_type = T_FLOAT2;
        break;
    }
    case T_FLOAT3:
    case T_INT3:
    {
        if (r_type != T_INT3 && r_type != T_FLOAT3 && r_type != T_INT && r_type != T_FLOAT)
        {
            // ERROR: Cannot assign [r_type] / [l_type].
            register_error("Types ... and ... incompatible.", _expr->lhs->tok.line, _expr->lhs->tok.column);
            return false;
        }
        _expr->variable_type = T_FLOAT3;
        break;
    }
    case T_FLOAT4:
    case T_INT4:
    {
        if (r_type != T_INT4 && r_type != T_FLOAT4 && r_type != T_INT && r_type != T_FLOAT)
        {
            // ERROR: Cannot assign [r_type] / [l_type].
            register_error("Types ... and ... incompatible.", _expr->lhs->tok.line, _expr->lhs->tok.column);
            return false;
        }
        _expr->variable_type = T_FLOAT4;
        break;
    }

    default:
        // ERROR: Cannot operate on [l_type].
        register_error("Cannot operate on ....", _expr->lhs->tok.line, _expr->lhs->tok.column);
        return false;
    }

    return true;
}

AST::AST()
{
    function_expectations["float2"].push_back(FunctionExpectation({ T_FLOAT, T_FLOAT }, T_FLOAT2));
    function_expectations["float3"].push_back(FunctionExpectation({ T_FLOAT, T_FLOAT, T_FLOAT }, T_FLOAT3));
    function_expectations["float4"].push_back(FunctionExpectation({ T_FLOAT, T_FLOAT, T_FLOAT, T_FLOAT }, T_FLOAT4));

    function_expectations["int2"].push_back(FunctionExpectation({ T_INT, T_INT}, T_INT2));
    function_expectations["int3"].push_back(FunctionExpectation({ T_INT, T_INT, T_INT}, T_INT3));
    function_expectations["int4"].push_back(FunctionExpectation({ T_INT, T_INT, T_INT, T_INT}, T_INT4));

    function_expectations["dot_product"].push_back(FunctionExpectation({ T_FLOAT4, T_FLOAT4 }, T_FLOAT));
    function_expectations["dot_product"].push_back(FunctionExpectation({ T_FLOAT3, T_FLOAT3 }, T_FLOAT));
    function_expectations["dot_product"].push_back(FunctionExpectation({ T_FLOAT2, T_FLOAT2 }, T_FLOAT));

    function_expectations["cross_product"].push_back(FunctionExpectation({ T_FLOAT3, T_FLOAT3 }, T_FLOAT3));

    function_expectations["normalize"].push_back(FunctionExpectation({ T_FLOAT4 }, T_FLOAT4));
    function_expectations["normalize"].push_back(FunctionExpectation({ T_FLOAT3 }, T_FLOAT3));
    function_expectations["normalize"].push_back(FunctionExpectation({ T_FLOAT2 }, T_FLOAT2));

    function_expectations["length"].push_back(FunctionExpectation({ T_FLOAT4 }, T_FLOAT));
    function_expectations["length"].push_back(FunctionExpectation({ T_FLOAT3 }, T_FLOAT));
    function_expectations["length"].push_back(FunctionExpectation({ T_FLOAT2 }, T_FLOAT));

    function_expectations["lengthSqr"].push_back(FunctionExpectation({ T_FLOAT4 }, T_FLOAT));
    function_expectations["lengthSqr"].push_back(FunctionExpectation({ T_FLOAT3 }, T_FLOAT));
    function_expectations["lengthSqr"].push_back(FunctionExpectation({ T_FLOAT2 }, T_FLOAT));

    function_expectations["lerp"].push_back(FunctionExpectation({ T_FLOAT4, T_FLOAT4, T_FLOAT }, T_FLOAT4));
    function_expectations["lerp"].push_back(FunctionExpectation({ T_FLOAT3, T_FLOAT3, T_FLOAT }, T_FLOAT3));
    function_expectations["lerp"].push_back(FunctionExpectation({ T_FLOAT2, T_FLOAT2, T_FLOAT }, T_FLOAT2));
    function_expectations["lerp"].push_back(FunctionExpectation({ T_FLOAT, T_FLOAT, T_FLOAT }, T_FLOAT));

    function_expectations["pi"].push_back(FunctionExpectation({}, T_FLOAT));
    function_expectations["sqrt"].push_back(FunctionExpectation({T_FLOAT}, T_FLOAT));
    function_expectations["cos"].push_back(FunctionExpectation({T_FLOAT}, T_FLOAT));
    function_expectations["sin"].push_back(FunctionExpectation({T_FLOAT}, T_FLOAT));
    function_expectations["tan"].push_back(FunctionExpectation({T_FLOAT}, T_FLOAT));

    function_expectations["clamp"].push_back(FunctionExpectation({T_FLOAT, T_FLOAT, T_FLOAT}, T_FLOAT));

    function_expectations["size"].push_back(FunctionExpectation({T_TEXTURE1D}, T_INT));
    function_expectations["size"].push_back(FunctionExpectation({T_TEXTURE2D}, T_INT2));

    function_expectations["set"].push_back(FunctionExpectation({T_TEXTURE1D, T_INT, T_FLOAT4}, T_NIL));
    function_expectations["set"].push_back(FunctionExpectation({T_TEXTURE2D, T_INT2, T_FLOAT4}, T_NIL));

    function_expectations["sample"].push_back(FunctionExpectation({T_TEXTURE1D, T_INT}, T_FLOAT4));
    function_expectations["sample"].push_back(FunctionExpectation({T_TEXTURE2D, T_INT2}, T_FLOAT4));

    function_expectations["min"].push_back(FunctionExpectation({T_FLOAT, T_FLOAT }, T_FLOAT));
    function_expectations["min"].push_back(FunctionExpectation({T_INT, T_INT}, T_INT));

    function_expectations["max"].push_back(FunctionExpectation({T_FLOAT, T_FLOAT }, T_FLOAT));
    function_expectations["max"].push_back(FunctionExpectation({T_INT, T_INT}, T_INT));
}

bool VariableInfo::is_scalar()
{
    return type <= T_FLOAT;
}
