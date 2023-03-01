#ifndef __TELEGRAM_BOT_H__
#define __TELEGRAM_BOT_H__

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include <time.h>
#include <sys/time.h>
#include "esp_http_client.h"

#define SEND_AUDIO_STATUS_STOP 0
#define SEND_AUDIO_STATUS_PLAY 1
#define SEND_AUDIO_STATUS_RUNNING 2
#define MAX_HTTP_OUTPUT_BUFFER  2048

#define ANSWER_OK 0
#define ANSWER_FAIL 1

void set_state_answer(int chat_id);

int get_send_state(void);

void set_send_state(int state);

esp_err_t _http_event_handler(esp_http_client_event_t *evt);

esp_err_t https_telegram_getUpdates(void);

esp_err_t https_telegram_sendMessage(int chat_id, char *message);

esp_err_t https_telegram_getFile(char * file_id);

esp_err_t voice_notification(char * file_path);

void http_test_task(void *pvParameters);

esp_err_t azz_telegram_bot(void);

#endif
