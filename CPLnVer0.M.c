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
#include <python.h> //Python呼び出し用 
#include <SDL2/SDL.h> // グラフィックス用
#include <stdint.h>
#include <math.h> // 冒頭に追加
#include <SDL2/SDL_ttf.h> //文字感知用
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

// これを `#include` のすぐ下に貼る
typedef struct Coord Coord;
typedef struct GMState GMState;
typedef struct CPLnGroup CPLnGroup;
typedef struct CPLnEngine CPLnEngine;
typedef struct ASTNode ASTNode;
typedef struct Lexer Lexer;
typedef void (*CPLnCommand)(struct CPLnEngine* eng, const char* code, int* pos);

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
} cplnTokenType;

typedef enum {
    AST_NUM,        // 数値
    AST_VAR,        // 変数 (M, S, X, Y)
    AST_ADD,        // +
    AST_SUB,        // -
    AST_MUL,        // *
    AST_DIV,        // /
    AST_MOD,        // %
    AST_NEG,        // 単項 
    AST_INVALID,     // エラー用
    AST_MAIN_MEM,
    AST_SUB_MEM,
    AST_SIN,
    AST_COS, // ← 高度な数学用に追加
}ASTType;

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
    char value[MAX_STR];
    int is_set;
    char group[MAX_STR];
} Coord;

typedef struct {
    int enabled;

    int screen_w;
    int screen_h;

    int sel_x1, sel_y1;
    int sel_x2, sel_y2;

    unsigned int color; // 0xRRGGBB



renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    FB_MAX_W,
    FB_MAX_H
);
} GMState;

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

// グローバル状態に時間経過モードフラグ
typedef struct {
    int enabled;      // 現在時間経過モードか
    double delay_sec; // D(n) で設定された秒数
} TimeMode;

typedef struct {
    char name[32];
    char code[MAX_GROUP_CODE];
    int visible;   // 1=表示, 0=非表示
} CPLnGroup;

typedef struct ASTNode ASTNode;
typedef struct Lexer Lexer;

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
    CPLnCommand table[256]; // 文字解析テーブル
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



// ===== AST parser prototypes =====
ASTNode* parse_expr(Lexer* lx);
ASTNode* parse_term(Lexer* lx);
ASTNode* parse_unary(Lexer* lx);
ASTNode* parse_primary(Lexer* lx);
void next_token(Lexer* lx);




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

        cplnTokenType op = lx->current.type;
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
    } else{ 
        /* 束縛確定 */
        cpln_add_binding(cpln_bind_word, ip);
        cpln_bind_mode = 0;

    return;   /* 他の処理はしない */
}
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
    
