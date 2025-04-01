#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Make sure lexer.h defines TokenType enum and Token struct
#include "lexer.h"
#include "parser.h"

// --- Utility Functions ---

// Error reporting
void parser_error(const char *message, size_t line_number)
{
  fprintf(stderr, "Parser Error (Line %lu): %s\n", (unsigned long)line_number, message);
  exit(EXIT_FAILURE);
}

// Node Creation
Node *create_node(char *value, TokenType type)
{
  Node *node = malloc(sizeof(Node));
  if (!node)
  {
    perror("Failed to allocate memory for Node");
    exit(EXIT_FAILURE);
  }
  // Allocate memory and copy the value
  if (value != NULL)
  {
    node->value = malloc(strlen(value) + 1);
    if (!node->value)
    {
      perror("Failed to allocate memory for Node value");
      free(node);
      exit(EXIT_FAILURE);
    }
    strcpy(node->value, value);
  }
  else
  {
    node->value = NULL; // Allow nodes without string values (like 'PROGRAM')
  }

  node->type = type;
  node->child1 = NULL;
  node->child2 = NULL;
  node->child3 = NULL;
  node->next = NULL;
  return node;
}

// Free AST
void free_tree(Node *node)
{
  if (node == NULL)
  {
    return;
  }
  free_tree(node->child1);
  free_tree(node->child2);
  free_tree(node->child3);
  free_tree(node->next); // Free statement sequences
  free(node->value);     // Free the copied string
  free(node);
}

// Print AST (Updated for new structure)
void print_tree(Node *node, int indent, const char *identifier)
{
  if (node == NULL)
  {
    return;
  }
  for (int i = 0; i < indent; i++)
  {
    printf("  ");
  }

  printf("%s -> ", identifier);
  // Print node type and value (if available)
  // You might want a function to convert TokenType enum back to string for better printing
  printf("Type: %d", node->type);
  if (node->value)
  {
    printf(", Value: \"%s\"", node->value);
  }
  printf("\n");

  print_tree(node->child1, indent + 1, "Child1");
  print_tree(node->child2, indent + 1, "Child2");
  print_tree(node->child3, indent + 1, "Child3");
  print_tree(node->next, indent, "NextStmt"); // Next statement is at the same level
}

// --- Token Handling Helper ---

// Consumes the current token if it matches the expected type and optionally value.
// Advances the token pointer. Errors out if mismatch.
Token consume_token(Token **current_token_ptr, TokenType expected_type, const char *expected_value)
{
  Token current = **current_token_ptr;
  if (current.type == END_OF_TOKENS)
  {
    char error_msg[200]; // Increased buffer size for safety
    if (expected_value)
    {
      sprintf(error_msg, "Unexpected end of input. Expected token type %d (\"%s\")", expected_type, expected_value);
    }
    else
    {
      sprintf(error_msg, "Unexpected end of input. Expected token type %d", expected_type);
    }
    parser_error(error_msg, current.line_num);
  }
  if (current.type != expected_type)
  {
    char error_msg[100];
    sprintf(error_msg, "Expected token type %d but got %d ('%s')", expected_type, current.type, current.value);
    parser_error(error_msg, current.line_num);
  }
  if (expected_value != NULL && strcmp(current.value, expected_value) != 0)
  {
    char error_msg[100];
    sprintf(error_msg, "Expected token value '%s' but got '%s'", expected_value, current.value);
    parser_error(error_msg, current.line_num);
  }
  (*current_token_ptr)++; // Advance the pointer in the caller
  return current;
}

// Peeks at the current token type without consuming
TokenType peek_token_type(Token **current_token_ptr)
{
  return (**current_token_ptr).type;
}

// --- Parsing Functions ---

// Forward declarations for recursive parsing
Node *parse_statement(Token **current_token_ptr);
Node *parse_expression(Token **current_token_ptr);
Node *parse_block(Token **current_token_ptr);

// Parses a simple factor (INT, IDENTIFIER)
Node *parse_factor(Token **current_token_ptr)
{
  Token current = **current_token_ptr;
  Node *node = NULL;

  if (current.type == INT || current.type == IDENTIFIER || current.type == STRING)
  {
    node = create_node(current.value, current.type);
    (*current_token_ptr)++; // Consume the token
  }
  // TODO: Add handling for parenthesized expressions '(' expression ')' here
  else
  {
    parser_error("Expected Integer, Identifier, String, or '('", current.line_num);
  }
  return node;
}

