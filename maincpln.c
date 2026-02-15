#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
  #include <winsock2.h>
  #include <windows.h>
#endif

#include "globalbuild.h"
#include "globalrun.h"

// ファイル実行用の関数
void run_file(CPLnEngine* eng, const char* filename) {
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

// メイン関数
int main(int argc, char* argv[]) {
    CPLnEngine engine;
    
    // 一旦、構造体の初期化を最小限にします。
    // もし globalbuild.h に以下の変数がなければ、エラーの原因になります。
    // engine.main_mem = 0;
    // engine.sub_mem = 0;

    #ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif

    if (argc > 1) {
        printf("CPLn Loading: %s\n", argv[1]);
        run_file(&engine, argv[1]);
    } else {
        printf("CPLn Test Mode...\n");
        run(&engine, "M=100 L(127.0.0.1) U"); 
    }

    #ifdef _WIN32
    WSACleanup();
    #endif

    return 0;
}
