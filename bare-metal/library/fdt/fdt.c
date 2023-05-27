#include "fdt.h"
#include <fdt/fdt_api.h>

/******************************************************************************/
/* Common Procedures */
/******************************************************************************/

/**********************************************************************/
/* Low-level functions (you probably don't need these)                */
/**********************************************************************/

/**
 * fdt_check_header - sanity check a device tree or possible device tree
 * @fdt: pointer to data which might be a flattened device tree
 *
 * fdt_check_header() checks that the given buffer contains what
 * appears to be a flattened device tree with sane information in its
 * header. Check the magic and compatible versions.
 *
 * returns:
 *     0, if the buffer appears to contain a valid device tree
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE, standard meanings, as above
 */
int fdt_check_header(const void *fdt)
{
    if (FDT_MAGIC(fdt) == FDT_VALID_MAGIC) {
        if (FDT_VERSION(fdt) < FDT_FIRST_SUPPORTED_VERSION)
            return -FDT_ERR_BADVERSION;
        if (FDT_LAST_COMP_VERSION(fdt) > FDT_LAST_SUPPORTED_VERSION)
            return -FDT_ERR_BADVERSION;
    } else {
        return -FDT_ERR_BADMAGIC;
    }

    return 0;
}

/**
 * fdt_move - move a device tree around in memory
 * @fdt: pointer to the device tree to move
 * @buf: pointer to memory where the device is to be moved
 * @bufsize: size of the memory space at buf
 *
 * fdt_move() relocates, if possible, the device tree blob located at
 * fdt to the buffer at buf of size bufsize.  The buffer may overlap
 * with the existing device tree blob at fdt.  Therefore,
 *     fdt_move(fdt, fdt, fdt_totalsize(fdt))
 * should always succeed.
 *
 * returns:
 *     0, on success
 *     -FDT_ERR_NOSPACE, bufsize is insufficient to contain the device tree
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE, standard meanings
 */
int fdt_move(const void *fdt, void *buf, int bufsize)
{
	int ret = fdt_check_header(fdt);
    if (ret) return ret;

	if (FDT_TOTALSIZE(fdt) > bufsize)
		return -FDT_ERR_NOSPACE;

	memmove(buf, fdt, FDT_TOTALSIZE(fdt));
	return 0;
}



/* Convert an offset to its Node or Property address */
const void *fdt_offset_to_ptr(const void *fdt, int offset, unsigned int len)
{
    /* watch out, int => unsigned int */
    unsigned abs_offset = offset + FDT_STRUCT(fdt);

    if (fdt_check_header(fdt) < 0)
        return NULL;

    if (((offset + len) < offset) || ((offset + len) > FDT_TOTALSIZE(fdt)))
        return NULL;

    if ((abs_offset < offset) || ((abs_offset + len) < abs_offset)
        || ((abs_offset + len) > FDT_TOTALSIZE(fdt)))
        return NULL;

    return (const char *)fdt + FDT_STRUCT(fdt) + offset;
}

/*
 * Return the tag of the 'curr_offset', fill '*next_offset' with the offset of the
 * next tag
 */
uint32_t fdt_tag_next_offset(const void *fdt, int offset, int *next_offset)
{
    uint32_t tag;
    const fdt32_t *ptag, *plen;

    const char *p;

    ptag = fdt_offset_to_ptr(fdt, offset, FDT_TAGSIZE);
    if (!ptag)      /* we reach the end during last iteration */
        return FDT_END;

    tag = fdt32_to_cpu(*ptag);
    offset += FDT_TAGSIZE;
    if (!next_offset) return tag;

    *next_offset = -FDT_ERR_BADSTRUCTURE;
    switch (tag) {
        case FDT_BEGIN_NODE:
            /* skip node name */
            do {
                p = fdt_offset_to_ptr(fdt, offset++, 1);
            } while (p && (*p != '\0'));

            if (!p) return FDT_END;
            break;

        case FDT_PROPERTY:
            plen = fdt_offset_to_ptr(fdt, offset, sizeof(*plen));
            if (!plen)  return FDT_END;

            /* skip property and its value */
            offset += sizeof(struct fdt_property) - FDT_TAGSIZE
                + fdt32_to_cpu(*plen);
            break;

        case FDT_END_NODE:
        case FDT_END:
        case FDT_NOP:
            break;

        default:    return FDT_END;
    }

    *next_offset = FDT_TAGALIGN(offset);
    return tag;
}


static inline int node_next_offset(const void *fdt, int offset)
{
    if ((offset < 0) || (offset % FDT_TAGSIZE) ||
        (fdt_tag_next_offset(fdt, offset, &offset) != FDT_BEGIN_NODE))
        return -FDT_ERR_BADOFFSET;

    return offset;
}


