#ifndef VM_H
#define VM_H

#include <fstream>
#include <string>
#include <vector>

#define DEFAULT_STACK_SIZE 48
#define DEFAULT_TEXTURE_SIZE 1

union Color;
struct Texture;

class VirtualMachine {
public :
    void set_bytecode(std::string _filepath);
    
    void set_entry(uint16_t _index, const void* _data, uint16_t _size);
    
    void set_pixels(std::vector<Color>& _pixels);
    void set_pixels(const Color* _pixels, uint32_t _width, uint32_t _height);
    
    void set_image_size(uint32_t _width, uint32_t _height);
    
    void set_entry_texture(uint8_t _index, const Texture& _data);
    void set_entry_texture(uint8_t _index, const Texture* _data);
    
    std::vector<Color> interpret();
    
private:
    union alignas(16) Register {
        float  f[4];
        __m128 v;
    };
    
    struct BytecodeHeader {
        uint16_t magic_number;
        uint8_t version;
        uint8_t register_size;
        uint16_t constant_stack_size;
        uint16_t entries_size;
    };
    
    struct Instruction {
        uint16_t l1;
        uint16_t l2;
        uint8_t op;
        uint8_t s1;
        uint8_t s2;
        uint8_t s3;
    };
    
    std::vector<char> stack;
    std::vector<char> const_stack;
    
    std::vector<Register> registers; // Here we have float and float4
    
    std::vector<std::shared_ptr<Texture>> textures;
    
    std::vector<uint8_t> bytecode;
    uint32_t current_byte = 0;
    
    uint16_t entries_size = 0;
    
private:
    uint8_t* advance(uint32_t _count);
    
    uint8_t* peek(uint32_t _count);
    
    Instruction retrieve_instruction(const uint8_t* _data);
    
    // LOAD_SS   F   STACK
    void load_ss(Instruction _instruct);

    // LOAD_PS   F4   STACK
    void load_ps(Instruction _instruct);
    
    // LOAD_CSS F STACK
    void load_css(Instruction _instruct);
    
    // LOAD_CPS F4 STACK
    void load_cps(Instruction _instruct);

    // STORE_SS F STACK
    void store_ss(Instruction _instruct);
    
    // STORE_PS F4 STACK
    void store_ps(Instruction _instruct);
    
    // MOVE	F/F4 F/F4		
    void move(Instruction _instruct);
    
    // SET1_PS  F4  F  F	
    void set1_ps(Instruction _instruct);

    // GET1_PS	F   F4  F
    void get1_ps(Instruction _instruct);

    // ADD	F4/F	F4/F	F4/F	
    void add(Instruction _instruct);

    // SUB	F4/F	F4/F	F4/F	
    void sub(Instruction _instruct);

    // MUL	F/F4	F/F4	F/F4	
    void mul(Instruction _instruct);

    // DIV	F/F4	F/F4	F/F4	
    void div(Instruction _instruct);

    // CROSS	F4	F4	F4	
    void cross_prod(Instruction _instruct);

    // DOT	F	F4	F4	
    void dot_prod(Instruction _instruct);

    // SQRT	F/F4	F/F4		
    void sqrt(Instruction _instruct);

    // COS	F	F
    void cos(Instruction _instruct);

    // SIN	F	F
    void sin(Instruction _instruct);

    // TAN	F	F		
    void tan(Instruction _instruct);

    // LENGTH	F	F4		
    void length(Instruction _instruct);

    // SQR_LENGHT	F	F4		
    void sqr_length(Instruction _instruct);

    // SIZE_1D	F	TEXTURE1D		
    void size_1d(Instruction _instruct);

    // SIZE_2D	F4	TEXTURE2D		
    void size_2d(Instruction _instruct);

    // SET_1D	TEXTURE1D	F4	F	
    void set_1d(Instruction _instruct);

    // SET_2D	TEXTURE2D	F4	F4	
    void set_2d(Instruction _instruct);

    // GET_1D	F	TEXTURE1D	F	
    void get_1d(Instruction _instruct);

    // GET_2D	F4	TEXTURE2D	F4	
    void get_2d(Instruction _instruct);

    // MAX	F	F	F	
    void max(Instruction _instruct);

    // MIN	F	F	F
    void min(Instruction _instruct);
};

#endif
