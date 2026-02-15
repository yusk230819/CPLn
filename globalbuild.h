/*
MIT License

Copyright (c) 2025 [日食ボール]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define HAVE_ROUND
#include <python.h> //python呼び出し用 
#include "SDL.h"      // <SDL2/SDL.h> から変更
#include <stdint.h>
#include <math.h>
#include "SDL_ttf.h"  // <SDL2/SDL_ttf.h> から変更
#include <winsock2.h>


#ifndef CPLN_ENGINE_STRUCT
#define CPLN_ENGINE_STRUCT
typedef struct {
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif


#define MAX_COORD 60
#define MAX_STR 512
#define MAX_DEF 1024
#define MAX_GROUPS 128
#define MAX_GROUP_CODE 4096
#define AST_MAX_DEPTH 128
#define MAX_STACK 256
#define DATA_STACK_SIZE 1024
#define FB_WIDTH  1980
#define FB_HEIGHT  880
#define FB_MAX_W 2048
#define FB_MAX_H 2048
#define MAX_RULES 128

/* ==== CPLn binding system ==== */

typedef struct {
    char word[64];
    int ip;              /* 実行座標（instruction pointer） */
} CPLnBinding;

#define CPLN_BIND_MAX 1024

static CPLnBinding cpln_bind_table[CPLN_BIND_MAX];
static int cpln_bind_count = 0;

/* バインド用状態 */
static int cpln_bind_mode = 0;      /* 0=通常, 1=単語待ち */
static char cpln_bind_word[64];

typedef struct {
    char condition[MAX_STR];
    char action[MAX_GROUP_CODE];
} CPLnRule;

uint32_t framebuffer[FB_MAX_H][FB_MAX_W];

typedef struct {
    uint32_t* pixels;     // フレームバッファ
    int width;
    int height;
    int dirty;            // 再描画必要フラグ
} FrameBuffer;

// Engine構造体に追加
// VMState stack[MAX_STACK];
// int stack_ptr;

typedef struct {
    int tick;          // 現在フレーム
    int max_groups;    // 使用するグループ数
    int enabled;       // Clock ON/OFF
} CPLnClock;

    typedef struct {
    int data_stack[DATA_STACK_SIZE];
    int ds_ptr;
    CPLnClock clock;
    int main_mem;
    int sub_mem;
    int flip;
    int x, y;
    int coord_mode;
    int active_group;   // -1 = 未選択
    Coord coords[MAX_COORD][MAX_COORD];

    GMState gm;
    FrameBuffer fb;

    CPLnGroup groups[MAX_GROUPS];
    int group_count;
    int call_stack[MAX_STACK];
    int stack_ptr;
    int grouping;  // 0=通常 1=#中 2=]後D待ち
    char group_buffer[MAX_GROUP_CODE];
    int group_buf_pos;
    CPLnRule rules[MAX_RULES];
int rule_count;
int last_key;   // 押されたキー（ASCII / SDL）
int z_loop_enabled;
char z_loop_code[MAX_GROUP_CODE];
int sock;                // 通信用のソケット
    struct sockaddr_in addr; // 接続先アドレス
    int net_connected;      // 接続フラグ(0:未接続, 1:接続済)
} CPLnEngine;

typedef struct {
    int tick;          // 現在フレーム
    int interval;      // 切り替え周期
    int enabled;       // Clock有効/無効
} ClockState;

typedef struct {
    int m, s, x, y;
} VMState;

static void cpln_add_binding(const char *word, int ip)
{
    if (cpln_bind_count >= CPLN_BIND_MAX) return;

    strncpy(cpln_bind_table[cpln_bind_count].word, word, 63);
    cpln_bind_table[cpln_bind_count].word[63] = '\0';
    cpln_bind_table[cpln_bind_count].ip = ip;
    cpln_bind_count++;
}

static int cpln_resolve_binding(const char *word)
{
    for (int i = cpln_bind_count - 1; i >= 0; i--) {
        if (strcmp(cpln_bind_table[i].word, word) == 0) {
            return cpln_bind_table[i].ip;
        }
    }
    return -1;  /* 未束縛 */
}

typedef struct {
    char name[32];
    char code[MAX_GROUP_CODE];
    int visible;   // 1=表示, 0=非表示
} CPLnGroup;

