idf_component_register(SRCS "test_sensor.c" "test_display.c" "test_controls.c" "test_heating.c" "test_storage.c"
                       "test_temp_regulation.c" "test_pressing_cycle.c" "test_menu_navigation.c" "test_settings_persistence.c"
                       "unit/test_validation.c" "unit/test_performance.c"
                       INCLUDE_DIRS "." "unit"
                       REQUIRES unity main)
