/*
 * Cerberus PKU-based Sandbox
 *
 * Check Cerberus/cerberus_ReMon/README.md for licensing terms.
 */

#include UNISTD_HDR

#define INTERP              "/lib64/ld-linux-x86-64.so.2"
#define INTERP_SHORT        "MVEE Variant %d >"
#define INTERP_ARCH         "amd64"

// MVEE_USE_MVEE_LD: if this is defined, the MVEE loader will load the ld-linux binary in
// <MVEE Root>/patched_binaries/ld-linux/amd64/, rather than the system provided LD
#define MVEE_USE_MVEE_LD

// #define MVEE_DEBUG

typedef Elf64_auxv_t Elf_auxv_t;
typedef Elf64_Ehdr   Elf_Ehdr;
typedef Elf64_Addr   Elf_Addr;
typedef Elf64_Phdr   Elf_Phdr;
typedef Elf64_Off    Elf_Off;

#define PTRSTR              "%016lx"
#define LONGINTSTR          "%ld"

#define REAL_AT_PHDR_OFFSET 0x40
#define REAL_AT_PHENT       56

#define ARCH_JMP_TO_LD(new_sp, new_entry)    \
	__asm__ __volatile__                     \
	(                                        \
		"movq %0, %%rsp\n\t"                 \
		"jmp *%1\n\t"                        \
		:: "m" (new_sp), "m" (new_entry) :); \

#define MVEE_FAKE_SYSCALL_BASE 0x6FFFFFFF
