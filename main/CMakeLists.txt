idf_component_register(
    SRCS
        "main.c"
        "ui_state.c"
        "ui_helpers.c"
        "ui_renderers.c"
        "data_model.c"
        "utils/application_state.c"
        "utils/system_config.c"
        "pid/pid_controller.c"
        "pid/pid_autotune.c"
    INCLUDE_DIRS
        "include"      # Public API headers
    PRIV_INCLUDE_DIRS
        "."            # Private implementation headers
        "utils"        # Private utility headers
        "pid"          # Private PID implementation headers
    REQUIRES
        display
        sensors
        controls
        heating
        storage
)
