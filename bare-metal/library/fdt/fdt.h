#ifndef __FDT_H
#define __FDT_H

#include "fdt_env.h"

struct fdt_header {
	fdt32_t magic;			 /* magic word FDT_MAGIC */
	fdt32_t totalsize;		 /* total size of DT block */
	fdt32_t off_dt_struct;		 /* offset to structure */
	fdt32_t off_dt_strings;		 /* offset to strings */
	fdt32_t off_mem_rsvmap;		 /* offset to memory reserve map */
	fdt32_t version;		 /* format version */
	fdt32_t last_comp_version;	 /* last compatible version */

	/* version 2 fields below */
	fdt32_t boot_cpuid_phys;	 /* Which physical CPU id we're
					    booting on */
	/* version 3 fields below */
	fdt32_t size_dt_strings;	 /* size of the strings block */

	/* version 17 fields below */
	fdt32_t size_dt_struct;		 /* size of the structure block */
};

#define FDT_ALIGN(x, a)		(((x) + (a) - 1) & ~((a) - 1))
#define FDT_TAGALIGN(x)		(FDT_ALIGN((x), FDT_TAGSIZE))

/* APIs don't expose the structure of fdt and void pointer is used.
 * 
 * The following macros are provided to help access fdt's members.
 */
#define fetch_fdt_field(p, field)  \
    (fdt32_to_cpu(((const struct fdt_header *)(p))->field))

#define FDT_MAGIC(fdt)      fetch_fdt_field(fdt, magic)
#define FDT_VERSION(fdt)    fetch_fdt_field(fdt, version)
#define FDT_LAST_COMP_VERSION(fdt)  fetch_fdt_field(fdt, last_comp_version)

#define FDT_TOTALSIZE(fdt)  fetch_fdt_field(fdt, totalsize)
#define FDT_STRUCT(fdt)     fetch_fdt_field(fdt, off_dt_struct)
#define FDT_STRING(fdt)     fetch_fdt_field(fdt, off_dt_strings)
#define FDT_RSVMEM(fdt)     fetch_fdt_field(fdt, off_mem_rsvmap)

#define FDT_RSVMEM_ENTRY(fdt, n)    \
    ((const struct fdt_reserve_entry *)((const char *)fdt + FDT_RSVMEM(fdt)) + n)

//#undef fetch_fdt_field

struct fdt_reserve_entry {
	fdt64_t address;
	fdt64_t size;
};

struct fdt_node_header {
	fdt32_t tag;
	char name[0];
};

struct fdt_property {
	fdt32_t tag;
	fdt32_t len;
	fdt32_t nameoff;
	char value[0];
};


#define FDT_VALID_MAGIC	0xd00dfeed	/* 4: version, 4: total size */
#define FDT_TAGSIZE	    sizeof(fdt32_t)

#define FDT_BEGIN_NODE	0x1		/* Start node: full name */
#define FDT_END_NODE	0x2		/* End node */
#define FDT_PROPERTY	0x3		/* Property: name off, size, content */

#define FDT_NOP		0x4		/* nop */
#define FDT_END		0x9

#define FDT_V1_SIZE	(7*sizeof(fdt32_t))
#define FDT_V2_SIZE	(FDT_V1_SIZE + sizeof(fdt32_t))
#define FDT_V3_SIZE	(FDT_V2_SIZE + sizeof(fdt32_t))
#define FDT_V16_SIZE	FDT_V3_SIZE
#define FDT_V17_SIZE	(FDT_V16_SIZE + sizeof(fdt32_t))
#endif  /* __FDT_H */