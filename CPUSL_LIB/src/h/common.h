#ifndef COMMON_H
#define COMMON_H

#include <cstdint>
#include <vector>

#define MAGIC_NUMBER 0b00100110'01001010
#define VERSION 1
#define M_PI 3.14159265358979323846

enum OpCode : uint8_t {
    OP_LOAD_SS,
    OP_LOAD_PS,
    OP_LOAD_CSS,
    OP_LOAD_CPS,
    OP_STORE_SS,
    OP_STORE_PS,
    OP_MOVE,
    OP_SET1_PS,
    OP_GET1_PS,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_CROSS_PROD,
    OP_DOT_PROD,
    OP_SQRT,
    OP_COS,
    OP_SIN,
    OP_TAN,
    OP_LENGTH,
    OP_SQR_LENGTH,
    OP_SIZE_1D,
    OP_SIZE_2D,
    OP_SET_1D,
    OP_SET_2D,
    OP_GET_1D,
    OP_GET_2D,
    OP_MAX,
    OP_MIN
};

union alignas(16) Color
{
    struct{ float r, g, b, a; };
    float f[4];
};

struct Texture
{
    std::vector<Color> pixels;
    uint32_t width;
    uint32_t height;
};

#endif