typedef enum {
    AST_NUM,        // 数値
    AST_VAR,        // 変数 (M, S, X, Y)
    AST_ADD,        // +
    AST_SUB,        // -
    AST_MUL,        // *
    AST_DIV,        // /
    AST_MOD,        // %
    AST_NEG,        // 単項 
    AST_INVALID     // エラー用
    AST_MAIN_MEM
    AST_SUB_MEM,
    AST_SIN
    AST_COS // ← 高度な数学用に追加
}ASTType;


typedef struct ASTNode ASTNode;
typedef struct Lexer Lexer;

// ===== AST parser prototypes =====
ASTNode* parse_expr(Lexer* lx);
ASTNode* parse_term(Lexer* lx);
ASTNode* parse_unary(Lexer* lx);
ASTNode* parse_primary(Lexer* lx);
void next_token(Lexer* lx);


typedef enum {
    TOK_NUM,
    TOK_VAR,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_MOD,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_END,
    TOK_ERR
} TokenType;

typedef struct {
    int enabled;

    int screen_w;
    int screen_h;

    int sel_x1, sel_y1;
    int sel_x2, sel_y2;

    unsigned int color; // 0xRRGGBB

    // === 追加部分 ===
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Init(SDL_INIT_VIDEO);

window = SDL_CreateWindow(
    "CPLn",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    800, 600,
    SDL_WINDOW_SHOWN
);


renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    FB_MAX_W,
    FB_MAX_H
);
} GMState;

// グローバル状態に時間経過モードフラグ
typedef struct {
    int enabled;      // 現在時間経過モードか
    double delay_sec; // D(n) で設定された秒数
} TimeMode;

TimeMode tmode = {0, 0};

// run() 内 T コマンド処理
void handle_T(CPLnEngine* eng, const char* code, int* i){
    tmode.enabled = 1; // 時間経過モード開始

    // D(n) で秒数を設定する
    int len = strlen(code);
    if (*i+2 < len && code[*i+1] == 'D' && code[*i+2] == '(') {
        int j = *i + 3;
        char val[16]; int vi = 0;
        while (j < len && code[j] != ')' && vi < 15) val[vi++] = code[j++];
        val[vi] = '\0';
        tmode.delay_sec = atof(val); // 秒数設定
        *i = j; // ')' まで進める
    }
}

// 時間経過モード中の待機
void time_mode_wait() {
    if (tmode.enabled && tmode.delay_sec > 0) {
#ifdef _WIN32
        Sleep((DWORD)(tmode.delay_sec * 1000));
#else
        usleep((useconds_t)(tmode.delay_sec * 1000000));
#endif
    }
}


typedef struct {
    TokenType type;
    int value;
    char var;
} Token;

typedef struct {
    const char* src;
    int pos;
    Token current;
} Lexer;



typedef struct {
    char value[MAX_STR];
    int is_set;
    char group[MAX_STR];
} Coord;






ASTNode* new_node(ASTType t, ASTNode* l, ASTNode* r) {
    ASTNode* n = malloc(sizeof(ASTNode));
    n->type = t;
    n->left = l;
    n->right = r;
    n->value = 0;
    n->var = 0;
    return n;
}


int eval_ast(ASTNode* n, CPLnEngine* eng) {
    if (!n) return 0;

    switch(n->type) {
        case AST_NUM: return n->value;
        case AST_VAR:
            if (n->var=='M') return eng->main_mem;
            if (n->var=='S') return eng->sub_mem;
            if (n->var=='X') return eng->x;
            if (n->var=='Y') return eng->y;
            return 0;
        case AST_ADD: return eval_ast(n->left,eng)+eval_ast(n->right,eng);
        case AST_SUB: return eval_ast(n->left,eng)-eval_ast(n->right,eng);
        case AST_MUL: return eval_ast(n->left,eng)*eval_ast(n->right,eng);
        case AST_DIV: {
            int r = eval_ast(n->right,eng);
            return r?eval_ast(n->left,eng)/r:0;
        }
        // eval_ast 関数内の switch(node->type) の中に追加
case AST_SIN:  return sin(eval_ast(eng, node->left));
case AST_COS:  return cos(eval_ast(eng, node->left));
case AST_TAN:  return tan(eval_ast(eng, node->left));
case AST_POW:  return pow(eval_ast(eng, node->left), eval_ast(eng, node->right));
case AST_SQRT: return sqrt(eval_ast(eng, node->left));
case AST_ABS:  return fabs(eval_ast(eng, node->left));

        case AST_MOD: {
            int r = eval_ast(n->right,eng);
            return r?eval_ast(n->left,eng)%r:0;
        }
        case AST_NEG: return -eval_ast(n->left,eng);
        default: return 0;
    }
}


