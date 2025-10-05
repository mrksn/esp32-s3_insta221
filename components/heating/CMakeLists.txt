idf_component_register(
    SRCS "heating.c"
    INCLUDE_DIRS "include" "../../main/pid" "../../main/utils"
    REQUIRES driver esp_timer controls sensors
)
