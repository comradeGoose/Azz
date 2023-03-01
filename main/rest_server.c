//
#include <string.h>

//cJSON
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

//SD
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include <dirent.h>


#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_err.h"
#include "esp_log.h"

#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "esp_http_server.h"

#include "UrlDecoder.h"
#include "rest_server.h"
#include "Authorization.h"
#include "esp_player_wrapper.h"


static const char *REST_TAG = "[ REST SERVER ]";

static int vol = 10;

static char token[64] = {0};

char * get_token() {
    return token;
}

void set_token(char *value) {
    ESP_LOGW(REST_TAG, "set_token : %s", value);
    // token = (char*)malloc(sizeof(value));
    memset(token,0,64);
    //sprintf(token, "%s", value);
    strlcat(token, value, sizeof(token));
    ESP_LOGW(REST_TAG, "token : %s", token);
    ESP_LOGI(REST_TAG, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
}

int get_vol() {
    return vol;
}

void set_vol(int value) {
    vol = value;
}

static bool search_audio_files(char *file_name) {
        if (strstr(file_name, ".mp3") != NULL) return true;
        if (strstr(file_name, ".amr") != NULL) return true;
        if (strstr(file_name, ".Wamr") != NULL) return true;
        if (strstr(file_name, ".ogg") != NULL) return true;
        if (strstr(file_name, ".oga") != NULL) return true;
        if (strstr(file_name, ".flac") != NULL) return true;
        if (strstr(file_name, ".wav") != NULL) return true;
        if (strstr(file_name, ".aac") != NULL) return true;
        if (strstr(file_name, ".m4a") != NULL) return true;
        if (strstr(file_name, ".ts") != NULL) return true;
        if (strstr(file_name, ".mp4") != NULL) return true;
        return false;
}

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath) {
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req) {
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    
    return ESP_OK;
}

