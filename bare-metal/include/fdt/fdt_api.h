#ifndef _FDT_API_H
#define _FDT_API_H

#include "../library/fdt/fdt.h"

/* Current version is 17, which is compatible with version 16. */
#define FDT_FIRST_SUPPORTED_VERSION	0x10
#define FDT_LAST_SUPPORTED_VERSION	0x11

/******************************************************************************/
/*                          Error Codes                                       */
/******************************************************************************/
/* Error codes: informative error codes */
#define FDT_ERR_NOTFOUND	1
	/* FDT_ERR_NOTFOUND: The requested node or property does not exist */
#define FDT_ERR_EXISTS		2
	/* FDT_ERR_EXISTS: Attemped to create a node or property which
	 * already exists */
#define FDT_ERR_NOSPACE		3
	/* FDT_ERR_NOSPACE: Operation needed to expand the device
	 * tree, but its buffer did not have sufficient space to
	 * contain the expanded tree. Use fdt_open_into() to move the
	 * device tree to a buffer with more space. */

/* Error codes: codes for bad parameters */
#define FDT_ERR_BADOFFSET	4
	/* FDT_ERR_BADOFFSET: Function was passed a structure block
	 * offset which is out-of-bounds, or which points to an
	 * unsuitable part of the structure for the operation. */
#define FDT_ERR_BADPATH		5
	/* FDT_ERR_BADPATH: Function was passed a badly formatted path
	 * (e.g. missing a leading / for a function which requires an
	 * absolute path) */
#define FDT_ERR_BADPHANDLE	6
	/* FDT_ERR_BADPHANDLE: Function was passed an invalid phandle
	 * value.  phandle values of 0 and -1 are not permitted. */
#define FDT_ERR_BADSTATE	7
	/* FDT_ERR_BADSTATE: Function was passed an incomplete device
	 * tree created by the sequential-write functions, which is
	 * not sufficiently complete for the requested operation. */

/* Error codes: codes for bad device tree blobs */
#define FDT_ERR_TRUNCATED	8
	/* FDT_ERR_TRUNCATED: Structure block of the given device tree
	 * ends without an FDT_END tag. */
#define FDT_ERR_BADMAGIC	9
	/* FDT_ERR_BADMAGIC: Given "device tree" appears not to be a
	 * device tree at all - it is missing the flattened device
	 * tree magic number. */
#define FDT_ERR_BADVERSION	10
	/* FDT_ERR_BADVERSION: Given device tree has a version which
	 * can't be handled by the requested operation.  For
	 * read-write functions, this may mean that fdt_open_into() is
	 * required to convert the tree to the expected version. */
#define FDT_ERR_BADSTRUCTURE	11
	/* FDT_ERR_BADSTRUCTURE: Given device tree has a corrupt
	 * structure block or other serious error (e.g. misnested
	 * nodes, or subnodes preceding properties). */
#define FDT_ERR_BADLAYOUT	12
	/* FDT_ERR_BADLAYOUT: For read-write functions, the given
	 * device tree has it's sub-blocks in an order that the
	 * function can't handle (memory reserve map, then structure,
	 * then strings).  Use fdt_open_into() to reorganize the tree
	 * into a form suitable for the read-write operations. */

/* "Can't happen" error indicating a bug in libfdt */
#define FDT_ERR_INTERNAL	13
	/* FDT_ERR_INTERNAL: libfdt has failed an internal assertion.
	 * Should never be returned, if it is, it indicates a bug in
	 * libfdt itself. */

/* Errors in device tree content */
#define FDT_ERR_BADNCELLS	14
	/* FDT_ERR_BADNCELLS: Device tree has a #address-cells, #size-cells
	 * or similar property with a bad format or value */

#define FDT_ERR_BADVALUE	15
	/* FDT_ERR_BADVALUE: Device tree has a property with an unexpected
	 * value. For example: a property expected to contain a string list
	 * is not NUL-terminated within the length of its value. */

#define FDT_ERR_MAX		15

/**********************************************************************/
/* Low-level functions (you probably don't need these)                */
/**********************************************************************/

int fdt_check_header(const void *fdt);

int fdt_move(const void *fdt, void *buf, int bufsize);


const void *fdt_offset_to_ptr(const void *fdt, int offset, unsigned int checklen);

/* for write, as a left-value */
#if 0
static inline void *fdt_offset_to_ptr_w(void *fdt, int offset, int checklen)
{
	return (void *)(uintptr_t)fdt_offset_ptr(fdt, offset, checklen);
}
#endif

uint32_t fdt_tag_next_offset(const void *fdt, int offset, int *next_offset);

/**********************************************************************/
/* Node Traversal: return the offset.                                */
/**********************************************************************/

const char *fdt_node_name(const void *fdt, int offset, int *pnamelen);
int fdt_nodename_equal(const void *fdt, int offset, const char *s, int len);

