// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#define _ul(a) ((u64)(a))
#define _ptr(a) ((void *)(a))

#define _STR(x) #x
#define STR(x)  _STR(x)

typedef long i64;
typedef int i32;

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned char u8;
typedef double f64;
#define __aligned(x) __attribute__((aligned(x)))
#ifndef noinline
#define noinline __attribute__((noinline))
#endif
