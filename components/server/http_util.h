#pragma once
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#define MIN(a,b) (a)>(b)?(b):(a) 
#define BLACK   "\033[30m" //:黑
#define RED     "\033[31m" //:红
#define GREEN   "\033[32m" //:绿
#define YELLOW  "\033[33m" //:黄
#define BLUE    "\033[34m" //:蓝
#define DGREEN  "\033[35m" //:深绿
#define WHITE   "\033[36m" //:白
#define CLOSE_COL   "\033[0m" //:关闭颜色

// #define LOG_FMT(x) "%s " x,TAG
#define LOG_FMT(x) YELLOW"%s"CLOSE_COL": "x"\n" ,TAG

#define ALIGN_UP(x, alignment) (((x) + (alignment) - 1) & ~((alignment) - 1))

// 打印 MD5 值 (十六进制)
#define PRINT_HEX(arr, len) do{          \
    for (int i = 0; i < len; i++) {  \
        printf("%02x", (unsigned char)arr[i]);     \
    }                               \
    printf("\n");                   \
}while (0)

// #define HEX
inline void print_client_data(const char* str, const uint32_t len) {
#ifdef HEX
    for (size_t i = 0; i < len; i++)
        printf("%02x ", str[i]);
#else
    printf("\nClient: %lu\n", len);
    printf("%.*s\n", (int)len, str);
#endif
    printf("\n\n");
}

const char* get_path_from_uri(char* dest, const char* base_path, const char* uri, size_t destsize);

/**
 * @brief 文件读取方法
 * 文件名
 * 偏移
 * 读取长度
 * 尝试次数
 */
typedef char(*read_file_func_t)(const char*, uint32_t, uint32_t*, int);
char* get_resouce(read_file_func_t read_file_func, const char* path, uint32_t* data_len);
