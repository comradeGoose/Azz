PROJECT_NAME := restful_server

EXTRA_COMPONENT_DIRS = ./components

include $(IDF_PATH)/make/project.mk

WEB_SRC_DIR = $(shell pwd)/front/azz-bell-web
SPIFFS_IMAGE_FLASH_IN_PROJECT := 1
$(eval $(call spiffs_create_partition_image,spiffs,$(WEB_SRC_DIR)/dist))
