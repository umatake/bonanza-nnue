#ifndef USI_H
#define USI_H

#include "shogi.h"

void usi_set_eval_file(const char *path);
const char *usi_get_eval_file(void);
int usi_procedure(tree_t *ptree);
extern char usi_mode;

#endif
