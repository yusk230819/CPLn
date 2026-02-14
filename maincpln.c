// main.c
#include <stdio.h>
#include "globalbuild.h"      // build.h を読み込む（ここで構造体が定義される）
#include "globalrun.h"  // globalrun.h を読み込む（二重定義防止が効く）

void run_file(CPLnEngine* eng, const char* filename) {
    // 拡張子のチェック
    const char *ext = strrchr(filename, '.');
    if (ext == NULL || strcmp(ext, ".cpln") != 0) {
        printf("Error: Invalid file extension. Please use '.cpln'\n");
        return;
    }

    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        run(eng, buffer);
    }
    fclose(fp);
}


int main(int argc, char* argv[]) {
    CPLnEngine engine;
    
    // --- 必須の初期化処理 ---
    engine.main_mem = 0;
    engine.sub_mem = 0;
    engine.x = 0;
    engine.y = 0;
    engine.net_connected = 0;
    engine.sock = -1;  // 最初は未接続状態
    // -----------------------

    #ifdef _WIN32
    // Windowsでネットワークを使うためのおまじない
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

    if (argc > 1) {
        printf("CPLn Loading: %s\n", argv[1]);
        run_file(&engine, argv[1]);
    } else {
        printf("CPLn Network Test Mode...\n");
        run(&engine, "M=100 L(127.0.0.1) U"); 
    }

    // 後片付け
    #ifdef _WIN32
    WSACleanup();
    #endif

    return 0;
}


int main(int argc, char* argv[]) {
    CPLnEngine engine;
    // ... 初期化処理 (以前のコード通り) ...

    if (argc > 1) {
        // 例: CPLn.exe test.cpln と入力された場合
        printf("Running file: %s\n", argv[1]);
        run_file(&engine, argv[1]);
    } else {
        // 引数がない場合は、対話モードやデフォルト動作
        printf("No input file. Running default command...\n");
        run(&engine, "M=100 L(127.0.0.1) U"); 
    }

    return 0;
}
