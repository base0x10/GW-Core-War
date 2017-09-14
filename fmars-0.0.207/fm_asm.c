/* hash.gperf:
struct sp_tok_st { char *name; int tok; } sp_tok_t;
%%
DAT, TOK_DAT
SPL, TOK_SPL
MOV, TOK_MOV
DJN, TOK_DJN
ADD, TOK_ADD
JMZ, TOK_JMZ
SUB, TOK_SUB
MOD, TOK_MOD
CMP, TOK_SEQ
SEQ, TOK_SEQ
JMP, TOK_JMP
JMN, TOK_JMN
SNE, TOK_SNE
MUL, TOK_MUL
DIV, TOK_DIV
SLT, TOK_SLT
NOP, TOK_NOP
LDP, TOK_LDP
STP, TOK_STP
ORG, TOK_ORG
END, TOK_END
PIN, TOK_PIN
EQU, TOK_EQU
FOR, TOK_FOR
ROF, TOK_ROF
F,   TOK_F
A,   TOK_A
B,   TOK_B
AB,  TOK_AB
BA,  TOK_BA
X,   TOK_X
I,   TOK_I
*/

/* tok_tab.py:
decl = {}
decl["$"] = "DIRECT"
decl["#"] = "IMMEDIATE"
decl["@"] = "BINDIR"
decl["<"] = "BPREDEC", 10
decl[">"] = "BPOSTINC", 10
decl["*"] = "AINDIR", 13
decl["{"] = "APREDEC"
decl["}"] = "APOSTINC"
decl["("] = "LPAREN"
decl[")"] = "RPAREN"
decl[","] = "COMMA"
decl["."] = "DOT"
decl[":"] = "COLON"
decl["-"] = "MINUS", 12
decl["+"] = "PLUS", 12
decl["/"] = "OP_DIV", 13
decl["%"] = "OP_MOD", 13
decl["!"] = "BANG", 14
keys = [ord (ch) for ch in decl.keys ()]
keys.sort ()
print "static tok_t toks[] = {"
for key in range (keys[-1] + 1):
    ch = chr (key)
    if ch in decl:
        val = decl[ch]
        if type (val) == tuple:
            print "{TOK_%s, {%s}}," % val
        else:
            print "{TOK_%s, {0}}," % val
    else:
        print "{0, {0}},"
print "{TOK_UNMINUS, {14}},"
print "{TOK_UNPLUS, {14}},"
print "{TOK_EOF, {0}},"
print "{TOK_EOL, {0}}"
print "};"
print "#define LAST_TOK ((unsigned char) '%s')" % chr (keys[-1])
print "#define tok_unminus (toks[LAST_TOK + 1])"
print "#define tok_unplus (toks[LAST_TOK + 2])"
print "#define tok_eof (toks[LAST_TOK + 3])"
print "#define tok_eol (toks[LAST_TOK + 4])"
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "exhaust.h"
#include "insn.h"

#include "fm_asm.h"

#define E

typedef struct sp_tok_st
{
   char *name;
   int tok;
} sp_tok_t;

typedef struct
{
   int id;
   union
   {
      int num;
      char *str;
   } val;
} tok_t;

typedef union data_un
{
   int num;
   char *str;
   tok_t tok;
   struct stack_st *stack;
   struct sym_rec_st
   {
      char *name;
      union sym_val_un
      {
         int num;
         char *str;
         struct stack_st *stack;
      } val;
   } sym_rec;
} data_t;

typedef struct sym_rec_st sym_rec_t;

typedef struct stack_st
{
   int tail, size, head;
   data_t *buf;
} stack_t;

typedef struct
{
   int opcode, modifier, bmode, amode;
   stack_t *avalue, *bvalue;
} preasm_t;

typedef struct
{
   char *s;
   stack_t *m;
   stack_t *label_stack;
   stack_t *equs, *labels;
   stack_t *equ_stack;
   stack_t *expr;
   stack_t *stack1, *stack2;
   stack_t *org, *pin;
   preasm_t *preasm, *curline, *preasm_end;
   int *curline_predef;
   int finished;
} env_t;

#define TOK_IS_NEWLINE(t)    (t.id >= TOK_EOF && t.id <= TOK_EOL)
#define TOK_IS_TERMINATOR(t) (t.id >= TOK_EOF && t.id <= TOK_COMMA)
#define TOK_IS_OPERATOR(t) (t.id >= TOK_UNMINUS && t.id <= TOK_BPOSTINC)
#define TOK_RIGHT_ASSOC(t) (t.id >= TOK_UNMINUS && t.id <= TOK_BANG)
#define TOK_IS_ADDRMODE(t) (t.id >= TOK_AINDIR && t.id <= TOK_APREDEC)
#define TOK_IS_OPCODE(t) (t.id >= TOK_DAT && t.id <= TOK_STP)
#define TOK_BINDS_LABEL(t) (t.id >= TOK_DAT && t.id <= TOK_PIN)
#define TOK_HAS_DATA(t)    (t.id >= TOK_INT && t.id <= TOK_I)
#define TOK_IS_LABEL(t)    (t.id >= TOK_STR && t.id <= TOK_I)
#define TOK_IS_MODIFIER(t) (t.id >= TOK_F   && t.id <= TOK_I)

#define OP_UNARY(op) (op >= TOK_UNMINUS && op <= TOK_BANG)

enum
{
   TOK_UNKNOWN, TOK_EOF, TOK_EOL, TOK_COMMA, TOK_LPAREN, TOK_UNMINUS,
   TOK_UNPLUS, TOK_BANG, TOK_LTE, TOK_GTE, TOK_OP_DIV, TOK_PLUS, TOK_MINUS,
   TOK_OP_MOD, TOK_EQ, TOK_NEQ, TOK_AND, TOK_OR, TOK_AINDIR, TOK_BPREDEC,
   TOK_BPOSTINC, TOK_DIRECT, TOK_IMMEDIATE, TOK_BINDIR, TOK_APOSTINC,
   TOK_APREDEC, TOK_DAT, TOK_SPL, TOK_MOV, TOK_DJN, TOK_ADD, TOK_JMZ,
   TOK_SUB, TOK_SEQ, TOK_SNE, TOK_SLT, TOK_JMN, TOK_JMP, TOK_NOP, TOK_MUL,
   TOK_MOD, TOK_DIV, TOK_LDP, TOK_STP, TOK_ORG, TOK_END, TOK_PIN, TOK_FOR,
   TOK_EQU, TOK_ROF, TOK_INT, TOK_STR, TOK_F, TOK_A, TOK_B, TOK_AB, TOK_BA,
   TOK_X, TOK_I, TOK_RPAREN, TOK_COLON, TOK_DOT
};

/* ANSI-C code produced by gperf version 3.0.1 */
/* Command-line: gperf --ignore-case -T -t -L ANSI-C -N lookup_tok -S 1 hash.gperf  */
/* Computed positions: -k'1,3' */

