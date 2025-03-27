#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum {
    BEGINNING,
    INT,
    KEYWORD,
    SEPARATOR,
    OPERATOR,
    IDENTIFIER,
    STRING,
    COMP,
    END_OF_TOKENS,
} TokenType;

typedef struct {
    TokenType type;
    char* value;
    size_t line_num;
} Token;

size_t line_num = 0;
size_t tokens_index = 0;

void print_token(Token token){
  printf("TOKEN VALUE: ");
  printf("'");
  for(int i = 0; token.value[i] != '\0'; i++){
    printf("%c", token.value[i]);
  }
  printf("'");
  printf("\nline number: %zu", token.line_num);

  switch(token.type){
    case BEGINNING:
      printf("BEGINNING\n");
      break;
    case INT:
      printf(" TOKEN TYPE: INT\n");
      break;
    case KEYWORD:
      printf(" TOKEN TYPE: KEYWORD\n");
      break;
    case SEPARATOR:
      printf(" TOKEN TYPE: SEPARATOR\n");
      break;
    case OPERATOR:
      printf(" TOKEN TYPE: OPERATOR\n");
      break;
    case IDENTIFIER:
      printf(" TOKEN TYPE: IDENTIFIER\n");
      break;
    case STRING:
      printf(" TOKEN TYPE: STRING\n");
      break;
    case COMP:
      printf(" TOKEN TYPE: COMPARATOR\n");
      break;
    case END_OF_TOKENS:
      printf(" END OF TOKENS\n");
      break;
  }
}

Token* generate_number(char *current, int *current_index) {
    Token *token = malloc(sizeof(Token));
    token->line_num = line_num; // Directly assign the value of line_num
    token->type = INT;
    char *value = malloc(sizeof(char) * 8);
    int value_index = 0;
    while (isdigit(current[*current_index]) && current[*current_index] != '\0') {
        value[value_index] = current[*current_index];
        value_index++;
        *current_index += 1;
    }
    value[value_index] = '\0';
    token->value = value;
    return token;
}

Token* generate_keyword_or_identifier(char *current, int *current_index) {
    Token *token = malloc(sizeof(Token));
    token->line_num = line_num; // Directly assign the value of line_num
    char *keyword = malloc(sizeof(char) * 64); // Increase buffer size for longer keywords
    int keyword_index = 0;

    while (isalpha(current[*current_index]) && current[*current_index] != '\0') {
        keyword[keyword_index] = current[*current_index];
        keyword_index++;
        *current_index += 1;
    }
    keyword[keyword_index] = '\0';

    keyword[keyword_index] = '\0';
  if(strcmp(keyword, "exit") == 0){
    token->type = KEYWORD;
    token->value = "EXIT";
  } else if(strcmp(keyword, "int") == 0){
    token->type = KEYWORD;
    token->value = "INT";
  } else if(strcmp(keyword, "if") == 0){
    token->type = KEYWORD;
    token->value = "IF";
  } else if(strcmp(keyword, "while") == 0){
    token->type = KEYWORD;
    token->value = "WHILE";
  } else if(strcmp(keyword, "write") == 0){
    token->type = KEYWORD;
    token->value = "WRITE";
  } else if(strcmp(keyword, "eq") == 0){
    token->type = COMP;
    token->value = "EQ";
  } else if(strcmp(keyword, "neq") == 0){
    token->type = COMP;
    token->value = "NEQ";
  } else if(strcmp(keyword, "less") == 0){
    token->type = COMP;
    token->value = "LESS";
  } else if(strcmp(keyword, "greater") == 0){
    token->type = COMP;
    token->value = "GREATER";
  } else {
    token->type = IDENTIFIER;
    token->value = keyword;
  }
  return token;
}

Token* generate_string_token(char *current, int *current_index) {
    Token *token = malloc(sizeof(Token));
    token->line_num = line_num;
    token->type = STRING;
    char *value = malloc(sizeof(char) * 256); // Allocate memory for the string
    int value_index = 0;

    *current_index += 1; // Skip the opening quote
    while (current[*current_index] != '"' && current[*current_index] != '\0') {
        if (current[*current_index] == '\n') {
            line_num++; // Handle multiline strings
        }
        value[value_index++] = current[*current_index];
        *current_index += 1;
    }

    if (current[*current_index] == '"') {
        *current_index += 1; // Skip the closing quote
    } else {
        printf("Error: Unterminated string on line %zu\n", line_num);
        exit(1);
    }

    value[value_index] = '\0'; // Null-terminate the string
    token->value = value;
    return token;
}

