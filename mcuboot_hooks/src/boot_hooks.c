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

/* PM-generated partition IDs */
#include <pm_config.h>

/* 0 = bank0, 1 = bank1 */
#define HARDCODED_BOOT_IMAGE_INDEX 0

static struct image_header selected_hdr;

fih_ret boot_go_hook(struct boot_rsp *rsp)
{
  const struct flash_area *fap = NULL;
  uint8_t partition_id;
  int rc;
  FIH_DECLARE(fih_rc, FIH_FAILURE);

  if (HARDCODED_BOOT_IMAGE_INDEX == 0) {
    partition_id = PM_MCUBOOT_PRIMARY_ID;
  } else {
    partition_id = PM_MCUBOOT_PRIMARY_1_ID;
  }

  printk("boot_go_hook: selecting image %d (partition id %u)\n",
         HARDCODED_BOOT_IMAGE_INDEX, partition_id);

  rc = flash_area_open(partition_id, &fap);
  if (rc != 0) {
    printk("boot_go_hook: flash_area_open failed: %d\n", rc);
    FIH_RET(FIH_FAILURE);
  }

  rc = boot_image_load_header(fap, &selected_hdr);
  if (rc != 0) {
    printk("boot_image_load_header: failed: %d\n", rc);
    flash_area_close(fap);
    FIH_RET(FIH_FAILURE);
  }

  /* TODO: verify the image with `bootutil_img_validate()` */

  rsp->br_flash_dev_id = flash_area_get_device_id(fap);
  rsp->br_image_off    = flash_area_get_off(fap);
  rsp->br_hdr          = &selected_hdr;

  flash_area_close(fap);

  printk("boot_go_hook: jumping to image at off 0x%x\n",
         (unsigned int)rsp->br_image_off);

  (void)fih_rc; /* suppress unused-variable warning */

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
