#include "../h/vm.h"

#include "common.h"
#define USE_SSE2
#include "../h/sse_mathfun.h"

void VirtualMachine::set_bytecode(std::string _filepath)  {
    std::ifstream file(_filepath, std::ios::binary);
        
    if (!file)
        throw std::runtime_error("Could not open the file.");
            
    bytecode = std::vector<uint8_t>(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    current_byte = 0;
    
    BytecodeHeader header;
    memcpy(&header, advance(sizeof(BytecodeHeader)), sizeof(BytecodeHeader));
    
    if (header.magic_number != MAGIC_NUMBER)
        throw std::runtime_error("magic number in file is incorrect.");
    if (header.version != VERSION)
        throw std::runtime_error("make sure the file and the VM version are the same.");
    
    stack.resize(0xFFFF); // Max size for now
    textures.resize(0xFF); // Max size for now
    const_stack.resize(header.constant_stack_size);
    registers.resize(header.register_size);
    entries_size = header.entries_size;
    
    textures[0] = std::make_shared<Texture>();
}

void VirtualMachine::set_entry(uint16_t _index, const void* _data, uint16_t _size) {
    if (_index + _size > entries_size)
        return;
    memcpy(stack.data() + _index + DEFAULT_STACK_SIZE, _data, _size);
}

void VirtualMachine::set_entry_texture(uint8_t _index, const Texture& _data) {
    textures.at(_index + DEFAULT_TEXTURE_SIZE) = std::make_shared<Texture>(_data);
}

void VirtualMachine::set_entry_texture(uint8_t _index, const Texture* _data) {
    textures.at(_index + DEFAULT_TEXTURE_SIZE) = std::make_shared<Texture>(*_data);
}

void VirtualMachine::set_pixels(std::vector<Color>& _pixels) {
    textures[0]->pixels = _pixels;
}

void VirtualMachine::set_pixels(const Color* _pixels, uint32_t _width, uint32_t _height) {
    textures[0]->width = _width;
    textures[0]->height = _height;
    textures[0]->pixels.resize(_width * _height);
    memcpy(textures[0]->pixels.data(), _pixels, sizeof(Color) * _width * _height);
}

void VirtualMachine::set_image_size(uint32_t _width, uint32_t _height) {
    textures[0]->width = _width;
    textures[0]->height = _height;
}

std::vector<Color> VirtualMachine::interpret() {
    memcpy(const_stack.data(), advance(const_stack.size()), const_stack.size());
    
    uint32_t pixels_width = textures[0]->width;
    uint32_t pixels_height = textures[0]->height;
    uint32_t ouput_size = pixels_width * pixels_height;
    std::vector<Color> output(ouput_size);
    
    uint32_t count = 0;
    while (count < ouput_size) 
    {
        float x = static_cast<float>(count % pixels_width);
        float y = static_cast<float>(count / pixels_width);
        
        // Same UV as in glsl
        float u =  x / static_cast<float>(pixels_width);
        float v = 1.f - (y / static_cast<float>(pixels_height));
        
        // Have the 24 first byte to be default data aka the pixel and its position
        memcpy(stack.data(), textures[0]->pixels[count].f, sizeof(Color));
        memcpy(stack.data() + 16, &x, sizeof(float));
        memcpy(stack.data() + 20, &y, sizeof(float));
        memcpy(stack.data() + 32, &u, sizeof(float));
        memcpy(stack.data() + 36, &v, sizeof(float));
    
        uint32_t before_loop_count = current_byte;
        while (current_byte <= bytecode.size() - 4)
        {
            Instruction instruct = retrieve_instruction(advance(4));
            switch (static_cast<OpCode>(instruct.op)) {
                case OP_LOAD_SS:     load_ss(instruct);    break;
                case OP_LOAD_PS:     load_ps(instruct);    break;
                case OP_LOAD_CSS:    load_css(instruct);   break;
                case OP_LOAD_CPS:    load_cps(instruct);   break;
                case OP_STORE_SS:    store_ss(instruct);   break;
                case OP_STORE_PS:    store_ps(instruct);   break;
                case OP_MOVE:        move(instruct);       break;
                case OP_SET1_PS:     set1_ps(instruct);    break;
                case OP_GET1_PS:     get1_ps(instruct);    break;
                case OP_ADD:         add(instruct);        break;
                case OP_SUB:         sub(instruct);        break;
                case OP_MUL:         mul(instruct);        break;
                case OP_DIV:         div(instruct);        break;
                case OP_CROSS_PROD:  cross_prod(instruct); break;
                case OP_DOT_PROD:    dot_prod(instruct);   break;
                case OP_SQRT:        sqrt(instruct);       break;
                case OP_COS:         cos(instruct);        break;
                case OP_SIN:         sin(instruct);        break;
                case OP_TAN:         tan(instruct);        break;
                case OP_LENGTH:      length(instruct);     break;
                case OP_SQR_LENGTH:  sqr_length(instruct); break;
                case OP_SIZE_1D:     size_1d(instruct);    break;
                case OP_SIZE_2D:     size_2d(instruct);    break;
                case OP_SET_1D:      set_1d(instruct);     break;
                case OP_SET_2D:      set_2d(instruct);     break;
                case OP_GET_1D:      get_1d(instruct);     break;
                case OP_GET_2D:      get_2d(instruct);     break;
                case OP_MAX:         max(instruct);        break;
                case OP_MIN:         min(instruct);        break;
            }
        }
        current_byte = before_loop_count;
        memcpy(&output[count], stack.data(), sizeof(Color));
        count++;
    }
    
    return output;
}

uint8_t* VirtualMachine::advance(uint32_t _count) {
    uint8_t* vector = peek(_count);
    current_byte += _count;
    return vector;
}

uint8_t* VirtualMachine::peek(uint32_t _count) {
    if (current_byte + _count > bytecode.size())
        return  nullptr;
    return bytecode.data() + current_byte;
}

VirtualMachine::Instruction VirtualMachine::retrieve_instruction(const uint8_t* _data) {
    Instruction instruct;
    instruct.op = _data[0];
    instruct.s1 = _data[1];
    instruct.s2 = _data[2];
    instruct.s3 = _data[3];
    instruct.l1 = static_cast<uint16_t>((_data[1] << 8) | _data[2]);
    instruct.l2 = static_cast<uint16_t>((_data[2] << 8) | _data[3]);
    return instruct;
}

void VirtualMachine::load_ss(Instruction _instruct) {
    uint8_t rgtr = _instruct.s1;
    uint16_t index = _instruct.l2;
    
    float f;
    memcpy(&f, stack.data() + index, sizeof(float));
    registers[rgtr].v = _mm_set_ps1(f);
}

void VirtualMachine::load_ps(Instruction _instruct) {
    uint8_t rgtr = _instruct.s1;
    uint16_t index = _instruct.l2;
    
    Register reg;
    memcpy(&reg, stack.data() + index, sizeof(Register));
    registers[rgtr] = reg;
}

void VirtualMachine::load_css(Instruction _instruct) {
    uint8_t rgtr = _instruct.s1;
    uint16_t index = _instruct.l2;
    
    float f;
    memcpy(&f, const_stack.data() + index, sizeof(float));
    registers[rgtr].v = _mm_set_ps1(f);
}

void VirtualMachine::load_cps(Instruction _instruct) {
    uint8_t rgtr = _instruct.s1;
    uint16_t index = _instruct.l2;
    
    Register reg;
    memcpy(&reg, const_stack.data() + index, sizeof(Register));
    registers[rgtr] = reg;
}

void VirtualMachine::store_ss(Instruction _instruct) {
    uint16_t index = _instruct.l1;
    uint8_t rgtr = _instruct.s3;
    
    memcpy(stack.data() + index, &registers[rgtr], sizeof(float));
}

void VirtualMachine::store_ps(Instruction _instruct) {
    uint16_t index = _instruct.l1;
    uint8_t rgtr = _instruct.s3;
    
    memcpy(stack.data() + index, &registers[rgtr], sizeof(Register));
}

void VirtualMachine::move(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    
    registers[dest] = registers[source];
}

void VirtualMachine::set1_ps(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    uint8_t index = _instruct.s3; // Not a register but an immediate between 0 and 3
    
    registers[dest].f[index] = registers[source].f[0];
}

void VirtualMachine::get1_ps(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    uint8_t index = _instruct.s3; // Not a register but an immediate between 0 and 3
    
    float scalar = registers[source].f[index];
    registers[dest].v = _mm_set_ps1(scalar);
}

void VirtualMachine::add(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source1 = _instruct.s2;
    uint8_t source2 = _instruct.s3;
    
    registers[dest].v = _mm_add_ps(registers[source1].v, registers[source2].v);
}

void VirtualMachine::sub(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source1 = _instruct.s2;
    uint8_t source2 = _instruct.s3;
    
    registers[dest].v = _mm_sub_ps(registers[source1].v, registers[source2].v);
}

void VirtualMachine::mul(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source1 = _instruct.s2;
    uint8_t source2 = _instruct.s3;
    
    registers[dest].v = _mm_mul_ps(registers[source1].v, registers[source2].v);
}

void VirtualMachine::div(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source1 = _instruct.s2;
    uint8_t source2 = _instruct.s3;
    
    registers[dest].v = _mm_div_ps(registers[source1].v, registers[source2].v);
}

void VirtualMachine::cross_prod(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source1 = _instruct.s2;
    uint8_t source2 = _instruct.s3;
    
    __m128 a_yzx = _mm_shuffle_ps(registers[source1].v, registers[source1].v, _MM_SHUFFLE(3, 0, 2, 1));
    __m128 b_zxy = _mm_shuffle_ps(registers[source2].v, registers[source2].v, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 a_zxy = _mm_shuffle_ps(registers[source1].v, registers[source1].v, _MM_SHUFFLE(3, 1, 0, 2));
    __m128 b_yzx = _mm_shuffle_ps(registers[source2].v, registers[source2].v, _MM_SHUFFLE(3, 0, 2, 1));

    registers[dest].v = _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
}

void VirtualMachine::dot_prod(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source1 = _instruct.s2;
    uint8_t source2 = _instruct.s3;
    
    __m128 v1 = registers[source1].v;
    __m128 v2 = registers[source2].v;
    
    __m128 v = _mm_mul_ps(v1, v2);
    // H_ADD
    __m128 hi = _mm_movehl_ps(v, v);
    __m128 s  = _mm_add_ps(v, hi);
    s = _mm_add_ss(s, _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1)));
    
    registers[dest].v = _mm_set_ps1(_mm_cvtss_f32(s));
}

