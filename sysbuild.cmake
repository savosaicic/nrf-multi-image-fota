# Register bank0 as an external Zephyr image built under sysbuild.
ExternalZephyrProject_Add(
  APPLICATION bank0
  SOURCE_DIR  ${APP_DIR}/bank0
  BOARD       nrf9151dk/nrf9151/ns
)

# Add bank0 to the list sysbuild uses to propagate MCUboot image number
# configuration and to drive signing.
UpdateableImage_Add(APPLICATION bank0)

# Tell Partition Manager that bank0 is an app image. PM will assign it the
# mcuboot_primary_1 / mcuboot_secondary_1 slot pair from pm_static.yml.
set_property(GLOBAL APPEND PROPERTY PM_APP_IMAGES bank0)

# Force CONFIG_BOOTLOADER_MCUBOOT=y on bank0 build to enable image signing
# and ensure mcuboot_pad_1 header. Without this, bank0 builds an unsigned
# binary and the magic word at mcuboot_pad_1 stays 0xff, causing
# boot_go_hook to reject the image.
set_config_bool(bank0 CONFIG_BOOTLOADER_MCUBOOT y)

# Propagate the signing key path. sysbuild sets this automatically for
# DEFAULT_IMAGE only.
set_config_string(bank0 CONFIG_MCUBOOT_SIGNATURE_KEY_FILE
  "${ZEPHYR_MCUBOOT_MODULE_DIR}/root-ec-p256.pem")

# Inject the MCUboot hook module into the mcuboot build.
set(mcuboot_EXTRA_ZEPHYR_MODULES
    "${APP_DIR}/mcuboot_hooks"
    CACHE INTERNAL "Extra Zephyr modules for mcuboot image" FORCE)