#define MIN_WORD_LENGTH 1
#define MAX_WORD_LENGTH 3
#define MIN_HASH_VALUE 1
#define MAX_HASH_VALUE 108

static unsigned char gperf_downcase[256] = {
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
   21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
   40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
   59, 60, 61, 62, 63, 64, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
   108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
   91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
   108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
   123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137,
   138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
   153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
   168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182,
   183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197,
   198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212,
   213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225, 226, 227,
   228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239, 240, 241, 242,
   243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

static int gperf_case_strcmp (register const char *s1, register const char *s2)
{
   for (;;)
   {
      unsigned char c1 = gperf_downcase[(unsigned char) *s1++];
      unsigned char c2 = gperf_downcase[(unsigned char) *s2++];

      if (c1 != 0 && c1 == c2)
         continue;
      return (int) c1 - (int) c2;
   }
}

static inline unsigned int hash (register const char *str,
                                 register unsigned int len)
{
   static unsigned char asso_values[] = {
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 0, 5, 1, 0, 55, 25, 1, 60,
      10, 55, 109, 50, 15, 40, 1, 15, 109, 45, 5, 1, 109, 60, 35, 20, 109,
      109, 109, 20, 109, 109, 109, 109, 0, 5, 1, 0, 55, 25, 1, 60, 10, 55, 109,
      50, 15, 40, 1, 15, 109, 45, 5, 1, 109, 60, 35, 20, 109, 109, 109, 20,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109, 109,
      109, 109, 109, 109, 109, 109, 109
   };
   register int hval = len;

   switch (hval)
   {
      default:
         hval += asso_values[(unsigned char) str[2] + 2];
      case 2:
      case 1:
         hval += asso_values[(unsigned char) str[0]];
         break;
   }
   return hval;
}

static inline struct sp_tok_st *lookup_tok (register const char *str,
                                            register unsigned int len)
{
   static struct sp_tok_st wordlist[] = {
      {"A", TOK_A}, {"AB", TOK_AB}, {"B", TOK_B}, {"BA", TOK_BA},
      {"SUB", TOK_SUB}, {"SNE", TOK_SNE}, {"I", TOK_I}, {"SEQ", TOK_SEQ},
      {"ORG", TOK_ORG}, {"DJN", TOK_DJN}, {"X", TOK_X}, {"DIV", TOK_DIV},
      {"F", TOK_F}, {"ADD", TOK_ADD}, {"FOR", TOK_FOR}, {"PIN", TOK_PIN},
      {"MOV", TOK_MOV}, {"MOD", TOK_MOD}, {"SPL", TOK_SPL}, {"CMP", TOK_SEQ},
      {"STP", TOK_STP}, {"MUL", TOK_MUL}, {"DAT", TOK_DAT}, {"SLT", TOK_SLT},
      {"JMN", TOK_JMN}, {"JMZ", TOK_JMZ}, {"END", TOK_END}, {"NOP", TOK_NOP},
      {"EQU", TOK_EQU}, {"LDP", TOK_LDP}, {"JMP", TOK_JMP}, {"ROF", TOK_ROF}
   };

   if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
   {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= MIN_HASH_VALUE)
      {
         register struct sp_tok_st *resword;

         switch (key - 1)
         {
            case 0:
               resword = &wordlist[0];
               goto compare;
            case 1:
               resword = &wordlist[1];
               goto compare;
            case 5:
               resword = &wordlist[2];
               goto compare;
            case 6:
               resword = &wordlist[3];
               goto compare;
            case 7:
               resword = &wordlist[4];
               goto compare;
            case 8:
               resword = &wordlist[5];
               goto compare;
            case 10:
               resword = &wordlist[6];
               goto compare;
            case 12:
               resword = &wordlist[7];
               goto compare;
            case 13:
               resword = &wordlist[8];
               goto compare;
            case 17:
               resword = &wordlist[9];
               goto compare;
            case 20:
               resword = &wordlist[10];
               goto compare;
            case 22:
               resword = &wordlist[11];
               goto compare;
            case 25:
               resword = &wordlist[12];
               goto compare;
            case 27:
               resword = &wordlist[13];
               goto compare;
            case 28:
               resword = &wordlist[14];
               goto compare;
            case 32:
               resword = &wordlist[15];
               goto compare;
            case 37:
               resword = &wordlist[16];
               goto compare;
            case 42:
               resword = &wordlist[17];
               goto compare;
            case 47:
               resword = &wordlist[18];
               goto compare;
            case 48:
               resword = &wordlist[19];
               goto compare;
            case 52:
               resword = &wordlist[20];
               goto compare;
            case 57:
               resword = &wordlist[21];
               goto compare;
            case 62:
               resword = &wordlist[22];
               goto compare;
            case 67:
               resword = &wordlist[23];
               goto compare;
            case 72:
               resword = &wordlist[24];
               goto compare;
            case 77:
               resword = &wordlist[25];
               goto compare;
            case 82:
               resword = &wordlist[26];
               goto compare;
            case 87:
               resword = &wordlist[27];
               goto compare;
            case 92:
               resword = &wordlist[28];
               goto compare;
            case 97:
               resword = &wordlist[29];
               goto compare;
            case 102:
               resword = &wordlist[30];
               goto compare;
            case 107:
               resword = &wordlist[31];
               goto compare;
         }
         return 0;
       compare:
         {
            register const char *s = resword->name;

            if ((((unsigned char) *str ^ (unsigned char) *s) & ~32) == 0
                && !gperf_case_strcmp (str, s))
               return resword;
         }
      }
   }
   return 0;
}

