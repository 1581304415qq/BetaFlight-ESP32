#include "http_util.h"

static bool check_path(const char* path) {
    printf(GREEN"%s:%s\n"CLOSE_COL, __FUNCTION__, path);
    // return (access(path, 0) == 0);
    return true;
}

char* get_resouce(
    read_file_func_t read_file_func,
    const char* path,
    uint32_t* data_len
) {
    // 检查路径
    if (read_file_func && check_path(path) && read_file_func != NULL) {
        // 读取资源
        return read_file_func(path, 0, data_len, 3);
    }
    printf(RED"%s\n"CLOSE_COL, "Resouce read fail.Please check the path or read method.");
    *data_len = 0;
    return NULL;
}


/* Copies the full path into destination buffer and returns
 * pointer to path (skipping the preceding base path) */
const char* get_path_from_uri(
    char* dest,
    const char* base_path,
    const char* uri,
    size_t destsize
) {
    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char* quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char* hash = strchr(uri, '#');
    if (hash) {
        pathlen = MIN(pathlen, hash - uri);
    }

    if (base_pathlen + pathlen + 1 > destsize) {
        /* Full path string won't fit into destination buffer */
        return NULL;
    }

    /* Construct full path (base + path) */
    strcpy(dest, base_path);
    strlcpy(dest + base_pathlen, uri, pathlen + 1);

    /* Return pointer to path, skipping the base */
    return dest + base_pathlen;
}
