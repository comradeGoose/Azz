#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <freertos/semphr.h>

#include "esp_log.h"
#include "esp_audio.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_mem.h"
#include "board.h"
#include "audio_common.h"
#include "audio_hal.h"
#include "filter_resample.h"
#include "fatfs_stream.h"
#include "raw_stream.h"
#include "i2s_stream.h"
#include "spiffs_stream.h"
#include "ogg_decoder.h"
#include "wav_decoder.h"
#include "mp3_decoder.h"
#include "pcm_decoder.h"
#include "http_stream.h"
#include "audio_idf_version.h"

#include "model_path.h"

#include "audio_recorder.h"
#include "recorder_sr.h"

static char *TAG = "AUDIO_SETUP";

#ifndef CODEC_ADC_SAMPLE_RATE
#warning "Please define CODEC_ADC_SAMPLE_RATE first, default value is 48kHz may not correctly"
#define CODEC_ADC_SAMPLE_RATE    48000
#endif

#ifndef CODEC_ADC_BITS_PER_SAMPLE
#warning "Please define CODEC_ADC_BITS_PER_SAMPLE first, default value 16 bits may not correctly"
#define CODEC_ADC_BITS_PER_SAMPLE  I2S_BITS_PER_SAMPLE_16BIT
#endif

#ifndef CODEC_ADC_I2S_PORT
#define CODEC_ADC_I2S_PORT  (0)
#endif

static audio_element_handle_t raw_read;

static int _http_stream_event_handle(http_stream_event_msg_t *msg)
{
    if (msg->event_id == HTTP_STREAM_RESOLVE_ALL_TRACKS) {
        return ESP_OK;
    }

    if (msg->event_id == HTTP_STREAM_FINISH_TRACK) {
        return http_stream_next_track(msg->el);
    }
    if (msg->event_id == HTTP_STREAM_FINISH_PLAYLIST) {
        return http_stream_restart(msg->el);
    }
    return ESP_OK;
}


void *setup_player(void* cb, void *ctx)
{
    esp_audio_cfg_t cfg = DEFAULT_ESP_AUDIO_CONFIG();
    // cfg.out_stream_buf_size = 10*1024;
    // cfg.in_stream_buf_size = 10*1024;
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
    

    cfg.vol_handle = board_handle->audio_hal;
    cfg.vol_set = (audio_volume_set)audio_hal_set_volume;
    cfg.vol_get = (audio_volume_get)audio_hal_get_volume;
    cfg.resample_rate = 48000;
    cfg.prefer_type = ESP_AUDIO_PREFER_MEM;
    cfg.cb_func = cb;
    cfg.cb_ctx = ctx;
    esp_audio_handle_t handle = esp_audio_create(&cfg);
    esp_audio_play_timeout_set(handle, 500);

    // Create readers and add to esp_audio
    fatfs_stream_cfg_t fs_reader = FATFS_STREAM_CFG_DEFAULT();
    fs_reader.type = AUDIO_STREAM_READER;

    esp_audio_input_stream_add(handle, fatfs_stream_init(&fs_reader));
    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_cfg.event_handle = _http_stream_event_handle;
    http_cfg.type = AUDIO_STREAM_READER;
    http_cfg.enable_playlist_parser = true;
    audio_element_handle_t http_stream_reader = http_stream_init(&http_cfg);
    esp_audio_input_stream_add(handle, http_stream_reader);

    spiffs_stream_cfg_t flash_cfg = SPIFFS_STREAM_CFG_DEFAULT();
    flash_cfg.type = AUDIO_STREAM_READER;
    esp_audio_input_stream_add(handle, spiffs_stream_init(&flash_cfg));

    // // Create bluetooth stream
    // a2dp_stream_config_t a2dp_config = {
	// 	.type = AUDIO_STREAM_READER,
	// 	.user_callback.user_a2d_cb = {bt_cb},
    //     .audio_hal = board_handle->audio_hal,
	// };
    // audio_element_handle_t bt_stream_reader = a2dp_stream_init(&a2dp_config);
	// esp_audio_input_stream_add(handle, bt_stream_reader);

    mp3_decoder_cfg_t mp3_dec_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_dec_cfg.task_core = 1;
    esp_audio_codec_lib_add(handle, AUDIO_CODEC_TYPE_DECODER, mp3_decoder_init(&mp3_dec_cfg));

    ogg_decoder_cfg_t ogg_dec_cfg = DEFAULT_OGG_DECODER_CONFIG();
    ogg_dec_cfg.task_core = 1;
    esp_audio_codec_lib_add(handle, AUDIO_CODEC_TYPE_DECODER, ogg_decoder_init(&ogg_dec_cfg));   

    audio_element_handle_t oga_dec_cfg = ogg_decoder_init(&ogg_dec_cfg);
    audio_element_set_tag(oga_dec_cfg, "oga");
    esp_audio_codec_lib_add(handle, AUDIO_CODEC_TYPE_DECODER, oga_dec_cfg);

    // Create writers and add to esp_audio
    i2s_stream_cfg_t i2s_writer = I2S_STREAM_CFG_DEFAULT();
    i2s_writer.type = AUDIO_STREAM_WRITER;
    i2s_writer.i2s_config.sample_rate = 48000;
    i2s_writer.i2s_config.bits_per_sample = CODEC_ADC_BITS_PER_SAMPLE;
    i2s_writer.need_expand = (CODEC_ADC_BITS_PER_SAMPLE != I2S_BITS_PER_SAMPLE_16BIT);
    esp_audio_output_stream_add(handle, i2s_stream_init(&i2s_writer));

    // Set default volume
    esp_audio_vol_set(handle, 10);
    AUDIO_MEM_SHOW(TAG);
    ESP_LOGI(TAG, "esp_audio instance is:%p", handle);
    return handle;
}