Token* generate_separator_or_operator(char *current, int *current_index, TokenType type) {
    Token *token = malloc(sizeof(Token));
    token->line_num = line_num; // Directly assign the value of line_num
    char *value = malloc(sizeof(char) * 2);
    value[0] = current[*current_index];
    value[1] = '\0';
    token->value = value;
    token->type = type;
    *current_index += 1;
    return token;
}

Token *lexer(FILE *file) {
    int length;
    char *current = NULL;

    fseek(file, 0, SEEK_END);
    length = ftell(file);
    printf("File size: %d bytes\n", length); // Debugging output
    fseek(file, 0, SEEK_SET);

    current = malloc(sizeof(char) * (length + 1)); // +1 for null terminator
    if (current == NULL) {
        printf("Error: Memory allocation failed\n");
        exit(1);
    }

    size_t bytes_read = fread(current, 1, length, file);
    fclose(file);

    if (bytes_read != length) {
        printf("Warning: Bytes read (%zu) do not match file size (%d)\n", bytes_read, length);
    }

    current[length] = '\0'; // Null-terminate the string
    printf("Input buffer:\n%s\n", current); 

    int current_index = 0;

    int number_of_tokens = 12;
    Token *tokens = malloc(sizeof(Token) * number_of_tokens);
    if (tokens == NULL) {
        printf("Error: Memory allocation failed\n");
        exit(1);
    }

    size_t tokens_index = 0;

    while (current[current_index] != '\0') {
        if (isspace(current[current_index])) {
            if (current[current_index] == '\n') {
                line_num++;
            }
            current_index++;
            continue; // Skip whitespace
        }

        Token *token = NULL;

        if (current[current_index] == ';' || current[current_index] == ',' ||
            current[current_index] == '(' || current[current_index] == ')' ||
            current[current_index] == '{' || current[current_index] == '}') {
            token = generate_separator_or_operator(current, &current_index, SEPARATOR);
        } else if (current[current_index] == '=' || current[current_index] == '+' ||
                   current[current_index] == '-' || current[current_index] == '*' ||
                   current[current_index] == '/' || current[current_index] == '%') {
            token = generate_separator_or_operator(current, &current_index, OPERATOR);
        } else if (current[current_index] == '"') {
            token = generate_string_token(current, &current_index);
        } else if (isdigit(current[current_index])) {
            token = generate_number(current, &current_index);
        } else if (isalpha(current[current_index])) {
            token = generate_keyword_or_identifier(current, &current_index);
        } else if (current[current_index] == '>' || current[current_index] == '<' ||
                   current[current_index] == '=' || current[current_index] == '!') {
            if (current[current_index + 1] == '=') {
                token = generate_separator_or_operator(current, &current_index, COMP);
                current_index++; // Skip the second character of the operator
            } else {
                token = generate_separator_or_operator(current, &current_index, COMP);
            }
        } else {
            printf("Warning: Unrecognized character '%c' on line %zu\n", current[current_index], line_num);
            current_index++; // Skip the unrecognized character
            continue;
        }

        if (token != NULL) {
            if (tokens_index >= number_of_tokens) {
                number_of_tokens *= 2; // Double the size of the array
                tokens = realloc(tokens, sizeof(Token) * number_of_tokens);
                if (tokens == NULL) {
                    printf("Error: Memory allocation failed\n");
                    exit(1);
                }
            }
            tokens[tokens_index] = *token;
            tokens_index++;
            free(token); // Free the temporary token structure
        }
    }

    // Append END_OF_TOKENS
    tokens[tokens_index].value = "\0";
    tokens[tokens_index].type = END_OF_TOKENS;
    tokens[tokens_index].line_num = line_num;

    printf("END_OF_TOKENS assigned at index %zu, line number: %zu\n", tokens_index, line_num);

    free(current); // Free the input buffer
    return tokens;
}