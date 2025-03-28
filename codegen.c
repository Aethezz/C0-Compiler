#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "parser.h"
#include "./hashmap/hmoperators.h"
#include "./hashmap/hashmap.h"

#define MAX_STACK_SIZE_SIZE 1024

size_t stack_size = 0;
int current_stack_size_size = 0;
int label_number = 0;
int loop_label_number = 0;
size_t current_stack_size[MAX_STACK_SIZE_SIZE];
const unsigned initial_size = 100;
struct hashmap_s hashmap;

typedef enum
{
  ADD,
  SUB,
  DIV,
  MUL,
  MOD,
  NOT_OPERATOR
} OperatorType;

void create_label(FILE *file, int num);
void create_end_loop(FILE *file);
void create_loop_label(FILE *file);
void if_label(FILE *file, char *comp, int num);
void stack_push(size_t value);
size_t stack_pop();
void push(char *reg, FILE *file);
void pop(char *reg, FILE *file);
void mov(char *reg1, char *reg2, FILE *file);
OperatorType check_operator(Node *node);
void generate_operator_code(Node *node, FILE *file);
void traverse_tree(Node *node, FILE *file);
int generate_code(Node *root, char *filename);

void create_label(FILE *file, int num)
{
  fprintf(file, "label%d:\n", num);
}

void create_end_loop(FILE *file)
{
  fprintf(file, "  j loop%d\n", --loop_label_number);
}

void create_loop_label(FILE *file)
{
  fprintf(file, "loop%d:\n", loop_label_number++);
}

void if_label(FILE *file, char *comp, int num)
{
  if (strcmp(comp, "EQ") == 0)
  {
    fprintf(file, "  beq a0, a1, label%d\n", num);
  }
  else if (strcmp(comp, "NEQ") == 0)
  {
    fprintf(file, "  bne a0, a1, label%d\n", num);
  }
  else if (strcmp(comp, "LESS") == 0)
  {
    fprintf(file, "  blt a0, a1, label%d\n", num);
  }
  else if (strcmp(comp, "GREATER") == 0)
  {
    fprintf(file, "  bgt a0, a1, label%d\n", num);
  }
  else
  {
    printf("ERROR: Unexpected comparator\n");
    exit(1);
  }
  label_number++;
}

void stack_push(size_t value)
{
  current_stack_size[current_stack_size_size++] = value;
}

size_t stack_pop()
{
  if (current_stack_size_size == 0)
  {
    printf("ERROR: stack is empty\n");
    exit(1);
  }
  return current_stack_size[--current_stack_size_size];
}

void push(char *reg, FILE *file)
{
  fprintf(file, "  addi sp, sp, -8\n");
  fprintf(file, "  sd %s, 0(sp)\n", reg);
  stack_size++;
}

void pop(char *reg, FILE *file)
{
  fprintf(file, "  ld %s, 0(sp)\n", reg);
  fprintf(file, "  addi sp, sp, 8\n");
  stack_size--;
}

void mov(char *reg1, char *reg2, FILE *file)
{
  fprintf(file, "  mv %s, %s\n", reg1, reg2);
}

OperatorType check_operator(Node *node)
{
  if (node->type != OPERATOR)
  {
    return NOT_OPERATOR;
  }
  if (strcmp(node->value, "+") == 0)
    return ADD;
  if (strcmp(node->value, "-") == 0)
    return SUB;
  if (strcmp(node->value, "/") == 0)
    return DIV;
  if (strcmp(node->value, "*") == 0)
    return MUL;
  if (strcmp(node->value, "%") == 0)
    return MOD;

  return NOT_OPERATOR;
}

void generate_operator_code(Node *node, FILE *file)
{
  if (!node || node->type != OPERATOR)
    return;

  // Traverse the left subtree
  traverse_tree(node->left, file);
  pop("a1", file);

  // Traverse the right subtree
  traverse_tree(node->right, file);
  pop("a0", file);

  // Perform the operation
  switch (check_operator(node))
  {
  case ADD:
    fprintf(file, "  add a0, a1, a0\n");
    break;
  case SUB:
    fprintf(file, "  sub a0, a1, a0\n");
    break;
  case MUL:
    fprintf(file, "  mul a0, a1, a0\n");
    break;
  case DIV:
    fprintf(file, "  div a0, a1, a0\n");
    break;
  case MOD:
    fprintf(file, "  rem a0, a1, a0\n");
    break;
  default:
    printf("ERROR: Invalid Syntax\n");
    exit(1);
  }
  push("a0", file);
}

void traverse_tree(Node *node, FILE *file)
{
  if (!node)
    return;

  if (strcmp(node->value, "EXIT") == 0)
  {
    fprintf(file, "  li a7, 93\n"); // syscall for exit
    fprintf(file, "  ecall\n");
    return;
  }

  if (node->type == OPERATOR)
  {
    generate_operator_code(node, file);
  }
  else if (node->type == INT)
  {
    fprintf(file, "  li a0, %s\n", node->value);
    push("a0", file);
  }
  else if (node->type == IDENTIFIER)
  {
    fprintf(file, "  ld a0, 0(sp)\n");
    push("a0", file);
  }

  traverse_tree(node->left, file);
  traverse_tree(node->right, file);
}

int generate_code(Node *root, char *filename)
{
  FILE *file = fopen(filename, "w");
  if (file == NULL)
  {
    perror("Error opening file");
    return -1;
  }

  if (hashmap_create(initial_size, &hashmap) != 0)
  {
    fprintf(stderr, "ERROR: Could not create hashmap\n");
    fclose(file);
    return -1;
  }

  fprintf(file, ".data\n");
  fprintf(file, "  fmt: .asciz \"%%d\\n\"\n");
  fprintf(file, ".text\n");
  fprintf(file, "  .globl main\n");
  fprintf(file, "main:\n");

  traverse_tree(root, file);

  fprintf(file, "  li a7, 10\n"); // Exit syscall
  fprintf(file, "  ecall\n");

  fclose(file);
  return 0;
}
