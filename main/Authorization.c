#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include "esp_spiffs.h"
#include "esp_err.h"
#include "esp_log.h"

#include "Authorization.h"

static const char *AUTH_TAG = "[ AUTH ]";

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

esp_err_t create_DB_users(void)
{
    cJSON *users_json = cJSON_CreateObject();
    cJSON_AddStringToObject(users_json, "azz_id", "goose");
    cJSON_AddNumberToObject(users_json, "max_admins", 1);
    cJSON_AddNumberToObject(users_json, "admins_n", 0);
    cJSON *admins_array = cJSON_CreateArray();
    cJSON_AddItemToObject(users_json, "admins", admins_array);
    cJSON_AddNumberToObject(users_json, "users_n", 0);
    cJSON *users_array = cJSON_CreateArray();
    cJSON_AddItemToObject(users_json, "users", users_array);
    const char *users_data = cJSON_Print(users_json);
    ESP_LOGW(AUTH_TAG, "users_data : %s", users_data);
    FILE *fp;
    fp = fopen(USERS_FILEPATH, "w");
    if (!fp)
    {
        ESP_LOGE(AUTH_TAG, "Failed to create DB users.json");
        return ESP_OK;
    }    
    fputs(users_data, fp);
    fclose(fp);
    cJSON_Delete(users_json);
    return ESP_OK;
    // cJSON * goose_admin = cJSON_CreateObject();
    // cJSON_AddStringToObject(goose_admin, "email", "goose@.gs");
    // cJSON_AddStringToObject(goose_admin, "password", "goose");
    // cJSON_AddItemToArray(admins_array, goose_admin);
}

esp_err_t clear_DB_users(void)
{
    create_DB_users();
    return ESP_OK;
}

esp_err_t create_user(char *email, char *password, char *azz_id)
{
    ESP_LOGI(AUTH_TAG, "Create user . . .\nemail: %s\npassword: %s\nazz_id: %s", email, password, azz_id);
    check_DB_users();

    FILE *fp;
    int file_size = fsize(USERS_FILEPATH);
    ESP_LOGW(AUTH_TAG, "file_size : %d", file_size);

    fp = fopen(USERS_FILEPATH, "r");
    ESP_LOGW(AUTH_TAG, "file read . . .");

    const char data[file_size];
    fread(data, file_size, 1, fp);
    fclose(fp);

    cJSON *_users = cJSON_Parse(data);
    const char *DB_azz_id = cJSON_GetObjectItem(_users, "azz_id")->valuestring;

    if (!strstr(azz_id, DB_azz_id))
    {
        return ESP_FAIL;
    }

    cJSON *new_user = cJSON_CreateObject();
    cJSON_AddStringToObject(new_user, "email", email);
    cJSON_AddStringToObject(new_user, "password", password);

    int max_admins = cJSON_GetObjectItem(_users, "max_admins")->valueint;
    int admins_n = cJSON_GetObjectItem(_users, "admins_n")->valueint;
    cJSON *admins = cJSON_GetObjectItem(_users, "admins");
    int users_n = cJSON_GetObjectItem(_users, "users_n")->valueint;
    cJSON *users =cJSON_GetObjectItem(_users, "users");

    if (admins_n < max_admins)
    {
        cJSON_AddItemToArray(admins, new_user);
        admins_n++;
    }
    else
    {
        cJSON_AddItemToArray(users, new_user);
        users_n++;
    }

    cJSON *users_json = cJSON_CreateObject();
    cJSON_AddStringToObject(users_json, "azz_id", DB_azz_id);
    cJSON_AddNumberToObject(users_json, "max_admins", max_admins);
    cJSON_AddNumberToObject(users_json, "admins_n", admins_n);
    cJSON_AddItemToObject(users_json, "admins", admins);
    cJSON_AddNumberToObject(users_json, "users_n", users_n);
    cJSON_AddItemToObject(users_json, "users", users);
    const char *users_data = cJSON_Print(users_json);
    ESP_LOGW(AUTH_TAG, "users_data : %s", users_data);

    fp = fopen(USERS_FILEPATH, "w");
    if (!fp)
    {
        ESP_LOGE(AUTH_TAG, "Failed to create DB users.json");
        return ESP_FAIL;
    }
    fputs(users_data, fp);
    fclose(fp);

    cJSON_Delete(_users);
    return ESP_OK;
}

