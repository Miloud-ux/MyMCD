#include "./AST.h"

void init_AST(AST *t) {
    if (!t) {
        // LOG: Error AST doesn't exist
        return;
    }
    t->head = t->tail = NULL;
    t->ASTNode_num = 0;
}

ASTNode *create_ast_node(Arena *a, Command *cmd, Priority p) {
    if (!cmd) {
        // LOG: Error : command doesn't exist
        return NULL;
    }

    ASTNode *node = ARENA_PUSH_OBJECT(a, ASTNode);
    // no need to check for failure since we are
    // already checking when requesting memory
    // with Arena_alloc

    node->p = p;
    node->next = NULL;
    node->cmd = cmd;
    return node;
}

bool add_ast_node(Arena *a, AST *t, Command *cmd) {
    if (!t || !cmd) {
        // TODO: check if undefined triggers the null check
        // LOG: Error tree or command doesnt exist
        return false;
    }

    // By default every command has low Priority
    Priority pr = LOW;
    ASTNode *n = create_ast_node(a, cmd, pr);

    if (!t->head) {
        t->head = t->tail = n;
    } else {
        t->tail->next = n;
        t->tail = n;
    }
    return true;
}

bool delete_ast_node(AST *t) {
    if (!t || !t->head) {
        return false;
    }

    if (!t->head->next) {
        t->head = t->tail = NULL;
        return true;
    }

    ASTNode *temp = t->head;
    while (temp->next->next != NULL) {
        temp = temp->next;
    }
    t->tail = temp;
    t->tail->next = NULL;
    return true;
    /* we don't free memory
     * because we are using arenas
     */
}