void init_cmd_table(CPLnEngine* eng) {
    // これを書かないと、テーブルの中身は NULL のままです
    eng->table['='] = cmd_equal; 
    eng->table['!'] = cmd_exit;
    eng->table['X'] = cmd_X;
    eng->table['Y'] = cmd_Y;
    eng->table['#'] = 
    
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

    
// ===== グループ開始 =====
if (code[i] == '#' && eng->grouping == 0) {//グループ化をする
    eng->grouping = 1;
    eng->group_buf_pos = 0;
    eng->group_buffer[0] = '\0';
    continue;
}

        // ===== グループ収集中 =====
        if (eng->grouping == 1) {
            if (code[i] == ';') {
                eng->grouping = 2;
            } else {
                  if (eng->group_buf_pos < MAX_GROUP_CODE - 1) {
    eng->group_buffer[eng->group_buf_pos++] = code[i];
}
eng->group_buffer[eng->group_buf_pos] = '\0';
                }
            continue;
        }

        char cmd = code[i];
        // run() の for ループ冒頭あたり
if (eng->grouping == 2) {
    if (isspace(code[i])) continue;
    if (code[i] != 'B') continue;

}

case '=': {//実メモリ制御モード
    eng->real_mem_mode = !eng->real_mem_mode;
    if (eng->real_mem_mode) {
        printf("[実メモリモード: ON]\n");
    } else {
        printf("[実メモリモード: OFF]\n");
    }
    break;
}

            case '!': printf("\n[プログラム終了]\n"); return;//プログラムの最後のピリオドみたいな感じ
            case 'C': eng->coord_mode=1; break;//座標モードもう必要なさそうなので消す予定    
            move_coord(eng, cmd);
    break;
            case ':': {//所属
                char group[MAX_STR];
                int gi=0;
                i++;
                while(i<len && !strchr("=!CWD+-XY38254AB%@<>#<^", code[i]) && gi<MAX_STR-1){
                    group[gi++]=code[i++];
                }
                group[gi]='\0';
                strncpy(eng->coords[eng->x][eng->y].group, group, MAX_STR-1);
                eng->coords[eng->x][eng->y].group[MAX_STR-1]='\0';
                i--;
                break;
            }

            case '^': eng->coords[eng->x][eng->y].group[0]='\0'; break;//グループ化
            
            case 'G'://画面出力構文例　　GM(1980*880)[0.0:1980.880]<#FF0000>            
            if (i+1 < len && code[i+1] == 'M') {
            eng->gm.enabled = 1;
            i += 2;
           

        // GM(1980x880)
        if (code[i] == '(') {
            parse_gm_screen(eng, code, &i);
        }

        // GM[ x1,y1:x2,y2 ]
        if (code[i] == '[') {
            parse_gm_select(eng, code, &i);
        }

        // GM<#RRGGBB>
        if (code[i] == '<' && code[i+1] == '#') {
            parse_gm_color(eng, code, &i);
        }

        i--; // forループ補正
    }
    break;
    
    case 'O': {//グループ化
    if (eng->active_group < 0) {
        printf("[警告] グループが選択されていません\n");
        break;
    }

    if (i+1 < len && code[i+1]=='(') {
        int j = i + 2;
        char op = code[j];
        i = j + 1;

        CPLnGroup* g = &eng->groups[eng->active_group];

        switch (op) {
            case 'R':   // Runグループ間で実行
                GMState backup = eng->gm;
if (eng->active_group >= 0)
    run(eng, eng->groups[eng->active_group].code);
eng->gm = backup;
                break;

            case 'L':   // Length分からない
                printf("[GroupLength] %lu\n", strlen(g->code));
                break;

            case 'P':   // Printグループのコードを見る
                printf("[GroupCode] %s\n", g->code);
                break;
}
    
// --- U (接続) 命令の強化 ---
case 'L': {
    // ... (IP解析処理) ...
    if (connect(eng->sock, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        printf("Network: Connected\n");
        eng->net_connected = 1;
        eng->sub_mem = 0; // 成功
    } else {
        printf("Network: Error\n");
        eng->net_connected = 0;
        eng->sub_mem = -1; // 失敗をSに記録
    }
    break;
}

// --- L (送信) 命令の強化 ---
case 'U': {
    if (eng->net_connected) {
        char buf[64];
        sprintf(buf, "%f", eng->main_mem);
        if (send(eng->sock, buf, strlen(buf), 0) < 0) {
            eng->sub_mem = -1; // 送信エラー
            eng->net_connected = 0; // 切断されたとみなす
        }
    } else {
        eng->sub_mem = -1; // 未接続エラー
    }
    break;
}

// --- ¥ (受信) 命令の強化 ---
case 0xA5: case '¥': {
    if (eng->net_connected) {
        char buf[64];
        // 非ブロッキング受信
        int len = recv(eng->sock, buf, sizeof(buf)-1, 0); 
        if (len > 0) {
            buf[len] = '\0';
            eng->main_mem = atof(buf);
            eng->sub_mem = 0; // 受信成功
        } else if (len == 0) {
            // 相手が切断した
            eng->net_connected = 0;
            eng->sub_mem = -2; 
        } else {
            // データがまだ来ていないだけなら何もしない、
            // 本当の通信エラーなら -1
            // (errno等を確認して判定)
        }
    }
    break;
}




    
    case 'H': {
    if (i+1 < len && code[i+1]=='(') {
        int j = i + 2;
        char name[MAX_STR];
        int ni = 0;

        while (j < len && code[j] != ')' && ni < MAX_STR-1)
            name[ni++] = code[j++];

        name[ni] = '\0';
        i = j;

        int gi = find_group(eng, name);
        if (gi >= 0) {
            eng->active_group = gi;
        } else {
            printf("[警告] グループ未定義 '%s'\n", name);
        }
    }
    break;
}

case 'K':   // KROK
    if (i+3 < len && code[i+1]=='R' && code[i+2]=='O' && code[i+3]=='K') {
        eng->clock.enabled = 1;
        eng->clock.tick = 0;
        i += 3;
    }
    break;
    
    case 'A':
    if (strncmp(&code[i], "AOFF", 4) == 0) {
        eng->clock.enabled = 0;
        i += 3;
    }
    break;
    
    case 'D': {
    if(i+1<len && code[i+1]=='('){
        int j=i+2;
        char val[MAX_STR]; int vi=0;
        while(j<len && code[j]!=')' && vi<MAX_STR-1)
            val[vi++]=code[j++];
        val[vi]='\0';

        Lexer lx = { val, 0 };
        next_token(&lx);
        ASTNode* ast = parse_expr(&lx);
        eng->main_mem = eval_ast(ast, eng);
        free_ast(ast);

        i=j;
    }
    break;
}

            case 'S': set_coord_val(eng,get_coord_val(eng)); break;
            case '5': eng->flip^=1; break;
            case 'A': if(eng->flip) eng->flip=0; break;
           case 'B': {

    /* ===== ① グループ確定（最優先） ===== */
    if (eng->grouping == 2 && i+1 < len && code[i+1] == '(') {
        i += 2;
        char name[32];
        int ni = 0;

        while (i < len && code[i] != ')')
            name[ni++] = code[i++];
        name[ni] = '\0';

        if (eng->group_count >= MAX_GROUPS) {
            printf("[エラー] グループ数上限超過\n");
        } else {
            CPLnGroup* g = &eng->groups[eng->group_count++];
            strcpy(g->name, name);
            strcpy(g->code, eng->group_buffer);
            g->visible = 1;
        }

        eng->grouping = 0;
        eng->group_buf_pos = 0;
        break;
    }

    /* ===== ② 表示制御 ===== */
    if (i+1 < len && code[i+1] == 'H') {   // BH(name)
        i += 2;
        char name[MAX_STR]; int ni = 0;
        if (code[i] == '(') i++;
        while (i < len && code[i] != ')')
            name[ni++] = code[i++];
        name[ni] = '\0';

        int gi = find_group(eng, name);
        if (gi >= 0) eng->groups[gi].visible = 0;
        break;
    }

    if (i+1 < len && code[i+1] == 'S') {   // BS(name)
        i += 2;
        char name[MAX_STR]; int ni = 0;
        if (code[i] == '(') i++;
        while (i < len && code[i] != ')')
            name[ni++] = code[i++];
        name[ni] = '\0';

        int gi = find_group(eng, name);
        if (gi >= 0) eng->groups[gi].visible = 1;
        break;
    }

    if (i+1 < len && code[i+1] == 'T') {   // BT(name)
        i += 2;
        char name[MAX_STR]; int ni = 0;
        if (code[i] == '(') i++;
        while (i < len && code[i] != ')')
            name[ni++] = code[i++];
        name[ni] = '\0';

        int gi = find_group(eng, name);
        if (gi >= 0) eng->groups[gi].visible ^= 1;
        break;
    }

    break;
} 

case 'Q': {
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    eng->sub_mem = -1; // 何も押されていない場合は -1

    // A-Z: 0 〜 25
    for (int i = 0; i < 26; i++) {
        if (state[SDL_SCANCODE_A + i]) { eng->sub_mem = i; goto key_found; }
    }
    // 0-9: 28 〜 37
    if (state[SDL_SCANCODE_0]) { eng->sub_mem = 37; goto key_found; }
    for (int i = 0; i < 9; i++) {
        if (state[SDL_SCANCODE_1 + i]) { eng->sub_mem = 28 + i; goto key_found; }
    }
    // F1-F12: 38 〜 49
    for (int i = 0; i < 12; i++) {
        if (state[SDL_SCANCODE_F1 + i]) { eng->sub_mem = 38 + i; goto key_found; }
    }
    // 矢印4キー: 51, 52, 53, 54 (上, 右, 左, 下)
    if (state[SDL_SCANCODE_UP])    { eng->sub_mem = 51; goto key_found; }
    if (state[SDL_SCANCODE_RIGHT]) { eng->sub_mem = 52; goto key_found; }
    if (state[SDL_SCANCODE_LEFT])  { eng->sub_mem = 53; goto key_found; }
    if (state[SDL_SCANCODE_DOWN])  { eng->sub_mem = 54; goto key_found; }
    // 特殊: Shift(56), Ctrl(57), Alt(58), Space(59)
    if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT]) { eng->sub_mem = 56; goto key_found; }
    if (state[SDL_SCANCODE_LCTRL]  || state[SDL_SCANCODE_RCTRL])  { eng->sub_mem = 57; goto key_found; }
    if (state[SDL_SCANCODE_LALT]   || state[SDL_SCANCODE_RALT])   { eng->sub_mem = 58; goto key_found; }
    if (state[SDL_SCANCODE_SPACE]) { eng->sub_mem = 59; goto key_found; }

    // 記号とその他 (60 〜 77)
    const int symbols[] = {SDL_SCANCODE_COMMA, SDL_SCANCODE_PERIOD, SDL_SCANCODE_SLASH, SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_MINUS, SDL_SCANCODE_EQUALS};
    for (int i = 0; i < 6; i++) {
        if (state[symbols[i]]) { eng->sub_mem = 60 + i; goto key_found; }
    }
    for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
        if (state[i]) { eng->sub_mem = 77; break; }
    }

key_found:
    break;
}



// 読み込み命令 'R' (Read)
case 'R': {
    if (code[*pos + 1] == '(') {
        (*pos) += 2;
        char filename[MAX_STR];
        int j = 0;
        // ) が来るまでファイル名として読み込む
        while (code[*pos] != ')' && code[*pos] != '\0' && j < MAX_STR - 1) {
            filename[j++] = code[(*pos)++];
        }
        filename[j] = '\0';
        if (code[*pos] == ')') (*pos)++;

        FILE *fp = fopen(filename, "r");
        if (fp) {
            // 例: ファイルの最初の数値をメインメモリ(M)に読み込む
            fscanf(fp, "%lf", &eng->main_mem);
            fclose(fp);
            printf("Loaded value from %s\n", filename);
        } else {
            printf("Failed to open %s for reading\n", filename);
        }
    }
    break;
}

// 書き込み命令 'W' (Write)
case 'W': {
    if (code[*pos + 1] == '(') {
        (*pos) += 2;
        char filename[MAX_STR];
        int j = 0;
        while (code[*pos] != ')' && code[*pos] != '\0' && j < MAX_STR - 1) {
            filename[j++] = code[(*pos)++];
        }
        filename[j] = '\0';
        if (code[*pos] == ')') (*pos)++;

        FILE *fp = fopen(filename, "w");
        if (fp) {
            // 例: 現在のメインメモリ(M)の値を保存する
            fprintf(fp, "%f", eng->main_mem);
            fclose(fp);
            printf("Saved value to %s\n", filename);
        } else {
            printf("Failed to open %s for writing\n", filename);
        }
    }
    break;
}


            case 'F':
                if(eng->coord_mode){
                    eng->x=MAX_COORD-1-eng->x;
                    eng->y=MAX_COORD-1-eng->y;
                } else {
                    int tmp=eng->main_mem; eng->main_mem=eng->sub_mem; eng->sub_mem=tmp;
                }
                break;

            case '1': eng->main_mem = -eng->main_mem; break;
            case '2':　if (eng->main_mem < 0) eng->main_mem = -eng->main_mem;
    break;

            case '%': {
                char* val=get_coord_val(eng);
                if(val) printf("[座標(%d,%d)の値] %s\n",eng->x,eng->y,val);
                else printf("[座標(%d,%d)は未設定]\n",eng->x,eng->y);
                break;
            }

            case '@':
                if(i+3<len && code[i+1]=='D' && code[i+2]=='('){
                    int j=i+3; char val[16]; int vi=0;
                    while(j<len && code[j]!=')') val[vi++]=code[j++];
                    val[vi]='\0';
                    int rep=_(val);
                    char last_cmd=code[i-1];
                    if(rep>1000000){ printf("[警告] ループが大きすぎます (%d)\n",rep); break; }
                    for (int r = 0; r < rep; r++) {
    char tmp[2] = { last_cmd, '\0' };
    run(eng, tmp);
}
                    i=j;
                    break;
        }
    

                }
                break;

            case 'u': eng->main_mem=0; eng->sub_mem=0; set_coord_val(eng,NULL); break;

            case '>': {
                char* val=get_coord_val(eng);
                if(val) printf("%s",val);
                else if(eng->main_mem) printf("%c",eng->main_mem);
                break;
            }

            case 'S': {
                if(i+1<len && code[i+1]=='_'){
                    int j=i+2; char cond[MAX_STR]; int ci=0;
                    while(j<len && code[j]!='_') cond[ci++]=code[j++];
                    cond[ci]='\0';
                    i=j;
                    if(!eval_condition(eng, cond)) i++;
                }
                break;
            }