# Patch bank0's pm_config.h so its TFM child build uses bank0 partition
# addresses instead of the main app's.
#
# The partition manager generates one pm_config.h per image, but all images
# in the APP domain share the same partition defines (PM_TFM_ADDRESS, etc.).
# Bank0's TFM must be linked for addresses inside mcuboot_primary_1, not
# mcuboot_primary. We fix this by appending #undef / #define overrides
# after PM has run. cmake_language(DEFER) ensures this executes at the end
# of the sysbuild CMakeLists.txt (after partition_manager.cmake).
function(_patch_bank0_pm_config)
  set(pm_cfg "${APPLICATION_BINARY_DIR}/bank0/zephyr/include/generated/pm_config.h")
  if(NOT EXISTS "${pm_cfg}")
    return()
  endif()

  file(READ "${pm_cfg}" _content)

  # Only patch once (idempotent guard)
  string(FIND "${_content}" "BANK0_PM_OVERRIDE" _already_patched)
  if(NOT _already_patched EQUAL -1)
    return()
  endif()

  # The addresses below must match pm_static_nrf9151dk_nrf9151_ns.yml
  set(_overrides [=[

/* ---------- BANK0_PM_OVERRIDE ---------- */
/* Redirect TFM partition defines to bank0's slot addresses so that the   */
/* TFM child image built inside bank0 is linked for the correct location. */

#undef PM_TFM_OFFSET
#undef PM_TFM_ADDRESS
#undef PM_TFM_END_ADDRESS
#undef PM_TFM_SIZE
#define PM_TFM_OFFSET      0x70200
#define PM_TFM_ADDRESS     0x70200
#define PM_TFM_END_ADDRESS 0x78000
#define PM_TFM_SIZE        0x7e00

#undef PM_TFM_SECURE_OFFSET
#undef PM_TFM_SECURE_ADDRESS
#undef PM_TFM_SECURE_END_ADDRESS
#undef PM_TFM_SECURE_SIZE
#define PM_TFM_SECURE_OFFSET      0x70000
#define PM_TFM_SECURE_ADDRESS     0x70000
#define PM_TFM_SECURE_END_ADDRESS 0x78000
#define PM_TFM_SECURE_SIZE        0x8000

#undef PM_TFM_NONSECURE_OFFSET
#undef PM_TFM_NONSECURE_ADDRESS
#undef PM_TFM_NONSECURE_END_ADDRESS
#undef PM_TFM_NONSECURE_SIZE
#define PM_TFM_NONSECURE_OFFSET       0x78000
#define PM_TFM_NONSECURE_ADDRESS      0x78000
#define PM_TFM_NONSECURE_END_ADDRESS  0xb4000
#define PM_TFM_NONSECURE_SIZE         0x3c000

#undef PM_APP_OFFSET
#undef PM_APP_ADDRESS
#undef PM_APP_END_ADDRESS
#undef PM_APP_SIZE
#define PM_APP_OFFSET      0x78000
#define PM_APP_ADDRESS     0x78000
#define PM_APP_END_ADDRESS 0xb4000
#define PM_APP_SIZE        0x3c000

#undef PM_MCUBOOT_PAD_OFFSET
#undef PM_MCUBOOT_PAD_ADDRESS
#undef PM_MCUBOOT_PAD_END_ADDRESS
#undef PM_MCUBOOT_PAD_SIZE
#define PM_MCUBOOT_PAD_OFFSET      0x70000
#define PM_MCUBOOT_PAD_ADDRESS     0x70000
#define PM_MCUBOOT_PAD_END_ADDRESS 0x70200
#define PM_MCUBOOT_PAD_SIZE        0x200

#undef PM_MCUBOOT_PRIMARY_OFFSET
#undef PM_MCUBOOT_PRIMARY_ADDRESS
#undef PM_MCUBOOT_PRIMARY_END_ADDRESS
#undef PM_MCUBOOT_PRIMARY_SIZE
#define PM_MCUBOOT_PRIMARY_OFFSET      0x70000
#define PM_MCUBOOT_PRIMARY_ADDRESS     0x70000
#define PM_MCUBOOT_PRIMARY_END_ADDRESS 0xb4000
#define PM_MCUBOOT_PRIMARY_SIZE        0x44000

#undef PM_MCUBOOT_PRIMARY_APP_OFFSET
#undef PM_MCUBOOT_PRIMARY_APP_ADDRESS
#undef PM_MCUBOOT_PRIMARY_APP_END_ADDRESS
#undef PM_MCUBOOT_PRIMARY_APP_SIZE
#define PM_MCUBOOT_PRIMARY_APP_OFFSET      0x70200
#define PM_MCUBOOT_PRIMARY_APP_ADDRESS     0x70200
#define PM_MCUBOOT_PRIMARY_APP_END_ADDRESS 0xb4000
#define PM_MCUBOOT_PRIMARY_APP_SIZE        0x43e00

#undef PM_APP_IMAGE_OFFSET
#undef PM_APP_IMAGE_ADDRESS
#undef PM_APP_IMAGE_END_ADDRESS
#undef PM_APP_IMAGE_SIZE
#define PM_APP_IMAGE_OFFSET       0x70200
#define PM_APP_IMAGE_ADDRESS      0x70200
#define PM_APP_IMAGE_END_ADDRESS  0xb4000
#define PM_APP_IMAGE_SIZE         0x43e00

#undef PM_MCUBOOT_SECONDARY_OFFSET
#undef PM_MCUBOOT_SECONDARY_ADDRESS
#undef PM_MCUBOOT_SECONDARY_END_ADDRESS
#undef PM_MCUBOOT_SECONDARY_SIZE
#define PM_MCUBOOT_SECONDARY_OFFSET       0x48000
#define PM_MCUBOOT_SECONDARY_ADDRESS      0x48000
#define PM_MCUBOOT_SECONDARY_END_ADDRESS  0x8c000
#define PM_MCUBOOT_SECONDARY_SIZE         0x44000

]=])

  # Insert overrides before the #endif guard
  string(REPLACE "#endif /* PM_CONFIG_H__ */"
    "${_overrides}\n#endif /* PM_CONFIG_H__ */"
    _content "${_content}")
  file(WRITE "${pm_cfg}" "${_content}")
  message(STATUS "Patched bank0 pm_config.h with bank0 TFM addresses")
endfunction()

cmake_language(DEFER DIRECTORY ${CMAKE_SOURCE_DIR} CALL _patch_bank0_pm_config)
