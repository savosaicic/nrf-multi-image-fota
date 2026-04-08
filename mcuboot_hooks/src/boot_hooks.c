/*
 * Custom MCUboot image selection.
 * For now, the selection is hardcoded.
 * Later, replace it with a counter read from the
 * settings_storage partition to choose which image to run.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/storage/flash_map.h>

#include "bootutil/boot_hooks.h"
#include "bootutil/bootutil.h"
#include "bootutil/bootutil_public.h"
#include "bootutil/image.h"
#include "bootutil/fault_injection_hardening.h"

/* 0 = bank0, 1 = bank1 */
#define HARDCODED_BOOT_IMAGE_INDEX 0

fih_ret boot_go_hook(struct boot_rsp *rsp)
{
  FIH_DECLARE(fih_rc, FIH_FAILURE);

  printk("boot_go_hook: selecting mcuboot image %d\n",
         HARDCODED_BOOT_IMAGE_INDEX);

  /*
   * Runs the full mcuboot pipeline
   * (state init, swap if needed, header read, full image
   * validation via bootutil_img_validate) for a single mcuboot image.
   */
  FIH_CALL(boot_go_for_image_id, fih_rc, rsp,
           (uint32_t)HARDCODED_BOOT_IMAGE_INDEX);

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
