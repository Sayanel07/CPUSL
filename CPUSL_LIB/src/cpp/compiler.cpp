#include "compiler.h"

#include <fstream>
#include <sstream>

void Compiler::write(uint8_t _byte)
{
	memcpy(compiled + pointer, &_byte, sizeof(int8_t));
	pointer += 1;
}

void Compiler::write(uint16_t _byte)
{
	memcpy(compiled + pointer, &_byte, sizeof(int16_t));
	pointer += 2;
}

void Compiler::write(uint32_t _byte)
{
	memcpy(compiled + pointer, &_byte, sizeof(int32_t));
	pointer += 4;
}

void Compiler::write(void* _bytes, size_t _size)
{
	memcpy(compiled + pointer, _bytes, _size);
	pointer += _size;
}

void Compiler::compile_from_file(std::string _source_path, std::string _destination_path, std::string _output_name)
{
	std::ifstream source_file;
	source_file.open(_source_path, std::fstream::in);

	if (!source_file)
	{
		std::cout << "Error: Could not open file.";
		return;
	}

	std::stringstream source_stream;
	source_stream << source_file.rdbuf();
	source_file.close();

	Lexer lexer;
	std::string source_text = source_stream.str();
	lexer.set_source(source_text);
	lexer.scan_source();

	Parser parser = Parser(lexer.get_tokens());
	parser.parse();

	AST _ast;
	ast = &_ast;

	ast->exprs = parser.exprs;
	ast->check_semantics();

	if (ast->has_errors()) return;

	compile(_destination_path + _output_name);
}

void Compiler::compile(std::string _destination_path)
{
    is_reg_used.fill(false);

	compile_category(ast->exprs["ps"]);

	write_bitcode(_destination_path);
}

void Compiler::compile_category(const Category& _category)
{
	int size = _category.statements.size();
	for (int i = 0; i < size; i++)
	{
		std::shared_ptr<Expr> expr = _category.statements[i];

		compile_statement(expr);
	}
}

