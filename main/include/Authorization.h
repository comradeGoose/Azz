#ifndef __AUTHORIZATION_H__
#define __AUTHORIZATION_H__

#include "esp_err.h"

#define USERS_FILEPATH "/spiffs/users.json"

typedef struct user_data{
    char *email;
    int last_action_min;
} user_data_t;

int fsize(char *file);

esp_err_t create_DB_users(void);

esp_err_t clear_DB_users(void);

esp_err_t create_user(char *email, char *password, char *azz_id);

esp_err_t del_user(char *email, char *password);

esp_err_t edit_max_admins(int new_max_admins);

esp_err_t check_user(char *email, char *password);

esp_err_t check_DB_users(void);

esp_err_t auth(char *buf);

#endif