case '#':
    eng->grouping = 1;
    eng->group_buf_pos = 0;
    eng->group_buffer[0] = '\0';
    break;

// run() 内の switch文に追加
case '\\': {
    if (cpln_bind_mode == 0) {
        cpln_bind_mode = 1; // 単語入力開始
        cpln_bind_word[0] = '\0';
    } else {
        // 紐付け確定：現在の実行位置（i）を保存（簡易的なIPとして）
        cpln_add_binding(cpln_bind_word, i);
        cpln_bind_mode = 0;
    }
    break;
}

case 'M': {
    if (eng->font) {
        char text[64];
        sprintf(text, "M: %.2f  S: %.2f", eng->main_mem, eng->sub_mem);
        
        SDL_Color color = {255, 255, 255, 255}; // 白
        SDL_Surface* surface = TTF_RenderText_Blended(eng->font, text, color);
        SDL_Texture* texture = SDL_CreateTextureFromSurface(eng->gm.renderer, surface);
        
        int w, h;
        SDL_QueryTexture(texture, NULL, NULL, &w, &h);
        SDL_Rect rect = {10, 10, w, h}; // 画面左上(10,10)に表示
        
        SDL_RenderCopy(eng->gm.renderer, texture, NULL, &rect);
        
        SDL_FreeSurface(surface);
        SDL_DestroyTexture(texture);
    }
    break;
}


