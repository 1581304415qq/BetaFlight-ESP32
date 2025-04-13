idf_component_register(SRCS "tcp_server.c" "http_server.c" "http_util.c" 
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES esp_http_server esp_ringbuf cJSON multipart_parser ota wifi storage device)