static tok_t toks[] = {
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {TOK_BANG, {14}},
   {0, {0}}, {TOK_IMMEDIATE, {0}}, {TOK_DIRECT, {0}}, {TOK_OP_MOD, {13}},
   {0, {0}}, {0, {0}}, {TOK_LPAREN, {0}}, {TOK_RPAREN, {0}},
   {TOK_AINDIR, {13}}, {TOK_PLUS, {12}}, {TOK_COMMA, {0}}, {TOK_MINUS, {12}},
   {TOK_DOT, {0}}, {TOK_OP_DIV, {13}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {TOK_COLON, {0}}, {0, {0}}, {TOK_BPREDEC, {10}}, {0, {0}},
   {TOK_BPOSTINC, {10}}, {0, {0}}, {TOK_BINDIR, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}}, {0, {0}},
   {TOK_APREDEC, {0}}, {0, {0}}, {TOK_APOSTINC, {0}}, {TOK_UNMINUS, {14}},
   {TOK_UNPLUS, {14}}, {TOK_EOF, {0}}, {TOK_EOL, {0}}
};

#define LAST_TOK ((unsigned char) '}')
#define tok_unminus (toks[LAST_TOK + 1])
#define tok_unplus (toks[LAST_TOK + 2])
#define tok_eof (toks[LAST_TOK + 3])
#define tok_eol (toks[LAST_TOK + 4])

static void *realloc2 (void *ptr, size_t size)
{
   void *nptr;

   if (nptr = realloc (ptr, size))
      return nptr;
   free (ptr);
   return NULL;
}

static stack_t *alloc_stack (int size)
{
   stack_t *s;
   data_t *buf;

   s = (stack_t *) malloc (sizeof (stack_t));
   buf = (data_t *) malloc (size * sizeof (data_t));
   if (!s || !buf)
      return free (s), free (buf), NULL;

   s->buf = buf;
   s->size = size;
   s->head = s->tail = 0;
   return s;
}

static void free_stack (stack_t *s)
{
   if (s)
   {
      free (s->buf);
      free (s);
   }
}

static inline int push_ (stack_t *s, data_t val)
{
   data_t *nptr;

   if (s->tail == s->size)
   {
      nptr = (data_t *) realloc (s->buf, 2 * s->size * sizeof (data_t));
      if (!nptr)
         return FM_ERR_ALLOC;
      s->size *= 2;
      s->buf = nptr;
   }

   s->buf[s->tail++] = val;
   return 0;
}

#define push(s, val) push_ ((s), (data_t) (val))

static inline data_t pop (stack_t *s)
{
   return s->buf[--s->tail];
}

static inline data_t top (stack_t *s)
{
   return s->buf[s->tail - 1];
}

static inline data_t pop_front (stack_t *s)
{
   return s->buf[s->head++];
}

static inline data_t front (stack_t *s)
{
   return s->buf[s->head];
}

static inline int empty (stack_t *s)
{
   return s->head == s->tail;
}

static inline data_t *top_ptr (stack_t *s)
{
   return s->buf + s->tail - 1;
}

static sym_rec_t *define_ (stack_t *s, char *name, union sym_val_un val)
{
   sym_rec_t rec;

   rec.name = name;
   rec.val = val;
   if (push (s, rec))
      return NULL;
   return (sym_rec_t *) top_ptr (s);
}

#define define(s, name, val) define_ ((s), (name), (union sym_val_un) (val))

static sym_rec_t *lookup (stack_t *s, char *name)
{
   sym_rec_t *rec;

   for (rec = (sym_rec_t *) top_ptr (s); rec >= (sym_rec_t *) s->buf; rec--)
      if (rec->name && !strcmp (rec->name, name))
         return rec;
   return NULL;
}

static void undef (stack_t *s, char *name)
{
   sym_rec_t *rec;

   if (rec = lookup (s, name))
      rec->name = NULL;
   while (s->tail && !top (s).sym_rec.name)
      pop (s);
}

static int save_stack (stack_t *ms, stack_t *s)
{
   if (push (ms, s))
      return FM_ERR_ALLOC;
   if (push (ms, (char *) s->buf))
      return pop (ms), FM_ERR_ALLOC;
   return 0;
}

