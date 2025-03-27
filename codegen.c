#include "codegen.h"
#include "parser.h"
#include "lexer.h"
#include "hashmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    NOT_OPERATOR
} OPERATOR_TYPE;

typedef struct {
    char *value;
    OPERATOR_TYPE type;
} Operator;     

