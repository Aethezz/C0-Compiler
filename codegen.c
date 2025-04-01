#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "lexer.h"
#include "parser.h"
#include "./hashmap/hashmap.h" 

#define INITIAL_HASHMAP_SIZE 100
#define FRAME_POINTER "s0" // Use s0 as frame pointer (fp alias often used)
#define WORD_SIZE 4        // RV32

// --- Global State ---
int label_count = 0;
struct hashmap_s variable_map;
int current_stack_offset = 0;

// --- Helper Functions ---

void generate_label(char *buffer, size_t len) {
    snprintf(buffer, len, "L%d", label_count++);
}

// Push a register onto the runtime stack (RV32)
void emit_push(const char *reg, FILE *file) {
    fprintf(file, "  addi sp, sp, -%d\n", WORD_SIZE);
    fprintf(file, "  sw %s, 0(sp)\n", reg);
}

// Pop from the runtime stack into a register (RV32)
void emit_pop(const char *reg, FILE *file) {
    fprintf(file, "  lw %s, 0(sp)\n", reg);
    fprintf(file, "  addi sp, sp, %d\n", WORD_SIZE);
}

// --- Forward Declaration ---
// *** Changed generate_expression to return the register holding the result ***
// *** (or indicate value is immediate, though not fully implemented here) ***
// *** For now, it still primarily uses a0, but avoids stack for simple cases ***
void generate_expression(Node *node, FILE *file); 
void generate_statement(Node *node, FILE *file);

