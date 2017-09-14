#ifndef EXHAUST_H
#define EXHAUST_H
/*  exhaust.h:  Global constants, structures, and types
 * $Id: exhaust.h,v 1.1.1.1 2004/02/14 04:14:23 michal Exp $
 */  
    
/* This file is part of `exhaust', a memory array redcode simulator.
 * Copyright (C) 2002 M Joonas Pihlaja
 * Public Domain.
 */ 
    
/*
 * Global types
 *
 */ 
    
/* misc. integral types */ 
typedef unsigned char u8_t;
typedef unsigned short u16_t;
typedef unsigned long u32_t;
typedef long s32_t;


typedef u16_t field_t;

/*
 * Instructions in core:
 */
#ifndef SWIG
typedef struct insn_st
{
   field_t a, b;              /* a-value, b-value */
   u16_t i;                  /* flags, opcode, modifier, a- and b-modes */
} insn_t;
#endif

typedef struct warrior_st
{
#ifdef SWIG
   private:
#endif
   insn_t *code;    /* code of warrior */
   unsigned int len;          /* length of -"- */
   unsigned int start;       /* start relative to first insn */
   int have_pin;            /* does warrior have pin? */
   int pin;                /* pin of warrior or garbage. */
} ExhaustWarrior;
typedef ExhaustWarrior warrior_t;

#endif  /* EXHAUST_H */