static inline int property_next_offset(const void *fdt, int offset)
{
    if ((offset < 0) || (offset % FDT_TAGSIZE) ||
        (fdt_tag_next_offset(fdt, offset, &offset) != FDT_PROPERTY))
        return -FDT_ERR_BADOFFSET;

    return offset;
}


/**********************************************************************/
/* Node Traversals, return the offset.                                */
/**********************************************************************/

/**
 * fdt_node_name - retrieve the name of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of the starting node
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_node_name() retrieves the name (including unit address) of the
 * device tree node at structure block offset nodeoffset.  If lenp is
 * non-NULL, the length of this name is also returned, in the integer
 * pointed to by lenp.
 *
 * returns:
 *	pointer to the node's name, on success
 *		If lenp is non-NULL, *lenp contains the length of that name (>=0)
 *	NULL, on error
 *		if lenp is non-NULL *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE, standard meanings
 */
const char *fdt_node_name(const void *fdt, int nodeoffset, int *pnamelen)
{
    const struct fdt_node_header *header = fdt_offset_to_ptr(fdt,
        nodeoffset, sizeof(*header));

    /* check node header and size */
    int err = node_next_offset(fdt, nodeoffset);
    if (err < 0) {
        if (pnamelen) *pnamelen = err;
        return NULL;
    }

    if (pnamelen)
        *pnamelen = strlen(header->name);

    return header->name;
}

/* Whether the specified string (before '@') matches the node name */
int fdt_nodename_equal(const void *fdt, int offset, const char *s, int len)
{
    /* find and skip the Node tag, in order to obtain its name */
    const char *p = fdt_offset_to_ptr(fdt, offset + FDT_TAGSIZE, len+1);

    if (!p) return 0;
    if (memcmp(p, s, len) != 0) return 0;

    /* Even if it matches the string in the specific length, we need to 
     * consider the @address as well. */
    if (p[len] == '\0')     /* all matches, no matther '@' exists or not */
        return 1;
    /* it is ok as well if only the head part before '@' matches */
    else if ((p[len] == '@') && (memchr(s, '@', len) == NULL))
        return 1;

    return 0;
}

int fdt_next_node(const void *fdt, int offset, int *pdepth)
{
    int next_offset = 0;
    uint32_t tag;

    if (offset >=0) {
        if ((next_offset = node_next_offset(fdt, offset)) < 0)
                         return next_offset;
    }

    /* go on checking whether the next tag is a node */
    do {
        offset = next_offset;
        tag = fdt_tag_next_offset(fdt, offset, &next_offset);

        switch (tag) {
            case FDT_PROPERTY:
            case FDT_NOP:
                break;

            case FDT_BEGIN_NODE:
                if (pdepth) (*pdepth)++;
                break;

            case FDT_END_NODE:
                if (pdepth && ((--(*pdepth)) < 0))
                    return next_offset;
                break;

            case FDT_END:
                if ((next_offset >= 0)
                    || ((next_offset == -FDT_ERR_TRUNCATED) && !pdepth))
                    return -FDT_ERR_NOTFOUND;
                else
                    return next_offset;
        }
    } while (tag != FDT_BEGIN_NODE);

    return offset;
}


/**
 * fdt_first_child_node() - get offset of first direct child node.
 *
 * @fdt:	FDT blob
 * @offset:	Offset of node to check
 * @return offset of first child node, or -FDT_ERR_NOTFOUND if there is none
 */
int fdt_first_child_node(const void *fdt, int offset)
{
    int depth = 0;

    offset = fdt_next_node(fdt, offset, &depth);
    if (offset < 0 || depth != 1)
        return -FDT_ERR_NOTFOUND;

    return offset;
}


/**
 * fdt_next_sibling_node() - get offset of next sibling node
 *
 * After first calling fdt_first_child_node(), call this function repeatedly to
 * get each sibling node.
 *
 * @fdt:	FDT blob
 * @offset:	Offset of previous child node
 * @return offset of next child node, or -FDT_ERR_NOTFOUND if there are no more
 * subnodes
 */
int fdt_next_sibling_node(const void *fdt, int offset)
{
    /* We have already met an Node start, but not the Node end yet. */
    int depth = 1;

    do {
        offset = fdt_next_node(fdt, offset, &depth);
        if (offset < 0 || depth < 1)
            return -FDT_ERR_NOTFOUND;
    } while (depth > 1);

    return offset;
}


/**********************************************************************/
/* Node advanced features                                         */
/**********************************************************************/

