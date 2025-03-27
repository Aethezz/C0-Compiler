#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <search.h>
#include <ctype.h>
#include <assert.h>

#include "lexer.h"
#include "parser.h"
#include "./hashmap/hmoperators.h"
#include "./hashmap/hashmap.h"

#define MAX_STACK_SIZE_SIZE 1024

char *curly_stack[MAX_STACK_SIZE_SIZE];
size_t curly_stack_size = 0;
int curly_count = 0;
int global_curly = 0;
size_t stack_size = 0;
int current_stack_size_size = 0;
int label_number = 0;
int loop_label_number = 0;
int text_label = 0;
size_t current_stack_size[MAX_STACK_SIZE_SIZE];
const unsigned initial_size = 100;
struct hashmap_s hashmap;

typedef enum{
  ADD,
  SUB,
  DIV,
  MUL,
  MOD,
  NOT_OPERATOR
} OperatorType;

void create_label(FILE *file, int num){
  fprintf(file, "label%d:\n", num);
}

void create_end_loop(FILE *file){
  loop_label_number--;
  fprintf(file, "  j loop%d\n", loop_label_number);
}

void create_loop_label(FILE *file){
  fprintf(file, "loop%d:\n", loop_label_number);
  loop_label_number++;
}

void if_label(FILE *file, char *comp, int num){
  if(strcmp(comp, "EQ") == 0){
    fprintf(file, "  bne a0, a1, label%d\n", num);
  } else if(strcmp(comp, "NEQ") == 0){
    fprintf(file, "  beq a0, a1, label%d\n", num);
  } else if(strcmp(comp, "LESS") == 0){
    fprintf(file, "  bge a0, a1, label%d\n", num);
  } else if(strcmp(comp, "GREATER") == 0){
    fprintf(file, "  ble a0, a1, label%d\n", num);
  } else {
    printf("ERROR: Unexpected comparator\n");
    exit(1);
  }
  label_number++;
}

void stack_push(size_t value){
  current_stack_size_size++;
  current_stack_size[current_stack_size_size] = value;
}

size_t stack_pop(){
  if(current_stack_size_size == 0){
    printf("ERROR: stack is empty\n");
    exit(1);
  }
  size_t result = current_stack_size[current_stack_size_size];
  current_stack_size_size--;
  return result;
}

void push(char *reg, FILE *file){
  fprintf(file, "  addi sp, sp, -8\n");
  fprintf(file, "  sd %s, 0(sp)\n", reg);
  stack_size++;
}

void pop(char *reg, FILE *file){
  fprintf(file, "  ld %s, 0(sp)\n", reg);
  fprintf(file, "  addi sp, sp, 8\n");
  stack_size--;
}

void mov(char *reg1, char *reg2, FILE *file){
  fprintf(file, "  mv %s, %s\n", reg1, reg2);
}

OperatorType check_operator(Node *node){
  if(node->type != OPERATOR){
    return NOT_OPERATOR;
  }

  if(strcmp(node->value, "+") == 0){
    return ADD;
  }
  if(strcmp(node->value, "-") == 0){
    return SUB;
  }
  if(strcmp(node->value, "/") == 0){
    return DIV;
  }
  if(strcmp(node->value, "*") == 0){
    return MUL;
  }
  if(strcmp(node->value, "%") == 0){
    return MOD;
  }
  return NOT_OPERATOR;
}

Node *generate_operator_code(Node *node, FILE *file){
  if (node == NULL) return NULL;

  mov("a0", "a1", file);
  push("a0", file);

  Node *tmp = node;
  OperatorType oper_type = check_operator(tmp);

  while (tmp->type == OPERATOR) {
    pop("a1", file);

    tmp = tmp->right;
    if(tmp->type != OPERATOR){
      break;
    }

    mov("a0", "a1", file);

    switch (oper_type) {
      case ADD:
        fprintf(file, "  add a0, a0, a1\n");
        break;
      case SUB:
        fprintf(file, "  sub a0, a0, a1\n");
        break;
      case DIV:
        fprintf(file, "  div a0, a0, a1\n");
        break;
      case MUL:
        fprintf(file, "  mul a0, a0, a1\n");
        break;
      case MOD:
        fprintf(file, "  rem a0, a0, a1\n");
        break;
      case NOT_OPERATOR:
        printf("ERROR: Invalid Syntax\n");
        exit(1);
        break;
    }

    push("a0", file);
    oper_type = check_operator(tmp);
  }

  return node;
}

void traverse_tree(Node *node, int is_left, FILE *file, int syscall_number) {
  if (node == NULL) return;

  if (strcmp(node->value, "EXIT") == 0) {
    fprintf(file, "  li a7, 93\n");  // syscall for exit
    fprintf(file, "  ecall\n");
    return;
  }

  if (node->type == OPERATOR) {
    generate_operator_code(node, file);
  } 
  if (node->type == INT) {
    fprintf(file, "  li a0, %s\n", node->value);
    push("a0", file);
  }
  if (node->type == IDENTIFIER) {
    // Load variable into `a0`
    fprintf(file, "  ld a0, 0(sp)\n");
    push("a0", file);
  }

  traverse_tree(node->left, 1, file, syscall_number);
  traverse_tree(node->right, 0, file, syscall_number);
}

int generate_code(Node *root, char *filename){
  insert('-', "sub");
  insert('+', "add");
  insert('*', "mul");
  insert('/', "div");

  FILE *file = fopen(filename, "w");
  assert(file != NULL && "FILE COULD NOT BE OPENED\n");

  assert(hashmap_create(initial_size, &hashmap) == 0 && "ERROR: Could not create hashmap\n");

  fprintf(file, ".data\n");
  fprintf(file, "  fmt: .asciz \"%d\\n\"\n");
  fprintf(file, ".text\n");
  fprintf(file, "  .globl main\n");
  fprintf(file, "main:\n");

  traverse_tree(root, 0, file, 0);

  fclose(file);
  return 0;
}
