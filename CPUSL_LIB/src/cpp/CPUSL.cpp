#include "../h/CPUSL.h"

#include "compiler.h"
#include "vm.h"

void CPUSL_compile(CPUSL* instance, const char* _source, const char* _dest, const char* _output_name) 
{
    instance->compiler.compile_from_file(_source, _dest, _output_name);
}

void CPUSL_execute(CPUSL* instance,
    const char* _source, 
    const Color* _pixels, uint32_t _width, uint32_t _height, 
    const void* _entry, uint8_t _entry_size, 
    const Texture* _entry_texture, uint8_t _texture_size) 
{
    VirtualMachine* vm = &instance->vm;
    vm->set_bytecode(_source);
    vm->set_pixels(_pixels, _width, _height);
    vm->set_entry(0, _entry, _entry_size);
    for (uint8_t i = 0; i < _texture_size; i++) {
        vm->set_entry_texture(i, _entry_texture[i]);
    }
    
    std::vector<Color> result = vm->interpret();
    
    free(instance->output);
    instance->output = static_cast<Color*>(malloc(result.size() * sizeof(Color)));
    memcpy(instance->output, result.data(), result.size() * sizeof(Color));
    instance->output_nbr = result.size();
}

CPUSL* CPUSL_create() {
    CPUSL* p = new CPUSL();   // () zero-initializes the plain members like output
    return p;
}

void CPUSL_free(CPUSL* instance) {
    if (!instance) return;
    free(instance->output);   // fine as long as output was allocated with malloc
    delete instance;
}