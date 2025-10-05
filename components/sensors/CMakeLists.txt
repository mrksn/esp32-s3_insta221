idf_component_register(SRCS "sensors.c"
                       INCLUDE_DIRS "include" "../../main/utils"
                       REQUIRES driver esp_timer main)
