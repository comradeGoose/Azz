#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_sntp.h"
#include <stdio.h>
// cJSON
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_vfs.h"
#include "cJSON.h"
// SD
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include <dirent.h>
//
#include <stdarg.h>
#include <stdlib.h>
#include "sdkconfig.h"
#include "esp_peripherals.h"
#include "audio_setup.h"
#include "wifi_service.h"
#include "esp_wifi.h"
#include "airkiss_config.h"
#include "smart_config.h"
#include "esp_dispatcher.h"
#include "esp_action_exe_type.h"
#include "wifi_action.h"
#include "player_action.h"
#include "esp_delegate.h"
#include "audio_thread.h"
#include "audio_mem.h"
#include "esp_spiffs.h"
#include "esp_random.h"
#include "esp_tls.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sys.h"
//
#include "sntp_server_time.h"
#include "esp_player_wrapper.h"
#include "rest_server.h"
#include "board.h"
#include "telegram_bot.h"
#include "ds1302.h"

static const char *TAG_SNTP = "[ SNTP ]";

static char path_ad_sd[128] = {0};
static char azz_check_time[64] = {0};


//  *** -------------------- *** ipInfo *** -------------------- ***
#include <tcpip_adapter.h>
tcpip_adapter_ip_info_t ipInfo;
//  *** -------------------- *** end ipInfo *** -------------------- ***


//  *** -------------------- *** dispatcher *** -------------------- ***
esp_dispatcher_info_t *dispatcher_info = NULL;
//  *** -------------------- *** end dispatcher *** -------------------- ***

//  *** -------------------- *** time *** -------------------- ***
static int server_sec = 0;
static int server_min = 0;
static int server_hour = 0;
static int server_wday = 0;

#if CONFIG_SET_CLOCK
    #define NTP_SERVER CONFIG_NTP_SERVER
#endif
#if CONFIG_GET_CLOCK
    #define NTP_SERVER " "
#endif
#if CONFIG_DIFF_CLOCK
    #define NTP_SERVER CONFIG_NTP_SERVER
#endif

RTC_DATA_ATTR static int boot_count = 0;
static const char *TAG_DS1302 = "[ DS1302 ]";
//  *** -------------------- *** end time *** -------------------- ***

//  *** -------------------- *** functions *** -------------------- ***

static void esp_audio_callback_func(esp_audio_state_t *audio, void *ctx)
{
    ESP_LOGW(TAG_SNTP, "ESP_AUDIO_CALLBACK_FUNC, st:%d,err:%d,src:%x", audio->status, audio->err_msg, audio->media_src);
    if (audio->status == AUDIO_STATUS_FINISHED)
    {
        if (dispatcher_info->state == TM_STATE_WIFI_OK)
        {
            dispatcher_info->state = TM_STATE_BT_SINK;
            esp_player_flash_music_play("spiffs://spiffs/ok_wifi.mp3", 0);
        }
        else if (dispatcher_info->state == TM_STATE_WIFI_WAIT)
        {
            dispatcher_info->state = TM_STATE_BT_SINK;
            esp_player_flash_music_play("spiffs://spiffs/no_wifi.mp3", 0);
        }
    }
}

static esp_err_t wifi_service_cb(periph_service_handle_t handle, periph_service_event_t *evt, void *ctx)
{
    ESP_LOGD(TAG_SNTP, "event type:%d,source:%p, data:%p,len:%d,ctx:%p",
             evt->type, evt->source, evt->data, evt->len, ctx);
    esp_dispatcher_info_t *d = (esp_dispatcher_info_t *)ctx;
    dispatcher_info->connected = evt->type == WIFI_SERV_EVENT_CONNECTED;
    if (evt->type == WIFI_SERV_EVENT_CONNECTED)
    {
        d->wifi_setting_flag = false;
        ESP_LOGI(TAG_SNTP, "WIFI CONNECTED");
        if (dispatcher_info->state == TM_STATE_BT_SINK)
        {
            esp_player_flash_music_play("spiffs://spiffs/ok_wifi.mp3", 0);
        }
        else if (dispatcher_info->state == TM_STATE_INIT)
        {
            dispatcher_info->state = TM_STATE_WIFI_OK;
        }
    }
    return ESP_OK;
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG_SNTP, "Notification of a time synchronization event");
}