/**
 * fdt_get_path - determine the full path of a node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose path to find
 * @buf: character buffer to contain the returned path (will be overwritten)
 * @buflen: size of the character buffer at buf
 *
 * fdt_get_path() computes the full path of the node at offset
 * nodeoffset, and records that path in the buffer at buf.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:
 *	0, on success
 *		buf contains the absolute path of the node at
 *		nodeoffset, as a NUL-terminated string.
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_NOSPACE, the path of the given node is longer than (bufsize-1)
 *		characters and will not fit in the given buffer.
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_get_path(const void *fdt, int nodeoffset, char *buf, int buflen);

/**
 * fdt_supernode_atdepth_offset - find a specific ancestor of a node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 * @supernodedepth: depth of the ancestor to find
 * @nodedepth: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_supernode_atdepth_offset() finds an ancestor of the given node
 * at a specific depth from the root (where the root itself has depth
 * 0, its immediate subnodes depth 1 and so forth).  So
 *	fdt_supernode_atdepth_offset(fdt, nodeoffset, 0, NULL);
 * will always return 0, the offset of the root node.  If the node at
 * nodeoffset has depth D, then:
 *	fdt_supernode_atdepth_offset(fdt, nodeoffset, D, NULL);
 * will return nodeoffset itself.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:

 *	structure block offset of the node at node offset's ancestor
 *		of depth supernodedepth (>=0), on success
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
*	-FDT_ERR_NOTFOUND, supernodedepth was greater than the depth of nodeoffset
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_supernode_atdepth_offset(const void *fdt, int nodeoffset,
				 int supernodedepth, int *nodedepth);

/**
 * fdt_node_depth - find the depth of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 *
 * fdt_node_depth() finds the depth of a given node.  The root node
 * has depth 0, its immediate subnodes depth 1 and so forth.
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset.
 *
 * returns:
 *	depth of the node at nodeoffset (>=0), on success
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_depth(const void *fdt, int nodeoffset);

/**
 * fdt_parent_offset - find the parent of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose parent to find
 *
 * fdt_parent_offset() locates the parent node of a given node (that
 * is, it finds the offset of the node which contains the node at
 * nodeoffset as a subnode).
 *
 * NOTE: This function is expensive, as it must scan the device tree
 * structure from the start to nodeoffset, *twice*.
 *
 * returns:
 *	structure block offset of the parent of the node at nodeoffset
 *		(>=0), on success
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_parent_offset(const void *fdt, int nodeoffset);


/**********************************************************************/
/* Property Traversal: return the offset.                                */
/**********************************************************************/

/**
 * fdt_property_entry - retrieve the property at a given offset
 * @fdt: pointer to the device tree blob
 * @offset: offset of the property to retrieve
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_property_entry() retrieves a pointer to the
 * fdt_property structure within the device tree blob at the given
 * offset.  If lenp is non-NULL, the length of the property value is
 * also returned, in the integer pointed to by lenp.
 *
 * returns:
 *	pointer to the structure representing the property
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_PROP tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const struct fdt_property *fdt_property_entry(const void *fdt, int offset, int *pvaluelen)
{
    int err;
    const struct fdt_property *entry;

    if ((err = property_next_offset(fdt, offset)) < 0) {
        if (pvaluelen) *pvaluelen = err;
        return NULL;
    }

    entry = (const struct fdt_property *)((const char *)fdt + FDT_STRUCT(fdt) + offset);

    if (pvaluelen) *pvaluelen = fdt32_to_cpu(entry->len);
    return entry;
}

static int lookup_valid_property(const void *fdt, int offset)
{
    uint32_t tag;
    int next_offset;

    do {
        if ((tag = fdt_tag_next_offset(fdt, offset, &next_offset)) < 0)
            return tag;

        if (tag == FDT_PROPERTY) {
            return offset;
        } else
            /* tag is FDT_NOP, skip it */
            offset = next_offset;
    } while (tag == FDT_NOP);


    if (tag == FDT_END) {
        if (offset > 0)
            return -FDT_ERR_BADSTRUCTURE;
        else
            return offset;
    } else {
        /* tag is FDT_NODE_{START,END} */
        return -FDT_ERR_NOTFOUND;
    }
}


