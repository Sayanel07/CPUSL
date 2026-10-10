#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "common.h"

#include <iostream>
#include <array>
#include <stack>

#define REG_COUNT 256

struct Instruction
{
    uint8_t opcode = 0;
    uint8_t op1 = 0;
    uint8_t op2 = 0;
    uint8_t op3 = 0;
};

struct Compiler
{
    AST* ast;

    std::array<bool, REG_COUNT> is_reg_used;
    std::stack<uint8_t> reg_index_stack;
    std::vector<Instruction> instructions;

    uint8_t* compiled;
    size_t pointer;

    void write(uint8_t _byte);
    void write(uint16_t _bytes);
    void write(uint32_t _bytes);
    void write(void* _bytes, size_t size);

    void compile_from_file(std::string _source_path, std::string _destination_path, std::string _output_name);

    void compile(std::string _destination_path);
    void compile_category(const Category& _category);
    void compile_statement(std::shared_ptr<Expr> _statement);
    void write_bitcode(std::string _destination_path);

    uint8_t reserve_reg();
    uint8_t get_reg_index(bool silent = false);

    size_t max_reg_used = 0;
    size_t current_reg_used = 0;

    bool is_scalar(std::shared_ptr<IdentifierExpr> _expr);

    void write_instruction(uint8_t _opcode, uint8_t _op1, uint8_t _op2, uint8_t _op3);
    void write_instruction(uint8_t _opcode, uint8_t _op1, uint8_t _op2);
    void write_instruction(uint8_t _opcode, uint16_t _op1, uint8_t _op2);
    void write_instruction(uint8_t _opcode, uint8_t _op1, uint16_t _op2);
};

#endif