char *azz_check(int azz_w_day, int azz_hour, int azz_min)
{
    if (!audio_board_sdcard_is_mounted())
    {
        esp_player_flash_music_play("spiffs://spiffs/no_sdcard.mp3", 0);
        ESP_LOGE(TAG_SNTP, "Where is the sd card");
        return "";
    }

    ESP_LOGI(TAG_SNTP, "azz_check_start");
    ESP_LOGI(TAG_SNTP, "azz_check_len_hour_if:");
    if (azz_hour > 9)
    {
        if (azz_min > 9)
        {
            sprintf(azz_check_time, "%d:%d", azz_hour, azz_min);
        }
        else
        {
            sprintf(azz_check_time, "%d:0%d", azz_hour, azz_min);
        }
    }
    else
    {

        if (azz_min > 9)
        {

            sprintf(azz_check_time, "0%d:%d", azz_hour, azz_min);
        }
        else
        {

            sprintf(azz_check_time, "0%d:0%d", azz_hour, azz_min);
        }
    }

    ESP_LOGI(TAG_SNTP, "azz_check_time: %s", azz_check_time);

    int fsize(char *file)
    {
        int size;
        FILE *fh;
        fh = fopen(file, "rb"); // binary mode
        if (fh != NULL)
        {
            if (fseek(fh, 0, SEEK_END))
            {
                fclose(fh);
                return -1;
            }
            size = ftell(fh);
            fseek(fh, 0, SEEK_SET);
            fclose(fh);
            return size;
        }
        return -1; // error
    }

    FILE *fp;
    if ((fp = fopen(FILE_PATH_CALL_FILS, "r")) == NULL)
    {
        fclose(fp);
        fp = fopen(FILE_PATH_CALL_FILS, "w");
        if (!fp)
        {
            ESP_LOGE(TAG_SNTP, "Failed to create file calls.json");
        }
        fputs("[]", fp);
        fclose(fp);
    }
    fclose(fp);
    int sd_file_size = fsize(FILE_PATH_CALL_FILS);

    fp = fopen(FILE_PATH_CALL_FILS, "r");

    char data[sd_file_size];
    fread(data, sd_file_size, 1, fp);
    fclose(fp);
    cJSON *root = cJSON_Parse(data);
    const char *sys_info = cJSON_Print(root);
    ESP_LOGI(TAG_SNTP, "%s", sys_info);
    free((void *)sys_info);

    int size_bell_list = cJSON_GetArraySize(root);
    ESP_LOGI(TAG_SNTP, "size_bell_list: %d", size_bell_list);

    for (int i = 0; i < size_bell_list; i++)
    {

        ESP_LOGI(TAG_SNTP, "---------------------------------------------");
        cJSON *selecred_item = cJSON_GetArrayItem(root, i);
        int flag_play = cJSON_GetObjectItem(selecred_item, "play")->valueint;

        if (!flag_play)
        {
            ESP_LOGI(TAG_SNTP, "continue --> flag: play");
            ESP_LOGI(TAG_SNTP, "---------------------------------------------");
            continue;
        }
        ESP_LOGI(TAG_SNTP, "SELECTED ITEM %d: %d", i, flag_play);

        char time_wday[1];
        itoa(azz_w_day, time_wday, 10);
        cJSON *week_days = cJSON_GetObjectItem(selecred_item, "week_days");
        char *w_days = cJSON_Print(week_days);

        if (strstr(w_days, time_wday) == NULL)
        {
            ESP_LOGI(TAG_SNTP, "continue -->  week_days: %s", w_days);
            ESP_LOGI(TAG_SNTP, "---------------------------------------------");
            continue;
        }
        // free((void *)time_wday);
        ESP_LOGI(TAG_SNTP, "WEEK_DAYS %d: %s", i, w_days);

        char *time_start = cJSON_GetObjectItem(selecred_item, "time_start")->valuestring;
        ESP_LOGI(TAG_SNTP, "time_start: %s", time_start);
        char *time_end = cJSON_GetObjectItem(selecred_item, "time_end")->valuestring;
        ESP_LOGI(TAG_SNTP, "time_end: %s", time_end);

        if (strstr(time_start, azz_check_time) == NULL &&
            strstr(time_end, azz_check_time) == NULL)
        {

            ESP_LOGI(TAG_SNTP, "continue --> azz_check_time: %s,time_start: %s, time_end: %s", azz_check_time, time_start, time_end);
            ESP_LOGI(TAG_SNTP, "---------------------------------------------");
            continue;
        }
        memset(azz_check_time, 0, 64);
        free((void *)time_start);
        free((void *)time_end);

        ESP_LOGI(TAG_SNTP, "ITEM %d TIME: %s", i, w_days);

        free((void *)w_days);

        char *music_name = cJSON_GetObjectItem(selecred_item, "music_name")->valuestring;
        ESP_LOGI(TAG_SNTP, "PLAY azz-SONG . . .");
        ESP_LOGI(TAG_SNTP, "%s", music_name);
        ESP_LOGI(TAG_SNTP, "---------------------------------------------");

        memset(path_ad_sd, 0, sizeof(path_ad_sd));
        sprintf(path_ad_sd, "%s/%s", "sdcard", music_name);
        FILE *music; 
        music = fopen(audio_path, "rb");
        if (music == NULL)
        {
            fclose(music);
            esp_player_flash_music_play("spiffs://spiffs/bell.mp3", 0);
        }
        memset(path_ad_sd, 0, sizeof(path_ad_sd));
        sprintf(path_ad_sd, "%s/%s", "file://sdcard", music_name);

        free((void *)music_name);

        ESP_LOGI(TAG_SNTP, "- - - path_ad_sd: %s - - -", path_ad_sd);
        cJSON_Delete(root);
        return &path_ad_sd;
    }
    cJSON_Delete(root);
    return "";
}