/**
 * fdt_property_value - retrieve the value of a property at a given offset
 * @fdt: pointer to the device tree blob
 * @ffset: offset of the property to read
 * @namep: pointer to a string variable (will be overwritten) or NULL
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_property_value() retrieves a pointer to the value of the
 * property at structure block offset 'offset' (this will be a pointer
 * to within the device blob itself, not a copy of the value).  If
 * lenp is non-NULL, the length of the property value is also
 * returned, in the integer pointed to by lenp.  If namep is non-NULL,
 * the property's namne will also be returned in the char * pointed to
 * by namep (this will be a pointer to within the device tree's string
 * block, not a new copy of the name).
 *
 * returns:
 *	pointer to the property's value
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *		if namep is non-NULL *namep contiains a pointer to the property
 *		name.
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_PROP tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const void *fdt_property_value(const void *fdt, int offset,
				  const char **pname, int *pvaluelen)
{
	const struct fdt_property *prop;

	prop = fdt_property_entry(fdt, offset, pvaluelen);
	if (!prop)
		return NULL;
	if (pname)
		*pname = fdt_string(fdt, fdt32_to_cpu(prop->nameoff));
	return prop->value;
}


/**
 * fdt_first_property - find the offset of a node's first property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of a node
 *
 * fdt_first_property() finds the first property of the node at
 * the given structure block offset.
 *
 * returns:
 *	structure block offset of the property (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the requested node has no properties
 *	-FDT_ERR_BADOFFSET, if nodeoffset did not point to an FDT_BEGIN_NODE tag
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_first_property(const void *fdt, int node_offset)
{
    int offset;

    if ((offset = node_next_offset(fdt, node_offset)) < 0)
        return offset;
    
    return lookup_valid_property(fdt, offset);
}

/**
 * fdt_next_property - step through a node's properties
 * @fdt: pointer to the device tree blob
 * @offset: structure block offset of a property
 *
 * fdt_next_property() finds the property immediately after the
 * one at the given structure block offset.  This will be a property
 * of the same node as the given property.
 *
 * returns:
 *	structure block offset of the next property (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the given property is the last in its node
 *	-FDT_ERR_BADOFFSET, if nodeoffset did not point to an FDT_PROP tag
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_next_property(const void *fdt, int offset)
{
    if ((offset = property_next_offset(fdt, offset)) < 0)
        return offset;

    return lookup_valid_property(fdt, offset);
}


/**
 * fdt_stringlist_contains - check a string list property for a string
 * @strlist: Property containing a list of strings to check
 * @listlen: Length of property
 * @str: String to search for
 *
 * This is a utility function provided for convenience. The list contains
 * one or more strings, each terminated by \0, as is found in a device tree
 * "compatible" property.
 *
 * @return: 1 if the string is found in the list, 0 not found, or invalid list
 */
int fdt_stringlist_contains(const char *strlist, int listlen, const char *str)
{
	int len = strlen(str);
	const char *p;

	while (listlen >= len) {
		if (memcmp(str, strlist, len+1) == 0)
			return 1;
		p = memchr(strlist, '\0', listlen);
		if (!p)
			return 0; /* malformed strlist.. */
		listlen -= (p-strlist) + 1;
		strlist = p + 1;
	}
	return 0;
}


/**
 * fdt_stringlist_count - count the number of strings in a string list
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @property: name of the property containing the string list
 * @return:
 *   the number of strings in the given property
 *   -FDT_ERR_BADVALUE if the property value is not NUL-terminated
 *   -FDT_ERR_NOTFOUND if the property does not exist
 */
int fdt_stringlist_count(const void *fdt, int nodeoffset, const char *property)
{
	const char *list, *end;
	int length, count = 0;

	list = fdt_lookup_property_value_by_name(fdt, nodeoffset, property, &length);
	if (!list)
		return -length;

	end = list + length;

	while (list < end) {
		length = strnlen(list, end - list) + 1;

		/* Abort if the last string isn't properly NUL-terminated. */
		if (list + length > end)
			return -FDT_ERR_BADVALUE;

		list += length;
		count++;
	}

	return count;
}


/**
 * fdt_stringlist_search - find a string in a string list and return its index
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @property: name of the property containing the string list
 * @string: string to look up in the string list
 *
 * Note that it is possible for this function to succeed on property values
 * that are not NUL-terminated. That's because the function will stop after
 * finding the first occurrence of @string. This can for example happen with
 * small-valued cell properties, such as #address-cells, when searching for
 * the empty string.
 *
 * @return:
 *   the index of the string in the list of strings
 *   -FDT_ERR_BADVALUE if the property value is not NUL-terminated
 *   -FDT_ERR_NOTFOUND if the property does not exist or does not contain
 *                     the given string
 */