int fdt_next_node(const void *fdt, int offset, int *depth);
int fdt_first_child_node(const void *fdt, int offset);
int fdt_next_sibling_node(const void *fdt, int offset);

/**********************************************************************/
/* Node advanced features                                         */
/**********************************************************************/

int fdt_get_path(const void *fdt, int nodeoffset, char *buf, int buflen);
int fdt_supernode_atdepth_offset(const void *fdt, int nodeoffset,
				 int supernodedepth, int *nodedepth);
int fdt_node_depth(const void *fdt, int nodeoffset);
int fdt_parent_offset(const void *fdt, int nodeoffset);

/**********************************************************************/
/* Node Lookup: return the offset.                                */
/**********************************************************************/

int fdt_lookup_child_node_by_namelen(const void *fdt, int offset,
    const char *name, int namelen);
int fdt_lookup_child_node_by_name(const void *fdt, int offset,
	const char *name);

int fdt_lookup_node_by_pathlen(const void *fdt, const char *path, int pathlen);
int fdt_lookup_node_by_path(const void *fdt, const char *path);


int fdt_lookup_node_by_property_value(const void *fdt, int startoffset,
				  const char *propname,
				  const void *propval, int proplen);
int fdt_lookup_node_by_phandle(const void *fdt, uint32_t phandle);
int fdt_lookup_node_by_compatible(const void *fdt, int startoffset,
				  const char *compatible);
int fdt_node_check_compatible(const void *fdt, int nodeoffset,
			      const char *compatible);


/**********************************************************************/
/* Property Traversal: return the offset.                                */
/**********************************************************************/

const struct fdt_property *fdt_property_entry(const void *fdt, int offset,
						      int *pvaluelen);
const void *fdt_property_value(const void *fdt, int offset,
				  const char **pname, int *pvaluelen);


int fdt_first_property(const void *fdt, int nodeoffset);
int fdt_next_property(const void *fdt, int offset);


int fdt_stringlist_contains(const char *strlist, int listlen, const char *str);
int fdt_stringlist_count(const void *fdt, int nodeoffset, const char *property);
int fdt_stringlist_search(const void *fdt, int nodeoffset, const char *property,
			  const char *string);
const char *fdt_stringlist_get(const void *fdt, int nodeoffset,
			       const char *property, int index,
			       int *lenp);

/**********************************************************************/
/* Property Lookup: return the offset or pointer.                     */
/**********************************************************************/

const struct fdt_property *fdt_lookup_property_entry_by_namelen(const void *fdt,
						    int nodeoffset,
						    const char *name,
						    int namelen, int *pvaluelen);
const struct fdt_property *fdt_lookup_property_entry_by_name(
	const void *fdt, int nodeoffset, const char *name, int *pvaluelen);

#if 0
static inline struct fdt_property *fdt_get_property_w(void *fdt, int nodeoffset,
						      const char *name,
						      int *lenp)
{
	return (struct fdt_property *)(uintptr_t)
		fdt_lookup_property_entry_by_name(fdt, nodeoffset, name, lenp);
}
#endif


const void *fdt_lookup_property_value_by_namelen(const void *fdt, int nodeoffset,
				const char *name, int namelen, int *lenp);
const void *fdt_lookup_property_value_by_name(const void *fdt, int nodeoffset,
			const char *name, int *pvaluelen);

#if 0
static inline void *fdt_getprop_w(void *fdt, int nodeoffset,
				  const char *name, int *lenp)
{
	return (void *)(uintptr_t)fdt_lookup_property_value_by_name(fdt, nodeoffset, name, lenp);
}
#endif


const char *fdt_lookup_alias_value_by_namelen(const void *fdt,
				  const char *name, int namelen);
const char *fdt_lookup_alias_value_by_name(const void *fdt, const char *name);


uint32_t fdt_fetch_phandle(const void *fdt, int nodeoffset);
/**
 * FDT_MAX_NCELLS - maximum value for #address-cells and #size-cells
 *
 * This is the maximum value for #address-cells, #size-cells and
 * similar properties that will be processed by libfdt.  IEE1275
 * requires that OF implementations handle values up to 4.
 * Implementations may support larger values, but in practice higher
 * values aren't used.
 */
#define FDT_MAX_NCELLS		4

int fdt_address_cells(const void *fdt, int nodeoffset);
int fdt_size_cells(const void *fdt, int nodeoffset);


/**********************************************************************/
/* String Block                                                       */
/**********************************************************************/
const char *fdt_string(const void *fdt, int stroffset);

/**********************************************************************/
/* Reserved-Memory Regions                                            */
/**********************************************************************/
int fdt_num_mem_rsv(const void *fdt);
int fdt_get_mem_rsv(const void *fdt, int n, uint64_t *address, uint64_t *size);













/**********************************************************************/
/* Debugging / informational functions                                */
/**********************************************************************/

const char *fdt_strerror(int errval);

#endif /* _FDT_API_H */