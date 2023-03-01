#ifndef __AZZ_PARSER_H__
#define __AZZ_PARSER_H__

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include <stdarg.h>
#include "sdkconfig.h"
#include "esp_log.h"

// #define MAX_MESSAGE_LENGTH 2048

// #define MAX_ID_LENGTH 24
#define MAX_USERNAME_LENGTH 32
#define MAX_LANGUAGE_CODE_LENGTH 8
#define MAX_DATE_LANGTH 64

#define MAX_FILE_NAME_LENGTH 64
#define MAX_FILE_ID_LENGTH 128
#define MAX_FILE_PATH_LENGTH 64
#define MAX_FILE_SIZE_TG_BOT 20971520

#define MAX_TEXT_MESSAGE_LENGTH 64
#define BOT_COMMAND 16

#define TYPE_MESSAGE_TEXT 0
#define TYPE_MESSAGE_VOICE 1
#define TYPE_MESSAGE_AUDIO 2
#define TYPE_MESSAGE_ERROR -1

// #define STATE_
// #define STATE_

// #define IS_BOT_TRUE 0
// #define IS_BOT_FALSE 1

// /**
//  * @brief Audio message
//  */
// typedef struct struct_message_audio
// {
//     char file_name_audio_message[MAX_FILE_NAME_LENGTH];
//     int file_id_audio_message;
//     int file_size_audio_message;
//     char file_path_audio_message[MAX_FILE_PATH_LENGTH];
// } message_audio_t;

/**
 * @brief Voice message
 */
typedef struct struct_message_voice
{
    char file_id_voice_message[MAX_FILE_ID_LENGTH];
    int file_size_voice_message;
    char file_path_voice_message[MAX_FILE_PATH_LENGTH];
} message_voice_t;

// /**
//  * @brief Text message
//  */
// typedef struct struct_message_text
// {
//     char text_message[MAX_TEXT_MESSAGE_LENGTH];
// } message_text_t;

/**
 * @brief Last message
 */
typedef struct struct_azz_parser
{
    // last message
    // char last_message[MAX_MESSAGE_LENGTH];
    // last message options
    
    int last_message_id;
    int last_message_user_id;
    bool last_message_is_bot;
    char last_message_first_name[MAX_USERNAME_LENGTH];
    char last_message_last_name[MAX_USERNAME_LENGTH];
    char last_message_username[MAX_USERNAME_LENGTH];
    char last_message_language_code[MAX_LANGUAGE_CODE_LENGTH];
    char last_message_date[MAX_DATE_LANGTH];

    // types message

    int type_message;
    // message_text_t *last_message_text;
    message_voice_t *last_message_voice;
    // message_audio_t *last_message_audio;
} azz_parser_t;


/**
 * @brief Init azz_parser(struct last message)
 */
void init_last_message(void);

/**
 * @brief Deinit azz_parser(struct last message)
 */
void deinit_last_message(void);


/**
 * @brief Set identifier type message
 */
void set_type_id_message(int type_id);

/**
 * @brief Get identifier type message
 */
int get_type_id_message(void);

/**
 * @brief Init init_type_message(struct last message)
 */
esp_err_t init_type_message(void);

/**
 * @brief Deinit deinit_type_message(struct last message)
 */
esp_err_t deinit_type_message(void);


void set_last_message_id(int id);
int get_last_message_id(void);


void set_last_message_is_bot(int is_bot);
bool get_last_message_is_bot();


void set_last_message_user_id(int id);
int get_last_message_user_id(void);


void set_last_message_first_name(char *first_name);
char* get_last_message_first_name(void);


void set_last_message_last_name(char *last_name);
char* get_last_message_last_name(void);


void set_last_message_username(char *username);
char* get_last_message_username(void);


void set_last_message_language_code(char *language_code);
char* get_last_message_language_code(void);


void set_last_message_date(char *date);
char* get_last_message_date(void);


void print_last_message_info(void);


// void set_text_message_command(char *command);
// char* get_text_message_command(void);


// voice message
void print_last_voice_message_info(void);

void set_last_message_file_id_voice_message(char * file_id);
char * get_last_message_file_id_voice_message(void);

void set_last_message_file_size_voice_message(int file_size);
int get_last_message_file_size_voice_message(void);

void set_last_message_file_path_voice_message(char * file_path);
char * get_last_message_file_path_voice_message(void);

esp_err_t voice_message_parser(char *output_buffer);
//____________________________________________________


esp_err_t azz_parser(char *output_buffer);


char * get_bot_main_menu(void);


#endif