// run関数内の switch(cmd) の中に追加
case 'N': {
    // 既存の「座標モード」と競合する場合は、別の文字（例：'K'など）にしてください
    // ファイルを経由せず、直接あらかじめ定義したCの関数へジャンプする
    exec_native_c_logic(eng); 
    break;
}


case '-': {
    int addr = 0;
    i++;
    // 数値を読み取る
    while (i < len && isdigit(code[i])) {
        addr = addr * 10 + (code[i++] - '0');
    }
    // 次の '-' までスキップ
    if (code[i] == '-') {
        // 例: 座標のフラグを読み取る、あるいは特定のアドレス空間から取得
        // ここでは簡易的に「addr番目の座標のis_set状態」をメインメモリに入れる例
        int target_x = addr % MAX_COORD;
        int target_y = addr / MAX_COORD;
        if (target_y < MAX_COORD) {
            eng->main_mem = eng->coords[target_x][target_y].is_set;
        }
    }
    break;
}

case 'I': { // I(条件){実行コード}
    if (i + 1 < len && code[i + 1] == '(') {
        i += 2;
        char cond[MAX_STR]; int ci = 0;
        while (code[i] != ')' && i < len) cond[ci++] = code[i++];
        cond[ci] = '\0';
        
        // 条件が真なら {} 内を実行、偽ならスキップ
        if (code[++i] == '{') {
            i++;
            int start = i;
            int depth = 1;
            while (depth > 0 && i < len) {
                if (code[i] == '{') depth++;
                if (code[i] == '}') depth--;
                i++;
            }
            if (eval_condition_ext(eng, cond)) {
                char* sub = strndup(code + start, i - start - 1);
                [span_4](start_span)run(eng, sub);[span_4](end_span)
                free(sub);
            }
        }
    }
    break;
}