static void obtain_time(void)
{
    initialize_sntp();
    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    const int retry_count = 44;

    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
        ESP_LOGI(TAG_SNTP, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    time(&now);
    localtime_r(&now, &timeinfo);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG_SNTP, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, NTP_SERVER);
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    sntp_init();
}

static void init_time(void)
{
    ESP_LOGI(TAG_SNTP, "[ Step 9.0 ] : SETTING THE CURRENT DATE");
    time_t now;
    struct tm timeinfo;

    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900))
    {
        ESP_LOGI(TAG_SNTP, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    char strftime_buf[64];
    setenv("TZ", "CST-9", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG_SNTP, "The current date/time in Blagoveshchensk is: %s", strftime_buf);

    if ((timeinfo.tm_year + 1900) < 2000)
    {
        ESP_LOGE(TAG_SNTP, "timeinfo.tm_year : %d", timeinfo.tm_year + 1900);
        esp_player_flash_music_play("spiffs://spiffs/error.mp3", 0);
    }
    else
    {
        esp_player_flash_music_play("spiffs://spiffs/adf_music.mp3", 0);
    }

    // server_sec = timeinfo.tm_sec;
    // server_min = timeinfo.tm_min;
    // server_hour = timeinfo.tm_hour;
    // server_wday = timeinfo.tm_wday;
}

static void update_time(void)
{
    while (1)
    {
        // ESP_LOGI(TAG_SNTP, "[ UPDATE TIME ] : SETTING THE CURRENT DATE ---------------------|");
        time_t now;
        struct tm timeinfo;

        time(&now);
        localtime_r(&now, &timeinfo);
        // Is time set? If not, tm_year will be (1970 - 1900).
        if (timeinfo.tm_year < (2016 - 1900))
        {
            ESP_LOGI(TAG_SNTP, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
            obtain_time();
            // update 'now' variable with current time
            time(&now);
        }
        char strftime_buf[64];
        setenv("TZ", "CST-9", 1);
        tzset();
        localtime_r(&now, &timeinfo);
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG_SNTP, "[ UPDATE TIME ] : The current date/time in Blagoveshchensk is: %s", strftime_buf);

        if (timeinfo.tm_sec == 0)
        {
            ESP_LOGI(TAG_SNTP, "server_wday: %d | server_hour: %d | server_min: %d", timeinfo.tm_wday, timeinfo.tm_hour, timeinfo.tm_min);
            char *audio_path = azz_check(timeinfo.tm_wday, timeinfo.tm_hour, timeinfo.tm_min);
            if (strncmp(audio_path, "", 1) != 0)
            {
                ESP_LOGI(TAG_SNTP, "audio_path : %s", audio_path);

                set_send_state(SEND_AUDIO_STATUS_PLAY);
                ESP_ERROR_CHECK(azz_telegram_bot());
                ESP_LOGI(TAG_SNTP, "SLEEP");
                sleep(1);
                ESP_LOGI(TAG_SNTP, "PLAY");
                fclose(music);
                esp_player_sdcard_music_play(path_ad_sd, 0);
            }
            else
            {
                ESP_LOGI(TAG_SNTP, "NOT CALLs");
            }
        }
        esp_audio_state_t st = {0};
        esp_player_state_get(&st);
        if (st.status != AUDIO_STATUS_RUNNING && timeinfo.tm_sec % 5 == 0)
        {
            ESP_ERROR_CHECK(azz_telegram_bot());
        }
        ESP_LOGI(TAG_SNTP, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
        sleep(1);
    }
}

static void setClock(void)
{
    // obtain time over NTP
    ESP_LOGI(TAG_DS1302, "Connecting to WiFi and getting time over NTP.");
    obtain_time();

    // update 'now' variable with current time
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];
    time(&now);
    now = now + (CONFIG_TIMEZONE*60*60);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG_DS1302, "The current date/time is: %s", strftime_buf);


    // Initialize RTC
    DS1302_Dev dev;
    if (!DS1302_begin(&dev, CONFIG_CLK_GPIO, CONFIG_IO_GPIO, CONFIG_CE_GPIO)) {
        ESP_LOGE(TAG_DS1302, "Error: DS1302 begin");
        while (1) { vTaskDelay(1); }
    }
    ESP_LOGI(TAG_DS1302, "Set initial date time...");

    /*
    Member    Type Meaning(Range)
    tm_sec    int  seconds after the minute(0-60)
    tm_min    int  minutes after the hour(0-59)
    tm_hour   int  hours since midnight(0-23)
    tm_mday   int  day of the month(1-31)
    tm_mon    int  months since January(0-11)
    tm_year   int  years since 1900
    tm_wday   int  days since Sunday(0-6)
    tm_yday   int  days since January 1(0-365)
    tm_isdst  int  Daylight Saving Time flag	
    */
    ESP_LOGD(TAG_DS1302, "timeinfo.tm_sec=%d",timeinfo.tm_sec);
    ESP_LOGD(TAG_DS1302, "timeinfo.tm_min=%d",timeinfo.tm_min);
    ESP_LOGD(TAG_DS1302, "timeinfo.tm_hour=%d",timeinfo.tm_hour);
    ESP_LOGD(TAG_DS1302, "timeinfo.tm_wday=%d",timeinfo.tm_wday);
    ESP_LOGD(TAG_DS1302, "timeinfo.tm_mday=%d",timeinfo.tm_mday);
    ESP_LOGD(TAG_DS1302, "timeinfo.tm_mon=%d",timeinfo.tm_mon);
    ESP_LOGD(TAG_DS1302, "timeinfo.tm_year=%d",timeinfo.tm_year);

    // Set initial date and time
    DS1302_DateTime dt;
    dt.second = timeinfo.tm_sec;
    dt.minute = timeinfo.tm_min;
    dt.hour = timeinfo.tm_hour;
    dt.dayWeek = timeinfo.tm_wday; // 0= Sunday 1 = Monday
    dt.dayMonth = timeinfo.tm_mday;
    dt.month = (timeinfo.tm_mon + 1);
    dt.year = (timeinfo.tm_year + 1900);
    DS1302_setDateTime(&dev, &dt);

    if ((dt.year + 1900) < 2000)
    {
        ESP_LOGE(TAG_SNTP, "dt.year : %d", dt.year + 1900);
        esp_player_flash_music_play("spiffs://spiffs/error.mp3", 0);
    }
    else
    {
        esp_player_flash_music_play("spiffs://spiffs/adf_music.mp3", 0);
    }

    // Check write protect state
    if (DS1302_isWriteProtected(&dev)) {
        ESP_LOGE(TAG_DS1302, "Error: DS1302 write protected");
        while (1) { vTaskDelay(1); }
    }

    // Check write protect state
    if (DS1302_isHalted(&dev)) {
        ESP_LOGE(TAG_DS1302, "Error: DS1302 halted");
        while (1) { vTaskDelay(1); }
    }
    ESP_LOGI(TAG_DS1302, "Set initial date time done");

    // goto deep sleep
    const int deep_sleep_sec = 1;
    ESP_LOGI(TAG_DS1302, "Entering deep sleep for %d seconds", deep_sleep_sec);
    esp_deep_sleep(1000000LL * deep_sleep_sec);
}

