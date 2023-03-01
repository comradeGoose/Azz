#ifndef __REST_SERVER_H__
#define __REST_SERVER_H__

#include "esp_http_server.h"

#define MOUNT_POINT "/sdcard"
#define MAX_FILE_SIZE   (30720*1024) // 30 MB
#define MAX_FILE_SIZE_STR "30MB"
#define ESP_VFS_PATH_MAX 15

#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)

#define SCRATCH_BUFSIZE (10240)

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

char * get_token();

void set_token(char *value);

int get_vol();

void set_vol(int value);

static bool search_audio_files(char *file_name);

static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath);

static esp_err_t rest_common_get_handler(httpd_req_t *req);

static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize);

static esp_err_t file_serving_post_handler(httpd_req_t *req);

static esp_err_t set_calls_post_handler(httpd_req_t *req);

static esp_err_t set_token_bot_uri_post_handler(httpd_req_t *req);

static esp_err_t auth_uri_post_handler(httpd_req_t *req);

static esp_err_t set_music_volume_post_uri_post_handler(httpd_req_t *req);

static esp_err_t rest_music_volume_get_handler(httpd_req_t *req);

static esp_err_t rest_audio_list_get_handler(httpd_req_t *req);

static esp_err_t rest_calls_list_get_handler(httpd_req_t *req);

esp_err_t start_rest_server(const char *base_path);

#endif