// Parses a simple expression (Factor [OPERATOR Factor]) - VERY basic!
// Does NOT handle precedence or associativity correctly.
Node *parse_expression(Token **current_token_ptr)
{
  Node *left_node = parse_factor(current_token_ptr);

  // Check if the next token is an operator (or comparison)
  TokenType next_type = peek_token_type(current_token_ptr);
  if (next_type == OPERATOR || next_type == COMP)
  {
    Token op_token = consume_token(current_token_ptr, next_type, NULL);
    Node *op_node = create_node(op_token.value, op_token.type);
    Node *right_node = parse_factor(current_token_ptr); // Parse the right side

    op_node->child1 = left_node;
    op_node->child2 = right_node;
    return op_node; // Return the operation node
  }
  else
  {
    // It's just a single factor
    return left_node;
  }
}

// Parses an EXIT statement: EXIT ( expression ) ;
Node *parse_exit_statement(Token **current_token_ptr)
{
  consume_token(current_token_ptr, KEYWORD, "EXIT");
  Node *exit_node = create_node("EXIT", KEYWORD);

  consume_token(current_token_ptr, SEPARATOR, "(");
  exit_node->child1 = parse_expression(current_token_ptr); // Argument
  consume_token(current_token_ptr, SEPARATOR, ")");
  consume_token(current_token_ptr, SEPARATOR, ";");

  return exit_node;
}

// Parses a WRITE statement: WRITE ( expression, expression ) ;
Node *parse_write_statement(Token **current_token_ptr)
{
  consume_token(current_token_ptr, KEYWORD, "WRITE");
  Node *write_node = create_node("WRITE", KEYWORD);

  consume_token(current_token_ptr, SEPARATOR, "(");
  write_node->child1 = parse_expression(current_token_ptr); // First arg (string/identifier)
  consume_token(current_token_ptr, SEPARATOR, ",");
  write_node->child2 = parse_expression(current_token_ptr); // Second arg (length/value)
  consume_token(current_token_ptr, SEPARATOR, ")");
  consume_token(current_token_ptr, SEPARATOR, ";");

  return write_node;
}

// Parses variable declaration or assignment
// INT identifier = expression ;  OR  identifier = expression ;
Node *parse_assignment_or_declaration(Token **current_token_ptr)
{
  Node *node = NULL;
  Token first_token = **current_token_ptr;

  if (first_token.type == KEYWORD && strcmp(first_token.value, "INT") == 0)
  {
    // Declaration: INT identifier = expression ;
    consume_token(current_token_ptr, KEYWORD, "INT");
    node = create_node("DECLARE_INT", KEYWORD); // Use a specific type

    Token identifier_token = consume_token(current_token_ptr, IDENTIFIER, NULL);
    node->child1 = create_node(identifier_token.value, IDENTIFIER); // Store identifier name

    consume_token(current_token_ptr, OPERATOR, "=");
    node->child2 = parse_expression(current_token_ptr); // Store initial value expression

    consume_token(current_token_ptr, SEPARATOR, ";");
  }
  else if (first_token.type == IDENTIFIER)
  {
    // Assignment: identifier = expression ;
    Token identifier_token = consume_token(current_token_ptr, IDENTIFIER, NULL);
    node = create_node("ASSIGN", OPERATOR); // Use a specific type

    node->child1 = create_node(identifier_token.value, IDENTIFIER); // Store identifier name

    consume_token(current_token_ptr, OPERATOR, "=");
    node->child2 = parse_expression(current_token_ptr); // Store value expression

    consume_token(current_token_ptr, SEPARATOR, ";");
  }
  else
  {
    parser_error("Expected 'INT' keyword or Identifier for declaration/assignment", first_token.line_num);
  }
  return node;
}

// Parses an IF statement: IF ( expression ) statement_or_block [ ELSE statement_or_block ]
Node *parse_if_statement(Token **current_token_ptr)
{
  consume_token(current_token_ptr, KEYWORD, "IF");
  Node *if_node = create_node("IF", KEYWORD);

  consume_token(current_token_ptr, SEPARATOR, "(");
  if_node->child1 = parse_expression(current_token_ptr); // Condition
  consume_token(current_token_ptr, SEPARATOR, ")");

  // Parse the 'then' part (can be a single statement or a block)
  if (peek_token_type(current_token_ptr) == SEPARATOR && strcmp((**current_token_ptr).value, "{") == 0)
  {
    if_node->child2 = parse_block(current_token_ptr);
  }
  else
  {
    if_node->child2 = parse_statement(current_token_ptr);
  }

  return if_node;
}

