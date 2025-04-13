#include "parse_part.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "queue.h"

#define TAG "Parse-part"


static Queue* queue = NULL;

void parser_init(struct parser* parser,
    char* boundary, size_t chunk_size,
    void (*content_disposition_handle)(char* str),
    void (*content_type_handle)(char* str),
    data_handle_callback callback,
    data_complect_callback complect)
{
    queue = createQueue(chunk_size * 2);
    memset(parser, 0, sizeof(struct parser));
    parser->boundary = strdup(boundary);
    parser->boundary_s = malloc(strlen(parser->boundary) + 1 + 2);
    sprintf(parser->boundary_s, "--%s", parser->boundary);
    parser->boundary_e = malloc(strlen(parser->boundary) + 1 + 6);
    sprintf(parser->boundary_e, "\r\n--%s--", parser->boundary);
    parser->content_disposition_handle = content_disposition_handle;
    parser->content_type_handle = content_type_handle;
    parser->data_handle = callback;
    parser->complect_handle = complect;
    parser->boundary_at = parser->boundary_e;
}

void parser_free(struct parser* parser)
{
    free(parser->part.name);
    free(parser->part.filename);
    free(parser->part.content_type);
    free(parser->boundary);
    free(parser->boundary_s);
    free(parser->boundary_e);
    freeQueue(queue);
    queue = NULL;
}

void byte_handle(struct parser* p, char c)
{
    // printf(LOG_FMT("%s %c"), __FUNCTION__, c);
    p->buffer[p->buffer_l++] = c;
    if (p->buffer_l > 1000) {
        p->data_handle(p, p->buffer, p->buffer_l);
        p->buffer_l = 0;
    }
}

/**
 * 发现与分隔符匹配,存储匹配部分数据
 *
*/
#define BUFFER_SIZE 256
char buffer[BUFFER_SIZE] = { 0 };
size_t buff_len = 0;
void save_buffer(char c) {
    // printf(LOG_FMT(RED"%s: %c"CLOSE_COL), __FUNCTION__, c);
    if (buff_len > BUFFER_SIZE)
        printf(LOG_FMT(RED"%s %s:%d"CLOSE_COL), "Cache size exceeded.", __FUNCTION__, __LINE__);
    buffer[buff_len++] = c;
}

void restore_buffer(struct parser* p)
{
    if (buff_len > 0) {
        for (int i = 0; i < buff_len; i++)
            byte_handle(p, buffer[i]);
        buff_len = 0;
    }
}

void complect_handle(struct parser* p)
{
    // clean buffer
    buff_len = 0;
    if (p->buffer_l > 0) {
        p->data_handle(p, p->buffer, p->buffer_l);
        p->buffer_l = 0;
    }
    p->complect_handle(p);
}

void parse_part(struct parser* parser, char* part_start, size_t len)
{
    printf(LOG_FMT(YELLOW"free size %d"CLOSE_COL), freeSize(queue));
    ENQUEUE(queue, part_start, len);
    char line[100] = { 0 }, c;
    int ret = 0;

#define PAS_SPACE(p)  skip_empty_lines(p)

    while (1) {
        if (!getSize(queue))return;
        switch (parser->state)
        {
        case BOUNDARY:
            ret = getLine(queue, line, sizeof(line));
            // printf(RED"%d %s\n"CLOSE_COL,ret, line);
            if (ret && strstr(line, parser->boundary) != NULL) {
                printf(LOG_FMT("found boundary="GREEN"%s"CLOSE_COL"\n"), line);
                parser->state = HEADERS_1;
            }
            else return;
            break;
        case HEADERS_1:
            ret = getLine(queue, line, sizeof(line));
            // printf(RED"%d %s\n"CLOSE_COL,ret, line);
            if (ret) {
                parser->state = HEADERS_2;
                parser->content_disposition_handle(line);
            }
            else return;
            break;
        case HEADERS_2:
            ret = getLine(queue, line, sizeof(line));
            // printf(RED"%d %s\n"CLOSE_COL,ret, line);
            if (ret) {
                parser->state = DATA;
                parser->content_type_handle(line);
                parser->boundary_flag = 1;
                PAS_SPACE(queue);
                continue;
            }
            else return;
            break;
        case DATA:
            // 检查是否存在尾部部分匹配
            // 如果存在部分匹配,保留一部分缓存
            // 如果不存在末尾匹配,则处理bufffer内容
            // 边界检查
            // ---- 1 检查尾部. 部分匹配缓存 .全部匹配
            // ---- 2 检查头部. 
            ret = dequeue(queue, (uint8_t*)&c);
            // printf("[%d] "GREEN"(%d) "CLOSE_COL, c, *parser->boundary_at);
            // printf("[%c] ", c);
            if (ret != 0)return;
            if (*parser->boundary_at == c) {
                save_buffer(c);
                parser->boundary_at++;
                if (*parser->boundary_at == '\0') {
                    parser->state = END;
                }
            }
            else {
                restore_buffer(parser);
                if (parser->boundary_e[0] != c) {
                    byte_handle(parser, c);
                    parser->boundary_at = parser->boundary_e;
                }
                else {
                    save_buffer(c);
                    parser->boundary_at = parser->boundary_e + 1;
                }
            }
            break;
        case END:
            printf("parse end\n");
            complect_handle(parser);
            return;
        default:
            printf(LOG_FMT(RED"parse part error."CLOSE_COL));
            return;
        }
    }
}


