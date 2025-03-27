#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "codegen.h"

int main() {
    FILE *file = fopen("test2.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open file\n");
        exit(1);
    }
    Token *tokens = lexer(file);
    if (tokens == NULL) {
        printf("Error: Could not generate tokens\n");
        exit(1);
    }

    printf("Tokens:\n");
    
    // Print all tokens, including END_OF_TOKENS
    for (int i = 0; ; i++) {
        print_token(tokens[i]);
        if (tokens[i].type == END_OF_TOKENS) {
            break;  // Exit after printing the END_OF_TOKENS marker
        }
    }

    Node *ast = parser(tokens);
    if (ast == NULL) {
        free(tokens);
        fclose(file);
        printf("Error: Could not generate AST\n");
        exit(1);
    }

    printf("\nAST:\n");
    print_tree(ast, 0, "root");
    
    // free_tree(ast);
    free(tokens);
    fclose(file);

    return 0;
}