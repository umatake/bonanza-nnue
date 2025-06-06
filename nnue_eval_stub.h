#ifndef NNUE_EVAL_STUB_H
#define NNUE_EVAL_STUB_H

#include "shogi.h"

void nnue_init(const char *path);
void nnue_auto_init(void);
int nnue_evaluate(const tree_t *ptree);

#endif
