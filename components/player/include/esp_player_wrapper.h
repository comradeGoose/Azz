
#ifndef ESP_PLAYER_WRAPPER_H
#define ESP_PLAYER_WRAPPER_H

#include "esp_audio.h"

/*
 * init audio wrapper
 *
 * @param cb  audio wrapper callback function
 *
 * @return ESP_OK or ESP_FAIL
 *
 */
audio_err_t esp_player_init(esp_audio_handle_t handle);

/*
 * deinit audio wrapper
 *
 * @param none
 *
 * @return ESP_OK or ESP_FAIL
 *
 */
audio_err_t esp_player_deinit();

audio_err_t esp_player_music_play(const char *url, int pos, media_source_type_t type);

audio_err_t esp_player_http_music_play(const char *url, int pos);

audio_err_t esp_player_bt_music_play();

bool esp_player_music_is_running();

audio_err_t esp_player_sdcard_music_play(const char *url, int pos);

audio_err_t esp_player_flash_music_play(const char *url, int pos);

audio_err_t esp_player_music_stop();

audio_err_t esp_player_music_pause();

audio_err_t esp_player_music_resume();

audio_err_t esp_player_tone_play(const char *url, bool blocked, media_source_type_t type);

audio_err_t esp_player_tone_stop();


audio_err_t esp_player_state_get(esp_audio_state_t *state);

media_source_type_t esp_player_media_src_get();

bool esp_player_is_backup();

audio_err_t esp_player_time_get(int *time);

audio_err_t esp_player_pos_get(int *pos);
audio_err_t esp_player_vol_get(int *vol);
audio_err_t esp_player_vol_set(int vol);


#endif //