void Compiler::compile_statement(std::shared_ptr<Expr> _statement)
{
	switch (_statement->type)
	{
		case IDENTIFIER: 
		{ 
			const auto identifier_expr = std::static_pointer_cast<IdentifierExpr>(_statement);
			if (identifier_expr->variable_type == T_TEXTURE1D || identifier_expr->variable_type == T_TEXTURE2D)
			{
				// Do nothing
				break;
			}

			switch (identifier_expr->decl_category)
			{
			case CAT_CONST:
			{
				uint16_t c_index = ast->get_constant_offset_by_name(std::string(identifier_expr->tok.lexeme));
				uint8_t dest = reserve_reg();

				if (is_scalar(identifier_expr))
				{
					write_instruction(OP_LOAD_CSS, dest, c_index);
				}
				else
				{
					write_instruction(OP_LOAD_CPS, dest, c_index);
				}
				break;
			}

			case CAT_ENTRY:
			case CAT_PS:
			{
				uint16_t index = ast->get_variable_offset(std::string(identifier_expr->tok.lexeme));
				uint8_t dest = reserve_reg();

				if (is_scalar(identifier_expr))
				{
					write_instruction(OP_LOAD_SS, dest, index);
				}
				else
				{
					write_instruction(OP_LOAD_PS, dest, index);
				}
				break;
			}
			}

			break; 
		}
		case LITERAL: 
		{ 
			uint16_t c_index = ast->get_constant_offset(_statement->tok.value);
			write_instruction(OP_LOAD_CSS, reserve_reg(), c_index);
			break;
		}
		case ACCESSOR: 
		{ 
			const auto accessor_expr = std::static_pointer_cast<AccessorExpr>(_statement);
			// load 
			uint8_t component_index = -1;
			switch (accessor_expr->tok.lexeme[0])
			{
			case 'x': { component_index = 0; break; }
			case 'y': { component_index = 1; break; }
			case 'z': { component_index = 2; break; }
			case 'w': { component_index = 3; break; }
			}

			switch (accessor_expr->decl_category)
			{
			case CAT_CONST:
			{
				uint16_t c_index = ast->get_constant_offset_by_name(accessor_expr->name);
				uint8_t dest = reserve_reg();

				write_instruction(OP_LOAD_CPS, reserve_reg(), c_index);
				write_instruction(OP_GET1_PS, dest, get_reg_index(), component_index);
				break;
			}

			case CAT_ENTRY:
			case CAT_PS:
			{
				uint16_t index = ast->get_variable_offset(accessor_expr->name);
				uint8_t dest = reserve_reg();

				write_instruction(OP_LOAD_PS, reserve_reg(), index);
				write_instruction(OP_GET1_PS, dest, get_reg_index(), component_index);
				break;
			}
			}


			break;
		}
		case UNARY: 
		{ 
			const auto unary_expr = std::static_pointer_cast<UnaryExpr>(_statement);
			compile_statement(unary_expr->expr);
			switch (unary_expr->tok.type)
			{
				case TOK_ADD:
				{
					// Do nothing
					break;
				}
				case TOK_SUB:
				{
					uint8_t reg_index = get_reg_index();

					write_instruction(OP_LOAD_CSS, reserve_reg(), uint16_t(ast->get_constant_offset(0)));

					uint8_t const_reg = get_reg_index();
					write_instruction(OP_SUB, reserve_reg(), const_reg, reg_index);
					break;
				}
			break;
			}
			break;
		}
		case BINARY: 
		{ 
			const auto binary_expr = std::static_pointer_cast<BinaryExpr>(_statement);

			switch (binary_expr->tok.type)
			{
				case TOK_ASS:
				{
					compile_statement(binary_expr->rhs);

					
					if (binary_expr->lhs->type == ACCESSOR)
					{
						const auto accessor_left = std::static_pointer_cast<AccessorExpr>(binary_expr->lhs);
						uint16_t var_index = ast->get_variable_offset(accessor_left->name);
						uint8_t dest = reserve_reg();
						dest = get_reg_index();
						uint8_t right = get_reg_index();
						write_instruction(OP_LOAD_PS, dest, var_index);
						write_instruction(OP_SET1_PS, dest, right, accessor_left->component_index);
						write_instruction(OP_STORE_PS, var_index, dest);
						break;
					}

					uint8_t right = get_reg_index();
					bool scalar = binary_expr->lhs->variable_type == T_INT || binary_expr->lhs->variable_type == T_FLOAT;
					if (scalar)
					{
						uint16_t var_index = ast->get_variable_offset(std::string(binary_expr->lhs->tok.lexeme));
						write_instruction(OP_STORE_SS, var_index, right);
					}
					else
					{
						uint16_t var_index = ast->get_variable_offset(std::string(binary_expr->lhs->tok.lexeme));
						write_instruction(OP_STORE_PS, var_index, right);
					}
					
					break;
				}

				case TOK_ADD:
				{
					compile_statement(binary_expr->lhs);
					compile_statement(binary_expr->rhs);

					int8_t right = get_reg_index();
					int8_t left = get_reg_index();
					write_instruction(OP_ADD, reserve_reg(), left, right);
					break;
				}
				case TOK_SUB:
				{
					compile_statement(binary_expr->lhs);
					compile_statement(binary_expr->rhs);

					uint8_t right = get_reg_index();
					uint8_t left = get_reg_index();
					write_instruction(OP_SUB, reserve_reg(), left, right);
					break;
				}
				case TOK_MUL:
				{
					compile_statement(binary_expr->lhs);
					compile_statement(binary_expr->rhs);

					uint8_t right = get_reg_index();
					uint8_t left = get_reg_index();
					write_instruction(OP_MUL, reserve_reg(), left, right);
					break;
				}
				case TOK_DIV:
				{
					compile_statement(binary_expr->lhs);
					compile_statement(binary_expr->rhs);

					uint8_t right = get_reg_index();
					uint8_t left = get_reg_index();
					write_instruction(OP_DIV, reserve_reg(), left, right);
					break;
				}
			}
			break;
		}
		case FUNCTION: 
		{ 
			const auto func_expr = std::static_pointer_cast<FunctionCallExpr>(_statement);

			for (int i = 0; i < func_expr->args.size(); i++)
			{
				compile_statement(func_expr->args[i]);
			}

			switch (func_expr->tok.type)
			{
				case TOK_DOT_PROD:
				{
					uint8_t right = get_reg_index();
					uint8_t left = get_reg_index();

					write_instruction(OP_DOT_PROD, reserve_reg(), left, right);

					break;
				}
				case TOK_CROSS_PROD:
				{
					uint8_t right = get_reg_index();
					uint8_t left = get_reg_index();

					write_instruction(OP_CROSS_PROD, reserve_reg(), left, right);

					break;
				}
				case TOK_MIN:
				{
					uint8_t right = get_reg_index();
					uint8_t left = get_reg_index();

					write_instruction(OP_MIN, reserve_reg(), left, right);

					break;
				}
				case TOK_MAX:
				{
					uint8_t right = get_reg_index();
					uint8_t left = get_reg_index();

					write_instruction(OP_MAX, reserve_reg(), left, right);

					break;
				}
				case TOK_LERP:
				{
					uint8_t t = get_reg_index();
					uint8_t right = get_reg_index();
					uint8_t left = get_reg_index(true);

					write_instruction(OP_SUB, reserve_reg(), right, left);
					uint8_t diff = get_reg_index();

					write_instruction(OP_MUL, reserve_reg(), diff, t);
					diff = get_reg_index();
					left = get_reg_index();
					
					write_instruction(OP_ADD, reserve_reg(), diff, left);

					break;
				}
				case TOK_CLAMP:
				{
					uint8_t max = get_reg_index();
					uint8_t min = get_reg_index();
					uint8_t value = get_reg_index();

					write_instruction(OP_MIN, reserve_reg(), value, min);
					value = get_reg_index();
					write_instruction(OP_MAX, reserve_reg(), value, max);

					break;
				}
				case TOK_SQRT:
				{
					uint8_t val = get_reg_index();
					write_instruction(OP_SQRT, reserve_reg(), val);

					break;
				}
				case TOK_PI:
				{
					write_instruction(OP_LOAD_CSS, reserve_reg(), ast->get_constant_offset(M_PI));
					break;
				}
				case TOK_COS:
				{
					uint8_t val = get_reg_index();
					write_instruction(OP_COS, reserve_reg(), val);

					break;
				}
				case TOK_SIN:
				{
					uint8_t val = get_reg_index();
					write_instruction(OP_SIN, reserve_reg(), val);

					break;
				}
				case TOK_TAN:
				{
					uint8_t val = get_reg_index();
					write_instruction(OP_TAN, reserve_reg(), val);

					break;
				}
				case TOK_SIZE:
				{
					uint8_t texture_index = ast->get_texture_index(std::string(func_expr->args[0]->tok.lexeme));
					
					if (func_expr->args[0]->variable_type == T_TEXTURE1D)
					{
						write_instruction(OP_SIZE_1D, reserve_reg(), texture_index);
					}
					else if (func_expr->args[0]->variable_type == T_TEXTURE2D)
					{
						write_instruction(OP_SIZE_2D, reserve_reg(), texture_index);
					}
					break;
				}
				case TOK_SAMPLE:
				{
					uint8_t coordinates = get_reg_index();
					uint8_t texture_index = ast->get_texture_index(std::string(func_expr->args[0]->tok.lexeme));

					if (func_expr->args[0]->variable_type == T_TEXTURE1D)
					{
						write_instruction(OP_GET_1D, reserve_reg(), texture_index, coordinates);
					}
					else if (func_expr->args[0]->variable_type == T_TEXTURE2D)
					{
						write_instruction(OP_GET_2D, reserve_reg(), texture_index, coordinates);
					}
					break;
				}
				case TOK_SET: // TODO
				{
					break;
				}
				case TOK_LENGTH:
				{
					uint8_t val = get_reg_index();
					write_instruction(OP_LENGTH, reserve_reg(), val);
					break;
				}
				case TOK_LENGTHSQR:
				{
					uint8_t val = get_reg_index();
					write_instruction(OP_SQR_LENGTH, reserve_reg(), val);
					break;
				}
				case TOK_NORMALIZE:
				{
					uint8_t val = get_reg_index();
					write_instruction(OP_LENGTH, reserve_reg(), val);

					uint8_t norm = get_reg_index();
					write_instruction(OP_DIV, reserve_reg(), val, norm);
					break;
				}

				case TOK_FLOAT2:
				case TOK_INT2:
				{
					uint8_t v2 = get_reg_index();
					uint8_t v1 = get_reg_index();

					uint8_t f2 = reserve_reg();
					write_instruction(OP_SET1_PS, f2, v1, 0);
					write_instruction(OP_SET1_PS, f2, v2, 1);
					break;
				}
				case TOK_FLOAT3:
				case TOK_INT3:
				{
					uint8_t v3 = get_reg_index();
					uint8_t v2 = get_reg_index();
					uint8_t v1 = get_reg_index();

					uint8_t f3 = reserve_reg();
					write_instruction(OP_SET1_PS, f3, v1, 0);
					write_instruction(OP_SET1_PS, f3, v2, 1);
					write_instruction(OP_SET1_PS, f3, v3, 2);
					break;
				}
				case TOK_FLOAT4:
				case TOK_INT4:
				{
					uint8_t v4 = get_reg_index();
					uint8_t v3 = get_reg_index();
					uint8_t v2 = get_reg_index();
					uint8_t v1 = get_reg_index();

					uint8_t f4 = reserve_reg();
					write_instruction(OP_SET1_PS, f4, v1, 0);
					write_instruction(OP_SET1_PS, f4, v2, 1);
					write_instruction(OP_SET1_PS, f4, v3, 2);
					write_instruction(OP_SET1_PS, f4, v4, 3);
					break;
				}
			}
			break;
		}
	}
}