static sym_rec_t *add_predef (env_t *p, char *name, int val)
{
   tok_t tok;
   stack_t *equ;

   if (!(equ = alloc_stack (1)) || save_stack (p->m, equ))
      return free_stack (equ), NULL;
   tok.id = TOK_INT;
   tok.val.num = val;
   push (equ, tok);
   return define (p->equs, name, equ);
}

static inline int read_tok (env_t *p, tok_t *tok)
{
   unsigned found_eol, i, j, len, cnt;
   char nbuf[32], *buf, *cbuf, *s;
   sym_rec_t *pptr;
   sp_tok_t *tptr;
   stack_t *equ;

   s = p->s;
   found_eol = 0;

   while (isspace (*s) || *s == ';')
   {
      if (*s == ';')
         while (*++s && *s != '\n' && *s != '\r') ;
      if (*s == 0)
         break;
      if (*s == '\n' || *s == '\r')
         found_eol = 1;
      while (*++s == ' ' || *s == '\t') ;
   }

   if (!*s)
      return p->s = s, tok->id = TOK_EOF, 0;

   if (found_eol)
      return p->s = s, tok->id = TOK_EOL, 0;

   if (isalpha (*s) || *s == '_')
   {
      for (i = 1; isalnum (s[i]) || s[i] == '_'; i++) ;
      if (!(buf = (char *) malloc (i + 1)))
         return FM_ERR_ALLOC;
      strncpy (buf, s, len = i);
      cbuf = NULL;

#undef E
#define E(stm) return free (buf), free (cbuf), stm;

      while (s[i] == '&' && (isalpha (s[i + 1]) || s[i + 1] == '_'))
      {
         for (j = i + 2; isalnum (s[j]) || s[j] == '_'; j++) ;
         if (!(cbuf = (char *) realloc2 (cbuf, j - i)))
            E (FM_ERR_ALLOC);
         strncpy (cbuf, s + i + 1, j - i - 1);
         cbuf[j - i - 1] = 0;

         if (!(pptr = lookup (p->equs, cbuf)))
            E (FM_ERR_UNDEF_COUNTER);
         if (empty (equ = pptr->val.stack) || front (equ).tok.id != TOK_INT)
            E (FM_ERR_WRONG_COUNTER_VAL);

         cnt = snprintf (nbuf, sizeof (nbuf), "%02d", front (equ).tok.val.num);
         if ((unsigned) cnt >= sizeof (nbuf))
            E (FM_ERR_WRONG_COUNTER_VAL);

         if (!(buf = (char *) realloc2 (buf, len + cnt + 1)))
            E (FM_ERR_ALLOC);
         strcpy (buf + len, nbuf);
         len += cnt;
         i = j;
      }

      buf[len] = 0;
      free (cbuf);

      p->s = s + i;
      tok->id = (tptr = lookup_tok (buf, len)) ? tptr->tok : TOK_STR;
      tok->val.str = buf;

      return push (p->m, buf) ? free (buf), FM_ERR_ALLOC : 0;
   }

   if (isdigit (*s))
   {
      tok->val.num = strtol (s, &p->s, 10);
      tok->id = TOK_INT;
      return 0;
   }

#define RETURN(ns, t, pr) return p->s = ns, tok->id = t, tok->val.num = pr, 0

   if (s[1] == '=')
      switch (*s)
      {
         case '!': RETURN (s + 2, TOK_NEQ, 9);
         case '<': RETURN (s + 2, TOK_LTE, 10);
         case '=': RETURN (s + 2, TOK_EQ, 9);
         case '>': RETURN (s + 2, TOK_GTE, 10);
      }
   if (s[0] == '&' && s[1] == '&')
      RETURN (s + 2, TOK_AND, 5);
   if (s[0] == '|' && s[1] == '|')
      RETURN (s + 2, TOK_OR, 4);

#undef RETURN

   p->s = s + 1;

   if ((unsigned char) *s <= LAST_TOK)
      return *tok = toks[(unsigned char) *s], 0;

   return tok->id = TOK_UNKNOWN, 0;
}

static int get_tok (env_t *p, tok_t *tok)
{
   stack_t *equ;

   while (p->equ_stack->tail && empty (equ = top (p->equ_stack).stack))
   {
      equ->head = 0;
      pop (p->equ_stack);
   }
   if (p->equ_stack->tail)
      return *tok = pop_front (equ).tok, 0;
   if (p->expr)
      return *tok = empty (p->expr) ? tok_eof : pop_front (p->expr).tok, 0;
   return read_tok (p, tok);
}

static int eget_tok (env_t *p, tok_t *tokp)
{
   tok_t tok;
   sym_rec_t *eptr;
   int e;

#undef E
#define E(stm) if (e = stm) return e

   for (;;)
   {
      E (get_tok (p, &tok));
      if (TOK_IS_LABEL (tok) && (eptr = lookup (p->equs, tok.val.str)))
      {
         if (eptr->val.stack->head)
            E (FM_ERR_RECURSIVE_EQU);
         E (push (p->equ_stack, eptr->val.stack));
      }
      else
         return *tokp = tok, 0;
   }
}

static int copy_equ (env_t *p, tok_t *tokp, stack_t **ret)
{
   tok_t tok;
   stack_t *copy;
   int e;

#undef E
#define E(stm) if (e = stm) return free_stack (copy), e

   if (!(*ret = copy = alloc_stack (16)))
      return FM_ERR_ALLOC;

   do
   {
      E (get_tok (p, &tok));
      while (!TOK_IS_NEWLINE (tok))
      {
         E (push (copy, tok));
         E (get_tok (p, &tok));
      }
      E (get_tok (p, &tok));
      if (tok.id == TOK_EQU)
         E (push (copy, tok_eol));
   }
   while (tok.id == TOK_EQU);

   E (save_stack (p->m, copy));
   *tokp = tok;
   return 0;
}

