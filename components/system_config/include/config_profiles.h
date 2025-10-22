/**
 * @file config_profiles.h
 * @brief Material profile presets configuration
 *
 * This header defines all preset profiles for different materials
 * (Cotton, Polyester, Blockout, Wood, Metal) with their optimal
 * temperature and timing settings.
 *
 * @author Insta Retrofit Development Team
 * @date 2025
 */

#ifndef CONFIG_PROFILES_H
#define CONFIG_PROFILES_H

#include <stdint.h>

// =============================================================================
// Profile Type Enumeration
// =============================================================================

/**
 * @brief Material profile types
 */
typedef enum {
    PROFILE_COTTON = 0,      ///< Cotton fabric profile
    PROFILE_POLYESTER = 1,   ///< Polyester fabric profile
    PROFILE_BLOCKOUT = 2,    ///< Blockout vinyl profile
    PROFILE_WOOD = 3,        ///< Wood substrate profile
    PROFILE_METAL = 4,       ///< Metal substrate profile
    PROFILE_COUNT            ///< Total number of profiles
} material_profile_type_t;

// =============================================================================
// Profile Configuration Structure
// =============================================================================

/**
 * @brief Material profile configuration
 *
 * Contains temperature and timing parameters for a specific material type.
 */
typedef struct {
    const char *name;              ///< Profile name (e.g., "Cotton")
    float target_temp_celsius;     ///< Target pressing temperature (°C)
    uint16_t stage1_duration_sec;  ///< Stage 1 duration (seconds)
    uint16_t stage2_duration_sec;  ///< Stage 2 duration (seconds)
    const char *description;       ///< Brief description of profile
} material_profile_t;

// =============================================================================
// Profile Presets
// =============================================================================

/**
 * @brief Predefined material profiles
 *
 * Array of preset profiles for different materials. These values are
 * optimized for the Insta 221 heat press.
 *
 * Usage:
 * @code
 * material_profile_t cotton = MATERIAL_PROFILES[PROFILE_COTTON];
 * settings.target_temp = cotton.target_temp_celsius;
 * @endcode
 */
static const material_profile_t MATERIAL_PROFILES[PROFILE_COUNT] = {
    [PROFILE_COTTON] = {
        .name = "Cotton",
        .target_temp_celsius = 140.0f,
        .stage1_duration_sec = 15,
        .stage2_duration_sec = 5,
        .description = "Cotton t-shirts and fabric"
    },
    [PROFILE_POLYESTER] = {
        .name = "Polyester",
        .target_temp_celsius = 125.0f,
        .stage1_duration_sec = 12,
        .stage2_duration_sec = 5,
        .description = "Polyester and poly-blend fabrics"
    },
    [PROFILE_BLOCKOUT] = {
        .name = "Blockout",
        .target_temp_celsius = 125.0f,
        .stage1_duration_sec = 12,
        .stage2_duration_sec = 5,
        .description = "Blockout vinyl and banner material"
    },
    [PROFILE_WOOD] = {
        .name = "Wood",
        .target_temp_celsius = 170.0f,
        .stage1_duration_sec = 20,
        .stage2_duration_sec = 5,
        .description = "Wood panels and substrates"
    },
    [PROFILE_METAL] = {
        .name = "Metal",
        .target_temp_celsius = 204.0f,
        .stage1_duration_sec = 80,
        .stage2_duration_sec = 5,
        .description = "Metal plates and hard substrates"
    }
};

// =============================================================================
// Default Profile Configuration
// =============================================================================

/**
 * @brief Default profile on system startup
 */
#define DEFAULT_PROFILE PROFILE_COTTON

/**
 * @brief Default profile settings (Cotton)
 */
#define DEFAULT_TEMP_CELSIUS      140.0f
#define DEFAULT_STAGE1_DURATION   15
#define DEFAULT_STAGE2_DURATION   5

// =============================================================================
// Default PID Parameters
// =============================================================================

/**
 * @brief Default PID controller parameters
 *
 * These conservative values are optimized for 2200W/230V heat press
 * with high thermal mass. Use auto-tune for optimal tuning to your
 * specific heat press characteristics.
 */
#define DEFAULT_PID_KP  3.5f    ///< Proportional gain - higher for better error response
#define DEFAULT_PID_KI  0.05f   ///< Integral gain - lower to prevent overshoot
#define DEFAULT_PID_KD  1.2f    ///< Derivative gain - higher to dampen oscillations

// =============================================================================
// Profile Validation Ranges
// =============================================================================

/**
 * @brief Valid temperature range for all profiles
 */
#define PROFILE_TEMP_MIN_CELSIUS  20.0f
#define PROFILE_TEMP_MAX_CELSIUS  250.0f

/**
 * @brief Valid stage duration range for all profiles
 */
#define PROFILE_STAGE_MIN_SECONDS  1
#define PROFILE_STAGE_MAX_SECONDS  300

// =============================================================================
// Helper Functions
// =============================================================================

/**
 * @brief Get profile by type
 *
 * @param profile_type Profile type enum value
 * @return Pointer to profile configuration, or NULL if invalid
 */
static inline const material_profile_t* get_profile(material_profile_type_t profile_type)
{
    if (profile_type >= PROFILE_COUNT) {
        return NULL;
    }
    return &MATERIAL_PROFILES[profile_type];
}

/**
 * @brief Validate profile parameters
 *
 * @param profile Profile to validate
 * @return true if profile is valid, false otherwise
 */
static inline bool validate_profile(const material_profile_t *profile)
{
    if (profile == NULL) {
        return false;
    }

    if (profile->target_temp_celsius < PROFILE_TEMP_MIN_CELSIUS ||
        profile->target_temp_celsius > PROFILE_TEMP_MAX_CELSIUS) {
        return false;
    }

    if (profile->stage1_duration_sec < PROFILE_STAGE_MIN_SECONDS ||
        profile->stage1_duration_sec > PROFILE_STAGE_MAX_SECONDS) {
        return false;
    }

    if (profile->stage2_duration_sec < PROFILE_STAGE_MIN_SECONDS ||
        profile->stage2_duration_sec > PROFILE_STAGE_MAX_SECONDS) {
        return false;
    }

    return true;
}

#endif // CONFIG_PROFILES_H
