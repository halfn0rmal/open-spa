#include "config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

bool init_nvm(void){
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if(err != ESP_OK){
        printf("Error (%s) initializing NVS!\n", esp_err_to_name(err));
        return false;
    }
    return true;
}

bool storeSetTemp(uint8_t temp){
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return false;
    }
    err = nvs_set_i32(my_handle, "setTemp", temp);
    if(err != ESP_OK){
        printf("Error (%s) setting setTemp in NVS!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }
    err = nvs_commit(my_handle);
    if(err != ESP_OK){
        printf("Error (%s) committing setTemp in NVS!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return false;
    }
    nvs_close(my_handle);
    return true;
}

uint8_t fetchSetTemp(void){
    esp_err_t err;
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
        return 0;
    }
    int32_t temp = 0;
    err = nvs_get_i32(my_handle, "setTemp", &temp);
    if(err != ESP_OK){
        printf("Error (%s) getting setTemp in NVS!\n", esp_err_to_name(err));
        nvs_close(my_handle);
        return 0;
    }
    nvs_close(my_handle);
    return (uint8_t)temp;
}
