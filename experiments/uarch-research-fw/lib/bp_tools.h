// SPDX-License-Identifier: GPL-3.0-only
// branch predictor tools

// clobbers rax
#define IRET(reg, pfx)                          \
    "movq   " #pfx "%ss, " #pfx "%rax     \n\t" \
    "pushq  " #pfx "%rax                  \n\t" \
    "lea    8(" #pfx "%rsp), " #pfx "%rax \n\t" \
    "pushq  " #pfx "%rax                  \n\t" \
    "pushf                                \n\t" \
    "movq   " #pfx "%cs, " #pfx "%rax     \n\t" \
    "pushq  " #pfx "%rax                  \n\t" \
    "pushq  " #pfx "%" #reg "             \n\t" \
    "iretq                                \n\t" \
    "int3\n"

#define cpuid asm volatile("cpuid" ::: "eax", "ebx","ecx","edx")

// clobbers eax, ecx
#define BHB_CLEAR_POST_ALDER(pfx) \
"      mov $12, " #pfx "%ecx  \n\t" \
"      call 801f              \n\t" \
"      jmp 805f               \n\t" \
"      .align 64              \n\t" \
"801:  call 802f              \n\t" \
"      ret                    \n\t" \
"      .align 64              \n\t" \
"802:  movl $7, " #pfx "%eax  \n\t" \
"803:  jmp 804f               \n\t" \
"      nop                    \n\t" \
"804:  sub $1, " #pfx "%eax   \n\t" \
"      jnz 803b               \n\t" \
"      sub $1, " #pfx "%ecx   \n\t" \
"      jnz 801b               \n\t" \
"      ret                    \n\t" \
"805:  lfence                 \n\t"

// clobbers eax, ecx
#define BHB_CLEAR_PRE_ALDER(pfx) \
"      mov $5, " #pfx "%ecx         \n\t" \
"      call 801f                    \n\t" \
"      jmp 805f                     \n\t" \
"      .align 64                    \n\t" \
"801:  call 802f                    \n\t" \
"      ret                          \n\t" \
"      .align 64                    \n\t" \
"802:  movl $5, " #pfx "%eax        \n\t" \
"803:  jmp 804f                     \n\t" \
"      nop                          \n\t" \
"804:  sub $1, " #pfx "%eax         \n\t" \
"      jnz 803b                     \n\t" \
"      sub $1, " #pfx "%ecx         \n\t" \
"      jnz 801b                     \n\t" \
"      ret                          \n\t" \
"805:  lfence                       \n\t" \
// eax, ecx

#define BHB_RAND                                                             \
    asm volatile(                                                            \
            ".align 0x10\n\t"                                                \
            "test $0x0000001, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000002, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000004, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000008, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000010, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000020, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000040, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000080, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000100, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000200, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000400, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000800, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0001000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0002000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0004000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0008000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0010000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0020000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0040000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0080000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0100000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0200000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0400000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0800000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x1000000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x2000000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x4000000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x8000000, %[foo]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000001, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000002, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000004, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000008, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000010, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000020, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000040, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000080, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000100, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000200, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000400, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0000800, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0001000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0002000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0004000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0008000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0010000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0020000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0040000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0080000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0100000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0200000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0400000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x0800000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x1000000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x2000000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x4000000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            "test $0x8000000, %[bar]\n\tjz 1f\n\t .skip 0x8, 0x90\n\t1:\n\t" \
            :/*out*/                            \
            :/*in*/                             \
            [foo] "r"((unsigned int)random()),  \
            [bar] "r"((unsigned int)random())   \
            :/*clobber*/                        \
            "cc","memory"                       \
    );


#undef str
#define str(s) #s
#undef xstr
#define xstr(s) str(s)

#define BHB_SETUP(len, stride) \
    ".rept " xstr(len)"          \n\t"\
    "jmp 1f                      \n\t"\
    ".skip " xstr(stride)"       \n\t"\
    "1:                          \n\t"\
    ".endr                       \n\t"

