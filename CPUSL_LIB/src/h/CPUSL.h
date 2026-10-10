#ifndef CPUSL_H
#define CPUSL_H

#include "common.h"
#include "Compiler.h"
#include "vm.h"

struct CPUSL {
    Compiler compiler;
    VirtualMachine vm;
    Color* output;
    unsigned long long output_nbr;
};

CPUSL* CPUSL_create();

void CPUSL_free(CPUSL* instance);

void CPUSL_compile(CPUSL* instance, const char* _source, const char* _dest, const char* _output_name);

void CPUSL_execute(CPUSL* instance,
    const char* _source, 
    const Color* _pixels, unsigned int _width, unsigned int _height, 
    const void* _entry = nullptr, unsigned char _entry_size = 0,
    const Texture* _entry_texture = nullptr, unsigned char _texture_size = 0);

#endif