ASTNode* parse_expr(Lexer* lx) {
    ASTNode* n = parse_term(lx);

    while (lx->current.type==TOK_PLUS ||
           lx->current.type==TOK_MINUS) {

        TokenType op = lx->current.type;
        next_token(lx);

        n = new_node(op==TOK_PLUS?AST_ADD:AST_SUB,
                     n, parse_term(lx));
    }
    return n;
}
/* token は現在のトークン文字列 */
/* ip は現在の実行座標（既存で使っている変数） */

if (strcmp(token, "\\") == 0) {
    if (cpln_bind_mode == 0) {
        cpln_bind_mode = 1;   /* 束縛開始 */
    } else {
        /* 束縛確定 */
        cpln_add_binding(cpln_bind_word, ip);
        cpln_bind_mode = 0;
    }
    return;   /* 他の処理はしない */
}

ASTNode* parse_term(Lexer* lx) {
    ASTNode* n = parse_unary(lx);

    while (lx->current.type==TOK_MUL ||
           lx->current.type==TOK_DIV ||
           lx->current.type==TOK_MOD) {

        TokenType op = lx->current.type;
        next_token(lx);

        ASTType t =
            (op==TOK_MUL)?AST_MUL:
            (op==TOK_DIV)?AST_DIV:
                          AST_MOD;

        n = new_node(t, n, parse_unary(lx));
    }
    return n;
}

ASTNode* parse_unary(Lexer* lx) {
    if (lx->current.type == TOK_MINUS) {
        next_token(lx);
        return new_node(AST_NEG, parse_unary(lx), NULL);
    }
    return parse_primary(lx);
}


ASTNode* parse_primary(Lexer* lx) {
    Token t = lx->current;

    if (t.type == TOK_NUM) {
        ASTNode* n = new_node(AST_NUM, NULL, NULL);
        n->value = t.value;
        next_token(lx);
        return n;
    }

    if (t.type == TOK_VAR) {
        ASTNode* n = new_node(AST_VAR, NULL, NULL);
        n->var = t.var;
        next_token(lx);
        return n;
    }

    if (t.type == TOK_LPAREN) {
        next_token(lx);
        ASTNode* n = parse_expr(lx);
        if (lx->current.type == TOK_RPAREN)
            next_token(lx);
        return n;
    }

    return new_node(AST_INVALID, NULL, NULL);
    
    // parse_primary 関数内での名前判定に追加
if (strncmp(ptr, "sin", 3) == 0) {
    *pos += 3;
    node = create_node(AST_SIN, parse_expr(eng, code, pos), NULL);
} else if (strncmp(ptr, "cos", 3) == 0) {
    *pos += 3;
    node = create_node(AST_COS, parse_expr(eng, code, pos), NULL);
}

}

// プロトタイプ宣言
void init_engine(CPLnEngine* eng);
void run(CPLnEngine* eng, const char* code);
void render_gm(CPLnEngine* eng);
// 簡易的な数字フォント描画（Engineに描画関数として追加）
void draw_number(Engine* eng, double value) {
    char buf[32];
    sprintf(buf, "%.2f", value); // 数値を文字列に変換
    
    // ここで一文字ずつ、eng->gm のルールに従って
    // SDL_RenderFillRect などで描画する
    // ※理論座標 (x, y) は使わず、画面上の絶対位置を指定して描画
}
void run_python(CPLnEngine* eng, const char* code);
void init_graphics(CPLnEngine* eng) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("[SDL Error] %s\n", SDL_GetError());
        return;
    }

    eng->gm.cell_size = 20; // 1マス20px
    eng->gm.screen_w = MAX_COORD * eng->gm.cell_size; // 30x20 = 600px
    eng->gm.screen_h = MAX_COORD * eng->gm.cell_size; 
