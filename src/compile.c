#include "compile.h"
#include "math.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

size_t if_global = 0;
size_t while_global = 0;

bool check_constant(node_t *node) {
    if (node->type == NUM) {
        return true;
    }
    else if (node->type == BINARY_OP) {
        binary_node_t *binary_node = (binary_node_t *) node;
        char op = binary_node->op;
        if (op == '<' || op == '>' || op == '=') {return false;}
        bool l = check_constant(binary_node->left);
        bool r = check_constant(binary_node->right);
        if (l && r) {
            return true;
        }
        else {
            return false;
        }
    }
    else {
        return false;
    }
}

size_t shift_left(int64_t val) {
    size_t pow_2 = 0;
    if (val != 0) {
        while (val != 1) {
            if (val % 2 != 0) { 
                return 0;
            }
            val /= 2;
            pow_2++;
        }
        return pow_2;
    }
    else {
        return 0;
    }
}

int64_t arith(node_t *node) {
    if (node->type == NUM) {
        num_node_t *num_node = (num_node_t *) node;
        return (num_node->value);
    }
    else {
        binary_node_t *binary_node = (binary_node_t *) node;
        int64_t left = arith(binary_node->left);
        int64_t right = arith(binary_node->right);
        char operation = binary_node->op;
        if (operation == '+') {
            return (left + right);
        }
        else if (operation == '-') {
            return (left - right);
        }
        else if (operation == '/') {
            return (left / right);
        }
        else if (operation == '*') {
            return (left * right);
        }
        else {
            assert(false);
        }
    }
}

bool compile_ast(node_t *node) {
    /* You should remove this cast to void in your solution.
     * It is just here so the code compiles without warnings. */
    node_type_t type = node->type;
    if (type == PRINT) {
        print_node_t *print_node = (print_node_t *) node;
        compile_ast(print_node->expr);
        printf("call print_int\n");
        return true;
    }
    else if (type == NUM) {
        num_node_t *num_node = (num_node_t *) node;
        printf("movq $%ld, %%rdi\n", num_node->value);
        return true;
    }
    else if (type == SEQUENCE) {
        sequence_node_t *sequence_node = (sequence_node_t *) node;
        for (size_t i = 0; i < sequence_node->statement_count; i++) {
            compile_ast(sequence_node->statements[i]);
        }
        return true;
    }
    else if (type == BINARY_OP) {
        binary_node_t *binary_node = (binary_node_t *) node;
        char op = binary_node->op;
        bool shifted = false;
        if (check_constant(node)) {
            int64_t val = arith(node);
            printf("movq $%lu, %%rdi\n", val);
            return true;
        }
        else {
            if (op == '*') {
                if (check_constant(binary_node->right)) {
                    // binary_node_t *right = (binary_node_t *) binary_node->right;
                    int64_t val = arith((node_t *) binary_node->right);
                    size_t shift = shift_left(val);
                    if (shift != 0) {
                        compile_ast(binary_node->left);
                        printf("shl $%zu, %%rdi\n", shift);
                        shifted = true;
                    }
                }
            }

            if (!shifted) {
                char op = binary_node->op;
                compile_ast((node_t *) binary_node->right);
                printf("push %%rdi\n");
                compile_ast((node_t *) binary_node->left);
                printf("pop %%rsi\n"); 
            
                //rdi is left, rsi is right

                if (op == '+') {
                    printf("addq %%rsi, %%rdi\n");
                }
                else if (op == '*') {
                    printf("imulq %%rsi, %%rdi\n");
                }
                else if (op == '-') {
                    printf("subq %%rsi, %%rdi\n");
                }
                else if (op == '/') {
                    printf("movq %%rdi, %%rax\n");
                    printf("cqto\n");
                    printf("idivq %%rsi\n");
                    printf("movq %%rax, %%rdi\n");
                }
                else {
                    printf("cmp %%rsi, %%rdi\n");
                }
            }
        }
        
        return true;
    }
    else if (type == VAR) { 
        var_node_t *var_node = (var_node_t *) node;
        printf("movq -0x%x(%%rbp), %%rdi\n", 8 * (var_node->name - 'A' + 1));
        return true;
    }
    else if (type == LET) { 
        let_node_t *let_node = (let_node_t *) node;
        compile_ast(let_node->value);
        printf("movq %%rdi, -0x%x(%%rbp)\n", 8 * (let_node->var - 'A' + 1));
        return true;
    }
    else if (type == IF) {
        size_t if_local = if_global++;
        if_node_t *if_node = (if_node_t *) node;

        binary_node_t *binary_node = if_node->condition;
        compile_ast((node_t *) binary_node);
        char operation = binary_node->op;
        if (operation == '=') {
            printf("jne ELSE_LABEL%zu\n", if_local);
        }
        else if (operation == '>') {
            printf("jng ELSE_LABEL%zu\n", if_local);
        }
        else if (operation == '<') {
            printf("jnl ELSE_LABEL%zu\n", if_local);
        }

        printf("IF_LABEL%zu:\n", if_local);
        compile_ast(if_node->if_branch);
        printf("jmp ENDIF%zu\n", if_local);

        printf("ELSE_LABEL%zu:\n", if_local);
        if (if_node->else_branch != NULL) {
            compile_ast(if_node->else_branch);
        }

        printf("ENDIF%zu:\n", if_local);
        return true;
    }
    else if (type == WHILE) {
        while_global++;
        size_t while_local = while_global;
        while_node_t *while_node = (while_node_t *) node;
        binary_node_t *binary_node = while_node->condition;
        char operation = binary_node->op;
        printf("WHILE_RETURN%zu:\n", while_local);
        compile_ast((node_t *) binary_node);
        if (operation == '=') {
            printf("jne WHILE_NOT_LABEL%zu\n", while_local);
        }
        else if (operation == '>') {
            printf("jng WHILE_NOT_LABEL%zu\n", while_local);
        }
        else if (operation == '<') {
            printf("jnl WHILE_NOT_LABEL%zu\n", while_local);
        }

        printf("WHILE_LABEL%zu:\n", while_local);
        compile_ast(while_node->body);
        printf("jmp WHILE_RETURN%zu\n", while_local);

        printf("WHILE_NOT_LABEL%zu:\n", while_local);
        return true;
    }
    return false; // for now, every statement causes a compilation failure
}