void VirtualMachine::sqrt(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    
    registers[dest].v = _mm_sqrt_ps(registers[source].v);
}

void VirtualMachine::cos(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    
    
    registers[dest].v = cos_ps(registers[source].v);
}

void VirtualMachine::sin(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    
    registers[dest].v = sin_ps(registers[source].v);
}

void VirtualMachine::tan(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    
    __m128 sin;
    __m128 cos;
    sincos_ps(registers[source].v, &sin, &cos);
    
    registers[dest].v = _mm_div_ps(sin, cos);
}

void VirtualMachine::length(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    
    __m128 sqrV = _mm_mul_ps(registers[source].v, registers[source].v);
    
    // H_ADD
    __m128 hi = _mm_movehl_ps(sqrV, sqrV);
    __m128 s  = _mm_add_ps(sqrV, hi);
    s = _mm_add_ss(s, _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1)));
    
    registers[dest].v = _mm_set_ps1(_mm_cvtss_f32(_mm_sqrt_ss(s)));
}

void VirtualMachine::sqr_length(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    
    __m128 sqrV = _mm_mul_ps(registers[source].v, registers[source].v);
    
    // H_ADD
    __m128 hi = _mm_movehl_ps(sqrV, sqrV);
    __m128 s  = _mm_add_ps(sqrV, hi);
    s = _mm_add_ss(s, _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1)));
    
    registers[dest].v = _mm_set_ps1(_mm_cvtss_f32(s));
}

