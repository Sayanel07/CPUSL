#ifndef AST_H
#define AST_H

#include "parser.h"

struct FunctionExpectation
{
	std::vector<VarType> arguments;
	VarType return_type;
	FunctionExpectation(std::vector<VarType> _args, VarType _ret) : arguments(_args), return_type(_ret) {}
};

struct Float4
{
	float x, y, z, w;
	Float4(float _val): x(_val), y(_val), z(_val), w(_val) {}
	Float4(float _x, float _y, float _z, float _w): x(_x), y(_y), z(_z), w(_w) {}
	Float4(): x(0), y(0), z(0), w(0) {}
	bool operator==(Float4 _other) {
		return x == _other.x &&
			y == _other.y &&
			z == _other.z &&
			w == _other.w;
	};
	Float4 operator+(Float4 _other) { return { x + _other.x, y + _other.y, z + _other.z, w + _other.w }; };
	Float4 operator-(Float4 _right) { return { x - _right.x, y - _right.y, z - _right.z, w - _right.w }; };
	Float4 operator*(Float4 _other) { return { x * _other.x, y * _other.y, z * _other.z, w * _other.w }; };
	Float4 operator/(Float4 _right) { return { x / _right.x, y / _right.y, z / _right.z, w / _right.w }; };
	Float4 operator/(float _right) { return { x / _right, y / _right, z / _right, w / _right }; };
};

struct VariableInfo
{
	std::string name;
	VarType type;
	Float4 precomputed_value;

	bool is_scalar();
};

struct AST
{
	std::vector<VariableInfo> const_variables;
	std::vector<VariableInfo> entry_variables;
	std::vector<VariableInfo> variables;
	std::vector<VariableInfo> entry_textures;

	std::unordered_map<std::string, Category> exprs;

	std::vector<std::string> errors;

	std::unordered_map<std::string, std::vector<FunctionExpectation>> function_expectations;

	void check_semantics();

	void register_error(std::string _error_text, int _line, int _column);
	void display_errors();
	bool has_errors();

	void check_category(const Category & _category, CategoryType _type);
	bool check_expression(std::shared_ptr<Expr> _statement, CategoryType _type);
	Float4 precompute_const_statement(std::shared_ptr<Expr> _statement);

	void place_constant(Float4 _value);
	void place_constant(float _value);
	uint16_t get_variable_offset(std::string _var_name);
	uint16_t get_constant_offset(Float4 _value);
	uint16_t get_constant_offset(float _value);
	uint16_t get_constant_offset_by_name(std::string _var_name);
	CategoryType get_category_by_name(std::string _var_name);
	VariableInfo* get_id_by_name(std::string _name);
	uint16_t get_entry_size();
	uint8_t get_texture_index(std::string _var_name);

	bool check_function_expectations(std::shared_ptr<FunctionCallExpr> _statement);

	bool does_variable_exist(std::string _var_name, CategoryType _type, VarType& _var_type, CategoryType& _decl_type);
	bool compare_types(VarType _prio, VarType _second);
	bool check_binary_exclusive(std::shared_ptr<BinaryExpr> _expr);
	bool check_binary_inclusive(std::shared_ptr<BinaryExpr> _expr);
	AST();
};

#endif