static const char* get_path_from_uri(char *dest, const char *base_path, const char *uri, size_t destsize) {

    const size_t base_pathlen = strlen(base_path);
    size_t pathlen = strlen(uri);

    const char *quest = strchr(uri, '?');
    if (quest) {
        pathlen = MIN(pathlen, quest - uri);
    }
    const char *hash = strchr(uri, '#');
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

static esp_err_t file_serving_post_handler(httpd_req_t *req) {
    
    esp_audio_state_t st = {0};
    esp_player_state_get(&st);
    if (st.status == AUDIO_STATUS_RUNNING) {
        ESP_LOGI(REST_TAG,"AUDIO_STATUS_RUNNING");
        httpd_resp_sendstr(req, "AUDIO STATUS RUNNING");
        return ESP_FAIL;
    }

    char filepath[FILE_PATH_MAX];
    FILE *fd = NULL;
    struct stat file_stat;

    ESP_LOGI(REST_TAG,"req->uri: %s", req->uri);
    ESP_LOGI(REST_TAG,"req->method: %d", req->method);
    ESP_LOGI(REST_TAG,"req->content_len: %d", req->content_len);
    ESP_LOGI(REST_TAG, "base_path: %s", ((struct rest_server_context *)req->user_ctx)->base_path);
    const char *decode_uri = url_decode(req->uri);
    ESP_LOGI(REST_TAG, "decode_uri: %s", decode_uri);
    const char *filename = get_path_from_uri(filepath, ((struct rest_server_context *)req->user_ctx)->base_path,
                                                decode_uri + sizeof("/upload_file/*?filename=") - 1, sizeof(filepath));
    ESP_LOGI(REST_TAG,"filename: %s", filename);
    ESP_LOGI(REST_TAG,"filepath: %s", filepath);

    if (!filename) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Filename too long");
        return ESP_FAIL;
    }
    ESP_LOGW(REST_TAG, "filename: %s", filename);
    /* Filename cannot have a trailing '/' */
    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGE(REST_TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    if (stat(filepath, &file_stat) == 0) {
        ESP_LOGE(REST_TAG, "File already exists : %s", filepath);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists");
        return ESP_FAIL;
    }

    /* File cannot be larger than a limit */
    if (req->content_len > MAX_FILE_SIZE) {
        ESP_LOGE(REST_TAG, "File too large : %d bytes", req->content_len);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                            "File size must be less than "
                            MAX_FILE_SIZE_STR "!");
        /* Return failure to close underlying connection else the
         * incoming file content will keep the socket busy */
        return ESP_FAIL;
    }

    fd = fopen(filepath, "w");
    if (!fd) {
        ESP_LOGE(REST_TAG, "Failed to create file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    ESP_LOGI(REST_TAG, "Receiving file : %s...", filename);

    /* Retrieve the pointer to scratch buffer for temporary storage */
    char *buf = ((struct rest_server_context *)req->user_ctx)->scratch;
    int received;

    /* Content length of the request gives
     * the size of the file being uploaded */
    int remaining = req->content_len;

    while (remaining > 0) {

        ESP_LOGI(REST_TAG, "Remaining size : %d", remaining);
        /* Receive the file part by part into a buffer */
        if ((received = httpd_req_recv(req, buf, MIN(remaining, SCRATCH_BUFSIZE))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            /* In case of unrecoverable error,
             * close and delete the unfinished file*/
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(REST_TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if (received && (received != fwrite(buf, 1, received, fd))) {
            /* Couldn't write everything to file!
             * Storage may be full? */
            fclose(fd);
            unlink(filepath);

            ESP_LOGE(REST_TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of
         * the file left to be uploaded */
        remaining -= received;
    }

    /* Close file upon upload completion */
    fclose(fd);
    ESP_LOGI(REST_TAG, "File reception complete");

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
#ifdef CONFIG_EXAMPLE_HTTPD_CONN_CLOSE_HEADER
    httpd_resp_set_hdr(req, "Connection", "close");
#endif
    httpd_resp_sendstr(req, "File uploaded successfully");
    return ESP_OK;
}

static esp_err_t set_calls_post_handler(httpd_req_t *req) {
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    cJSON *call_list = cJSON_GetObjectItem(root, "call_list");
    char *calls = cJSON_Print(call_list);
    ESP_LOGI(REST_TAG, "call_list: %s", calls);
    
    FILE *fp;
    fp = fopen("/sdcard/calls.json", "w");
    fputs(calls, fp);
    fclose(fp);

    free((void *)calls);
    cJSON_Delete(root);

    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

static esp_err_t auth_uri_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';


    if (auth(buf) == ESP_OK)
    {
        httpd_resp_sendstr(req, "Good Goose");
        return ESP_OK;
    }
    // httpd_resp_set_hdr(req, "Location", "/");
    // httpd_resp_set_hdr(req, "Connection", "close");
    // httpd_resp_sendstr(req, "Bad Goose");
    return ESP_OK;
}

static esp_err_t set_token_bot_uri_post_handler(httpd_req_t *req) {
    
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    const char *token_bot = cJSON_GetObjectItem(root, "token")->valuestring;
    ESP_LOGI(REST_TAG, "token_bot: %s", token_bot);
    set_token(token_bot);
    // free((void *)token_bot);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    // char *get_token_value = get_token();
    // ESP_LOGW(REST_TAG, "get_token_value: %s", get_token_value);
    return ESP_OK;
}

static esp_err_t set_music_volume_post_uri_post_handler(httpd_req_t *req) {
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    char *string_volume = cJSON_GetObjectItem(root, "volume")->valuestring;
    ESP_LOGI(REST_TAG, "string_volume: %s", string_volume);
    int music_volume = atoi(string_volume);
    set_vol(music_volume);
    ESP_LOGI(REST_TAG, "string -> int : volume: %d", music_volume);
    // free((void *)string_volume);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    ESP_ERROR_CHECK(esp_player_vol_set(vol));
    return ESP_OK;
}

static esp_err_t rest_music_volume_get_handler(httpd_req_t *req) {

    const int music_volume = get_vol(); // исходное число
    ESP_LOGW(REST_TAG, "esp_player_vol_get : %d", music_volume);
    char volume_string[4];  // буфер, в которую запишем число
    sprintf(volume_string, "%d", music_volume);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "volume", volume_string);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t rest_audio_list_get_handler(httpd_req_t *req) {

    struct dirent *dp;
    DIR *dir = opendir(MOUNT_POINT);
    if (dir == NULL) {
        ESP_LOGE(REST_TAG, "Error open directory");
    }
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateArray();
    while ((dp = readdir (dir)) != NULL) {
        if (search_audio_files(dp->d_name)) {
            ESP_LOGI(REST_TAG, "audio file: %s", dp->d_name);
            cJSON_AddStringToObject(root, "music_list", dp->d_name);
        }
    }
    closedir (dir);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

static esp_err_t rest_calls_list_get_handler(httpd_req_t *req) {

    const char *filepath = "/sdcard/calls.json";
    
    int fsize(char* file) {
        int size;
        FILE* fh;
        fh = fopen(file, "rb"); //binary mode
        if(fh != NULL) {
            if( fseek(fh, 0, SEEK_END) ) {
                fclose(fh);
                return -1;
                }
            size = ftell(fh);
            fseek(fh, 0, SEEK_SET);
            fclose(fh);
            return size;
        }
        return -1; //error
    }
    
    FILE *fp;
    if ((fp = fopen(filepath, "r")) == NULL) {
        fclose(fp);
        fp = fopen(filepath, "w");
        if (!fp) {
            ESP_LOGE(REST_TAG, "Failed to create file calls.json");
            }
        fputs("[]", fp);
        fclose(fp);
    }
    fclose(fp);
    int sd_file_size = fsize(filepath);

    fp = fopen(filepath, "r");

    const char data[sd_file_size];
    fread(data, sd_file_size, 1, fp);
    fclose(fp);
    cJSON *root = cJSON_Parse(data);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t start_rest_server(const char *base_path) {
    
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    const char *base_path_sdcard = "/sdcard/";

    REST_CHECK(base_path_sdcard, "wrong base path", err);
    rest_server_context_t *rest_context_sd = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context_sd->base_path, base_path_sdcard, sizeof(rest_context_sd->base_path));


    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    /* get_music volume*/
    httpd_uri_t music_volume_get_uri = {
        .uri = "/get_music_volume",
        .method = HTTP_GET,
        .handler = rest_music_volume_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &music_volume_get_uri);

    /* get list of music files on SD*/
    httpd_uri_t audio_list_get_uri = {
        .uri = "/music_list",
        .method = HTTP_GET,
        .handler = rest_audio_list_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &audio_list_get_uri);
    
    /* get calls.json on SD*/
    httpd_uri_t calls_list_get_uri = {
        .uri = "/ListLessonCall",
        .method = HTTP_GET,
        .handler = rest_calls_list_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &calls_list_get_uri);

    httpd_uri_t auth_post_uri = {
        .uri = "/auth",
        .method = HTTP_POST,
        .handler = auth_uri_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &auth_post_uri); 

    httpd_uri_t set_token_bot_post_uri = {
        .uri = "/set_token_bot",
        .method = HTTP_POST,
        .handler = set_token_bot_uri_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &set_token_bot_post_uri);     

    httpd_uri_t set_music_volume_post_uri = {
        .uri = "/set_music_volume",
        .method = HTTP_POST,
        .handler = set_music_volume_post_uri_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &set_music_volume_post_uri);  

    /* set calls on SD*/
    httpd_uri_t set_calls_post_uri = {
        .uri = "/set_calls",
        .method = HTTP_POST,
        .handler = set_calls_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &set_calls_post_uri);

    /* file serving*/
    httpd_uri_t file_serving_post_uri = {
        .uri = "/upload_file/*",
        .method = HTTP_POST,
        .handler = file_serving_post_handler,
        .user_ctx = rest_context_sd
    };
    httpd_register_uri_handler(server, &file_serving_post_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
