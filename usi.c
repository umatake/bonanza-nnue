#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shogi.h"
#include "usi.h"

char usi_mode = 0;
static char eval_file[512] = "";

void usi_set_eval_file(const char *path)
{
    if(path) {
        strncpy(eval_file, path, sizeof(eval_file)-1);
        eval_file[sizeof(eval_file)-1] = '\0';
    }
}

static int parse_sfen(const char *sfen, min_posi_t *p)
{
    int file = 0, rank = 0;
    memset(p->asquare, empty, sizeof(p->asquare));
    p->hand_black = p->hand_white = 0;
    p->turn_to_move = black;

    const char *ptr = sfen;
    while (*ptr && rank < 9) {
        if (*ptr == '/') { file = 0; rank++; ptr++; continue; }
        if (*ptr >= '1' && *ptr <= '9') {
            file += *ptr - '0';
            ptr++; continue;
        }
        int piece = empty;
        int promote = 0;
        if (*ptr == '+') { promote = 1; ptr++; }
        switch(*ptr) {
            case 'P': piece = pawn; break;
            case 'L': piece = lance; break;
            case 'N': piece = knight; break;
            case 'S': piece = silver; break;
            case 'G': piece = gold; break;
            case 'B': piece = bishop; break;
            case 'R': piece = rook; break;
            case 'K': piece = king; break;
            case 'p': piece = -pawn; break;
            case 'l': piece = -lance; break;
            case 'n': piece = -knight; break;
            case 's': piece = -silver; break;
            case 'g': piece = -gold; break;
            case 'b': piece = -bishop; break;
            case 'r': piece = -rook; break;
            case 'k': piece = -king; break;
            default: return -1;
        }
        if (promote) {
            int abs_piece = abs(piece);
            switch(abs_piece) {
                case pawn: piece = piece>0 ? pro_pawn : -pro_pawn; break;
                case lance: piece = piece>0 ? pro_lance : -pro_lance; break;
                case knight: piece = piece>0 ? pro_knight : -pro_knight; break;
                case silver: piece = piece>0 ? pro_silver : -pro_silver; break;
                case bishop: piece = piece>0 ? horse : -horse; break;
                case rook:   piece = piece>0 ? dragon : -dragon; break;
                default: break;
            }
        }
        if (file >= 9 || rank >= 9) return -1;
        p->asquare[rank*9 + file] = (signed char)piece;
        file++; ptr++;
    }
    if (*ptr != ' ') return -1;
    ptr++;
    if (*ptr == 'b') p->turn_to_move = black;
    else if (*ptr == 'w') p->turn_to_move = white;
    else return -1;
    while(*ptr && *ptr != ' ') ptr++;
    if(*ptr==' ') ptr++;
    if (*ptr == '-') {
        ptr++; 
    } else {
        while (*ptr && *ptr != ' ') {
            int count = 1;
            if (*ptr >= '1' && *ptr <= '9') { count = *ptr - '0'; ptr++; }
            int piece = 0;
            switch(*ptr) {
                case 'P': piece = flag_hand_pawn; break;
                case 'L': piece = flag_hand_lance; break;
                case 'N': piece = flag_hand_knight; break;
                case 'S': piece = flag_hand_silver; break;
                case 'G': piece = flag_hand_gold; break;
                case 'B': piece = flag_hand_bishop; break;
                case 'R': piece = flag_hand_rook; break;
                default: return -1;
            }
            p->hand_black += piece * count;
            ptr++;
        }
    }
    return 1;
}

int usi_procedure(tree_t *ptree)
{
    char *last;
    const char *token = strtok_r(str_cmdline, " ", &last);
    if(!token) return 1;

    if(!strcmp(token, "usi")) {
        usi_mode = 1;
        game_status |= flag_noprompt;
        Out("id name BonanzaNNUE\n");
        Out("id author Kunihito Hoki\n");
        Out("option name EvalFile type string default nn.bin\n");
        Out("usiok\n");
        return 1;
    }
    if(!strcmp(token, "isready")) {
        Out("readyok\n");
        return 1;
    }
    if(!strcmp(token, "setoption")) {
        const char *name = strtok_r(NULL, " ", &last);
        if(name && !strcmp(name, "name")) {
            const char *opt = strtok_r(NULL, " ", &last);
            if(opt && !strcmp(opt, "EvalFile")) {
                const char *val = strtok_r(NULL, " ", &last); // skip value
                if(val && !strcmp(val, "value")) val = strtok_r(NULL, "", &last);
                if(val) usi_set_eval_file(val);
            }
        }
        return 1;
    }
    if(!strcmp(token, "usinewgame")) {
        return ini_game(ptree, &min_posi_no_handicap, flag_history, NULL, NULL);
    }
    if(!strcmp(token, "position")) {
        const char *type = strtok_r(NULL, " ", &last);
        min_posi_t mp = min_posi_no_handicap;
        if(type && !strcmp(type, "startpos")) {
            // already initial position
            token = strtok_r(NULL, " ", &last);
        } else if(type && !strcmp(type, "sfen")) {
            const char *sfen = last;
            token = strchr(sfen, ' ');
            if(token) {
                size_t len = token - sfen;
                char board[256];
                strncpy(board, sfen, len);
                board[len] = '\0';
                if(parse_sfen(board, &mp) < 0) return -1;
                last = token + 1;
            } else {
                if(parse_sfen(sfen, &mp) < 0) return -1;
                last = NULL;
            }
            token = strtok_r(NULL, " ", &last);
        }
        ini_game(ptree, &mp, flag_history, NULL, NULL);
        while(token && !strcmp(token, "moves")) {
            token = strtok_r(NULL, " ", &last);
            if(!token) break;
            unsigned int move;
            if(interpret_CSA_move(ptree, &move, token) < 0) break;
            make_move_root(ptree, move, flag_history|flag_time|flag_rep|flag_detect_hang|flag_rejections);
            token = strtok_r(NULL, " ", &last);
        }
        return 1;
    }
    if(!strcmp(token, "go")) {
        get_elapsed(&time_turn_start);
        return com_turn_start(ptree, 0);
    }
    if(!strcmp(token, "stop")) {
        game_status |= flag_move_now;
        return 1;
    }
    if(!strcmp(token, "quit")) {
        game_status |= flag_quit;
        return 1;
    }
    return 1;
}
