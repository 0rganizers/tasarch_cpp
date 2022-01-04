#include "Callback.h"

#include <sys/mman.h>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

// binds the object as first argument to the function
// instead of hardcoding the bytes we could create function
// which just calls a hardcoded address with a hardcoded
// first argument and the rest of the args
// then copy the bytes of said function here and just replace the hardcoded
// values
void *create_callback(void *func, void *obj) {
    char *code = (char*) mmap(nullptr, 0x1000, PROT_READ | PROT_WRITE, 
            MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
    if(!code) {
        fprintf(stderr, "Couldn't get memory region for dynamic callbacks\n");
        exit(1);
    }

    int i = 0;
    code[i++] = 0x56; // push rsi
    code[i++] = 0x57; // push rdi
    code[i++] = 0x52; // push rdx
    code[i++] = 0x51; // push rcx
    code[i++] = 0x41; // push r8
    code[i++] = 0x50; 
    code[i++] = 0x41; // push r9
    code[i++] = 0x51; 

    code[i++] = 0x49; // mov r8, rcx
    code[i++] = 0x89; 
    code[i++] = 0xc8; 
    code[i++] = 0x48; // mov rcx, rdx
    code[i++] = 0x89; 
    code[i++] = 0xd1; 
    code[i++] = 0x48; // mov rdx, rsi
    code[i++] = 0x89; 
    code[i++] = 0xf2; 
    code[i++] = 0x48; // mov rsi, rdi
    code[i++] = 0x89; 
    code[i++] = 0xfe; 

    code[i++] = 0x48; // mov rdi, x
    code[i++] = 0xbf; 
    *((uint64_t*)(code+i)) = (uint64_t) obj;
    i += 8;
    code[i++] = 0x48; // mov rax, x
    code[i++] = 0xb8; 
    *((uint64_t*)(code+i)) = (uint64_t) func;
    i += 8;

    code[i++] = 0x48; // sub rsp, 8
    code[i++] = 0x83;
    code[i++] = 0xec;
    code[i++] = 0x08;

    code[i++] = 0xff; // call rax
    code[i++] = 0xd0;

    code[i++] = 0x48; // add rsp, 8
    code[i++] = 0x83;
    code[i++] = 0xc4;
    code[i++] = 0x08;

    code[i++] = 0x41; // pop r9
    code[i++] = 0x59; 
    code[i++] = 0x41; // pop r8
    code[i++] = 0x58; 
    code[i++] = 0x59; // pop rcx
    code[i++] = 0x5a; // pop rdx
    code[i++] = 0x5f; // pop rdi
    code[i++] = 0x5e; // pop rsi

    code[i++] = 0xc3; // ret

    mprotect(code, 0x1000, PROT_READ | PROT_EXEC);
    return code;
}