eng->clock.tick = 0;
eng->clock.interval = 60;   // デフォルト60tick
eng->clock.enabled = 0;
    eng->gm.window = SDL_CreateWindow("CPLn VM Visualizer",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        eng->gm.screen_w, eng->gm.screen_h,
        SDL_WINDOW_SHOWN);

    if (!eng->gm.window) {
        printf("[Window Error] %s\n", SDL_GetError());
        return;
    }

// init_graphics 内に追加
if (TTF_Init() == -1) {
    printf("TTF_Init Error: %s\n", TTF_GetError());
}
// フォントファイルの読み込み（例：Arial.ttf / サイズ24）
eng->font = TTF_OpenFont("arial.ttf", 24); 
if (!eng->font) {
    printf("Font Load Error: %s\n", TTF_GetError());
}

    eng->gm.renderer = SDL_CreateRenderer(eng->gm.window, -1, SDL_RENDERER_ACCELERATED);
    eng->gm.enabled = 1;
}

// 終了処理
void close_graphics(CPLnEngine* eng) {
    if (eng->gm.renderer) SDL_DestroyRenderer(eng->gm.renderer);
    if (eng->gm.window) SDL_DestroyWindow(eng->gm.window);
    SDL_Quit();
}

void clock_step(CPLnEngine* eng) {
    if (!eng->clock.enabled) return;
    if (eng->group_count == 0) return;

    eng->clock.tick++;

    if (eng->clock.tick % eng->clock.interval == 0) {
        eng->active_group++;
        if (eng->active_group >= eng->group_count)
            eng->active_group = 0;
    }
}

int find_group(CPLnEngine* eng, const char* name) {
    for (int i = 0; i < eng->group_count; i++) {
        if (strcmp(eng->groups[i].name, name) == 0)
            return i;
    }
    return -1;
}



int main() {
    CPLnEngine eng;
    init_engine(&eng);
    
    // グラフィック初期化
    init_graphics(&eng);

    Py_Initialize();
    if (!Py_IsInitialized()) { printf("[Python初期化失敗]\n"); return 1; }

    const char* code = "WW_D(_hello world_)300WW:>！";   
    SDL_Event e;
while (SDL_PollEvent(&e)) {
    if (e.type == SDL_QUIT) {
        exit(0);
    }
    if (e.type == SDL_KEYDOWN) {
        eng.last_key = e.key.keysym.sym;
    }
}
    run(&eng, code);

    // 実行完了後すぐに閉じないように少し待つ
    SDL_Delay(2000);

    Py_Finalize();
    
    // グラフィック終了
    close_graphics(&eng);
    
    return 0;
    SDL_DestroyTexture(texture);
SDL_DestroyRenderer(renderer);
SDL_DestroyWindow(window);
SDL_Quit();

while (1) {

    // 初期コード（1回だけの処理）
    run(&eng, code);

    // Zループが定義されていれば回す
    if (eng.z_loop_enabled) {
        run(&eng, eng.z_loop_code);
    }

    // 将来ここに入るもの
    // ・Clock (KROK)
    // ・キーボード入力
    // ・当たり判定
    // ・render_all_groups()
}

    if (argc > 1) {
        FILE *fp = fopen(argv[1], "r");
        if (fp) {
            char file_code[MAX_GROUP_CODE];
            // ファイルの中身を一気に読み込む
            size_t n = fread(file_code, 1, sizeof(file_code) - 1, fp);
            file_code[n] = '\0';
            fclose(fp);
            
            int pos = 0;
            printf("CPLn: Executing script [%s]...\n", argv[1]);
            run(&eng, file_code, &pos); // ファイルに書かれたコードを実行
            
            // スクリプト実行が終わったら終了するか、
            // そのまま対話モードを続けるか選べます。
            // 終了させる場合はここで return 0;
        } else {
            printf("CPLn: Error! Could not open file [%s]\n", argv[1]);
        }
    }
    // --- ここから追加：拡張子 .cpln の判別と実行 ---
    if (argc > 1) {
        char *filename = argv[1];
        char *ext = strrchr(filename, '.'); // ファイル名の最後のドットを探す

        // 拡張子が ".cpln" の場合のみ実行
        if (ext != NULL && strcmp(ext, ".cpln") == 0) {
            FILE *fp = fopen(filename, "r");
            if (fp) {
                char file_code[MAX_GROUP_CODE];
                // ファイルの内容を読み込む
                size_t n = fread(file_code, 1, sizeof(file_code) - 1, fp);
                file_code[n] = '\0';
                fclose(fp);
                
                int pos = 0;
                printf("CPLn System: Running script [%s]\n", filename);
                run(&eng, file_code, &pos);
                
                // スクリプト実行後に描画結果を確認したい場合は、ここで一時停止させる
                // printf("\nPress Enter to exit...");
                // getchar();
                return 0; // スクリプト実行が終わったらプログラムを終了する
            } else {
                printf("CPLn Error: Could not open file [%s]\n", filename);
                return 1;
            }
        }
    }
}

