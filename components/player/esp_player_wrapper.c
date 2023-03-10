#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <freertos/queue.h>
#include "esp_player_wrapper.h"
#include "esp_log.h"
#include "audio_mem.h"
#include "audio_error.h"

typedef struct {
    esp_audio_handle_t              handle;
    esp_audio_info_t                backup_info;
    bool                            backup_flag;
} esp_player_t;

static const char *TAG = "ESP_PLAYER";
static esp_player_t *esp_player;

audio_err_t esp_player_init(esp_audio_handle_t handle)
{
    esp_player = audio_calloc(1, sizeof(esp_player_t));
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    esp_player->handle = handle;
    return ESP_ERR_AUDIO_NO_ERROR;
}

audio_err_t esp_player_deinit()
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    audio_free(esp_player);
    return 0;
}

audio_err_t esp_player_music_play(const char *url, int pos, media_source_type_t type)
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    esp_audio_state_t st = {0};
    int ret = ESP_OK;
    esp_audio_state_get(esp_player->handle, &st);
    if (st.status == AUDIO_STATUS_RUNNING) {
        ret = esp_audio_stop(esp_player->handle, TERMINATION_TYPE_NOW);
        if (ret != ESP_ERR_AUDIO_NO_ERROR) {
            return ret;
        }
    }
    ret = esp_audio_media_type_set(esp_player->handle, type);
    ret |=  esp_audio_play(esp_player->handle, AUDIO_CODEC_TYPE_DECODER, url, pos);
    return ret;
}

audio_err_t esp_player_http_music_play(const char *url, int pos)
{
    ESP_LOGW(TAG, "play: %s", url);
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    esp_audio_state_t st = {0};
    int ret = ESP_OK;
    esp_audio_state_get(esp_player->handle, &st);
    if (st.status == AUDIO_STATUS_RUNNING) {
        ESP_LOGW(TAG, "stoping...");
        ret = esp_audio_stop(esp_player->handle, TERMINATION_TYPE_NOW);
        if (ret != ESP_ERR_AUDIO_NO_ERROR) {
            return ret;
        }
    }
    ret = esp_audio_media_type_set(esp_player->handle, MEDIA_SRC_TYPE_TONE_HTTP);
    ret |=  esp_audio_sync_play(esp_player->handle, url, pos);
    return ret;
}

audio_err_t esp_player_bt_music_play() {
    ESP_LOGW(TAG, "play: BT");
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    esp_audio_state_t st = {0};
    int ret = ESP_OK;
    esp_audio_state_get(esp_player->handle, &st);
    if (st.status == AUDIO_STATUS_RUNNING) {
        ESP_LOGW(TAG, "stoping...");
        ret = esp_audio_stop(esp_player->handle, TERMINATION_TYPE_NOW);
        if (ret != ESP_ERR_AUDIO_NO_ERROR) {
            ESP_LOGE(TAG, "Error stop");
            return ret;
        }
    }
    ret = esp_audio_media_type_set(esp_player->handle, MEDIA_SRC_TYPE_MUSIC_A2DP);
    ret |=  esp_audio_play(esp_player->handle, AUDIO_CODEC_TYPE_DECODER, "aadp://44100:2@bt/sink/stream.pcm", 0);
    return ret;
}

bool esp_player_music_is_running() {
    esp_audio_state_t st = {0};
    esp_audio_state_get(esp_player->handle, &st);
    ESP_LOGW(TAG, "state: %d", st.status);
    return st.status == AUDIO_STATUS_RUNNING;
}

audio_err_t esp_player_sdcard_music_play(const char *url, int pos)
{
    ESP_LOGW(TAG,"sd: %s",url);
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    esp_audio_state_t st = {0};
    int ret = ESP_OK;
    esp_audio_state_get(esp_player->handle, &st);
    if (st.status == AUDIO_STATUS_RUNNING) {
        ret = esp_audio_stop(esp_player->handle, TERMINATION_TYPE_NOW);
        if (ret != ESP_ERR_AUDIO_NO_ERROR) {
            return ret;
        }
    }
    ret = esp_audio_media_type_set(esp_player->handle, MEDIA_SRC_TYPE_MUSIC_SD);
    ret |=  esp_audio_play(esp_player->handle, AUDIO_CODEC_TYPE_DECODER, url, pos);
    return ret;
}

