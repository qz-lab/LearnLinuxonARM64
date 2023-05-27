/*
 * os_entry.c
 *
 * The first C function that executes at EL1.
 */

#include <fdt/fdt_api.h>

/* DTB base address when booting as "bare-metal", defined by QEMU */
#define DTB_ADDR    0x40000000

#define CHOSEN_PROPERTY_STDOUT_NAME "stdout-path"

void os_entry(void)
{
    int offset;
    const void *fdt = (const void *)DTB_ADDR;
    const char *prop_name, *prop_value;
    int vlen;

    offset = fdt_lookup_node_by_path(fdt, "/chosen");
    offset = fdt_first_property(fdt, offset);

    while (offset > 0) {
        prop_value = fdt_property_value(fdt, offset, &prop_name, &vlen);
        offset = fdt_next_property(fdt, offset);
    }

    while(1);
}