case 'v': { // 実メモリから読み込み (Value read)
    if (eng->real_mem_mode) {
        // メインメモリの値をアドレスと見なし、その先の1バイトを読み込む
        uintptr_t addr = (uintptr_t)eng->main_mem;
        unsigned char* ptr = (unsigned char*)addr;
        eng->main_mem = *ptr; // 値をメインメモリに上書き
    }
    break;
}

case 'w': { // 実メモリへ書き込み (Write)
    if (eng->real_mem_mode) {
        // メインメモリの値をアドレス、サブメモリの値をデータとして書き込む
        uintptr_t addr = (uintptr_t)eng->main_mem;
        unsigned char* ptr = (unsigned char*)addr;
        *ptr = (unsigned char)eng->sub_mem;
    }
    break;
}

case 's': { // 実メモリへ保存 (save)
    if (eng->real_mem_mode) {
        // メインメモリ(M)を住所、サブメモリ(S)をデータとして書き込む
        uintptr_t addr = (uintptr_t)eng->main_mem;
        unsigned char* ptr = (unsigned char*)addr;
        *ptr = (unsigned char)eng->sub_mem;
    }
    break;
}

case 'r': { // 実メモリから読込 (read/load)
    if (eng->real_mem_mode) {
        // メインメモリ(M)を住所として、その中身をサブメモリ(S)に入れる
        uintptr_t addr = (uintptr_t)eng->main_mem;
        unsigned char* ptr = (unsigned char*)addr;
        eng->sub_mem = (double)(*ptr); 
    }
    break;
}


