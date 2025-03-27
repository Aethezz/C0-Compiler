#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "codegen.h"

int main() {
    // Open file for reading
    FILE *file = fopen("test2.txt", "r");
    if (file == NULL) {
        printf("Error: Could not open file\n");
        exit(1);
    }
    
    // Perform lexical analysis
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

    // Parse the tokens into an AST
    Node *ast = parser(tokens);
    if (ast == NULL) {
        free(tokens);
        fclose(file);
        printf("Error: Could not generate AST\n");
        exit(1);
    }

    printf("\nAST:\n");
    print_tree(ast, 0, "root");
    
    // Generate code from the AST
    char *generated_code = generate_code(ast);
    if (generated_code == NULL) {
        free_tree(ast);
        free(tokens);
        fclose(file);
        printf("Error: Code generation failed\n");
        exit(1);
    }

    printf("\nGenerated Code:\n");
    printf("%s\n", generated_code);  // Print the generated code

    // Clean up resources
    free(generated_code);  // Assuming generated_code was dynamically allocated
    free_tree(ast);
    free(tokens);
    fclose(file);

    return 0;
}