void Compiler::write_bitcode(std::string _destination_path)
{
	pointer = 0;
	size_t total_size = 0;
	int constant_count = ast->const_variables.size();

	total_size += 8;
	uint16_t constant_size = 0;
	for (int i = 0; i < constant_count; i++)
	{
		if (ast->const_variables[i].type == T_FLOAT || ast->const_variables[i].type == T_INT)
		{
			constant_size += sizeof(float);
		}
		else
		{
			constant_size += sizeof(Float4);
		}
	}
	total_size += constant_size;
	total_size += instructions.size() * sizeof(Instruction);

	compiled = static_cast<uint8_t*>(realloc(nullptr, total_size));

	// Header (8)
    uint16_t magic_number = MAGIC_NUMBER;
    uint8_t version = VERSION;
	write(magic_number); // 2
	write(version); // 1
	write(static_cast<uint8_t>(max_reg_used));
	write(constant_size);
	write(ast->get_entry_size());

	for (int i = 0; i < constant_count; i++)
	{
		if (ast->const_variables[i].type == T_FLOAT || ast->const_variables[i].type == T_INT)
		{
			write(&ast->const_variables[i].precomputed_value.x, sizeof(float));
		}
		else
		{
			write(&ast->const_variables[i].precomputed_value, sizeof(Float4));
		}
	}

	// Body (Instructions x 4)
	int instruction_count = instructions.size();
	for (int i = 0; i < instruction_count; i++)
	{
		write(&instructions[i], sizeof(Instruction));
	}

	std::ofstream output_file;
	output_file.open(_destination_path + ".cpusl", std::fstream::out | std::fstream::binary);
	output_file.write(reinterpret_cast<char*>(compiled), total_size);
	output_file.close();
}

