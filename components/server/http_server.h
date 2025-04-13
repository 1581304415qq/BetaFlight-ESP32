#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "esp_err.h"
#include "http_util.h"


esp_err_t http_server_init(const char* base_path);
void http_server_deinit();
esp_err_t http_server_start(void);
void http_server_stop(void);
