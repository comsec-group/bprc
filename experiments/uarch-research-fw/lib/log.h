// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#define COLOR_GREEN  "\033[32m"
#define COLOR_RED    "\033[31m"
#define COLOR_BLUE   "\033[34m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_NC     "\033[0m"

#define COLOR_BG_RED        "\033[41m"
#define COLOR_BG_PRED       "\033[101m"
#define COLOR_BG_GRN        "\033[42m"
#define COLOR_BG_PGRN       "\033[102m"
#define COLOR_BG_YEL        "\033[43m"
#define COLOR_BG_PYEL       "\033[103m"
#define COLOR_BG_BLU        "\033[44m"
#define COLOR_BG_PBLU       "\033[104m"
#define COLOR_BG_MAG        "\033[45m"
#define COLOR_BG_PMAG       "\033[105m"
#define COLOR_BG_CYN        "\033[46m"
#define COLOR_BG_PCYN       "\033[106m"
#define COLOR_BG_WHT        "\033[47m"
#define COLOR_BG_PWHT       "\033[107m"

#define INFO(...)    printf("\r[" COLOR_BLUE "-" COLOR_NC"] " __VA_ARGS__)
#define SUCCESS(...) printf("\r[" COLOR_GREEN "*" COLOR_NC"] " __VA_ARGS__)
#define ERROR(...)   printf("\r[" COLOR_RED "!" COLOR_NC"] " __VA_ARGS__)
#define DEBUG(...)    printf("\r[" COLOR_YELLOW "d" COLOR_NC"] " __VA_ARGS__)

#define KINFO(...)    printk("\r[" COLOR_BLUE "-" COLOR_NC"] " __VA_ARGS__)
#define KSUCCESS(...) printk("\r[" COLOR_GREEN "*" COLOR_NC"] " __VA_ARGS__)
#define KERROR(...)   printk("\r[" COLOR_RED "!" COLOR_NC"] " __VA_ARGS__)
#define KDEBUG(...)    printk("\r[" COLOR_YELLOW "d" COLOR_NC"] " __VA_ARGS__)
#define WARN DEBUG
