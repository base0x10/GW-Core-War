#ifndef fm_asm_h
#define fm_asm_h

#ifdef __cplusplus
extern "C" {
#endif

#include "exhaust.h"
#include "insn.h"

/* Parse the string s. If ns was not NULL, it will contain the pointer
   to the position in s where parsing stopped - this allows to parse
   multiple warriors from one string. After succesful return *warp
   will contain a warrior in exhaust format (defined in exhaust.h,
   note that it differs from original exhaust format - the warrior
   code is dynamically allocated. In case of error *ns and *warp
   are not defined. Arguments: processes, cycles, pspacesize, mindist and
   rounds are used only as predefines (in case a warrior requests them) */
   
int fm_asm_string (char *s, char **ns, warrior_t **warp, int nwarr,
                   int coresize, int processes, int cycles, int pspacesize,
                   int maxlen, int mindist, int rounds);

int fm_disasm_line (char *buf, size_t size, insn_t in, int coresize);

void fm_disasm_core (insn_t *code, int start, int end, int coresize,
                    int have_org, int org, int have_pin, int pin);

void fm_disasm_war (warrior_t *war, int coresize);

enum
{
   FM_ERR_OK,
   FM_ERR_ALLOC,
   FM_ERR_BAD_EXPR,
   FM_ERR_BAD_TOK,
   FM_ERR_OVERFLOW,
   FM_ERR_DIV_ZERO,
   FM_ERR_UNDEF_LABEL,
   FM_ERR_UNDEF_VAR,
   FM_ERR_ORG_OUTSIDE,
   FM_ERR_TOO_LONG,
   FM_ERR_UNMATCHED_FOR,
   FM_ERR_UNMATCHED_ROF,
   FM_ERR_FOR_RECURSION,
   FM_ERR_WRONG_PARAM,
   FM_ERR_UNDEF_COUNTER,
   FM_ERR_WRONG_COUNTER_VAL,
   FM_ERR_RECURSIVE_EQU
};

#ifdef __cplusplus
}
#endif

#endif