case 'P': { // 座標コピー: P(M,S) や P(10,20)
    if (code[i+1] == '(') {
        i += 2;
        int tx = 0, ty = 0;

        // --- X座標の解析 ---
        if (code[i] == 'M') { tx = eng->main_mem; i++; }
        else if (code[i] == 'S') { tx = eng->sub_mem; i++; }
        else {
            while (isdigit(code[i])) tx = tx * 10 + (code[i++] - '0');
        }

        if (code[i] == ',') i++; // カンマを飛ばす

        // --- Y座標の解析 ---
        if (code[i] == 'M') { ty = eng->main_mem; i++; }
        else if (code[i] == 'S') { ty = eng->sub_mem; i++; }
        else {
            while (isdigit(code[i])) ty = ty * 10 + (code[i++] - '0');
        }

        if (code[i] == ')') {
            // 範囲チェックをしてコピー実行
            if (tx >= 0 && tx < MAX_COORD && ty >= 0 && ty < MAX_COORD) {
                char* source = eng->coords[tx][ty].data;
                // 現在地(eng->x, eng->y)へ上書き
                if (eng->coords[eng->x][eng->y].data) free(eng->coords[eng->x][eng->y].data);
                
                if (source) {
                    eng->coords[eng->x][eng->y].data = strdup(source);
                    eng->coords[eng->x][eng->y].is_set = 1;
                } else {
                    eng->coords[eng->x][eng->y].data = NULL;
                    eng->coords[eng->x][eng->y].is_set = 0;
                }
            }
        }
    }
    break;
}


case 'Z': {
    if (i + 1 < len && code[i + 1] == '(') {
        int j = i + 2;
        char buf[MAX_GROUP_CODE];
        int bi = 0;

        while (j < len && code[j] != ')' && bi < MAX_GROUP_CODE - 1) {
            buf[bi++] = code[j++];
        }
        buf[bi] = '\0';

        eng->z_loop_enabled = 1;
        strcpy(eng->z_loop_code, buf);

        i = j;
    }
    break;
}