int fdt_stringlist_search(const void *fdt, int nodeoffset, const char *property,
			  const char *string)
{
	int length, len, idx = 0;
	const char *list, *end;

	list = fdt_lookup_property_value_by_name(fdt, nodeoffset, property, &length);
	if (!list)
		return -length;

	len = strlen(string) + 1;
	end = list + length;

	while (list < end) {
		length = strnlen(list, end - list) + 1;

		/* Abort if the last string isn't properly NUL-terminated. */
		if (list + length > end)
			return -FDT_ERR_BADVALUE;

		if (length == len && memcmp(list, string, length) == 0)
			return idx;

		list += length;
		idx++;
	}

	return -FDT_ERR_NOTFOUND;
}


/**
 * fdt_stringlist_get() - obtain the string at a given index in a string list
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @property: name of the property containing the string list
 * @index: index of the string to return
 * @lenp: return location for the string length or an error code on failure
 *
 * Note that this will successfully extract strings from properties with
 * non-NUL-terminated values. For example on small-valued cell properties
 * this function will return the empty string.
 *
 * If non-NULL, the length of the string (on success) or a negative error-code
 * (on failure) will be stored in the integer pointer to by lenp.
 *
 * @return:
 *   A pointer to the string at the given index in the string list or NULL on
 *   failure. On success the length of the string will be stored in the memory
 *   location pointed to by the lenp parameter, if non-NULL. On failure one of
 *   the following negative error codes will be returned in the lenp parameter
 *   (if non-NULL):
 *     -FDT_ERR_BADVALUE if the property value is not NUL-terminated
 *     -FDT_ERR_NOTFOUND if the property does not exist
 */
const char *fdt_stringlist_get(const void *fdt, int nodeoffset,
			       const char *property, int idx,
			       int *lenp)
{
	const char *list, *end;
	int length;

	list = fdt_lookup_property_value_by_name(fdt, nodeoffset, property, &length);
	if (!list) {
		if (lenp)
			*lenp = length;

		return NULL;
	}

	end = list + length;

	while (list < end) {
		length = strnlen(list, end - list) + 1;

		/* Abort if the last string isn't properly NUL-terminated. */
		if (list + length > end) {
			if (lenp)
				*lenp = -FDT_ERR_BADVALUE;

			return NULL;
		}

		if (idx == 0) {
			if (lenp)
				*lenp = length - 1;

			return list;
		}

		list += length;
		idx--;
	}

	if (lenp)
		*lenp = -FDT_ERR_NOTFOUND;

	return NULL;
}

/**********************************************************************/
/* String Block                                                       */
/**********************************************************************/

/**
 * fdt_string - retrieve a string from the strings block of a device tree
 * @fdt: pointer to the device tree blob
 * @stroffset: offset of the string within the strings block (native endian)
 *
 * fdt_string() retrieves a pointer to a single string from the
 * strings block of the device tree blob at fdt.
 *
 * returns:
 *     a pointer to the string, on success
 *     NULL, if stroffset is out of bounds
 */

const char *fdt_string(const void *fdt, int stroffset)
{
	return (const char *)fdt + FDT_STRING(fdt) + stroffset;
}



/**********************************************************************/
/* Reserved-Memory Regions                                            */
/**********************************************************************/

/**
 * fdt_num_mem_rsv - retrieve the number of memory reserve map entries
 * @fdt: pointer to the device tree blob
 *
 * Returns the number of entries in the device tree blob's memory
 * reservation map.  This does not include the terminating 0,0 entry
 * or any other (0,0) entries reserved for expansion.
 *
 * returns:
 *     the number of entries
 */
int fdt_rsvmem_entry(const void *fdt, int n, uint64_t *address, uint64_t *size)
{
    if (fdt_check_header(fdt) < 0)
        return -FDT_ERR_BADMAGIC;

	*address = fdt64_to_cpu(FDT_RSVMEM_ENTRY(fdt, n)->address);
	*size = fdt64_to_cpu(FDT_RSVMEM_ENTRY(fdt, n)->size);
	return 0;
}


/**
 * fdt_get_mem_rsv - retrieve one memory reserve map entry
 * @fdt: pointer to the device tree blob
 * @address, @size: pointers to 64-bit variables
 *
 * On success, *address and *size will contain the address and size of
 * the n-th reserve map entry from the device tree blob, in
 * native-endian format.
 *
 * returns:
 *     0, on success
 *     -FDT_ERR_BADMAGIC,
 *     -FDT_ERR_BADVERSION,
 *     -FDT_ERR_BADSTATE, standard meanings
 */
int fdt_num_mem_rsv(const void *fdt)
{
	int i = 0;

	while (fdt64_to_cpu(FDT_RSVMEM_ENTRY(fdt, i)->size) != 0)
		i++;
	return i;
}
