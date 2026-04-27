/*
 * Custom MCUboot image selection via button press.
 *
 * At boot, a 5 second window opens:
 *   BTN1 (sw0) → app0
 *   BTN2 (sw1) → app1
 *
 * If no button is pressed within the window, app0 boots automatically.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/storage/flash_map.h>

#include "bootutil/boot_hooks.h"
#include "bootutil/bootutil.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/image.h"
#include "bootutil/fault_injection_hardening.h"

#define IMAGE_APP0    0
#define IMAGE_APP1    1
#define BTN_TIMEOUT_MS 5000

static const struct gpio_dt_spec btn1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static const struct gpio_dt_spec btn2 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);

#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>

/* Temp: erase external flash secondaries once */
static void erase_ext_secondaries(void)
{
  const struct flash_area *fa;

  if (flash_area_open(FIXED_PARTITION_ID(mcuboot_secondary), &fa) == 0) {
    flash_area_erase(fa, 0, fa->fa_size);
    flash_area_close(fa);
  }
  if (flash_area_open(FIXED_PARTITION_ID(mcuboot_secondary_1), &fa) == 0) {
    flash_area_erase(fa, 0, fa->fa_size);
    flash_area_close(fa);
  }
}

fih_ret boot_go_hook(struct boot_rsp *rsp)
{
  FIH_DECLARE(fih_rc, FIH_FAILURE);
  int     image_index = IMAGE_APP0;
  int64_t deadline;
  int     prev_sec = -1;

  /*
   * Check for a pending upgrade on either image before entering the
   * button selection window. If a swap is scheduled (TEST or PERM),
   * boot that image so the FOTA reboot actually installs
   * the new firmware.
   */
  for (int i = 0; i < 2; i++) {
    int swap = boot_swap_type_multi(i);
    if (swap == BOOT_SWAP_TYPE_TEST || swap == BOOT_SWAP_TYPE_PERM ||
        swap == BOOT_SWAP_TYPE_REVERT) {
      printk("boot_go_hook: pending swap (type %d) on image %d, booting it\n",
             swap, i);
      image_index = i;
      goto boot;
    }
  }

  if (!gpio_is_ready_dt(&btn1) || !gpio_is_ready_dt(&btn2)) {
    printk("boot_go_hook: GPIO not ready, defaulting to app0\n");
    goto boot;
  }

  gpio_pin_configure_dt(&btn1, GPIO_INPUT);
  gpio_pin_configure_dt(&btn2, GPIO_INPUT);

  printk("boot_go_hook: BTN1=app0 (default)  BTN2=app1\n");
  printk("boot_go_hook: waiting %d s for button press...\n",
         BTN_TIMEOUT_MS / 1000);

  deadline = k_uptime_get() + BTN_TIMEOUT_MS;
  while (k_uptime_get() < deadline) {
    int sec_left = (int)((deadline - k_uptime_get()) / 1000);

    if (sec_left != prev_sec) {
      printk("  %d s\n", sec_left);
      prev_sec = sec_left;
    }

    if (gpio_pin_get_dt(&btn1) > 0) {
      image_index = IMAGE_APP0;
      printk("boot_go_hook: BTN1 pressed -> app0\n");
      goto boot;
    }

    if (gpio_pin_get_dt(&btn2) > 0) {
      image_index = IMAGE_APP1;
      printk("boot_go_hook: BTN2 pressed -> app1\n");
      goto boot;
    }

    k_msleep(10);
  }

  printk("boot_go_hook: timeout -> app0 (default)\n");

boot:
  printk("boot_go_hook: booting image %d\n", image_index);

  FIH_CALL(boot_go_for_image_id, fih_rc, rsp, (uint32_t)image_index);

  if (FIH_NOT_EQ(fih_rc, FIH_SUCCESS)) {
    printk("boot_go_hook: boot_go_for_image_id failed\n");
    FIH_RET(FIH_FAILURE);
  }

  printk("boot_go_hook: jumping to image at off 0x%x\n",
         (unsigned int)rsp->br_image_off);

  FIH_RET(FIH_SUCCESS);
}

/*
 * Default hook stubs.
 * When CONFIG_BOOT_IMAGE_ACCESS_HOOKS=y, all hooks must be defined.
 * Returning BOOT_HOOK_REGULAR (or FIH equivalent) falls back to default
 * behavior.
 */

int boot_read_image_header_hook(int img_index, int slot,
                                struct image_header *img_head)
{
  return BOOT_HOOK_REGULAR;
}

fih_ret boot_image_check_hook(int img_index, int slot)
{
  FIH_RET(FIH_BOOT_HOOK_REGULAR);
}

int boot_perform_update_hook(int img_index, struct image_header *img_head,
                             const struct flash_area *area)
{
  return BOOT_HOOK_REGULAR;
}

int boot_copy_region_post_hook(int img_index, const struct flash_area *area,
                               size_t size)
{
  return 0;
}

int boot_read_swap_state_primary_slot_hook(int image_index,
                                           struct boot_swap_state *state)
{
  return BOOT_HOOK_REGULAR;
}

int boot_serial_uploaded_hook(int img_index, const struct flash_area *area,
                              size_t size)
{
  return 0;
}

int boot_reset_request_hook(bool force)
{
  return 0;
}