void VirtualMachine::size_1d(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t index = _instruct.s2;
    
    Texture* texture = textures[index].get();
    registers[dest].v = _mm_set1_ps(static_cast<float>(texture->width));
}

void VirtualMachine::size_2d(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t index = _instruct.s2;
    
    Texture* texture = textures[index].get();
    registers[dest].v = _mm_setr_ps(static_cast<float>(texture->width), static_cast<float>(texture->height), 0.0f,0.0f);
}

void VirtualMachine::set_1d(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    uint8_t index = _instruct.s3;
    
    int32_t x = static_cast<int32_t>(registers[index].f[0]);
    Texture* texture = textures[dest].get();
    _mm_storeu_ps(texture->pixels[x].f, registers[source].v);
}

void VirtualMachine::set_2d(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    uint8_t index = _instruct.s3;
    
    int32_t x = static_cast<int32_t>(registers[index].f[0]);
    int32_t y = static_cast<int32_t>(registers[index].f[1]);
    Texture* texture = textures[dest].get();
    _mm_storeu_ps(texture->pixels[x + y * texture->width].f, registers[source].v);
}

void VirtualMachine::get_1d(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    uint8_t index = _instruct.s3;
    
    int32_t x = static_cast<int32_t>(registers[index].f[0]);
    Texture* texture = textures[source].get();
    registers[dest].v = _mm_loadu_ps(texture->pixels[x].f);
}

void VirtualMachine::get_2d(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source = _instruct.s2;
    uint8_t index = _instruct.s3;
    
    int32_t x = static_cast<int32_t>(registers[index].f[0]);
    int32_t y = static_cast<int32_t>(registers[index].f[1]);
    Texture* texture = textures[source].get();
    registers[dest].v = _mm_loadu_ps(texture->pixels[x + y * texture->width].f);
}

void VirtualMachine::max(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source1 = _instruct.s2;
    uint8_t source2 = _instruct.s3;
    
    registers[dest].v  = _mm_max_ps(registers[source1].v, registers[source2].v);
}

void VirtualMachine::min(Instruction _instruct) {
    uint8_t dest = _instruct.s1;
    uint8_t source1 = _instruct.s2;
    uint8_t source2 = _instruct.s3;
    
    registers[dest].v  = _mm_min_ps(registers[source1].v, registers[source2].v);
}