case '&': {
    // 無限ループ開始
    int loop_start = i;  // ループ開始位置
    while (1) {
        int tmp_i = loop_start;
        for (; tmp_i < len; tmp_i++) {
            char loop_cmd = code[tmp_i];

            // 終了条件
            if (loop_cmd == '!') {
                printf("\n[プログラム終了]\n");
                return;
            }

            // 待機コマンド
            if (loop_cmd == '@' && tmp_i + 2 < len && code[tmp_i+1]=='D' && code[tmp_i+2]=='(') {
                int j = tmp_i + 3; char val[16]; int vi=0;
                while(j<len && code[j]!=')' && vi<15) val[vi++]=code[j++];
                val[vi]='\0';
                int wait_ms = _(val);
                tmp_i = j;

                // ミリ秒単位で待機
                #ifdef _WIN32
                    Sleep(wait_ms);
                #else
                    usleep(wait_ms*1000);
                #endif
                continue;
            }

            // 他のCPLⁿコマンドはrun()のswitchで処理
            char tmp_code[2] = { loop_cmd, '\0' };
            run(eng, tmp_code);
        }
    }
    break;
}

case '?': {//もし何何が何なら
    // 条件
    if (code[i+1] == '(') {
        i += 2;
        char cond[MAX_STR]; int ci = 0;
        while (code[i] != ')' && ci < MAX_STR-1)
            cond[ci++] = code[i++];
        cond[ci] = '\0';

        // 処理
        if (code[i+1] == '{') {
            i += 2;
            char act[MAX_GROUP_CODE]; int ai = 0;
            while (code[i] != '}' && ai < MAX_GROUP_CODE-1)
                act[ai++] = code[i++];
            act[ai] = '\0';

            // 登録
            strcpy(eng->rules[eng->rule_count].condition, cond);
            strcpy(eng->rules[eng->rule_count].action, act);
            eng->rule_count++;
        }
    }
    break;
}
case '{': { // ローカルスコープ開始
    VMState save = { eng->main_mem, eng->sub_mem, eng->x, eng->y };
    int depth = 1, j = i + 1, start = j;
    while (j < len && depth > 0) {
        if (code[j] == '{') depth++;
        if (code[j] == '}') depth--;
        j++;
    }
    char* sub = strndup(code + start, j - start - 1);
    run(eng, sub); // 再帰実行
    free(sub);
    // 状態を復元
    eng->main_mem = save.m; eng->sub_mem = save.s;
    eng->x = save.x; eng->y = save.y;
    i = j - 1;
    break;
}

case 'E': { // 座標実行 (Eval)
    char* target_code = get_coord_val(eng);
    if (target_code) {
        run(eng, target_code); 
    }
    break;
}

case 'V'://グループに何かする
    render_all_groups(eng);
break;



case 'g': eng->main_mem = eng->x; eng->sub_mem = eng->y; break;
case 'h': eng->main_mem = getchar(); break;
case 'j': snprintf(eng->buffer, MAX_STR, "%c", eng->main_mem); break;
case 'k': eng->main_mem = atoi(eng->buffer); break;


case 'T':   // T(30)　時間
    if (i+1 < len && code[i+1]=='(') {
        eng->clock.interval = _(code + i + 2);
        while (i < len && code[i] != ')') i++;
    }
    break;

            default: break;
 case '<'://Python呼び出し
    if (i+3 < len && code[i+1]=='P' && code[i+2]=='y' && code[i+3]=='|') {
        i += 4; // "<Py|" の次の文字から開始
        char py_code[MAX_STR];
        int pi = 0;

        // "<Py|" が再度出るまで読み込む
        while (i < len) {
            if (i+3 < len && code[i]=='<' && code[i+1]=='P' && code[i+2]=='y' && code[i+3]=='|')
                break; // 終了マーカー
            if (pi < MAX_STR-1) py_code[pi++] = code[i];
            i++;
        }
        py_code[pi] = '\0';

        run_python(py_code);  // Python 実行

        // 終了マーカー "<Py|" をスキップ
        if (i+3 < len) i += 3;

        break;
    }
    break;     
    
    
   case '[': { // 呼び出し（Call）
    if (eng->stack_ptr < MAX_STACK) {
        // 現在の命令位置(i)をスタックに積む
        eng->call_stack[eng->stack_ptr++] = i;
        
        // 現在の座標にあるコードを取得して実行
        char* target_code = get_coord_val(eng);
        if (target_code && strlen(target_code) > 0) {
            run(eng, target_code);
        }
    }
    break;
}