static int copy_expr (env_t *p, tok_t *tokp, stack_t **ret)
{
   tok_t tok;
   stack_t *copy;
   int e;

#undef E
#define E(stm) if (e = stm) return free_stack (copy), e

   if (!(*ret = copy = alloc_stack (16)))
      return FM_ERR_ALLOC;

   for (tok = *tokp; !TOK_IS_TERMINATOR (tok);)
   {
      E (push (copy, tok));
      E (eget_tok (p, &tok));
   }

   E (save_stack (p->m, copy));
   *tokp = tok;
   return 0;
}

static inline int calc (int op, stack_t *stack, int *ret)
{
   int a, b;

   if (stack->tail < 2 - OP_UNARY (op))
      return FM_ERR_BAD_EXPR;
   b = pop (stack).num;
   a = OP_UNARY (op) ? 0 : pop (stack).num;

#define RETURN(r_) do { *ret = r_; return 0; } while (0)

   switch (op)
   {
      case TOK_PLUS:
         if (a > 0 ? b > 0 && a > LONG_MAX - b : b < 0 && a < LONG_MIN - b)
            return FM_ERR_OVERFLOW;
         RETURN (a + b);

      case TOK_MINUS:
         if (a > 0 ? b < 0 && a > LONG_MAX + b : b > 0 && a < LONG_MAX + b)
            return FM_ERR_OVERFLOW;
         RETURN (a - b);

      case TOK_AINDIR:
         if ((a < -1 || a > 1) && (b < -1 || b > 1)
             && ((a > 0) == (b > 0) ? LONG_MAX : LONG_MIN) / b / a == 0)
            return FM_ERR_OVERFLOW;
         RETURN (a * b);

      case TOK_OP_DIV:
         if (b == 0)
            return FM_ERR_DIV_ZERO;
         RETURN (a / b);

      case TOK_OP_MOD:
         if (b == 0)
            return FM_ERR_DIV_ZERO;
         RETURN (a % b);

      case TOK_BANG:
         RETURN (!b);

      case TOK_UNMINUS:
         RETURN (-b);
      case TOK_UNPLUS:
         RETURN (b);

      case TOK_GTE:
         RETURN (a >= b);

      case TOK_LTE:
         RETURN (a <= b);

      case TOK_BPREDEC:
         RETURN (a < b);

      case TOK_BPOSTINC:
         RETURN (a > b);

      case TOK_EQ:
         RETURN (a == b);

      case TOK_NEQ:
         RETURN (a != b);

      case TOK_AND:
         RETURN (a && b);

      case TOK_OR:
         RETURN (a || b);
   }

   return FM_ERR_BAD_EXPR;
#undef RETURN
}

static int eval (env_t *p, stack_t *expr, int curline, int *ret)
{
   tok_t tok;
   stack_t *pfstack, *argstack, *opstack;
   sym_rec_t *rec;
   int e, unary, prec, result;

#undef E
#define E(stm) do { if (e = stm) return e; } while (0)

   if (!expr)
      return *ret = 0;

   pfstack = p->stack1;
   argstack = opstack = p->stack2;

   pfstack->head = pfstack->tail = 0;
   opstack->head = opstack->tail = 0;

   p->expr = expr;
   unary = 1;

   for (;;)
   {
      E (eget_tok (p, &tok));
      if (TOK_IS_TERMINATOR (tok))
         break;

      if (tok.id == TOK_LPAREN)
      {
         E (push (opstack, tok));
         unary = 1;
      }
      else if (tok.id == TOK_RPAREN)
      {
         while (opstack->tail && top (opstack).tok.id != TOK_LPAREN)
            E (push (pfstack, pop (opstack).tok));
         if (!opstack->tail--)
            E (FM_ERR_BAD_EXPR);
      }
      else if (TOK_IS_OPERATOR (tok))
      {
         if (unary)
         {
            if (tok.id == TOK_MINUS)
               tok = tok_unminus;
            else if (tok.id == TOK_PLUS)
               tok = tok_unplus;
         }
         while (opstack->tail)
         {
            if ((prec = top (opstack).tok.val.num) < tok.val.num)
               break;
            if (prec == tok.val.num && TOK_RIGHT_ASSOC (tok))
               break;
            E (push (pfstack, pop (opstack).tok));
         }
         E (push (opstack, tok));
         unary = 1;
      }
      else if (TOK_HAS_DATA (tok))
      {
         E (push (pfstack, tok));
         unary = 0;
      }
      else
         E (FM_ERR_BAD_TOK);
   }

   p->expr = NULL;

   while (opstack->tail)
   {
      tok = pop (opstack).tok;
      if (tok.id == TOK_LPAREN)
         E (FM_ERR_BAD_EXPR);
      E (push (pfstack, tok));
   }

   argstack->head = argstack->tail = 0;

   while (!empty (pfstack))
   {
      tok = pop_front (pfstack).tok;
      if (tok.id == TOK_INT)
         E (push (argstack, tok.val.num));
      else if (TOK_IS_LABEL (tok))
      {
         if (!(rec = lookup (p->labels, tok.val.str)))
            E (FM_ERR_UNDEF_LABEL);
         E (push (argstack, rec->val.num - curline));
      }
      else if (TOK_IS_OPERATOR (tok))
      {
         E (calc (tok.id, argstack, &result));
         E (push (argstack, result));
      }
   }

   if (argstack->tail != 1)
      E (FM_ERR_BAD_EXPR);
   *ret = front (argstack).num;
   return 0;
}

