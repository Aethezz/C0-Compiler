#include <stdio.h>
#include <stdlib.h>

#include "lexer.h"
#include "parser.h"
#include "codegen.h"

int main() {
    // Open file for reading
    FILE *file = fopen("test.txt", "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: Could not open file\n");
        return EXIT_FAILURE;
    }

    // Perform lexical analysis
    Token *tokens = lexer(file);
    fclose(file);  // Close the file immediately after tokenization
    if (tokens == NULL) {
        fprintf(stderr, "Error: Could not generate tokens\n");
        return EXIT_FAILURE;
    }

    printf("Tokens:\n");
    
    for (int i = 0;; i++)
    {
        print_token(tokens[i]);
        if (tokens[i].type == END_OF_TOKENS)
        {
            break; // Exit after printing the END_OF_TOKENS marker
        }
    }

    // Parse the tokens into an AST
    Node *ast = parser(tokens);
    if (ast == NULL) {
        free(tokens);
        fprintf(stderr, "Error: Could not generate AST\n");
        return EXIT_FAILURE;
    }

    printf("\nAST:\n");
    print_tree(ast, 0, "root");

    // Generate code from the AST
    char *output_file = "output.asm";
    int generated_code = generate_code(ast, output_file);
    if (generated_code != 0) {
        fprintf(stderr, "Error: Code generation failed\n");
        free_tree(ast);
        free(tokens);
        return EXIT_FAILURE;
    }

    printf("\nGenerated Code:\n");
    printf("Code successfully generated: %s\n", output_file);

    // Clean up resources
    free_tree(ast);
    free(tokens);

    return EXIT_SUCCESS;
}