audio_err_t esp_player_flash_music_play(const char *url, int pos)
{
    ESP_LOGW(TAG,"flash: %s",url);
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    esp_audio_state_t st = {0};
    int ret = ESP_OK;
    esp_audio_state_get(esp_player->handle, &st);
    if (st.status == AUDIO_STATUS_RUNNING) {
        ret = esp_audio_stop(esp_player->handle, TERMINATION_TYPE_NOW);
        if (ret != ESP_ERR_AUDIO_NO_ERROR) {
            return ret;
        }
    }
    ret = esp_audio_media_type_set(esp_player->handle, MEDIA_SRC_TYPE_MUSIC_FLASH);
    ret |=  esp_audio_play(esp_player->handle, AUDIO_CODEC_TYPE_DECODER, url, pos);
    return ret;
}

audio_err_t esp_player_music_stop()
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    int ret = esp_audio_media_type_set(esp_player->handle, MEDIA_SRC_TYPE_NULL);
    ret = esp_audio_stop(esp_player->handle, TERMINATION_TYPE_NOW);
    return ret;
}

audio_err_t esp_player_music_pause()
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    int ret = ESP_ERR_AUDIO_NOT_SUPPORT;
    esp_audio_state_t st = {0};
    esp_audio_state_get(esp_player->handle, &st);
    if (st.status == AUDIO_STATUS_RUNNING) {
        ret = esp_audio_pause(esp_player->handle);
        ret |= esp_audio_info_get(esp_player->handle, &esp_player->backup_info);
        esp_player->backup_flag = true;
        ESP_LOGD(TAG, "%s, i:%p, c:%p, f:%p, o:%p,status:%d", __func__,
                 esp_player->backup_info.in_el,
                 esp_player->backup_info.codec_el,
                 esp_player->backup_info.filter_el,
                 esp_player->backup_info.out_el, esp_player->backup_info.st.status);
        return ret;
    } else {
        return ret;
    }
}

audio_err_t esp_player_music_resume()
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    int ret = ESP_ERR_AUDIO_NO_ERROR;
    if (esp_player->backup_flag) {
        esp_audio_info_set(esp_player->handle, &esp_player->backup_info);
        esp_player->backup_flag = false;
        ret = esp_audio_resume(esp_player->handle);
    } else {
        ret = esp_audio_resume(esp_player->handle);
    }
    return ret;
}

audio_err_t esp_player_tone_play(const char *url, bool blocked, media_source_type_t type)
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    if ((type <= MEDIA_SRC_TYPE_TONE_BASE)
        || (type >= MEDIA_SRC_TYPE_TONE_MAX)
        || (url == NULL)) {
        ESP_LOGE(TAG, "Invalid parameters, url:%p, type:%x", url, type);
        return ESP_ERR_AUDIO_INVALID_PARAMETER;
    }
    int ret = esp_audio_media_type_set(esp_player->handle, type);
    if (blocked) {
        ret |=  esp_audio_sync_play(esp_player->handle, url, 0);
    } else {
        ret |=  esp_audio_play(esp_player->handle, AUDIO_CODEC_TYPE_DECODER, url, 0);
    }
    return ret;
}

audio_err_t esp_player_tone_stop()
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    int ret = esp_audio_stop(esp_player->handle, TERMINATION_TYPE_NOW);
    return ret;
}

audio_err_t esp_player_state_get(esp_audio_state_t *state)
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    return esp_audio_state_get(esp_player->handle, state);
}

media_source_type_t esp_player_media_src_get()
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    esp_audio_state_t st = {0};
    esp_audio_state_get(esp_player->handle, &st);
    return st.media_src;
}

bool esp_player_is_backup()
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    return esp_player->backup_flag;
}

audio_err_t esp_player_time_get(int *time)
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    return esp_audio_time_get(esp_player->handle, time);
}

audio_err_t esp_player_pos_get(int *pos)
{
    AUDIO_MEM_CHECK(TAG, esp_player, return ESP_ERR_AUDIO_MEMORY_LACK);
    return esp_audio_pos_get(esp_player->handle, pos);
}

audio_err_t esp_player_vol_get(int *vol)
{
    esp_audio_vol_get(esp_player->handle, vol);
    return ESP_OK;
}

audio_err_t esp_player_vol_set(int vol)
{
    esp_audio_vol_set(esp_player->handle, vol);
    return ESP_OK;
}