static inline int bind_labels (stack_t *dest, stack_t *labels, int val)
{
   while (labels->tail)
      if (!define (dest, pop (labels).str, val))
         return FM_ERR_ALLOC;
   return 0;
}

static inline int skip_for0 (env_t *p, int cnt)
{
   tok_t tok;
   int e;

#undef E
#define E(stm) do { if (e = stm) return e; } while (0)

   while (cnt)
   {
      E (get_tok (p, &tok));
      if (tok.id == TOK_FOR)
         cnt++;
      else if (tok.id == TOK_ROF)
         cnt--;
      else if (!TOK_IS_LABEL (tok))
      {
         while (!TOK_IS_NEWLINE (tok))
            E (get_tok (p, &tok));
         if (tok.id == TOK_EOF)
            E (FM_ERR_UNMATCHED_FOR);
      }
   }
   return 0;
}

static int for_rof_expand (env_t *p, int times, int *counter, int level)
{
   tok_t tok;
   char *s, *label;
   sym_rec_t *rec;
   int i, e, result, *cnt_val, empty_val;
   stack_t *expr;

#undef E
#define E(stm) do { if (e = stm) return e; } while (0)

   if (level >= 32)
      return FM_ERR_FOR_RECURSION;

   s = p->s;

   for (i = 1; i <= times; i++)
   {
      if (counter)
         *counter = i;

      p->s = s;
      E (eget_tok (p, &tok));

      for (;;)
      {
         if (tok.id == TOK_EOF)
            if (level)
               E (FM_ERR_UNMATCHED_FOR);
            else
               break;

         while (TOK_IS_LABEL (tok))
         {
            E (push (p->label_stack, tok.val.str));
            E (eget_tok (p, &tok));
            if (tok.id == TOK_COLON)
               E (eget_tok (p, &tok));
         }

         if (TOK_IS_NEWLINE (tok))
         {
            E (eget_tok (p, &tok));
            continue;
         }

         if (tok.id == TOK_FOR)
         {
            cnt_val = NULL;
            if (p->label_stack->tail)
            {
               label = pop (p->label_stack).str;
               if (!(rec = add_predef (p, label, 0)))
                  E (FM_ERR_ALLOC);
               cnt_val = &rec->val.stack->buf->tok.val.num;
               E (bind_labels (p->labels, p->label_stack, *p->curline_predef));
            }
            E (eget_tok (p, &tok));
            E (copy_expr (p, &tok, &expr));
            E (eval (p, expr, *p->curline_predef, &result));
            if (result > 0)
            {
               E (for_rof_expand (p, result, cnt_val, level + 1));
               if (p->finished)
               {
                  if (!level)
                     E (skip_for0 (p, p->finished));
                  return 0;
               }
            }
            else
               E (skip_for0 (p, 1));
            if (cnt_val)
               undef (p->equs, label);
            E (eget_tok (p, &tok));
            continue;
         }

         if (tok.id == TOK_EQU)
         {
            E (copy_equ (p, &tok, &expr));

            while (p->label_stack->tail)
               if (!define (p->equs, pop (p->label_stack).str, expr))
                  E (FM_ERR_ALLOC);

            if (TOK_IS_LABEL (tok) && (rec = lookup (p->equs, tok.val.str)))
            {
               if (rec->val.stack->head)
                  E (FM_ERR_RECURSIVE_EQU);
               E (push (p->equ_stack, rec->val.stack));
               E (eget_tok (p, &tok));
            }
            continue;
         }

         if (TOK_BINDS_LABEL (tok))
            E (bind_labels (p->labels, p->label_stack, *p->curline_predef));

         if (tok.id == TOK_ROF)
            if (level)
               break;
            else
               E (FM_ERR_UNMATCHED_ROF);

         if (tok.id == TOK_ORG)
         {
            E (eget_tok (p, &tok));
            E (copy_expr (p, &tok, &p->org));
            continue;
         }

         if (tok.id == TOK_PIN)
         {
            E (eget_tok (p, &tok));
            E (copy_expr (p, &tok, &p->pin));
            continue;
         }

         if (tok.id == TOK_END)
         {
            E (eget_tok (p, &tok));
            empty_val = TOK_IS_NEWLINE (tok);
            E (copy_expr (p, &tok, &expr));
            if (!empty_val && !p->org)
               p->org = expr;
            p->finished = level;
            return 0;
         }

         if (!TOK_IS_OPCODE (tok))
            E (FM_ERR_BAD_TOK);
         if (p->curline == p->preasm_end)
            E (FM_ERR_TOO_LONG);

         p->curline->opcode = tok.id;

         E (eget_tok (p, &tok));
         if (tok.id == TOK_DOT)
         {
            E (get_tok (p, &tok));
            if (!TOK_IS_MODIFIER (tok))
               E (FM_ERR_BAD_TOK);
            p->curline->modifier = tok.id;
            E (eget_tok (p, &tok));
         }

         if (TOK_IS_ADDRMODE (tok))
         {
            p->curline->amode = tok.id;
            E (eget_tok (p, &tok));
         }

         E (copy_expr (p, &tok, &p->curline->avalue));

         if (tok.id == TOK_COMMA)
         {
            E (eget_tok (p, &tok));
            if (TOK_IS_ADDRMODE (tok))
            {
               p->curline->bmode = tok.id;
               E (eget_tok (p, &tok));
            }
            E (copy_expr (p, &tok, &p->curline->bvalue));
         }

         if (!TOK_IS_NEWLINE (tok))
            E (FM_ERR_BAD_TOK);

         p->curline++;
         ++*p->curline_predef;

         E (eget_tok (p, &tok));
      }
   }
   return 0;
}