case ']': { // 戻る（Return）
    if (eng->stack_ptr > 0) {
        // スタックから戻り先を取り出す（今回は再帰実行なので、単に現在のrunを終了する）
        eng->stack_ptr--;
        return; 
    }
    break;
}
       
                // --- X座標の指定 ---
            case 'X': {//Xの後に数値を打ちその後にYを打つYがなければ構文エラー
                int val = 0;
                int sign = 1;
                int has_digit = 0;

                // 1. '-' 符号のチェック
                if (i + 1 < len && code[i + 1] == '-') {
                    sign = -1;
                    i++; 
                    if (!(i + 1 < len && isdigit(code[i + 1]))) {
                        val = 0; has_digit = 1; 
                    }
                }
                // 2. 数値のパース
                while (i + 1 < len && isdigit(code[i + 1])) {
                    val = val * 10 + (code[i + 1] - '0');
                    i++;
                    has_digit = 1;
                }
                // 3. 値の決定と反映 (指定なしは0)
                int final_val = (has_digit) ? (val * sign) : 0;
                if (final_val < 0) final_val = 0;
                if (final_val >= MAX_COORD) final_val = MAX_COORD - 1;
                eng->x = final_val;
                break;
            }

            // --- Y座標の指定 ---
            case 'Y': {//Xがなければ構文エラー　Yの後に数値を打つ
                int val = 0;
                int sign = 1;
                int has_digit = 0;

                // 1. '-' 符号のチェック
                if (i + 1 < len && code[i + 1] == '-') {
                    sign = -1;
                    i++; 
                    if (!(i + 1 < len && isdigit(code[i + 1]))) {
                        val = 0; has_digit = 1; 
                    }
                }
                // 2. 数値のパース
                while (i + 1 < len && isdigit(code[i + 1])) {
                    val = val * 10 + (code[i + 1] - '0');
                    i++;
                    has_digit = 1;
                }
                // 3. 値の決定と反映 (指定なしは0)
                int final_val = (has_digit) ? (val * sign) : 0;
                if (final_val < 0) final_val = 0;
                if (final_val >= MAX_COORD) final_val = MAX_COORD - 1;
                eng->y = final_val;
                break;
            }
            
            
        } // switch
        render_gm(eng);
        
        for(int r=0; r<eng->rule_count; r++){
    if(eval_rule(eng, eng->rules[r].condition)){
        if(hit_rect(
            eng->x, eng->y,
            eng->gm.sel_x1, eng->gm.sel_y1,
            eng->gm.sel_x2, eng->gm.sel_y2
        )){
            run(eng, eng->rules[r].action);
        }
    }
}

    } // for
    // run関数内の forループの最後の方
// ... switch文の後 ...

// 描画更新
render_realtime(eng);

// 時間経過処理
if (tmode.enabled && tmode.delay_sec > 0) {
    // ... 既存の待機コード ...

    // ループや各コマンド処理の後に時間経過待機
if (tmode.enabled && tmode.delay_sec > 0) {
#ifdef _WIN32
    Sleep((DWORD)(tmode.delay_sec * 1000));
#else
    usleep((useconds_t)(tmode.delay_sec * 1000000));
#endif

eng->clock.enabled = 0;
eng->clock.tick = 0;
eng->clock.max_groups = 0;
}

}
run(&eng, code);
render_all_groups(&eng);
if (eng->gm.enabled) {
    render_gm(eng);
    eng->gm.enabled = 0; // 次フレームに持ち越さない
    static inline void put_pixel(CPLnEngine* eng, int x, int y, uint32_t color){
    if (x < 0 || y < 0 ||
        x >= eng->fb.width ||
        y >= eng->fb.height)
        return;

    eng->fb.pixels[y * eng->fb.width + x] = color;
    eng->fb.dirty = 1;
}
}

present(&eng);
SDL_Delay(16); // 約60FPS
