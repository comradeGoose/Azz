#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_tls.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "driver/gpio.h"
#include "freertos/event_groups.h"
#include "cJSON.h"
#include <time.h>
#include <sys/time.h>

#include "esp_player_wrapper.h"
#include "rest_server.h"
#include "telegram_bot.h"
#include "azz_parser.h"

static const char *TAG3 = "[ TELEGRAM BOT ]";

//  *** -------------------- *** telegram certificate *** -------------------- ***
extern const char telegram_certificate_pem_start[] asm("_binary_telegram_certificate_pem_start");
extern const char telegram_certificate_pem_end[] asm("_binary_telegram_certificate_pem_end");
//  *** -------------------- *** end telegram certificate *** -------------------- ***


static char output_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0}; // Buffer to store response of http request
static int audio_play_send_message = SEND_AUDIO_STATUS_STOP;

int state_answer = ANSWER_OK;
int chat_id_reply;

void set_state_answer(int chat_id)
{
    chat_id_reply = chat_id;
    state_answer = ANSWER_FAIL;
}

int get_send_state(void)
{
    return audio_play_send_message;
}

void set_send_state(int state)
{
    audio_play_send_message = state;
}

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer_http_event; // Buffer to store response of http request from event handler
    static int output_len;                 // Stores number of bytes read
    switch (evt->event_id)
    {
    case HTTP_EVENT_ERROR:
        ESP_LOGD(TAG3, "HTTP_EVENT_ERROR");
        break;
    case HTTP_EVENT_ON_CONNECTED:
        ESP_LOGD(TAG3, "HTTP_EVENT_ON_CONNECTED");
        break;
    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGD(TAG3, "HTTP_EVENT_HEADER_SENT");
        break;
    case HTTP_EVENT_ON_HEADER:
        ESP_LOGD(TAG3, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;
    case HTTP_EVENT_ON_DATA:
        ESP_LOGD(TAG3, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
        /*
         *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
         *  However, event handler can also be used in case chunked encoding is used.
         */
        if (!esp_http_client_is_chunked_response(evt->client))
        {
            // If user_data buffer is configured, copy the response into the buffer
            if (evt->user_data)
            {
                memcpy(evt->user_data + output_len, evt->data, evt->data_len);
            }
            else
            {
                if (output_buffer_http_event == NULL)
                {
                    output_buffer_http_event = (char *)malloc(esp_http_client_get_content_length(evt->client));
                    output_len = 0;
                    if (output_buffer_http_event == NULL)
                    {
                        ESP_LOGE(TAG3, "Failed to allocate memory for output buffer");
                        return ESP_FAIL;
                    }
                }
                memcpy(output_buffer_http_event + output_len, evt->data, evt->data_len);
            }
            output_len += evt->data_len;
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        ESP_LOGD(TAG3, "HTTP_EVENT_ON_FINISH");
        if (output_buffer_http_event != NULL)
        {
            // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
            // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
            free(output_buffer_http_event);
            output_buffer_http_event = NULL;
        }
        output_len = 0;
        break;
    case HTTP_EVENT_DISCONNECTED:
        ESP_LOGI(TAG3, "HTTP_EVENT_DISCONNECTED");
        int mbedtls_err = 0;
        esp_err_t err = esp_tls_get_and_clear_last_error(evt->data, &mbedtls_err, NULL);
        if (err != 0)
        {
            if (output_buffer_http_event != NULL)
            {
                free(output_buffer_http_event);
                output_buffer_http_event = NULL;
            }
            output_len = 0;
            ESP_LOGI(TAG3, "Last esp error code: 0x%x", err);
            ESP_LOGI(TAG3, "Last mbedtls failure: 0x%x", mbedtls_err);
        }
        break;
    }
    return ESP_OK;
}

esp_err_t https_telegram_getUpdates(void)
{
    char *_token = get_token();
    ESP_LOGW(TAG3, "token : %s", _token);
    if ((_token == NULL) || (strncmp(_token, "", 1) == 0))
    {
        ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
        return ESP_FAIL;
    }

    char url[512] = {0};
    sprintf(url, "https://api.telegram.org/bot%s/getUpdates?offset=-1", _token);
    ESP_LOGW(TAG3, "url last message : %s", url);

    esp_http_client_config_t config = {
        .url = "https://api.telegram.org",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = telegram_certificate_pem_start,
        .user_data = output_buffer,
    };
    // POST
    ESP_LOGW(TAG3, "Iniciare");
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG3, "HTTP GET Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        ESP_LOGW(TAG3, "Desde Perform el output es: %s", output_buffer);
    }
    else
    {
        ESP_LOGE(TAG3, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    ESP_LOGW(TAG3, "Limpiare");
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
    
    // parser
    azz_parser(&output_buffer);

    memset(output_buffer, 0, sizeof(output_buffer));
    return ESP_OK;
}

esp_err_t https_telegram_sendMessage(int chat_id, char * message)
{
    char *_token = get_token();
    ESP_LOGW(TAG3, "token : %s", _token);
    if ((_token == NULL) || (strncmp(_token, "", 1) == 0))
    {
        ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
        return ESP_FAIL;
    }

    char url[512] = {0};
    sprintf(url, "https://api.telegram.org/bot%s/sendMessage", _token);
    ESP_LOGW(TAG3, "url last message : %s", url);

    esp_http_client_config_t config = {
        .url = "https://api.telegram.org",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = telegram_certificate_pem_start,
        .user_data = output_buffer,
    };

    ESP_LOGW(TAG3, "Iniciare");
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_url(client, url);

    // text message
    // if (state_answer == ANSWER_FAIL)
    // {
    //     esp_http_client_set_method(client, HTTP_METHOD_POST);
    //     esp_http_client_set_header(client, "Content-Type", "application/json");
    //     esp_http_client_set_post_field(client, ui_reply, sizeof(ui_reply));
    // }
    // else
    // {
    //     char post_data[512] = "";
    //     sprintf(post_data, "{\"chat_id\":%d,\"text\":\"%s\"}", chat_id, message);
    //     esp_http_client_set_method(client, HTTP_METHOD_POST);
    //     esp_http_client_set_header(client, "Content-Type", "application/json");
    //     esp_http_client_set_post_field(client, post_data, strlen(post_data));
    // }

    ESP_LOGW(TAG3, "Enviare POST");
    char post_data[512] = "";
    sprintf(post_data, "{\"chat_id\":%d,\"text\":\"%s\"}", chat_id, message);
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG3, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        ESP_LOGW(TAG3, "Desde Perform el output es: %s", output_buffer);
    }
    else
    {
        ESP_LOGE(TAG3, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    ESP_LOGW(TAG3, "Limpiare");
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
    memset(output_buffer, 0, sizeof(output_buffer));
    return ESP_OK;
}

esp_err_t https_telegram_getFile(char * file_id)
{
    char *_token = get_token();
    ESP_LOGW(TAG3, "token : %s", _token);
    if ((_token == NULL) || (strncmp(_token, "", 1) == 0))
    {
        ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
        return ESP_FAIL;
    }

    // https://api.telegram.org/bot<bot_token>/getFile?file_id=the_file_id
    char url[512] = {0};
    sprintf(url, "https://api.telegram.org/bot%s/getFile?file_id=%s", _token, file_id);
    ESP_LOGW(TAG3, "url last message : %s", url);

    esp_http_client_config_t config = {
        .url = "https://api.telegram.org",
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .event_handler = _http_event_handler,
        .cert_pem = telegram_certificate_pem_start,
        .user_data = output_buffer,
    };
    // POST
    ESP_LOGW(TAG3, "Iniciare");
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_url(client, url);
    esp_http_client_set_method(client, HTTP_METHOD_GET);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK)
    {
        ESP_LOGI(TAG3, "HTTP GET Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 esp_http_client_get_content_length(client));
        ESP_LOGW(TAG3, "Desde Perform el output es: %s", output_buffer);
    }
    else
    {
        ESP_LOGE(TAG3, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
    ESP_LOGW(TAG3, "Limpiare");
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    ESP_LOGI(TAG3, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
    
    // parser
    // azz_parser(&output_buffer);
    voice_message_parser(&output_buffer);

    memset(output_buffer, 0, sizeof(output_buffer));
    return ESP_OK;
}

esp_err_t voice_notification(char * file_path) 
{
    ESP_LOGE(TAG3, "filepath : %s", file_path);
    char notifi_url[512] = {0};
    sprintf(notifi_url, "https://api.telegram.org/file/bot%s/%s", get_token(), file_path);
    ESP_LOGE(TAG3, "url last message : %s", notifi_url);
    // esp_player_http_music_play(notifi_url, 0);
    return ESP_OK;
}

void http_test_task(void *pvParameters)
{
    switch (audio_play_send_message)
    {
    case SEND_AUDIO_STATUS_STOP:
        switch (state_answer)
        {
        case ANSWER_OK:
            https_telegram_getUpdates();
            break;
        case ANSWER_FAIL:
            https_telegram_getFile(get_last_message_file_id_voice_message());
            https_telegram_sendMessage(chat_id_reply, get_last_message_file_path_voice_message());
            voice_notification(get_last_message_file_path_voice_message());
            state_answer = ANSWER_OK;
            break;
        default:
            break;
        }
        break;
    case SEND_AUDIO_STATUS_PLAY:
        audio_play_send_message = SEND_AUDIO_STATUS_RUNNING;
        https_telegram_sendMessage(372705559, "AUDIO STATUS PLAY");
        break;
    case SEND_AUDIO_STATUS_RUNNING:
        audio_play_send_message = SEND_AUDIO_STATUS_STOP;
        https_telegram_sendMessage(372705559, "AUDIO STATUS FINISHED");
        break;
    default:
        break;
    }
    vTaskDelete(NULL);
}

esp_err_t azz_telegram_bot(void) 
{
    xTaskCreatePinnedToCore(&http_test_task, "http_test_task", 8192*2, NULL, 5, NULL, 1);
    return ESP_OK;
}