static int mod_to_exhaust (int tok)
{
   switch (tok)
   {
      case TOK_F:  return mF;
      case TOK_A:  return mA;
      case TOK_B:  return mB;
      case TOK_AB: return mAB;
      case TOK_BA: return mBA;
      case TOK_X:  return mX;
      case TOK_I:  return mI;
   }
   return -1;
}

static int op_to_exhaust (int tok)
{
   switch (tok)
   {
      case TOK_DAT: return DAT;
      case TOK_SPL: return SPL;
      case TOK_MOV: return MOV;
      case TOK_DJN: return DJN;
      case TOK_ADD: return ADD;
      case TOK_JMZ: return JMZ;
      case TOK_SUB: return SUB;
      case TOK_SEQ: return SEQ;
      case TOK_SNE: return SNE;
      case TOK_SLT: return SLT;
      case TOK_JMN: return JMN;
      case TOK_JMP: return JMP;
      case TOK_NOP: return NOP;
      case TOK_MUL: return MUL;
      case TOK_MOD: return MODM;
      case TOK_DIV: return DIV;
      case TOK_LDP: return LDP;
      case TOK_STP: return STP;
   }
   return -1;
}

static int addr_to_exhaust (int tok)
{
   switch (tok)
   {
      case TOK_DIRECT:    return DIRECT;
      case TOK_IMMEDIATE: return IMMEDIATE;
      case TOK_BINDIR:    return BINDIRECT;
      case TOK_BPREDEC:   return BPREDEC;
      case TOK_BPOSTINC:  return BPOSTINC;
      case TOK_AINDIR:    return AINDIRECT;
      case TOK_APREDEC:   return APREDEC;
      case TOK_APOSTINC:  return APOSTINC;
   }
   return -1;
}

static int default_mod (int op, int ma, int mb)
{
   switch (op)
   {
      case TOK_NOP:
      case TOK_DAT:

         return TOK_F;

      case TOK_MOV:
      case TOK_SEQ:
      case TOK_SNE:

         if (ma == TOK_IMMEDIATE)
            return TOK_AB;
         else if (mb == TOK_IMMEDIATE)
            return TOK_B;
         return TOK_I;

      case TOK_ADD:
      case TOK_SUB:
      case TOK_MUL:
      case TOK_DIV:
      case TOK_MOD:

         if (ma == TOK_IMMEDIATE)
            return TOK_AB;
         else if (mb == TOK_IMMEDIATE)
            return TOK_B;
         return TOK_F;

      case TOK_SLT:
      case TOK_STP:
      case TOK_LDP:

         if (ma == TOK_IMMEDIATE)
            return TOK_AB;
   }
   return TOK_B;
}

int fm_asm_string (char *s, char **ns, warrior_t **warp, int nwarr,
                   int coresize, int processes, int cycles, int pspacesize,
                   int maxlen, int mindist, int rounds)
{
   env_t p;
   int e, result;
   insn_t *code, *war_code, *nptr;
   warrior_t *war;
   sym_rec_t *rec;
   preasm_t *cur;

#undef E
#define E(stm) if (e = stm) goto panic

   if (!coresize)
      return FM_ERR_WRONG_PARAM;

   p.s = s;
   p.expr = p.org = p.pin = NULL;
   p.finished = 0;

   p.m = alloc_stack (8192);
   p.label_stack = alloc_stack (8);
   p.labels = alloc_stack (128);
   p.equs = alloc_stack (64);
   p.equ_stack = alloc_stack (16);
   p.stack1 = alloc_stack (32);
   p.stack2 = alloc_stack (32);

   p.curline = p.preasm =
      (preasm_t *) calloc ((unsigned) maxlen, sizeof (preasm_t));
   war_code = (insn_t *) malloc (maxlen * sizeof (insn_t));
   war = (warrior_t *) malloc (sizeof (warrior_t));

   if (!p.m || !p.label_stack || !p.labels || !p.equs || !p.equ_stack
       || !p.stack1 || !p.stack2 || !p.preasm
       || !add_predef (&p, "WARRIORS", nwarr)
       || !add_predef (&p, "MAXPROCESSES", processes)
       || !add_predef (&p, "MAXCYCLES", cycles)
       || !add_predef (&p, "MINDISTANCE", mindist)
       || !add_predef (&p, "ROUNDS", rounds)
       || !add_predef (&p, "CORESIZE", coresize)
       || !add_predef (&p, "MAXLENGTH", maxlen)
       || !add_predef (&p, "PSPACESIZE", pspacesize)
       || !(rec = add_predef (&p, "CURLINE", 0)))
      goto panic;

   p.preasm_end = p.preasm + maxlen;
   p.curline_predef = &rec->val.stack->buf->tok.val.num;

   E (for_rof_expand (&p, 1, NULL, 0));

   E (eval (&p, p.org, 0, &war->start));
   war->have_pin = !!p.pin;
   E (eval (&p, p.pin, 0, &war->pin));
   war->len = p.curline - p.preasm;

   if ((nptr = (insn_t *) realloc (war_code, war->len * sizeof (insn_t)))
       || !war->len)
      war_code = nptr;
   war->code = war_code;

   for (cur = p.preasm, code = war->code; cur < p.curline; cur++, code++)
   {
      code->i = 0;
      code->i |= op_to_exhaust (cur->opcode);
      code->i <<= moBITS;

      if (!cur->bvalue)
         if (cur->opcode == TOK_DAT)
         {
            cur->bmode = cur->amode;
            cur->bvalue = cur->avalue;
            cur->amode = TOK_IMMEDIATE;
            cur->avalue = NULL;
         }
         else
            cur->bmode = TOK_DIRECT;

      if (!cur->amode)
         cur->amode = TOK_DIRECT;
      if (!cur->bmode)
         cur->bmode = TOK_DIRECT;
      if (!cur->modifier)
         cur->modifier = default_mod (cur->opcode, cur->amode, cur->bmode);

      code->i |= mod_to_exhaust (cur->modifier);
      code->i <<= mBITS;
      code->i |= addr_to_exhaust (cur->bmode);
      code->i <<= mBITS;
      code->i |= addr_to_exhaust (cur->amode);

      E (eval (&p, cur->avalue, code - war->code, &result));
      code->a = result % coresize + (result < 0 ? coresize : 0);
      E (eval (&p, cur->bvalue, code - war->code, &result));
      code->b = result % coresize + (result < 0 ? coresize : 0);
   }

   if (ns)
      *ns = p.s;
   *warp = war;
   e = 0;

 panic:
   free (p.preasm);
   free_stack (p.label_stack);
   free_stack (p.labels);
   free_stack (p.equs);
   free_stack (p.equ_stack);
   free_stack (p.stack1);
   free_stack (p.stack2);
   if (p.m)
      while (p.m->tail)
         free (pop (p.m).str);
   free_stack (p.m);
   if (e)
   {
      free (war_code);
      free (war);
      *warp = NULL;
   }
   return e;
}