// Generate code for an expression (leaves result primarily in a0)
// Tries to use immediate instructions where possible.
void generate_expression(Node *node, FILE *file) {
    if (!node) return;

    switch (node->type) {
        case INT:
            // Load immediate value into a0
            fprintf(file, "  li a0, %s\n", node->value);
            break;

        case IDENTIFIER: {
            // Load variable from stack into a0
            int *offset_ptr = (int *)hashmap_get(&variable_map, node->value, strlen(node->value));
            if (!offset_ptr) {
                fprintf(stderr, "CodeGen Error: Undefined variable '%s'\n", node->value);
                exit(EXIT_FAILURE);
            }
            fprintf(file, "  lw a0, %d(%s)\n", *offset_ptr, FRAME_POINTER);
            break;
        }

        case OPERATOR:
        case COMP: { // Handle arithmetic and comparison operators
            if (node->child2->type == INT) {
                // Evaluate left operand into a0
                generate_expression(node->child1, file);
                // Perform operation with immediate value
                const char* imm_val = node->child2->value;
                if (strcmp(node->value, "+") == 0) fprintf(file, "  addi a0, a0, %s\n", imm_val);
                else if (strcmp(node->value, "-") == 0) {
                    // RISC-V doesn't have subi, so add negative immediate
                    // Need to handle potential negation overflow, but basic version:
                    long val = -atol(imm_val); // Calculate negative value
                    fprintf(file, "  addi a0, a0, %ld\n", val);
                }
                else if (strcmp(node->value, "*") == 0) { // No muli, need to load immediate
                    fprintf(file, "  li a1, %s\n", imm_val);
                    fprintf(file, "  mul a0, a0, a1\n");
                }
                 else if (strcmp(node->value, "/") == 0) { // No divi
                    fprintf(file, "  li a1, %s\n", imm_val);
                    fprintf(file, "  div a0, a0, a1\n");
                 }
                 else if (strcmp(node->value, "%") == 0) { // No remi
                    fprintf(file, "  li a1, %s\n", imm_val);
                    fprintf(file, "  rem a0, a0, a1\n");
                 }
                 // Comparisons with immediate
                 else if (strcmp(node->value, "==") == 0) { fprintf(file, "  li a1, %s\n", imm_val); fprintf(file, "  sub a0, a0, a1\n"); fprintf(file, "  seqz a0, a0\n"); } // Set if == 0
                 else if (strcmp(node->value, "!=") == 0) { fprintf(file, "  li a1, %s\n", imm_val); fprintf(file, "  sub a0, a0, a1\n"); fprintf(file, "  snez a0, a0\n"); } // set if != 0
                 else if (strcmp(node->value, "<") == 0)  { fprintf(file, "  slti a0, a0, %s\n", imm_val); } // Set if less than immediate
                 else if (strcmp(node->value, "<=") == 0) { // a <= imm -> !(a > imm) -> !(sgti a, imm)
                     // sgti doesn't exist directly, simulate with slti + swap or sltiu?
                     // Simpler: a <= imm  <=> a < imm+1
                     long val_plus_1 = atol(imm_val) + 1;
                     fprintf(file, "  slti a0, a0, %ld\n", val_plus_1);
                 }
                 else if (strcmp(node->value, ">") == 0)  { // a > imm -> slti imm, a
                      fprintf(file, "  li a1, %s\n", imm_val);
                      fprintf(file, "  slt a0, a1, a0\n"); // Set if a1 < a0
                 }
                 else if (strcmp(node->value, ">=") == 0) { // a >= imm -> ! (a < imm)
                      fprintf(file, "  slti a0, a0, %s\n", imm_val); // a0 = (a < imm)
                      fprintf(file, "  xori a0, a0, 1\n");          // a0 = !(a < imm)
                 }
                 else {
                    fprintf(stderr, "CodeGen Error: Unsupported operator '%s' with immediate\n", node->value);
                    exit(EXIT_FAILURE);
                }
            } else {
                // Right operand is not immediate - use registers (t0, t1)
                // Evaluate left operand into t0
                generate_expression(node->child1, file);
                fprintf(file, "  mv t0, a0\n"); // Move result to t0

                // Evaluate right operand into t1
                generate_expression(node->child2, file);
                fprintf(file, "  mv t1, a0\n"); // Move result to t1

                // Perform operation (t0 op t1) -> result in a0
                if (strcmp(node->value, "+") == 0) fprintf(file, "  add a0, t0, t1\n");
                else if (strcmp(node->value, "-") == 0) fprintf(file, "  sub a0, t0, t1\n");
                else if (strcmp(node->value, "*") == 0) fprintf(file, "  mul a0, t0, t1\n");
                else if (strcmp(node->value, "/") == 0) fprintf(file, "  div a0, t0, t1\n");
                else if (strcmp(node->value, "%") == 0) fprintf(file, "  rem a0, t0, t1\n");
                // Comparisons (register vs register)
                else if (strcmp(node->value, "==") == 0) { fprintf(file, "  sub a0, t0, t1\n"); fprintf(file, "  seqz a0, a0\n"); }
                else if (strcmp(node->value, "!=") == 0) { fprintf(file, "  sub a0, t0, t1\n"); fprintf(file, "  snez a0, a0\n"); }
                else if (strcmp(node->value, "<") == 0)  { fprintf(file, "  slt a0, t0, t1\n"); }
                else if (strcmp(node->value, "<=") == 0) { fprintf(file, "  sgt a0, t0, t1\n"); fprintf(file, "  xori a0, a0, 1\n");} // !(t0 > t1)
                else if (strcmp(node->value, ">") == 0)  { fprintf(file, "  sgt a0, t0, t1\n"); }
                else if (strcmp(node->value, ">=") == 0) { fprintf(file, "  slt a0, t0, t1\n"); fprintf(file, "  xori a0, a0, 1\n");} // !(t0 < t1)
                else {
                    fprintf(stderr, "CodeGen Error: Unsupported operator '%s'\n", node->value);
                    exit(EXIT_FAILURE);
                }
            }
            break; // End OPERATOR/COMP case
        } // End OPERATOR/COMP block

        default:
             fprintf(stderr, "CodeGen Error: Unexpected node type in expression: %d (%s)\n", node->type, node->value ? node->value : "N/A");
    }
}