void init_clock(CPLnEngine* eng, int group_count) {
    eng->clock.tick = 0;
    eng->clock.max_groups = group_count;
    eng->clock.enabled = 1;
}

void render_all_groups(CPLnEngine* eng){
    for (int i = 0; i < eng->group_count; i++) {
        if (!eng->groups[i].visible) continue;

        CPLnEngine tmp = *eng;
        tmp.gm.enabled = 0;

        run(&tmp, eng->groups[i].code);

        if (tmp.gm.enabled) {
            render_gm(&tmp);
        }
    }
}

void present(CPLnEngine* eng){
    SDL_UpdateTexture(
        texture,
        NULL,
        eng->framebuffer,
        FB_MAX_W * sizeof(uint32_t)
    );

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void next_token(Lexer* lx) {
    while (isspace(lx->src[lx->pos])) lx->pos++;

    char c = lx->src[lx->pos];

    if (c == '\0') {
        lx->current.type = TOK_END;
        return;
    }

    if (isdigit(c)) {
        int v = 0;
        while (isdigit(lx->src[lx->pos])) {
            v = v * 10 + (lx->src[lx->pos++] - '0');
        }
        lx->current.type = TOK_NUM;
        lx->current.value = v;
        return;
    }

    if (c=='M'||c=='S'||c=='X'||c=='Y') {
        lx->current.type = TOK_VAR;
        lx->current.var = c;
        lx->pos++;
        return;
    }

    lx->pos++;
    switch(c) {
        case '+': lx->current.type = TOK_PLUS; break;
        case '-': lx->current.type = TOK_MINUS; break;
        case '*': lx->current.type = TOK_MUL; break;
        case '/': lx->current.type = TOK_DIV; break;
        case '%': lx->current.type = TOK_MOD; break;
        case '(': lx->current.type = TOK_LPAREN; break;
        case ')': lx->current.type = TOK_RPAREN; break;
        default:  lx->current.type = TOK_ERR; break;
    }
}
//Python実行
void run_python(CPLnEngine* eng, const char* code){
    PyObject *globals = PyDict_New();
    PyObject *locals  = PyDict_New();

    // ===== CPLn → Python =====
    PyDict_SetItemString(locals, "M", PyLong_FromLong(eng->main_mem));
    PyDict_SetItemString(locals, "S", PyLong_FromLong(eng->sub_mem));
    PyDict_SetItemString(locals, "X", PyLong_FromLong(eng->x));
    PyDict_SetItemString(locals, "Y", PyLong_FromLong(eng->y));

    // 実行
    PyRun_String(code, Py_file_input, globals, locals);

    // ===== Python → CPLn =====
    PyObject* v;
    if ((v = PyDict_GetItemString(locals, "M"))) eng->main_mem = PyLong_AsLong(v);
    if ((v = PyDict_GetItemString(locals, "S"))) eng->sub_mem  = PyLong_AsLong(v);
    if ((v = PyDict_GetItemString(locals, "X"))) eng->x        = PyLong_AsLong(v);
    if ((v = PyDict_GetItemString(locals, "Y"))) eng->y        = PyLong_AsLong(v);

    Py_DECREF(globals);
    Py_DECREF(locals);
}
// ここに init_engine, run の定義を書く
typedef struct {
    char name[MAX_STR];     // 定義名
    char parent[MAX_STR];   // 所属先
} BelongDef;

BelongDef belongs[MAX_DEF];
int belong_count = 0;

// 所属を追加
void add_belong(const char *child, const char *parent) {
    if (belong_count >= MAX_DEF) {
        printf("所属上限に達しました\n");
        return;
    }
    strcpy(belongs[belong_count].name, child);
    strcpy(belongs[belong_count].parent, parent);
    belong_count++;
    printf("所属定義: %s : %s\n", child, parent);
}

// 所属関係を確認
void show_belongs() {
    printf("=== 所属関係一覧 ===\n");
    for (int i = 0; i < belong_count; i++) {
        printf("%s : %s\n", belongs[i].name, belongs[i].parent);
    }
}

// エンジンのメモリを直接いじる、純粋なC言語の関数
void exec_native_c_logic(Engine* eng) {
    // 例：メインメモリを使って複雑なループ処理を爆速で行う
    for(int i = 0; i < 1000; i++) {
        eng->main_mem += (eng->sub_mem * 0.01);
    }
    // ここで直接SDLの描画関数を呼ぶことも可能
    printf("CPLn: C-Native Mode Executed.\n");
}


// ===== atoi改変関数 =====
int _(const char* s) {
    int res = 0, sign = 1, i = 0;
    if(s[0]=='-') { sign=-1; i++; }
    for(; s[i]; i++){
        if(s[i]<'0'||s[i]>'9') break;
        res = res*10 + (s[i]-'0');
    }
    return res*sign;
}
// ===== エンジン初期化 =====
void init_engine(CPLnEngine* eng){
    eng->main_mem=0; eng->sub_mem=0; eng->flip=0;
    eng->x=0; eng->y=0; eng->coord_mode=0; eng->active_group = -1;
for (int i = 0; i < MAX_GROUPS; i++) {
    eng->groups[i].visible = 1;
}  // デフォルト表示
eng->fb.width = FB_WIDTH;
eng->fb.height = FB_HEIGHT;
eng->fb.pixels = malloc(sizeof(uint32_t) * FB_WIDTH * FB_HEIGHT);
memset(eng->fb.pixels, 0x00, sizeof(uint32_t) * FB_WIDTH * FB_HEIGHT);
eng->fb.dirty = 1;
    for(int i=0;i<MAX_COORD;i++)
        for(int j=0;j<MAX_COORD;j++){
            eng->coords[i][j].is_set=0;
            eng->coords[i][j].group[0]='\0';
        }
        eng->gm.enabled = 0;
        eng->gm.screen_w = 0;
        eng->gm.screen_h = 0;
        eng->gm.sel_x1 = eng->gm.sel_y1 = 0;
        eng->gm.sel_x2 = eng->gm.sel_y2 = 0;
        eng->gm.color = 0x000000; 
        eng->group_count = 0;
        eng->grouping = 0;
        eng->group_buf_pos = 0;  
        eng->fb.width = FB_WIDTH;
eng->fb.height = FB_HEIGHT;
eng->fb.pixels = malloc(sizeof(uint32_t) * FB_WIDTH * FB_HEIGHT);
memset(eng->fb.pixels, 0x00, sizeof(uint32_t) * FB_WIDTH * FB_HEIGHT);
eng->fb.dirty = 1;

eng->z_loop_enabled = 0;
eng->z_loop_code[0] = '\0';
}

void free_ast(ASTNode* n) {
    if (!n) return;
    free_ast(n->left);
    free_ast(n->right);
    free(n);
}

void parse_gm_screen(CPLnEngine* eng, const char* code, int* i){
    int w = 0, h = 0;
    (*i)++; // '('

    while (isdigit(code[*i])) {
        w = w * 10 + (code[*i] - '0');
        (*i)++;
    }

    if (code[*i] == 'x') (*i)++;

    while (isdigit(code[*i])) {
        h = h * 10 + (code[*i] - '0');
        (*i)++;
    }

    if (code[*i] == ')') (*i)++;

    eng->gm.screen_w = w;
    eng->gm.screen_h = h;
}

void parse_gm_select(CPLnEngine* eng, const char* code, int* i){
    int x1=0,y1=0,x2=0,y2=0;
    (*i)++; // '['

    while (isdigit(code[*i])) { x1 = x1*10 + (code[*i]-'0'); (*i)++; }
    if (code[*i]==',') (*i)++;
    while (isdigit(code[*i])) { y1 = y1*10 + (code[*i]-'0'); (*i)++; }

    if (code[*i]==':') (*i)++;

    while (isdigit(code[*i])) { x2 = x2*10 + (code[*i]-'0'); (*i)++; }
    if (code[*i]==',') (*i)++;
    while (isdigit(code[*i])) { y2 = y2*10 + (code[*i]-'0'); (*i)++; }

    if (code[*i]==']') (*i)++;

    eng->gm.sel_x1 = x1;
    eng->gm.sel_y1 = y1;
    eng->gm.sel_x2 = x2;
    eng->gm.sel_y2 = y2;
}

void parse_gm_color(CPLnEngine* eng, const char* code, int* i){
    char hex[7] = {0};
    (*i) += 2; // "<#"

    for (int k = 0; k < 6; k++) {
        hex[k] = code[*i];
        (*i)++;
    }

    eng->gm.color = (unsigned int)strtol(hex, NULL, 16);

    if (code[*i] == '>') (*i)++;
}
int hit_rect(int x,int y,int x1,int y1,int x2,int y2){
    return x>=x1 && x<=x2 && y>=y1 && y<=y2;
}
void render_gm(CPLnEngine* eng){
    if (!eng->gm.enabled) return;

    for(int y = eng->gm.sel_y1; y < eng->gm.sel_y2; y++){
    for(int x = eng->gm.sel_x1; x < eng->gm.sel_x2; x++){
        put_pixel(eng, x, y, eng->gm.color);
    }
}
}

void render_all_groups(CPLnEngine* eng){
    for (int i = 0; i < eng->group_count; i++) {
        if (!eng->groups[i].visible) continue;

        CPLnEngine tmp = *eng;
        tmp.gm.enabled = 0;

        run(&tmp, eng->groups[i].code);

        if (tmp.gm.enabled) {
            render_gm(&tmp);
        }
    }
}

void render_realtime(CPLnEngine* eng) {
    if (!eng->gm.enabled || !eng->gm.renderer) return;

    SDL_Renderer* r = eng->gm.renderer;
    int cs = eng->gm.cell_size;

    // 1. 背景クリア (黒)
    SDL_SetRenderDrawColor(r, 20, 20, 20, 255);
    SDL_RenderClear(r);

    // 2. グリッドとセルの描画
    for (int x = 0; x < MAX_COORD; x++) {
        for (int y = 0; y < MAX_COORD; y++) {
            SDL_Rect rect = { x * cs, y * cs, cs - 1, cs - 1 };

            // 値がセットされているセルは明るく表示
            if (eng->coords[x][y].is_set) {
                // グループ設定がある場合は緑、なければ白
                if (strlen(eng->coords[x][y].group) > 0)
                    SDL_SetRenderDrawColor(r, 100, 255, 100, 255); // Green
                else
                    SDL_SetRenderDrawColor(r, 200, 200, 200, 255); // White
                SDL_RenderFillRect(r, &rect);
            } else {
                // 空のセルは暗いグレー枠
                SDL_SetRenderDrawColor(r, 50, 50, 50, 255);
                SDL_RenderDrawRect(r, &rect);
            }
        }
    }

    // 3. 現在のカーソル位置 (eng->x, eng->y) を赤枠で強調
    SDL_Rect cur = { eng->x * cs, eng->y * cs, cs, cs };
    SDL_SetRenderDrawColor(r, 255, 50, 50, 255); // Red
    SDL_RenderDrawRect(r, &cur);
    // カーソルの内側を少し透明な赤で塗る（視認性向上）
    SDL_SetRenderDrawColor(r, 255, 50, 50, 100); 
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(r, &cur);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);

    // 4. GM選択範囲の描画 (あれば)
    if (eng->gm.sel_x2 > 0) {
        int x = eng->gm.sel_x1 * cs;
        int y = eng->gm.sel_y1 * cs;
        int w = (eng->gm.sel_x2 - eng->gm.sel_x1 + 1) * cs;
        int h = (eng->gm.sel_y2 - eng->gm.sel_y1 + 1) * cs;
        SDL_Rect sel = { x, y, w, h };
        
        // 指定色 (eng->gm.color) を分解して適用
        int R = (eng->gm.color >> 16) & 0xFF;
        int G = (eng->gm.color >> 8) & 0xFF;
        int B = eng->gm.color & 0xFF;
        
        SDL_SetRenderDrawColor(r, R, G, B, 128);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(r, &sel);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    // 5. ステータス情報をウィンドウタイトルに表示
    char title[256];
    snprintf(title, sizeof(title), "CPLn VM | Pos:(%d,%d) M:%d S:%d Flip:%d", 
             eng->x, eng->y, eng->main_mem, eng->sub_mem, eng->flip);
    SDL_SetWindowTitle(eng->gm.window, title);

    // 描画反映
    SDL_RenderPresent(r);
    
    // イベント処理（ウィンドウが固まらないように）
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            close_graphics(eng);
            exit(0);
        }
    }
    
    // アニメーション用ウェイト（速すぎると見えないため）
    SDL_Delay(20); 
}
    