// Parses a WHILE statement: WHILE ( expression ) statement_or_block
Node *parse_while_statement(Token **current_token_ptr)
{
  consume_token(current_token_ptr, KEYWORD, "WHILE");
  Node *while_node = create_node("WHILE", KEYWORD);

  consume_token(current_token_ptr, SEPARATOR, "(");
  while_node->child1 = parse_expression(current_token_ptr); // Condition
  consume_token(current_token_ptr, SEPARATOR, ")");

  // Parse the body (can be a single statement or a block)
  if (peek_token_type(current_token_ptr) == SEPARATOR && strcmp((**current_token_ptr).value, "{") == 0)
  {
    while_node->child2 = parse_block(current_token_ptr);
  }
  else
  {
    while_node->child2 = parse_statement(current_token_ptr);
  }

  return while_node;
}

// Parses a statement based on the current token
Node *parse_statement(Token **current_token_ptr)
{
  TokenType type = peek_token_type(current_token_ptr);
  Token current = **current_token_ptr; // For checking value

  if (type == KEYWORD)
  {
    if (strcmp(current.value, "EXIT") == 0)
    {
      return parse_exit_statement(current_token_ptr);
    }
    else if (strcmp(current.value, "INT") == 0)
    {
      return parse_assignment_or_declaration(current_token_ptr);
    }
    else if (strcmp(current.value, "IF") == 0)
    {
      return parse_if_statement(current_token_ptr);
    }
    else if (strcmp(current.value, "WHILE") == 0)
    {
      return parse_while_statement(current_token_ptr);
    }
    else if (strcmp(current.value, "WRITE") == 0)
    {
      return parse_write_statement(current_token_ptr);
    }
    // Add other keywords (WHILE, FOR, etc.) here
  }
  else if (type == IDENTIFIER)
  {
    // Must be an assignment if it starts with an identifier
    return parse_assignment_or_declaration(current_token_ptr);
  }
  else if (type == SEPARATOR && strcmp(current.value, "{") == 0)
  {
    return parse_block(current_token_ptr);
  }
  else if (type == SEPARATOR && strcmp(current.value, ";") == 0)
  {
    // Empty statement
    consume_token(current_token_ptr, SEPARATOR, ";");
    return NULL; // Represent empty statement as NULL or a specific node type
  }

  // If we get here, it's an unexpected token at the start of a statement
  parser_error("Unexpected token at start of statement", current.line_num);
  return NULL; // Should not reach here
}

// Parses a block of statements: { statement* }
Node *parse_block(Token **current_token_ptr)
{
  consume_token(current_token_ptr, SEPARATOR, "{");

  Node *block_node = create_node("BLOCK", SEPARATOR); // Represents the block scope
  Node *head_statement = NULL;
  Node *last_statement = NULL;

  while (peek_token_type(current_token_ptr) != END_OF_TOKENS &&
         !(peek_token_type(current_token_ptr) == SEPARATOR && strcmp((**current_token_ptr).value, "}") == 0))
  {
    Node *statement = parse_statement(current_token_ptr);
    if (statement != NULL)
    { // Handle empty statements if they return NULL
      if (head_statement == NULL)
      {
        head_statement = statement;
        last_statement = statement;
      }
      else
      {
        last_statement->next = statement; // Link statements using 'next'
        last_statement = statement;
      }
    }
  }

  consume_token(current_token_ptr, SEPARATOR, "}");

  block_node->child1 = head_statement; // First statement in the block
  return block_node;
}

// Main Parser Function
Node *parser(Token *tokens)
{
  // Check if the input token array is empty or NULL
  if (tokens == NULL || tokens[0].type == END_OF_TOKENS)
  {
    Node *root = create_node("PROGRAM", BEGINNING); // Still return a root
    root->child1 = NULL;                            // Indicate no statements
    return root;
  }

  Token *current_token = tokens; // Pointer to the current token
  Node *root = create_node("PROGRAM", BEGINNING);
  Node *head_statement = NULL;
  Node *last_statement = NULL;

  // Expecting a sequence of statements
  // Loop as long as we haven't reached the end
  // Pass the ADDRESS of current_token so helper functions can advance it
  while (peek_token_type(&current_token) != END_OF_TOKENS)
  {
    // Parse one statement. parse_statement will advance current_token
    Node *statement = parse_statement(&current_token);

    // Add the parsed statement to the linked list of statements
    if (statement != NULL)
    { // Check if parse_statement returned a valid node (e.g., not an empty ';')
      if (head_statement == NULL)
      {
        // First statement found
        head_statement = statement;
        last_statement = statement;
      }
      else
      {
        // Add to the end of the list
        last_statement->next = statement;
        last_statement = statement; // Update the tail pointer
      }
    }
    // If statement is NULL (e.g., empty statement), we just continue to the next token
  }

  // Link the first statement of the sequence to the program root
  root->child1 = head_statement;

  return root;
}
