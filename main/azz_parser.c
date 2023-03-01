#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include <stdarg.h>
#include "sdkconfig.h"
#include "esp_log.h"
#include <time.h>
#include <sys/time.h>

#include "azz_parser.h"
#include "telegram_bot.h"

static const char *TAG_PARSER = "[ AZZ PARSER ]";

azz_parser_t *last_message = NULL;


//  *** -------------------- *** init deinit *** -------------------- ***
void init_last_message(void)
{
    ESP_LOGW(TAG_PARSER, "INIT LAST MESSAGE");
    last_message = calloc(1, sizeof(azz_parser_t));
    ESP_LOGI(TAG_PARSER, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
}

void deinit_last_message(void)
{
    ESP_LOGW(TAG_PARSER, "DEINIT LAST MESSAGE");
    free(last_message);
    ESP_LOGI(TAG_PARSER, "esp_get_free_heap_size: %d", esp_get_free_heap_size());
}
//  *** -------------------- *** end init deinit *** -------------------- ***


//  *** -------------------- *** set get type id message *** -------------------- ***
void set_type_id_message(int type_id)
{
    last_message->type_message = type_id;
}

int get_type_id_message(void)
{
    return last_message->type_message;
}
//  *** -------------------- *** end set get type id message *** -------------------- ***


//  *** -------------------- *** init deinit type message *** -------------------- ***
esp_err_t init_type_message(void)
{
    int id = get_type_id_message();
    if (id == TYPE_MESSAGE_ERROR)
    {
        ESP_LOGW(TAG_PARSER, "Fail init type message!");
        return ESP_FAIL;
    }
    // if (id == TYPE_MESSAGE_TEXT)
    // {
    //     ESP_LOGW(TAG_PARSER, "Init text message . . .");
    //     last_message->last_message_text = calloc(1, sizeof(message_text_t));
    //     return ESP_OK;
    // }
    if (id == TYPE_MESSAGE_VOICE)
    {
        ESP_LOGW(TAG_PARSER, "Init voice message . . .");
        last_message->last_message_voice = calloc(1, sizeof(message_voice_t));
        return ESP_OK;
    }
    // if (id == TYPE_MESSAGE_AUDIO)
    // {
    //     ESP_LOGW(TAG_PARSER, "Init audio message . . .");
    //     last_message->last_message_audio = calloc(1, sizeof(message_audio_t));
    //     return ESP_OK;
    // }
    return ESP_FAIL;
}

esp_err_t deinit_type_message(void)
{
    int id = get_type_id_message();
    if (id == TYPE_MESSAGE_ERROR)
    {
        ESP_LOGW(TAG_PARSER, "Fail deinit type message!");
        return ESP_FAIL;
    }
    // if (id == TYPE_MESSAGE_TEXT)
    // {
    //     ESP_LOGW(TAG_PARSER, "Deinit text message . . .");
    //     free(last_message->last_message_text);
    //     return ESP_OK;
    // }
    if (id == TYPE_MESSAGE_VOICE)
    {
        ESP_LOGW(TAG_PARSER, "Deinit voice message . . .");
        free(last_message->last_message_voice);
        return ESP_OK;
    }
    // if (id == TYPE_MESSAGE_AUDIO)
    // {
    //     ESP_LOGW(TAG_PARSER, "Deinit audio message . . .");
    //     free(last_message->last_message_audio);
    //     return ESP_OK;
    // }
    return ESP_FAIL;
}
//  *** -------------------- *** end init deinit type message *** -------------------- ***


//  *** -------------------- *** last message *** -------------------- ***
// void set_last_message(char *message) 
// {
//     ESP_LOGW(TAG_PARSER, "SET LAST MESSAGE");
//     strlcat(last_message->last_message, message, MAX_MESSAGE_LENGTH);
// }
// char* get_last_message(void)
// {
//     ESP_LOGW(TAG_PARSER, "GET LAST MESSAGE");
//     return &last_message->last_message;
// }
//  *** -------------------- *** end last message *** -------------------- ***


//  *** -------------------- *** set get last message id *** -------------------- ***
void set_last_message_id(int id)
{
    last_message->last_message_id = id;
}

int get_last_message_id(void)
{
    if (!last_message)
    {
        return 0;
    }
    
    return last_message->last_message_id;
}
//  *** -------------------- *** end set get last message id *** -------------------- ***


//  *** -------------------- *** set get last message is bot *** -------------------- ***
void set_last_message_is_bot(int is_bot)
{
    last_message->last_message_is_bot = is_bot;
}

bool get_last_message_is_bot()
{
    return last_message->last_message_is_bot;
}
//  *** -------------------- *** end set get last message is bot *** -------------------- ***


//  *** -------------------- *** set get last message user id *** -------------------- ***
void set_last_message_user_id(int id)
{
    last_message->last_message_user_id = id;
}

int get_last_message_user_id(void)
{
    return last_message->last_message_user_id;
}
//  *** -------------------- *** end set get last message user id *** -------------------- ***


//  *** -------------------- *** set get last message first name *** -------------------- ***
void set_last_message_first_name(char *first_name)
{
    sprintf(last_message->last_message_first_name, "%s", first_name);
}

char* get_last_message_first_name(void)
{
    return &last_message->last_message_first_name;
}
//  *** -------------------- *** set get last message first name *** -------------------- ***


//  *** -------------------- *** set get last message last name *** -------------------- ***
void set_last_message_last_name(char *last_name)
{
    sprintf(last_message->last_message_last_name, "%s", last_name);
}

char* get_last_message_last_name(void)
{
    return &last_message->last_message_last_name;
}
//  *** -------------------- *** set get last message last name *** -------------------- ***


//  *** -------------------- *** set get last message username *** -------------------- ***
void set_last_message_username(char *username)
{
    sprintf(last_message->last_message_username, "%s", username);
}

char* get_last_message_username(void)
{
    return &last_message->last_message_username;
}
//  *** -------------------- *** set get last message username *** -------------------- ***


//  *** -------------------- *** set get last message language code *** -------------------- ***
void set_last_message_language_code(char *language_code)
{
    sprintf(last_message->last_message_language_code, "%s", language_code);
}

char* get_last_message_language_code(void)
{
    return &last_message->last_message_language_code;
}
//  *** -------------------- *** set get last message language code *** -------------------- ***


//  *** -------------------- *** set get last message date *** -------------------- ***
void set_last_message_date(char *date)
{
    sprintf(last_message->last_message_date, "%s", date);
}

char* get_last_message_date(void)
{
    return &last_message->last_message_date;
}
//  *** -------------------- *** set get last message date *** -------------------- ***


//  *** -------------------- *** print info *** -------------------- ***
void print_last_message_info(void)
{
    ESP_LOGI(TAG_PARSER, "Info . . .");
    ESP_LOGI(TAG_PARSER, "id : %d", last_message->last_message_id);
    ESP_LOGI(TAG_PARSER, "user_id : %d", last_message->last_message_user_id);
    ESP_LOGI(TAG_PARSER, "is_bot : %d", last_message->last_message_is_bot);
    ESP_LOGI(TAG_PARSER, "first_name : %s", last_message->last_message_first_name);
    ESP_LOGI(TAG_PARSER, "last_name : %s", last_message->last_message_last_name);
    ESP_LOGI(TAG_PARSER, "username : %s", last_message->last_message_username);
    ESP_LOGI(TAG_PARSER, "language_code : %s", last_message->last_message_language_code);
    ESP_LOGI(TAG_PARSER, "date : %s", last_message->last_message_date);
}
//  *** -------------------- *** end print info *** -------------------- ***


//  *** -------------------- *** set get text message command *** -------------------- ***
// void set_text_message_command(char *command)
// {
//     sprintf(last_message->last_message_text->text_message, "%s", command);
// }
// 
// char* get_text_message_command(void)
// {
//     return &last_message->last_message_text->text_message;
// }
//  *** -------------------- *** end set get text message command *** -------------------- ***


// voice message
void print_last_voice_message_info(void)
{
    ESP_LOGI(TAG_PARSER, "Voice message info . . .");
    ESP_LOGI(TAG_PARSER, "id_voice : %s", last_message->last_message_voice->file_id_voice_message);
    ESP_LOGI(TAG_PARSER, "size_voice : %d KB", last_message->last_message_voice->file_size_voice_message / 1024);
    ESP_LOGI(TAG_PARSER, "path_voice : %s", last_message->last_message_voice->file_path_voice_message);
}

void set_last_message_file_id_voice_message(char * file_id)
{
    sprintf(last_message->last_message_voice->file_id_voice_message, "%s", file_id);
}
char * get_last_message_file_id_voice_message(void)
{
    return &last_message->last_message_voice->file_id_voice_message;
}

void set_last_message_file_size_voice_message(int file_size)
{
    last_message->last_message_voice->file_size_voice_message = file_size;
}
int get_last_message_file_size_voice_message(void)
{
    return last_message->last_message_voice->file_size_voice_message;
}

void set_last_message_file_path_voice_message(char * file_path)
{
    sprintf(last_message->last_message_voice->file_path_voice_message, "%s", file_path);
}
char * get_last_message_file_path_voice_message(void)
{
    return &last_message->last_message_voice->file_path_voice_message;
}

esp_err_t voice_message_parser(char *output_buffer)
{
    cJSON *tg_message = cJSON_Parse(output_buffer);
    int ok = cJSON_GetObjectItem(tg_message, "ok")->valueint;
    if (ok == 0)
    {
        cJSON_Delete(tg_message);
        memset(output_buffer, 0, sizeof(&output_buffer));
        ESP_LOGE(TAG_PARSER, "Invalid bot token . . .");
        return ESP_FAIL;
    }
    cJSON *result_object = cJSON_GetObjectItem(tg_message, "result");
    // char * file_id = cJSON_GetObjectItem(result_object, "file_id")->valuestring;
    // int file_size = cJSON_GetObjectItem(result_object, "file_size")->valueint;
    char * file_path = cJSON_GetObjectItem(result_object, "file_path")->valuestring;
    // set_last_message_file_id_voice_message(file_id);
    // set_last_message_file_size_voice_message(file_size);
    set_last_message_file_path_voice_message(file_path);
    cJSON_Delete(tg_message);
    print_last_voice_message_info();
    return ESP_OK;
}
// ___________________________________________________

char * get_bot_main_menu(void)
{
    char *menu;
	cJSON *reply_markup = cJSON_CreateObject();
    cJSON *inline_keyboard = cJSON_CreateArray();
    cJSON *array_in = cJSON_CreateArray();
    cJSON *btn1 = cJSON_CreateObject();
    cJSON_AddStringToObject(btn1, "text", "🔔🔔🔔");
    cJSON_AddStringToObject(btn1, "callback_data", "/call_list");
    cJSON *btn2 = cJSON_CreateObject();
    cJSON_AddStringToObject(btn2, "text", "🎵🎶🎵");
    cJSON_AddStringToObject(btn2, "callback_data", "/music_list");
    cJSON *btn3 = cJSON_CreateObject();
    cJSON_AddStringToObject(btn3, "text", "➕🔔➕");
    cJSON_AddStringToObject(btn3, "callback_data", "/add_call");
    // cJSON *btn4 = cJSON_CreateObject();
    // cJSON_AddStringToObject(btn4, "text", "➕🎶➕");
    // cJSON_AddStringToObject(btn4, "callback_data", "/upload_music");
    cJSON_AddItemToArray(array_in, btn1);
    cJSON_AddItemToArray(array_in, btn2);
    cJSON_AddItemToArray(array_in, btn3);
    // cJSON_AddItemToArray(array_in, btn4);
    cJSON_AddItemToArray(inline_keyboard, array_in);
    cJSON_AddItemToObject(reply_markup, "inline_keyboard", inline_keyboard);
    // // cJSON_AddNumberToObject(reply_markup, "resize_keyboard", 1);
    // // cJSON_AddStringToObject(reply_markup, , "true");
    cJSON_bool *bool_ = 1;
    cJSON_AddBoolToObject(reply_markup, "resize_keyboard", bool_);
    // cJSON_AddBoolToObject(reply_markup, "selective", bool_);
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "chat_id", 372705559);
    cJSON_AddStringToObject(root, "text", "МЕНЮ: ☟\nСписок звонков : 🔔🔔🔔\nСписок аудиофайлов : 🎵🎶🎵\nДобавить звонок : ➕🔔➕");
    cJSON_AddItemToObject(root, "reply_markup", reply_markup);
	menu = cJSON_Print(root);
	ESP_LOGI(TAG_PARSER, "menu\n%s", menu);
	cJSON_Delete(root);
    return menu;
}