int fm_disasm_line (char *buf, size_t size, insn_t in, int coresize)
{
   char *op_s, *mo_s, *ma_s, *mb_s;
   int af, bf;

   switch ((in.i >> opPOS) & opMASK)
   {
      case DAT:  op_s = "DAT"; break;
      case SPL:  op_s = "SPL"; break;
      case MOV:  op_s = "MOV"; break;
      case JMP:  op_s = "JMP"; break;
      case JMZ:  op_s = "JMZ"; break;
      case JMN:  op_s = "JMN"; break;
      case ADD:  op_s = "ADD"; break;
      case SUB:  op_s = "SUB"; break;
      case SEQ:  op_s = "SEQ"; break;
      case SNE:  op_s = "SNE"; break;
      case MUL:  op_s = "MUL"; break;
      case DIV:  op_s = "DIV"; break;
      case DJN:  op_s = "DJN"; break;
      case SLT:  op_s = "SLT"; break;
      case MODM: op_s = "MOD"; break;
      case NOP:  op_s = "NOP"; break;
      case LDP:  op_s = "LDP"; break;
      case STP:  op_s = "STP"; break;
      default:   op_s = "???";
   }

   switch ((in.i >> moPOS) & moMASK)
   {
      case mF:  mo_s = "F "; break;
      case mA:  mo_s = "A "; break;
      case mB:  mo_s = "B "; break;
      case mAB: mo_s = "AB"; break;
      case mBA: mo_s = "BA"; break;
      case mX:  mo_s = "X "; break;
      case mI:  mo_s = "I "; break;
      default:  mo_s = "? ";
   }

   switch ((in.i >> maPOS) & mMASK)
   {
      case DIRECT:    ma_s = "$"; break;
      case IMMEDIATE: ma_s = "#"; break;
      case AINDIRECT: ma_s = "*"; break;
      case BINDIRECT: ma_s = "@"; break;
      case APREDEC:   ma_s = "{"; break;
      case APOSTINC:  ma_s = "}"; break;
      case BPREDEC:   ma_s = "<"; break;
      case BPOSTINC:  ma_s = ">"; break;
      default:        ma_s = "?";
   }

   switch ((in.i >> mbPOS) & mMASK)
   {
      case DIRECT:    mb_s = "$"; break;
      case IMMEDIATE: mb_s = "#"; break;
      case AINDIRECT: mb_s = "*"; break;
      case BINDIRECT: mb_s = "@"; break;
      case APREDEC:   mb_s = "{"; break;
      case APOSTINC:  mb_s = "}"; break;
      case BPREDEC:   mb_s = "<"; break;
      case BPOSTINC:  mb_s = ">"; break;
      default:        mb_s = "?";
   }

   af = in.a <= coresize / 2 ? in.a : in.a - coresize;
   bf = in.b <= coresize / 2 ? in.b : in.b - coresize;

   return snprintf (buf, size, "%s.%s %s %5d, %s %5d", op_s, mo_s, ma_s, af,
                    mb_s, bf);
}

void fm_disasm_core (insn_t *code, int start, int end, int coresize,
                     int have_org, int org, int have_pin, int pin)
{
   char buf[128];
   int i, k;

   if (have_org)
      printf ("       ORG      %5d\n", org);
   
   for (i = start; i < end; i++)
   {
      k = i >= coresize ? i - coresize : i;
      fm_disasm_line (buf, sizeof (buf), code[k], coresize);
      printf ("       %s     \n", buf);
   }
   
   if (have_pin)
      printf ("       PIN      %5d\n", pin);
   printf ("       END\n");
}

void fm_disasm_war (warrior_t *war, int coresize)
{
   fm_disasm_core (war->code, 0, war->len, coresize, 1, war->start,
                   war->have_pin, war->pin);
}
