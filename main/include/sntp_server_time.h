#ifndef __SNTP_SERVER_TIME_H__
#define __SNTP_SERVER_TIME_H__

#include "freertos/FreeRTOS.h"
#include "audio_service.h"
#include "periph_service.h"
#include "esp_dispatcher.h"

#define FILE_PATH_CALL_FILS "/sdcard/calls.json"

#define GPIO_OUTPUT_IO_IRQ      CONFIG_GPIO_IRQ
#define GPIO_OUTPUT_IO_RESET    CONFIG_GPIO_RESET
#define I2C_MASTER_SCL_IO       CONFIG_I2C_MASTER_SCL      
#define I2C_MASTER_SDA_IO       CONFIG_I2C_MASTER_SDA
#define HTTP_STAREAM_URL        CONFIG_HTTP_STAREAM_URL
#define MAX_HTTP_OUTPUT_BUFFER  2048
#define RANDOM_DEQUE_SIZE       256

#define SRC_INIT                0
#define SRC_HTTP                1
#define SRC_SD                  2

#define TM_STATE_INIT 0
#define TM_STATE_WIFI_WAIT 1
#define TM_STATE_WIFI_OK 2

#define TM_STATE_BT_SINK 10
#define TM_STATE_HTTP_STREAM 11
#define TM_STATE_HTTP_STREAM_LOAD 12

typedef struct $ {
    audio_service_handle_t          audio_serv;         /*!< Clouds service handle */
    periph_service_handle_t         wifi_serv;          /*!< WiFi manager service handle */
    periph_service_handle_t         input_serv;         /*!< Input event service handle */
    esp_dispatcher_handle_t         dispatcher; 
    char                            info[37];
    int                             src_state;
    void                            *player;            /*!< The esp_audio handle */
    void                            *recorder;          /*!< The audio recorder handle */
    xTimerHandle                    retry_login_timer;
    bool                            wifi_setting_flag;
    bool                            is_palying;
    uint16_t                        state;
    bool                            wait;
    bool                            play;
    bool                            connected;
} esp_dispatcher_info_t;

static esp_err_t wifi_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx);

void time_sync_notification_cb(struct timeval *tv);

char * azz_check(int azz_w_day, int azz_hour, int azz_min);

static void initialize_sntp(void);

static void obtain_time(void);

static void init_app(void);

static void init_time(void);

static void update_time(void);

static void setClock(void);

static void getClock(void);

static void diffClock(void);

static void rtc_time(void);

static void strange_watch(void);

static char my_ip(void);

void server_time_setting_and_call_handing(void);

#endif
