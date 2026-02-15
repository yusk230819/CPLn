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

include <stdio.h>

    static inline void put_pixel(CPLnEngine* eng, int x, int y, uint32_t color){
    if (x < 0 || y < 0 ||
        x >= eng->fb.width ||
        y >= eng->fb.height)
        return;

    eng->fb.pixels[y * eng->fb.width + x] = color;
    eng->fb.dirty = 1;
    }

// ===== 実行エンジン =====
void run(CPLnEngine* eng, const char* code){
int len = strlen(code);
// Clock駆動（1命令 = 1 tick）
clock_step(eng);
    for (int i = 0; i < len; i++) {
    
// ===== グループ開始 =====
if (code[i] == '#' && eng->grouping == 0) {//グループ化をする
    eng->grouping = 1;
    eng->group_buf_pos = 0;
    eng->group_buffer[0] = '\0';
    continue;
}

        // ===== グループ収集中 =====
        if (eng->grouping == 1) {
            if (code[i] == ']') {
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


        switch(cmd){
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
            case 'C': eng->coord_mode=1; break;    
            move_coord(eng, cmd);
    break;
            case ':': {//所属
                char group[MAX_STR];
                int gi=0;
                i++;
                while(i<len && !strchr("=!CWD+-XY&?(8u1{}254AB%@vwsr<>#<)^ABEFGHIJKLMNOPQRSTUVZ¥_/\\", code[i]) && gi<MAX_STR-1){
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
}
}
    present(&eng);
SDL_Delay(16); // 約60FPS

} // run