esp_err_t azz_parser(char *output_buffer)
{
    cJSON *tg_message = cJSON_Parse(output_buffer);
    cJSON *result_array = cJSON_GetObjectItem(tg_message, "result");
    cJSON *result_object = cJSON_GetArrayItem(result_array, 0);

    int ok = cJSON_GetObjectItem(tg_message, "ok")->valueint;

    if (ok == 0)
    {
        cJSON_Delete(tg_message);
        memset(output_buffer, 0, sizeof(&output_buffer));
        ESP_LOGE(TAG_PARSER, "Invalid bot token . . .");
        return ESP_FAIL;
    }

    if (result_object == NULL)
    {
        cJSON_Delete(tg_message);
        memset(output_buffer, 0, sizeof(&output_buffer));
        ESP_LOGE(TAG_PARSER, "There were no messages in the last 24 hours . . .");
        return ESP_FAIL;
    }
    
    cJSON *message_object = cJSON_GetObjectItem(result_object, "message");
    cJSON *from_object = cJSON_GetObjectItem(message_object, "from");

    int type_message_ID = TYPE_MESSAGE_ERROR;

    // cJSON *text_message = cJSON_GetObjectItem(message_object, "text");
    // cJSON *entities = cJSON_GetObjectItem(message_object, "entities");
    // if (text_message && entities)
    // {
    //     ESP_LOGE(TAG_PARSER, "TYPE MESSAGE : TEXT");
    //     type_message_ID = TYPE_MESSAGE_TEXT;
    // }
    
    cJSON *voice_message = cJSON_GetObjectItem(message_object, "voice");
    if (voice_message)
    {
        ESP_LOGE(TAG_PARSER, "TYPE MESSAGE : VOICE");
        type_message_ID = TYPE_MESSAGE_VOICE;
    }
    
    // cJSON *audio_message = cJSON_GetObjectItem(message_object, "audio");
    // if (audio_message)
    // {
    //     ESP_LOGE(TAG_PARSER, "TYPE MESSAGE : AUDIO");
    //     type_message_ID = TYPE_MESSAGE_AUDIO;
    // }
    
    int message_id = cJSON_GetObjectItem(message_object, "message_id")->valueint;
    int last_message_id = get_last_message_id();

    if (message_id == last_message_id)
    {
        cJSON_Delete(tg_message);
        ESP_LOGE(TAG_PARSER, "[message_id = %d] === [last_message_id = %d]", message_id, last_message_id);
        memset(output_buffer, 0, sizeof(&output_buffer));
        return ESP_FAIL;
    }
    // deinit_type_message();
    deinit_last_message();
    
    ESP_LOGE(TAG_PARSER, "NOT LAST MESSAGE . . . ");
    ESP_LOGE(TAG_PARSER, "last_message_id = %d", last_message_id);
    ESP_LOGE(TAG_PARSER, "message_id = %d", message_id);

    int id = cJSON_GetObjectItem(from_object, "id")->valueint;
    int is_bot = cJSON_GetObjectItem(from_object, "is_bot")->valueint;
    char *first_name = cJSON_GetObjectItem(from_object, "first_name")->valuestring;
    char *last_name = cJSON_GetObjectItem(from_object, "last_name")->valuestring;
    char *username = cJSON_GetObjectItem(from_object, "username")->valuestring;
    char *language_code = cJSON_GetObjectItem(from_object, "language_code")->valuestring;
    int date = cJSON_GetObjectItem(message_object, "date")->valueint;
    ESP_LOGE(TAG_PARSER, "id = %d", id);
    ESP_LOGE(TAG_PARSER, "is_bot = %d", is_bot);
    ESP_LOGE(TAG_PARSER, "first_name = %s", first_name);
    ESP_LOGE(TAG_PARSER, "last_name = %s", last_name);
    ESP_LOGE(TAG_PARSER, "username = %s", username);
    ESP_LOGE(TAG_PARSER, "language_code = %s", language_code);
    ESP_LOGE(TAG_PARSER, "date(sec) = %d", date);
    struct tm timeinfo;
    localtime_r(&date, &timeinfo);
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGE(TAG_PARSER, "%s", strftime_buf);

    ESP_LOGW(TAG_PARSER, "init last message");
    init_last_message();

    ESP_LOGW(TAG_PARSER, "set last message options");
    set_last_message_id(message_id);
    set_last_message_user_id(id);
    set_last_message_is_bot(is_bot);
    set_last_message_first_name(first_name);
    set_last_message_last_name(last_name);
    set_last_message_username(username);
    set_last_message_language_code(language_code);
    set_last_message_date(&strftime_buf);
    ESP_LOGW(TAG_PARSER, "Set type id message . . .");
    set_type_id_message(type_message_ID);
    ESP_LOGW(TAG_PARSER, "Init type message . . .");
    init_type_message();
    ESP_LOGW(TAG_PARSER, "Deinit type message . . .");
    // deinit_type_message();
    print_last_message_info();
    
    // text message
    // if (type_message_ID == TYPE_MESSAGE_TEXT)
    // {
    //     char *command = cJSON_Print(text_message);
    //     set_text_message_command(command);
    //     ESP_LOGW(TAG_PARSER, "command : %s", command);
    // }

    if (type_message_ID == TYPE_MESSAGE_VOICE)
    {
        char * file_id = cJSON_GetObjectItem(voice_message, "file_id")->valuestring;
        int file_size = cJSON_GetObjectItem(voice_message, "file_size")->valueint;
        ESP_LOGE(TAG_PARSER, "file_id : %s", file_id);
        ESP_LOGE(TAG_PARSER, "file_size : %d", file_size);
        set_last_message_file_id_voice_message(file_id);
        set_last_message_file_size_voice_message(file_size);
        set_state_answer(last_message->last_message_user_id);
        // text message
        // char *command = get_text_message_command();
        // if (strstr(command, "/menu"))
        // {
        //     set_chat_id_ui(last_message->last_message_user_id, get_bot_main_menu());
        //     return ESP_OK;
        // }
    }

    ESP_LOGW(TAG_PARSER, "delete last (cjson)message . . .");
    cJSON_Delete(tg_message);
    return ESP_OK;
}


    