static void getClock(void)
{
    DS1302_Dev dev;
    DS1302_DateTime dt;

    // Initialize RTC
    ESP_LOGI(TAG_DS1302, "Start");
    if (!DS1302_begin(&dev, CONFIG_CLK_GPIO, CONFIG_IO_GPIO, CONFIG_CE_GPIO))
    {
        ESP_LOGE(TAG_DS1302, "Error: DS1302 begin");
        while (1)
        {
            vTaskDelay(1);
        }
    }

    
    // Initialise the xLastWakeTime variable with the current time.
    struct tm timeinfo;
    char strftime_buf[64];
    esp_audio_state_t st = {0};
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while (1)
    {
        esp_player_state_get(&st);
        if (st.status != AUDIO_STATUS_RUNNING)
        {
            // Get RTC date and time
            if (!DS1302_getDateTime(&dev, &dt))
            {
                ESP_LOGE(TAG_DS1302, "Error: DS1302 read failed");
            }
            else
            {
                if (dt.second == 0)
                {
                    timeinfo.tm_sec = dt.second;
                    timeinfo.tm_min = dt.minute;
                    timeinfo.tm_hour = dt.hour;
                    timeinfo.tm_wday = dt.dayWeek; // 0= Sunday 1 = Monday
                    timeinfo.tm_mday = dt.dayMonth;
                    timeinfo.tm_mon = dt.month;
                    timeinfo.tm_year = dt.year - 1900;
                    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
                    ESP_LOGI(TAG_DS1302, "The current date/time is: %s", strftime_buf);
                    ESP_LOGI(TAG_DS1302, "%d %02d-%02d-%d %d:%02d:%02d",
                             dt.dayWeek, dt.dayMonth, dt.month, dt.year, dt.hour, dt.minute, dt.second);
                    char *audio_path = azz_check(dt.dayWeek, dt.hour, dt.minute);
                    if (strncmp(audio_path, "", 1) != 0)
                    {
                        ESP_LOGI(TAG_SNTP, "audio_path : %s", audio_path);
                        set_send_state(SEND_AUDIO_STATUS_PLAY);
                        ESP_ERROR_CHECK(azz_telegram_bot());
                        ESP_LOGI(TAG_SNTP, "SLEEP");
                        vTaskDelayUntil(&xLastWakeTime, 100);
                        ESP_LOGI(TAG_SNTP, "PLAY");
                        esp_player_sdcard_music_play(path_ad_sd, 0);
                    }
                    else
                    {
                        ESP_LOGI(TAG_SNTP, "NOT CALLs");
                    }
                }
                esp_audio_state_t st = {0};
                esp_player_state_get(&st);
                if (dt.second % 5 == 0)
                {
                    ESP_ERROR_CHECK(azz_telegram_bot());
                }
                ESP_LOGI(TAG_SNTP, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
            }
        }
        vTaskDelayUntil(&xLastWakeTime, 100);
    }
}

static void diffClock(void)
{
    // obtain time over NTP
    ESP_LOGI(TAG_DS1302, "Connecting to WiFi and getting time over NTP.");
    obtain_time();
    
    // update 'now' variable with current time
    time_t now;
    struct tm timeinfo;
    char strftime_buf[64];
    time(&now);
    now = now + (CONFIG_TIMEZONE*60*60);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%m-%d-%y %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG_DS1302, "NTP date/time is: %s", strftime_buf);

    DS1302_Dev dev;
    DS1302_DateTime dt;

    // Initialize RTC
    if (!DS1302_begin(&dev, CONFIG_CLK_GPIO, CONFIG_IO_GPIO, CONFIG_CE_GPIO)) {
        ESP_LOGE(TAG_DS1302, "Error: DS1302 begin");
        while (1) { vTaskDelay(1); }
    }

    // Get RTC date and time
    if (!DS1302_getDateTime(&dev, &dt)) {
        ESP_LOGE(TAG_DS1302, "Error: DS1302 read failed");
        while (1) { vTaskDelay(1); }
    }

    // update 'rtcnow' variable with current time
    struct tm rtcinfo;
    rtcinfo.tm_sec = dt.second;
    rtcinfo.tm_min = dt.minute;
    rtcinfo.tm_hour = dt.hour;
    rtcinfo.tm_mday = dt.dayMonth;
    rtcinfo.tm_mon = dt.month - 1; // 0 org.
    rtcinfo.tm_year = dt.year - 1900;
    rtcinfo.tm_isdst = -1;
    time_t rtcnow = mktime(&rtcinfo);
    localtime_r(&rtcnow, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%m-%d-%y %H:%M:%S", &timeinfo);
    ESP_LOGI(TAG_DS1302, "RTC date/time is: %s", strftime_buf);

    // Get the time difference
    double x = difftime(rtcnow, now);
    ESP_LOGI(TAG_DS1302, "Time difference is: %f", x);
    
    while(1) {
        vTaskDelay(1000);
    }
}

static void rtc_time(void)
{
    ++boot_count;
    ESP_LOGI(TAG_DS1302, "CONFIG_CLK_GPIO = %d", CONFIG_CLK_GPIO);
    ESP_LOGI(TAG_DS1302, "CONFIG_IO_GPIO = %d", CONFIG_IO_GPIO);
    ESP_LOGI(TAG_DS1302, "CONFIG_CE_GPIO = %d", CONFIG_CE_GPIO);
    ESP_LOGI(TAG_DS1302, "CONFIG_TIMEZONE= %d", CONFIG_TIMEZONE);
    ESP_LOGI(TAG_DS1302, "Boot count: %d", boot_count);

#if CONFIG_SET_CLOCK
    // Set clock & Get clock
    if (boot_count == 1)
    {
        setClock();
    }
    else
    {
        esp_player_flash_music_play("spiffs://spiffs/adf_music.mp3", 0);
        getClock();
    }
#endif

// #if CONFIG_GET_CLOCK
//     // Get clock
//     xTaskCreate(getClock, "getClock", 1024*4, NULL, 2, NULL);
// #endif

// #if CONFIG_DIFF_CLOCK
//     // Diff clock
//     xTaskCreate(diffClock, "diffClock", 1024*4, NULL, 2, NULL);
// #endif
}

static void init_app(void)
{
    ESP_LOGI(TAG_SNTP, "[Step 1.0] Create dispatcher_info instance");
    dispatcher_info = audio_calloc(1, sizeof(esp_dispatcher_info_t));
    AUDIO_MEM_CHECK(TAG_SNTP, dispatcher_info, return);
    dispatcher_info->state = TM_STATE_INIT;
    dispatcher_info->wait = false;
    dispatcher_info->connected = false;
    sprintf(dispatcher_info->info, "%s", "init");
    dispatcher_info->src_state = SRC_INIT;

    audio_board_init();

    esp_dispatcher_handle_t dispatcher = esp_dispatcher_get_delegate_handle();
    dispatcher_info->dispatcher = dispatcher;

    ESP_LOGI(TAG_SNTP, "[Step 2.0] Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 15,
        .format_if_mount_failed = true};

    esp_err_t spiffs_ret = esp_vfs_spiffs_register(&conf);

    if (spiffs_ret != ESP_OK)
    {
        if (spiffs_ret == ESP_FAIL)
        {
            ESP_LOGE(TAG_SNTP, "Failed to mount or format filesystem");
        }
        else if (spiffs_ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(TAG_SNTP, "Failed to find SPIFFS partition");
        }
        else
        {
            ESP_LOGE(TAG_SNTP, "Failed to initialize SPIFFS (%s)", esp_err_to_name(spiffs_ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    spiffs_ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (spiffs_ret != ESP_OK)
    {
        ESP_LOGE(TAG_SNTP, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(spiffs_ret));
        esp_spiffs_format(conf.partition_label);
        return;
    }
    else
    {
        ESP_LOGI(TAG_SNTP, "Partition size: total: %d, used: %d", total, used);
    }

    ESP_LOGI(TAG_SNTP, "[Step 3.0] Initialize esp player");
    dispatcher_info->player = setup_player(esp_audio_callback_func, NULL);
    esp_player_init(dispatcher_info->player);
    ESP_LOGI(TAG_SNTP, "[Step 4.0] Register wanted player execution type");
    esp_dispatcher_reg_exe_func(dispatcher, dispatcher_info->player, ACTION_EXE_TYPE_AUDIO_PLAY, player_action_play);
    esp_dispatcher_reg_exe_func(dispatcher, dispatcher_info->player, ACTION_EXE_TYPE_AUDIO_PAUSE, player_action_pause);
    esp_dispatcher_reg_exe_func(dispatcher, dispatcher_info->player, ACTION_EXE_TYPE_AUDIO_VOLUME_UP, player_action_vol_up);
    esp_dispatcher_reg_exe_func(dispatcher, dispatcher_info->player, ACTION_EXE_TYPE_AUDIO_VOLUME_DOWN, player_action_vol_down);

    ESP_LOGI(TAG_SNTP, "[Step 5.0] Create esp_periph_set_handle_t instance and initialize SDcard");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    periph_cfg.extern_stack = true;
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);
    audio_board_sdcard_init(set);

    ESP_LOGI(TAG_SNTP, "[Step 6.0] Create Wi-Fi service instance");
    wifi_config_t sta_cfg = {0};

    wifi_service_config_t service_cfg = WIFI_SERVICE_DEFAULT_CONFIG();
    service_cfg.evt_cb = wifi_service_cb;
    service_cfg.cb_ctx = dispatcher_info;
    service_cfg.setting_timeout_s = 60 * 10;
    dispatcher_info->wifi_serv = wifi_service_create(&service_cfg);
    wifi_service_set_sta_info(dispatcher_info->wifi_serv, &sta_cfg);
    int reg_idx = 0;
    smart_config_info_t info = SMART_CONFIG_INFO_DEFAULT();
    esp_wifi_setting_handle_t h = smart_config_create(&info);
    esp_wifi_setting_regitster_notify_handle(h, (void *)dispatcher_info->wifi_serv);
    wifi_service_register_setting_handle(dispatcher_info->wifi_serv, h, &reg_idx);
    wifi_service_get_last_ssid_cfg(dispatcher_info->wifi_serv, &sta_cfg);
    if (strlen((const char *)sta_cfg.sta.ssid))
    {
        ESP_LOGI(TAG_SNTP, "Found ssid %s", (const char *)sta_cfg.sta.ssid);
        ESP_LOGI(TAG_SNTP, "Found password %s", (const char *)sta_cfg.sta.password);
        wifi_service_connect(dispatcher_info->wifi_serv);
    }
    else
    {
        ESP_LOGI(TAG_SNTP, "Init esp Wi-Fi touch");
        dispatcher_info->state = TM_STATE_WIFI_WAIT;
        wifi_service_setting_start(dispatcher_info->wifi_serv, 0);
    }

    ESP_LOGI(TAG_SNTP, "[Step 7.0] Register wanted display service execution type");
    esp_dispatcher_reg_exe_func(dispatcher, dispatcher_info->wifi_serv,
                                ACTION_EXE_TYPE_WIFI_CONNECT, wifi_action_connect);
    esp_dispatcher_reg_exe_func(dispatcher, dispatcher_info->wifi_serv,
                                ACTION_EXE_TYPE_WIFI_DISCONNECT, wifi_action_disconnect);
    esp_dispatcher_reg_exe_func(dispatcher, dispatcher_info->wifi_serv,
                                ACTION_EXE_TYPE_WIFI_SETTING_STOP, wifi_action_setting_stop);
    esp_dispatcher_reg_exe_func(dispatcher, dispatcher_info->wifi_serv,
                                ACTION_EXE_TYPE_WIFI_SETTING_START, wifi_action_setting_start);

    ESP_LOGI(TAG_SNTP, "[Step 8.0 ] : START REST SERVER");
    ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));
}

static void strange_watch(void)
{
    ESP_LOGI(TAG_SNTP, "server_wday: %d | server_hour: %d | server_min: %d", server_wday, server_hour, server_min);

    while (1)
    {
        if (server_sec == 60)
        {
            server_sec = 0;
            server_min++;
        }
        if (server_min == 60)
        {
            server_min = 0;
            server_hour++;
        }
        if (server_hour == 24)
        {
            server_hour = 0;
            server_wday++;
        }
        if (server_wday == 7)
        {

            server_wday = 0;
        }
        if (server_sec == 0)
        {

            ESP_LOGI(TAG_SNTP, "server_wday: %d | server_hour: %d | server_min: %d", server_wday, server_hour, server_min);

            char *audio_path = azz_check(server_wday, server_hour, server_min);
            if (strncmp(audio_path, "", 1) != 0)
            {

                ESP_LOGI(TAG_SNTP, "audio_path : %s", audio_path);
                esp_player_sdcard_music_play(path_ad_sd, 0);
            }
            else
            {

                ESP_LOGI(TAG_SNTP, "NOT CALLs");
            }
        }
        if (server_sec % 5 == 0)
        {

            // xTaskCreatePinnedToCore(&http_test_task, "http_test_task", 8192*2, NULL, 5, NULL,1);
        }
        server_sec++;
        sleep(1);
    }
}

static char my_ip(void)
{
    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ipInfo);
    ESP_LOGW(TAG_SNTP, "My IP : " IPSTR, IP2STR(&ipInfo.ip));
    char my_address[20];
    sprintf(my_address, IPSTR, IP2STR(&ipInfo.ip));
    ESP_LOGW(TAG_SNTP, "my_address : %s", my_address);
    return my_address;
}

void server_time_setting_and_call_handing(void)
{

    init_app();

    rtc_time();

    // init_time();

    // update_time();

    // strange_watch();
}

//  *** -------------------- *** end functions *** -------------------- ***