esp_err_t del_user(char *email, char *password)
{
    ESP_LOGI(AUTH_TAG, "Delete user . . .\nemail: %s\npassword: %s\nazz_id: ", email, password);
    return ESP_OK;
}

esp_err_t edit_max_admins(int new_max_admins)
{
    if (new_max_admins < 1)
    {
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t check_user(char *email, char *password)
{
    FILE *fp;
    int file_size = fsize(USERS_FILEPATH);
    ESP_LOGW(AUTH_TAG, "file_size : %d", file_size);

    fp = fopen(USERS_FILEPATH, "r");
    ESP_LOGW(AUTH_TAG, "file read . . .");

    const char data[file_size];
    fread(data, file_size, 1, fp);
    fclose(fp);

    cJSON *_users = cJSON_Parse(data);
    cJSON *admins_array = cJSON_GetObjectItem(_users, "admins");
    int admins_array_size = cJSON_GetArraySize(admins_array);
    if (admins_array_size > 0)
    {
        for (size_t i = 0; i < admins_array_size; i++)
        {
            cJSON *_admin = cJSON_GetArrayItem(admins_array, i);
            char *_email = cJSON_GetObjectItem(_admin, "email")->valuestring;
            char *_pass = cJSON_GetObjectItem(_admin, "password")->valuestring;
            if (strstr(_email, email) && strstr(_pass, password))
            {
                ESP_LOGW(AUTH_TAG, "admin : %s", _email);
                // httpd_resp_sendstr(req, _email);
                cJSON_Delete(_users);
                return ESP_OK;
            }
        }
    }

    cJSON *users_array = cJSON_GetObjectItem(_users, "users");
    int users_array_size = cJSON_GetArraySize(users_array);
    if (users_array_size > 0)
    {
        for (size_t i = 0; i < users_array_size; i++)
        {
            cJSON *_user = cJSON_GetArrayItem(users_array, i);
            char *_email = cJSON_GetObjectItem(_user, "email")->valuestring;
            char *_pass = cJSON_GetObjectItem(_user, "password")->valuestring;
            if (strstr(_email, email) && strstr(_pass, password))
            {
                ESP_LOGW(AUTH_TAG, "user : %s", _email);
                // httpd_resp_sendstr(req, _email);
                cJSON_Delete(_users);
                return ESP_OK;
            }
        }
    }

    cJSON_Delete(_users);
    return ESP_FAIL;
}

esp_err_t check_DB_users(void)
{
    ESP_LOGI(AUTH_TAG, "Check DB users");
    FILE *fp;
    if ((fp = fopen(USERS_FILEPATH, "r")) == NULL)
    {
        fclose(fp);
        create_DB_users();
        return ESP_OK;
    }
    fclose(fp);
    return ESP_OK;
}

esp_err_t auth(char *buf)
{
    cJSON *root = cJSON_Parse(buf);
    const char *email = cJSON_GetObjectItem(root, "email")->valuestring;
    const char *password = cJSON_GetObjectItem(root, "password")->valuestring;
    const char *azz_id = cJSON_GetObjectItem(root, "azz_id")->valuestring;

    check_DB_users();
    

    if (azz_id && check_user(email, password) == ESP_FAIL)
    {
        if (create_user(email, password, azz_id) == ESP_FAIL)
        {
            return ESP_FAIL;
        }
    }

    ESP_LOGI(AUTH_TAG, "User . . .\nemail: %s\npassword: %s", email, password);
    cJSON_Delete(root);
    return ESP_OK;
}
