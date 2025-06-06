#include <stdio.h>
#include <string.h>
#include "shogi.h"
#include "nnue_eval_stub.h"
#include "usi.h"

#define INPUT_SIZE 28
#define HIDDEN_SIZE 32

static float w1[HIDDEN_SIZE][INPUT_SIZE];
static float b1[HIDDEN_SIZE];
static float w2[HIDDEN_SIZE];
static float b2;

static int piece_index[16] = {
    -1, /* empty */
    0,  /* pawn */
    1,  /* lance */
    2,  /* knight */
    3,  /* silver */
    4,  /* gold */
    5,  /* bishop */
    6,  /* rook */
    7,  /* king */
    8,  /* pro_pawn */
    9,  /* pro_lance */
    10, /* pro_knight */
    11, /* pro_silver */
    -1, /* piece_null */
    12, /* horse */
    13  /* dragon */
};

void nnue_init(const char *path)
{
    const char *file = path ? path : "nn.bin";
    FILE *fp = fopen(file, "rb");
    if (!fp)
        return;
    fread(w1, sizeof(float), HIDDEN_SIZE*INPUT_SIZE, fp);
    fread(b1, sizeof(float), HIDDEN_SIZE, fp);
    fread(w2, sizeof(float), HIDDEN_SIZE, fp);
    fread(&b2, sizeof(float), 1, fp);
    fclose(fp);
}

static void extract_features(const tree_t *ptree, float input[INPUT_SIZE])
{
    memset(input, 0, sizeof(float)*INPUT_SIZE);
    const signed char *sq = ptree->posi.asquare;
    for (int i=0;i<nsquare;i++) {
        int p = sq[i];
        if (p>0) {
            int idx = piece_index[p];
            if (idx>=0) input[idx] += 1.0f;
        } else if (p<0) {
            int idx = piece_index[-p];
            if (idx>=0) input[14+idx] += 1.0f;
        }
    }
    unsigned int hb = ptree->posi.hand_black;
    unsigned int hw = ptree->posi.hand_white;
    input[piece_index[pawn]]      += I2HandPawn(hb);
    input[piece_index[lance]]     += I2HandLance(hb);
    input[piece_index[knight]]    += I2HandKnight(hb);
    input[piece_index[silver]]    += I2HandSilver(hb);
    input[piece_index[gold]]      += I2HandGold(hb);
    input[piece_index[bishop]]    += I2HandBishop(hb);
    input[piece_index[rook]]      += I2HandRook(hb);

    input[14+piece_index[pawn]]   += I2HandPawn(hw);
    input[14+piece_index[lance]]  += I2HandLance(hw);
    input[14+piece_index[knight]] += I2HandKnight(hw);
    input[14+piece_index[silver]] += I2HandSilver(hw);
    input[14+piece_index[gold]]   += I2HandGold(hw);
    input[14+piece_index[bishop]] += I2HandBishop(hw);
    input[14+piece_index[rook]]   += I2HandRook(hw);
}

int nnue_evaluate(const tree_t *ptree)
{
    float input[INPUT_SIZE];
    extract_features(ptree, input);

    float hidden[HIDDEN_SIZE];
    for (int h=0; h<HIDDEN_SIZE; h++) {
        float sum = b1[h];
        for (int i=0;i<INPUT_SIZE;i++)
            sum += w1[h][i]*input[i];
        hidden[h] = sum > 0 ? sum : 0; /* ReLU */
    }
    float out = b2;
    for (int h=0; h<HIDDEN_SIZE; h++)
        out += w2[h]*hidden[h];
    return (int)out;
}

/* convenience: call init from ini.c */
void nnue_auto_init(void)
{
    nnue_init(usi_get_eval_file());
}
