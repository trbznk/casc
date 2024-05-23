#include "parser.h"

AST *interp(AST*);

AST* interp_from_string(char*);

bool ast_match(AST*, AST*);
bool ast_match_type(AST*, AST*);

char *ast_to_string(AST*);
char *ast_to_debug_string(AST*);