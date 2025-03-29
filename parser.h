#ifndef PARSER_H_
#define PARSER_H_

typedef struct Node
{
  char *value;
  TokenType type;
  struct Node *child1; // Renamed left -> child1 (e.g., first operand, condition, first statement)
  struct Node *child2; // Renamed right -> child2 (e.g., second operand, then-block, next statement)
  struct Node *child3; // For things like IF-ELSE (else-block)
  struct Node *next;   // For sequences of statements within a block
} Node;


Node *parser(Token *tokens);
void print_tree(Node *node, int indent, const char *identifier);
Node *init_node(Node *node, char *value, TokenType type);
void print_error(char *error_type);
void free_tree(Node *node);


#endif