uint8_t Compiler::reserve_reg()
{
	current_reg_used++;
	max_reg_used = std::max(current_reg_used, max_reg_used);

	for (int i = 0; i < REG_COUNT; i++)
	{
		if (is_reg_used[i] == false)
		{
			is_reg_used[i] = true;
			reg_index_stack.push(i);
			return i;
		}
	}

	throw std::runtime_error("Not enough registries");
	return 0;
}

uint8_t Compiler::get_reg_index(bool silent)
{
	uint8_t reg_index = reg_index_stack.top();

	if (silent) return reg_index;

	is_reg_used[reg_index] = false;
	current_reg_used--;
	reg_index_stack.pop();

	return reg_index;
}

bool Compiler::is_scalar(std::shared_ptr<IdentifierExpr> _expr)
{
	return _expr->variable_type <= T_FLOAT;
}

void Compiler::write_instruction(uint8_t _opcode, uint8_t _op1, uint8_t _op2, uint8_t _op3)
{
	Instruction inst;
	inst.opcode = _opcode;
	inst.op1 = _op1;
	inst.op2 = _op2;
	inst.op3 = _op3;
	instructions.push_back(inst);
}

void Compiler::write_instruction(uint8_t _opcode, uint8_t _op1, uint8_t _op2)
{
	Instruction inst;
	inst.opcode = _opcode;
	inst.op1 = _op1;
	inst.op2 = _op2;
	instructions.push_back(inst);
}

void Compiler::write_instruction(uint8_t _opcode, uint16_t _op1, uint8_t _op2)
{
	Instruction inst;
	inst.opcode = _opcode;
	inst.op1 = (_op1 & 0b11111111'00000000) >> 8;
	inst.op2 = _op1 & 0b00000000'11111111;
	inst.op3 = _op2;
	instructions.push_back(inst);
}
void Compiler::write_instruction(uint8_t _opcode, uint8_t _op1, uint16_t _op2)
{
	Instruction inst;
	inst.opcode = _opcode;
	inst.op1 = _op1;
	inst.op2 = (_op2 & 0b11111111'00000000) >> 8;
	inst.op3 = _op2 & 0b00000000'11111111;
	instructions.push_back(inst);
}
