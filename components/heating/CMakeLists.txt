idf_component_register(
    SRCS "heating.c"
    INCLUDE_DIRS "include" "../../main/pid"
    REQUIRES driver esp_timer controls sensors system_config
)
