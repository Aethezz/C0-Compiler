#ifndef CODEGEN_H_
#define CODEGEN_H_

#include <stdio.h>
#include "parser.h" // Assuming Node is defined in parser.h

int generate_code(Node *node, char *filename);
void traverse_tree(Node *node, FILE *file);
void push(char *reg, FILE *file);
void pop(char *reg, FILE *file);
void mov(char *reg1, char *reg2, FILE *file);
void create_label(FILE *file, int num);
void create_end_loop(FILE *file);
void create_loop_label(FILE *file);
void if_label(FILE *file, char *comp, int num);
void generate_operator_code(Node *node, FILE *file);

#endif