// Generate code for a statement or block
void generate_statement(Node *node, FILE *file) {
    if (!node) return;

    char label1[20], label2[20]; // Buffers for label names

    if (node->type == BEGINNING && strcmp(node->value, "PROGRAM") == 0) {
         generate_statement(node->child1, file);
         return;
    }

    switch (node->type) {
        case KEYWORD:
            if (strcmp(node->value, "EXIT") == 0) {
                generate_expression(node->child1, file); // Result in a0
                fprintf(file, "  li a7, 93\n");
                fprintf(file, "  ecall\n");
            }
            else if (strcmp(node->value, "DECLARE_INT") == 0) {
                Node* identifier_node = node->child1;
                Node* value_expression = node->child2;

                // Allocate space on stack
                current_stack_offset -= WORD_SIZE;
                int* offset_copy = malloc(sizeof(int));
                if (!offset_copy) {perror("malloc failed"); exit(EXIT_FAILURE); }
                *offset_copy = current_stack_offset;

                // Store variable info
                if (hashmap_put(&variable_map, identifier_node->value, strlen(identifier_node->value), offset_copy) != 0) {
                     fprintf(stderr, "CodeGen Error: Failed to insert variable '%s'\n", identifier_node->value);
                     free(offset_copy);
                     exit(EXIT_FAILURE);
                 }
                 fprintf(file, "  # Variable Declaration: %s at %d(%s)\n", identifier_node->value, current_stack_offset, FRAME_POINTER);

                // Evaluate initial value
                generate_expression(value_expression, file); // Result in a0

                // Store initial value
                fprintf(file, "  sw a0, %d(%s)\n", current_stack_offset, FRAME_POINTER);
            }
            else if (strcmp(node->value, "IF") == 0) {
                 generate_label(label1, sizeof(label1)); // else/end label
                 generate_label(label2, sizeof(label2)); // end label (if else exists)

                 fprintf(file, "  # IF Statement\n");

                 Node* condition = node->child1;
                 if (condition->type == COMP) {
                      // Evaluate left operand of comparison -> a0
                      generate_expression(condition->child1, file);
                      // Evaluate right operand of comparison -> a1 (or use immediate)
                      if(condition->child2->type == INT) {
                          // Compare a0 with immediate
                          const char* imm_val = condition->child2->value;
                          if (strcmp(condition->value, "==") == 0) fprintf(file, "  bne a0, %s, %s\n", imm_val, label1); // Branch if NOT equal
                          else if (strcmp(condition->value, "!=") == 0) fprintf(file, "  beq a0, %s, %s\n", imm_val, label1); // Branch if equal
                          else if (strcmp(condition->value, "<") == 0)  fprintf(file, "  bge a0, %s, %s\n", imm_val, label1); // Branch if NOT less (>=)
                          else if (strcmp(condition->value, "<=") == 0) fprintf(file, "  bgt a0, %s, %s\n", imm_val, label1); // Branch if greater
                          else if (strcmp(condition->value, ">") == 0)  fprintf(file, "  ble a0, %s, %s\n", imm_val, label1); // Branch if NOT greater (<=)
                          else if (strcmp(condition->value, ">=") == 0) fprintf(file, "  blt a0, %s, %s\n", imm_val, label1); // Branch if less
                          else { fprintf(stderr, "Unsupported comparison: %s\n", condition->value); exit(1); }
                      } else {
                          // Compare a0 with register a1
                          fprintf(file, "  mv t0, a0\n"); // Save left result
                          generate_expression(condition->child2, file); // Right result -> a0
                          fprintf(file, "  mv t1, a0\n"); // Move right result to t1
                          // Now compare t0 and t1
                          if (strcmp(condition->value, "==") == 0) fprintf(file, "  bne t0, t1, %s\n", label1);
                          else if (strcmp(condition->value, "!=") == 0) fprintf(file, "  beq t0, t1, %s\n", label1);
                          else if (strcmp(condition->value, "<") == 0)  fprintf(file, "  bge t0, t1, %s\n", label1);
                          else if (strcmp(condition->value, "<=") == 0) fprintf(file, "  bgt t0, t1, %s\n", label1);
                          else if (strcmp(condition->value, ">") == 0)  fprintf(file, "  ble t0, t1, %s\n", label1);
                          else if (strcmp(condition->value, ">=") == 0) fprintf(file, "  blt t0, t1, %s\n", label1);
                          else { fprintf(stderr, "Unsupported comparison: %s\n", condition->value); exit(1); }
                      }
                 } else {
                      // Fallback: Condition is not a simple comparison
                      generate_expression(condition, file); // Result (0/1) in a0
                      fprintf(file, "  beqz a0, %s\n", label1); // Branch if false (0)
                 }

                 // Generate 'then' block code
                 fprintf(file, "  # THEN Block\n");
                 generate_statement(node->child2, file);

                 // Jump past 'else' block if it exists
                 if (node->child3) {
                     fprintf(file, "  j %s\n", label2);
                 }

                 // Else/End label
                 fprintf(file, "%s:\n", label1);

                 // Generate 'else' block code
                 if (node->child3) {
                      fprintf(file, "  # ELSE Block\n");
                      generate_statement(node->child3, file);
                      fprintf(file, "%s:\n", label2); // End label after else
                 }
                 fprintf(file, "  # END IF\n");
            }
            else if (strcmp(node->value, "WHILE") == 0) {
                generate_label(label1, sizeof(label1)); // loop_start (condition check)
                generate_label(label2, sizeof(label2)); // loop_end

                fprintf(file, "  # WHILE Loop\n");
                fprintf(file, "%s:\n", label1); // Loop start label

                 Node* condition = node->child1;
                 if (condition->type == COMP) {
                      // Evaluate left operand of comparison -> a0
                      generate_expression(condition->child1, file);
                      // Evaluate right operand of comparison -> a1 (or use immediate)
                      if(condition->child2->type == INT) {
                          // Compare a0 with immediate
                          const char* imm_val = condition->child2->value;
                          // Branch to loop END (label2) if condition is FALSE
                          if (strcmp(condition->value, "==") == 0) fprintf(file, "  bne a0, %s, %s\n", imm_val, label2); // Branch if NOT equal
                          else if (strcmp(condition->value, "!=") == 0) fprintf(file, "  beq a0, %s, %s\n", imm_val, label2); // Branch if equal
                          else if (strcmp(condition->value, "<") == 0)  fprintf(file, "  bge a0, %s, %s\n", imm_val, label2); // Branch if NOT less (>=)
                          else if (strcmp(condition->value, "<=") == 0) fprintf(file, "  bgt a0, %s, %s\n", imm_val, label2); // Branch if greater
                          else if (strcmp(condition->value, ">") == 0)  fprintf(file, "  ble a0, %s, %s\n", imm_val, label2); // Branch if NOT greater (<=)
                          else if (strcmp(condition->value, ">=") == 0) fprintf(file, "  blt a0, %s, %s\n", imm_val, label2); // Branch if less
                          else { fprintf(stderr, "Unsupported comparison: %s\n", condition->value); exit(1); }
                      } else {
                          // Compare a0 with register a1
                          fprintf(file, "  mv t0, a0\n"); // Save left result
                          generate_expression(condition->child2, file); // Right result -> a0
                          fprintf(file, "  mv t1, a0\n"); // Move right result to t1
                          // Now compare t0 and t1, branch to END (label2) if FALSE
                          if (strcmp(condition->value, "==") == 0) fprintf(file, "  bne t0, t1, %s\n", label2);
                          else if (strcmp(condition->value, "!=") == 0) fprintf(file, "  beq t0, t1, %s\n", label2);
                          else if (strcmp(condition->value, "<") == 0)  fprintf(file, "  bge t0, t1, %s\n", label2);
                          else if (strcmp(condition->value, "<=") == 0) fprintf(file, "  bgt t0, t1, %s\n", label2);
                          else if (strcmp(condition->value, ">") == 0)  fprintf(file, "  ble t0, t1, %s\n", label2);
                          else if (strcmp(condition->value, ">=") == 0) fprintf(file, "  blt t0, t1, %s\n", label2);
                          else { fprintf(stderr, "Unsupported comparison: %s\n", condition->value); exit(1); }
                      }
                 } else {
                    // Fallback: Condition is not a simple comparison
                    generate_expression(condition, file); // Result (0/1) in a0
                    fprintf(file, "  beqz a0, %s\n", label2); // Branch to end if false (0)
                 }

                // Generate loop body code
                fprintf(file, "  # WHILE Body\n");
                generate_statement(node->child2, file);

                // Jump back to the condition check
                fprintf(file, "  j %s\n", label1);

                // Loop end label
                fprintf(file, "%s:\n", label2);
                fprintf(file, "  # END WHILE\n");
            }
            else if (strcmp(node->value, "WRITE") == 0) {
                 // Evaluate the expression to print
                 generate_expression(node->child2, file); // Result in a0
                 // Use printf (adjust if using direct syscall)
                 fprintf(file, "  # WRITE using printf\n");
                 fprintf(file, "  mv a1, a0\n");
                 fprintf(file, "  la a0, fmt\n");
                 fprintf(file, "  call printf\n");
             }
            break; // End KEYWORD case

        case OPERATOR:
             if (strcmp(node->value, "ASSIGN") == 0) {
                Node* identifier_node = node->child1;
                Node* value_expression = node->child2;

                // Evaluate the value expression
                generate_expression(value_expression, file); // Result in a0

                // Look up the variable's offset
                int* offset_ptr = (int*)hashmap_get(&variable_map, identifier_node->value, strlen(identifier_node->value));
                 if (!offset_ptr) {
                     fprintf(stderr, "CodeGen Error: Assignment to undeclared variable '%s'\n", identifier_node->value);
                     exit(EXIT_FAILURE);
                 }
                 fprintf(file, "  # Assignment: %s = ...\n", identifier_node->value);
                 // Store the result
                 fprintf(file, "  sw a0, %d(%s)\n", *offset_ptr, FRAME_POINTER);
            } else {
                 fprintf(stderr, "CodeGen Error: Operator '%s' cannot be a standalone statement\n", node->value);
                 exit(EXIT_FAILURE);
            }
            break; // End OPERATOR case

         case SEPARATOR:
             if (strcmp(node->value, "BLOCK") == 0) {
                  fprintf(file, "  # Entering Block\n");
                  Node *current_stmt_in_block = node->child1;
                  while (current_stmt_in_block != NULL) {
                       generate_statement(current_stmt_in_block, file);
                       current_stmt_in_block = current_stmt_in_block->next;
                  }
                  fprintf(file, "  # Exiting Block\n");
             }
             break;

        default:
            fprintf(stderr, "CodeGen Warning: Unexpected node type as statement: %d (%s)\n", node->type, node->value ? node->value : "N/A");
            break;
    }
}