char* get_coord_val(CPLnEngine* eng){
    if(eng->coords[eng->x][eng->y].is_set) return eng->coords[eng->x][eng->y].value;
    return NULL;
}

void clock_step(CPLnEngine* eng) {
    if (!eng->clock.enabled) return;
    if (eng->group_count == 0) return;

    int prev = (eng->clock.tick - 1 + eng->group_count) % eng->group_count;
    int curr = eng->clock.tick % eng->group_count;

    // 前フレームを隠す
    eng->groups[prev].visible = 0;

    // 現フレームを表示
    eng->groups[curr].visible = 1;

    // 実行対象切替
    eng->active_group = curr;

    // 実行
    run(eng, eng->groups[curr].code);

    // 次tick
    eng->clock.tick++;
}

void set_coord_val(CPLnEngine* eng, const char* val){
    if(!val){ eng->coords[eng->x][eng->y].is_set=0; return; }
    strncpy(eng->coords[eng->x][eng->y].value,val,MAX_STR-1);
    eng->coords[eng->x][eng->y].value[MAX_STR-1]='\0';
    eng->coords[eng->x][eng->y].is_set=1;
}

// ===== 条件判定 =====
// 条件式を評価する関数
int eval_condition(CPLnEngine* eng, const char* cond) {
    if (!cond || strlen(cond) < 3) return 0;
    
    int lhs = 0;
    int rhs = 0;
    char op = '\0';
    char temp_cond[MAX_STR];
    strncpy(temp_cond, cond, MAX_STR);

    // 左辺の判定 (M=メイン, S=サブ, E=現在の座標にコードがあるか)
    if (temp_cond[0] == 'M') lhs = eng->main_mem;
    else if (temp_cond[0] == 'S') lhs = eng->sub_mem;
    else if (temp_cond[0] == 'E') {
        char* val = get_coord_val(eng);
        return (val && strlen(val) > 0); // E単体なら「中身があるか」で判定
    }

    op = temp_cond[1];
    rhs = atoi(temp_cond + 2); // 右辺を数値変換

    switch (op) {
        case '>': return lhs > rhs;
        case '<': return lhs < rhs;
        case '=': return lhs == rhs;
        case '!': return lhs != rhs;
        default: return 0;
    }


    // eval_condition 関数 (170行目付近) を拡張
int eval_condition_ext(CPLnEngine* eng, const char* cond) {
    if (!cond || strlen(cond) < 1) return 0;

    // 例: "M=S" (メインとサブが等しいか)
    if (cond[0] == 'M' && cond[1] == '=' && cond[2] == 'S') {
        return eng->main_mem == eng->sub_mem;
    }
    
    // 既存の M>10 などの判定を呼び出す
    [span_3](start_span)return eval_condition(eng, cond);[span_3](end_span)
}

}

   int eval_rule(CPLnEngine* eng, const char* cond){
    if(cond[0]=='T'){   // Clock
        return eng->clock == _(cond+2);
    }
    return eval_condition(eng, cond);
}


while (system_running) {
    clock_step(&eng);
    render_gm(&eng);
    apply_time_mode(&eng);
}
} CPLnEngine;
#endif
    
