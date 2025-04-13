#pragma once


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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


typedef void (*data_handle_callback)(struct parser* p, char* str, size_t len);
typedef void (*data_complect_callback)(struct parser* p);

typedef struct {
    char* name;
    char* filename;
    char* content_type;
} Part;

struct parser
{
    char* boundary;
    char* boundary_s;
    char* boundary_e;
    char* boundary_at; // boundray匹配位置
    uint8_t boundary_flag;
    uint8_t state;
    char* data_ptr;
    size_t content_length;
    size_t recv_count;
    size_t buffer_l;
    char buffer[1024]; // 待处理数据缓存
    Part part;
    void (*content_disposition_handle)(char* str);
    void (*content_type_handle)(char* str);
    data_handle_callback data_handle;
    data_complect_callback complect_handle;
};

enum state {
    BOUNDARY = 0,
    HEADERS_1,
    HEADERS_2,
    DATA,
    END,
};

void parser_init(struct parser* parser,
    char* boundary, size_t chunk_size,
    void (*content_disposition_handle)(char* str),
    void (*content_type_handle)(char* str),
    data_handle_callback callback,
    data_complect_callback complect);

void parser_free(struct parser* parser);

void parse_part(struct parser* parser, char* part_start, size_t len);