// --- Main Generation Function ---

int generate_code(Node *root, const char *filename) {
  // Basic check for valid root node
  if (!root || !(root->type == BEGINNING && strcmp(root->value, "PROGRAM") == 0)) {
       fprintf(stderr, "CodeGen Error: Invalid root node provided to generate_code.\n");
       return -1;
  }

  FILE *file = fopen(filename, "w");
  if (file == NULL) {
      perror("Error opening output file");
      return -1;
  }

  // Initialize the variable map
  if (hashmap_create(INITIAL_HASHMAP_SIZE, &variable_map) != 0) {
      fprintf(stderr, "CodeGen Error: Could not create hashmap\n");
      fclose(file);
      return -1;
  }
  current_stack_offset = 0; // Reset offset for each code generation run
  label_count = 0;          // Reset label counter


  // --- Data Segment ---
  fprintf(file, ".data\n");
  fprintf(file, "fmt: .asciz \"%%d\\n\" # Format string for printing integers\n");

  // --- Text Segment ---
  fprintf(file, "\n.text\n");
  fprintf(file, ".extern printf # Declare printf if used\n");
  fprintf(file, ".globl main\n");

  // --- Main Function Prologue (RV32) ---
  fprintf(file, "\nmain:\n");
  fprintf(file, "  # Function Prologue (RV32)\n");
  emit_push("ra", file);
  emit_push(FRAME_POINTER, file);
  fprintf(file, "  mv %s, sp\n", FRAME_POINTER);
  // Calculate total stack space needed *after* variable analysis if possible
  // For now, keep fixed allocation, but know locals are above fp
  fprintf(file, "  addi sp, sp, -128 # Allocate initial stack space (adjust size as needed)\n");


  // --- Generate Code from AST ---
  fprintf(file, "\n  # Start of generated code from AST\n");
  Node *current_stmt = root->child1;
  while (current_stmt != NULL) {
      generate_statement(current_stmt, file);
      current_stmt = current_stmt->next;
  }
  fprintf(file, "  # End of generated code from AST\n\n");


  // --- Main Function Epilogue (RV32) ---
  fprintf(file, "  # Function Epilogue (RV32)\n");
  fprintf(file, "  mv sp, %s\n", FRAME_POINTER); // Deallocate locals
  emit_pop(FRAME_POINTER, file);
  emit_pop("ra", file);
  fprintf(file, "  ret\n\n");

  // --- Cleanup ---
  fclose(file);
  
  hashmap_destroy(&variable_map);

  printf("RISC-V 32-bit code generation complete (optimized): %s\n", filename);
  return 0;
}