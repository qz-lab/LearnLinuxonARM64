#include "fdt.h"
#include <fdt/fdt_api.h>

/**********************************************************************/
/* Node Lookup: return the offset.                                */
/**********************************************************************/

/**
 * fdt_lookup_child_node_by_namelen - find a subnode based on substring
 * @fdt: pointer to the device tree blob
 * @parentoffset: structure block offset of a node
 * @name: name of the subnode to locate
 * @namelen: number of characters of name to consider
 *
 * Identical to fdt_lookup_child_node_by_name(), but only examine the first
 * namelen characters of name for matching the subnode name.  This is
 * useful for finding subnodes based on a portion of a larger string,
 * such as a full path.
 */
int fdt_lookup_child_node_by_namelen(const void *fdt, int offset,
    const char *name, int namelen)
{
    offset = fdt_first_child_node(fdt, offset);
    
    while (offset > 0) {
        if (fdt_nodename_equal(fdt, offset, name, namelen))
            return offset;

        offset = fdt_next_sibling_node(fdt, offset);
    }

    return offset;  /* error */
}

/**
 * fdt_lookup_child_node_by_name - find a subnode of a given node
 * @fdt: pointer to the device tree blob
 * @parentoffset: structure block offset of a node
 * @name: name of the subnode to locate
 *
 * fdt_lookup_child_node_by_name() finds a subnode of the node at structure block
 * offset parentoffset with the given name.  name may include a unit
 * address, in which case fdt_lookup_child_node_by_name() will find the subnode
 * with that unit address, or the unit address may be omitted, in
 * which case fdt_lookup_child_node_by_name() will find an arbitrary subnode
 * whose name excluding unit address matches the given name.
 *
 * returns:
 *	structure block offset of the requested subnode (>=0), on success
 *	-FDT_ERR_NOTFOUND, if the requested subnode does not exist
 *	-FDT_ERR_BADOFFSET, if parentoffset did not point to an FDT_BEGIN_NODE tag
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_lookup_child_node_by_name(const void *fdt, int offset, const char *name)
{
    return fdt_lookup_child_node_by_namelen(fdt, offset, name, strlen(name));
}


/**
 * fdt_lookup_node_by_pathlen - find a tree node by its full path
 * @fdt: pointer to the device tree blob
 * @path: full path of the node to locate
 *
 * fdt_lookup_node_by_pathlen() finds a node of a given path in the device tree.
 * Each path component may omit the unit address portion, but the
 * results of this are undefined if any such path component is
 * ambiguous (that is if there are multiple nodes at the relevant
 * level matching the given component, differentiated only by unit
 * address).
 *
 * returns:
 *	structure block offset of the node with the requested path (>=0), on success
 *	-FDT_ERR_BADPATH, given path does not begin with '/' or is invalid
 *	-FDT_ERR_NOTFOUND, if the requested node does not exist
 *      -FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings.
 */
int fdt_lookup_node_by_pathlen(const void *fdt, const char *path, int pathlen)
{
    const char *end = path + pathlen;
    const char *slash = path;
    const char *alias_path;
    int offset = 0;

    /* Either @path is an absolute path or alias to an absolute path. */
    if (path[0] != '/') {
        slash = memchr(path, '/', pathlen);
        if (!slash) slash = end;

        alias_path = fdt_lookup_alias_value_by_namelen(fdt, path, slash-path);
        if (!alias_path)
            return -FDT_ERR_BADPATH;

        /* iteration, figure out the alias offset at first */
        offset = fdt_lookup_node_by_pathlen(fdt, alias_path, strlen(alias_path));
        if (offset < 0)
            return offset;

        /* start from offset, lookup the remaining part of @path */
        path = slash;
    }

    while (path < end) {
        /* skip the starting '/'s */
        while (*path == '/') {
            path++;
            if (path == end)
                return offset;
        }

        slash = memchr(path, '/', end-path);
        if (!slash) slash = end;

        offset = fdt_lookup_child_node_by_namelen(fdt, offset, path, slash-path);
        if (offset < 0)
            return offset;

        path = slash;
    }

    return offset;
}

/**
 * fdt_lookup_node_by_path - find a tree node by its full path
 * @fdt: pointer to the device tree blob
 * @path: full path of the node to locate
 * @namelen: number of characters of path to consider
 *
 * Identical to fdt_lookup_node_by_path(), but only consider the first namelen
 * characters of path as the path name.
 */
int fdt_lookup_node_by_path(const void *fdt, const char *path)
{
	return fdt_lookup_node_by_pathlen(fdt, path, strlen(path));
}

/**
 * fdt_lookup_node_by_property_value - find nodes with a given property value
 * @fdt: pointer to the device tree blob
 * @startoffset: only find nodes after this offset
 * @propname: property name to check
 * @propval: property value to search for
 * @proplen: length of the value in propval
 *
 * fdt_lookup_node_by_property_value() returns the offset of the first
 * node after startoffset, which has a property named propname whose
 * value is of length proplen and has value equal to propval; or if
 * startoffset is -1, the very first such node in the tree.
 *
 * To iterate through all nodes matching the criterion, the following
 * idiom can be used:
 *	offset = fdt_lookup_node_by_property_value(fdt, -1, propname,
 *					       propval, proplen);
 *	while (offset != -FDT_ERR_NOTFOUND) {
 *		// other code here
 *		offset = fdt_lookup_node_by_property_value(fdt, offset, propname,
 *						       propval, proplen);
 *	}
 *
 * Note the -1 in the first call to the function, if 0 is used here
 * instead, the function will never locate the root node, even if it
 * matches the criterion.
 *
 * returns:
 *	structure block offset of the located node (>= 0, >startoffset),
 *		 on success
 *	-FDT_ERR_NOTFOUND, no node matching the criterion exists in the
 *		tree after startoffset
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_lookup_node_by_property_value(const void *fdt, int startoffset,
				  const char *propname, const void *propval, int proplen)
{
	int offset;
	const void *val;
	int len;

	//FDT_CHECK_HEADER(fdt);

	/* FIXME: The algorithm here is pretty horrible: we scan each
	 * property of a node in fdt_getprop(), then if that didn't
	 * find what we want, we scan over them again making our way
	 * to the next node.  Still it's the easiest to implement
	 * approach; performance can come later. */
	for (offset = fdt_next_node(fdt, startoffset, NULL);
	     offset >= 0;
	     offset = fdt_next_node(fdt, offset, NULL)) {
		val = fdt_lookup_property_value_by_namelen(fdt, offset, propname, strlen(propname), &len);
		if (val && (len == proplen)
		    && (memcmp(val, propval, len) == 0))
			return offset;
	}

	return offset; /* error from fdt_next_node() */
}


/**
 * fdt_lookup_node_by_phandle - find the node with a given phandle
 * @fdt: pointer to the device tree blob
 * @phandle: phandle value
 *
 * fdt_lookup_node_by_phandle() returns the offset of the node
 * which has the given phandle value.  If there is more than one node
 * in the tree with the given phandle (an invalid tree), results are
 * undefined.
 *
 * returns:
 *	structure block offset of the located node (>= 0), on success
 *	-FDT_ERR_NOTFOUND, no node with that phandle exists
 *	-FDT_ERR_BADPHANDLE, given phandle value was invalid (0 or -1)
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_lookup_node_by_phandle(const void *fdt, uint32_t phandle)
{
	int offset;

	if ((phandle == 0) || (phandle == -1))
		return -FDT_ERR_BADPHANDLE;

	//FDT_CHECK_HEADER(fdt);

	/* FIXME: The algorithm here is pretty horrible: we
	 * potentially scan each property of a node in
	 * fdt_get_phandle(), then if that didn't find what
	 * we want, we scan over them again making our way to the next
	 * node.  Still it's the easiest to implement approach;
	 * performance can come later. */
	for (offset = fdt_next_node(fdt, -1, NULL);
	     offset >= 0;
	     offset = fdt_next_node(fdt, offset, NULL)) {
		if (fdt_fetch_phandle(fdt, offset) == phandle)
			return offset;
	}

	return offset; /* error from fdt_next_node() */
}


/**
 * fdt_lookup_node_by_compatible - find nodes with a given 'compatible' value
 * @fdt: pointer to the device tree blob
 * @startoffset: only find nodes after this offset
 * @compatible: 'compatible' string to match against
 *
 * fdt_lookup_node_by_compatible() returns the offset of the first
 * node after startoffset, which has a 'compatible' property which
 * lists the given compatible string; or if startoffset is -1, the
 * very first such node in the tree.
 *
 * To iterate through all nodes matching the criterion, the following
 * idiom can be used:
 *	offset = fdt_lookup_node_by_compatible(fdt, -1, compatible);
 *	while (offset != -FDT_ERR_NOTFOUND) {
 *		// other code here
 *		offset = fdt_lookup_node_by_compatible(fdt, offset, compatible);
 *	}
 *
 * Note the -1 in the first call to the function, if 0 is used here
 * instead, the function will never locate the root node, even if it
 * matches the criterion.
 *
 * returns:
 *	structure block offset of the located node (>= 0, >startoffset),
 *		 on success
 *	-FDT_ERR_NOTFOUND, no node matching the criterion exists in the
 *		tree after startoffset
 * 	-FDT_ERR_BADOFFSET, nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_lookup_node_by_compatible(const void *fdt, int startoffset,
				  const char *compatible)
{
	int offset, err;

	/* FIXME: The algorithm here is pretty horrible: we scan each
	 * property of a node in fdt_node_check_compatible(), then if
	 * that didn't find what we want, we scan over them again
	 * making our way to the next node.  Still it's the easiest to
	 * implement approach; performance can come later. */
	for (offset = fdt_next_node(fdt, startoffset, NULL);
	     offset >= 0;
	     offset = fdt_next_node(fdt, offset, NULL)) {
		err = fdt_node_check_compatible(fdt, offset, compatible);
		if ((err < 0) && (err != -FDT_ERR_NOTFOUND))
			return err;
		else if (err == 0)
			return offset;
	}

	return offset; /* error from fdt_next_node() */
}


/**
 * fdt_node_check_compatible: check a node's compatible property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of a tree node
 * @compatible: string to match against
 *
 *
 * fdt_node_check_compatible() returns 0 if the given node contains a
 * 'compatible' property with the given string as one of its elements,
 * it returns non-zero otherwise, or on error.
 *
 * returns:
 *	0, if the node has a 'compatible' property listing the given string
 *	1, if the node has a 'compatible' property, but it does not list
 *		the given string
 *	-FDT_ERR_NOTFOUND, if the given node has no 'compatible' property
 * 	-FDT_ERR_BADOFFSET, if nodeoffset does not refer to a BEGIN_NODE tag
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE, standard meanings
 */
int fdt_node_check_compatible(const void *fdt, int nodeoffset,
			      const char *compatible)
{
	const void *prop;
	int len;

	prop = fdt_lookup_property_value_by_name(fdt, nodeoffset, "compatible", &len);
	if (!prop)
		return len;

	return !fdt_stringlist_contains(prop, len, compatible);
}


/**********************************************************************/
/* Property Lookup: return the offset or pointer.                     */
/**********************************************************************/

/**
 * fdt_lookup_property_entry_by_namelen - find a property based on substring
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @namelen: number of characters of name to consider
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * Identical to fdt_lookup_property_entry_by_name(), but only examine the first namelen
 * characters of name for matching the property name.
 */
const struct fdt_property *fdt_lookup_property_entry_by_namelen(const void *fdt,
    int offset, const char *name, int namelen, int *pvaluelen)
{
    const struct fdt_property *pentry;
    const char *prop_name = NULL;

    offset = fdt_first_property(fdt, offset);
    
    while (offset > 0) {
        if ((pentry = fdt_property_entry(fdt, offset, pvaluelen)) == 0) {
            offset = -FDT_ERR_INTERNAL;     /* for error handle */
            break;
        }

        prop_name = fdt_string(fdt, fdt32_to_cpu(pentry->nameoff));
        if (strlen(prop_name) == namelen && memcmp(prop_name, name, namelen) == 0)
            return pentry;

        offset = fdt_next_property(fdt, offset);
    }

    if (pvaluelen)  *pvaluelen = offset;
    return NULL;  /* error */
}

/**
 * fdt_lookup_property_entry_by_name - find a given property in a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_lookup_property_entry_by_name() retrieves a pointer to the fdt_property
 * structure within the device tree blob corresponding to the property
 * named 'name' of the node at offset nodeoffset.  If lenp is
 * non-NULL, the length of the property value is also returned, in the
 * integer pointed to by lenp.
 *
 * returns:
 *	pointer to the structure representing the property
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_NOTFOUND, node does not have named property
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const struct fdt_property *fdt_lookup_property_entry_by_name(
	const void *fdt, int nodeoffset, const char *name, int *pvaluelen)
{
	return fdt_lookup_property_entry_by_namelen(fdt, nodeoffset, name, strlen(name),
		pvaluelen);
}


/**
 * fdt_lookup_property_value_by_namelen - get property value based on substring
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @namelen: number of characters of name to consider
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * Identical to fdt_lookup_property_value_by_name(), but only examine the first namelen
 * characters of name for matching the property name.
 */
const void *fdt_lookup_property_value_by_namelen(const void *fdt, int nodeoffset,
				const char *name, int namelen, int *pvaluelen)
{
	const struct fdt_property *prop;

	prop = fdt_lookup_property_entry_by_namelen(fdt, nodeoffset, name, namelen,
        pvaluelen);
	if (! prop)
		return NULL;

	return prop->value;
}


/**
 * fdt_lookup_property_value_by_name - retrieve the value of a given property
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node whose property to find
 * @name: name of the property to find
 * @lenp: pointer to an integer variable (will be overwritten) or NULL
 *
 * fdt_lookup_property_value_by_name() retrieves a pointer to the value of the property
 * named 'name' of the node at offset nodeoffset (this will be a
 * pointer to within the device blob itself, not a copy of the value).
 * If lenp is non-NULL, the length of the property value is also
 * returned, in the integer pointed to by lenp.
 *
 * returns:
 *	pointer to the property's value
 *		if lenp is non-NULL, *lenp contains the length of the property
 *		value (>=0)
 *	NULL, on error
 *		if lenp is non-NULL, *lenp contains an error code (<0):
 *		-FDT_ERR_NOTFOUND, node does not have named property
 *		-FDT_ERR_BADOFFSET, nodeoffset did not point to FDT_BEGIN_NODE tag
 *		-FDT_ERR_BADMAGIC,
 *		-FDT_ERR_BADVERSION,
 *		-FDT_ERR_BADSTATE,
 *		-FDT_ERR_BADSTRUCTURE,
 *		-FDT_ERR_TRUNCATED, standard meanings
 */
const void *fdt_lookup_property_value_by_name(const void *fdt, int nodeoffset,
			const char *name, int *pvaluelen)
{
	return fdt_lookup_property_value_by_namelen(fdt, nodeoffset, name, strlen(name), pvaluelen);
}


/**
 * fdt_lookup_alias_value_by_namelen - get alias based on substring
 * @fdt: pointer to the device tree blob
 * @name: name of the alias th look up
 * @namelen: number of characters of name to consider
 *
 * Identical to fdt_lookup_alias_value_by_name(), but only examine the first namelen
 * characters of name for matching the alias name.
 */
const char *fdt_lookup_alias_value_by_namelen(const void *fdt,
				  const char *name, int namelen)
{
	int aliasoffset;

	aliasoffset = fdt_lookup_node_by_pathlen(fdt, "/aliases", 8);
	if (aliasoffset < 0)
		return NULL;

	return fdt_lookup_property_value_by_namelen(fdt, aliasoffset, name, namelen, NULL);
}


/**
 * fdt_lookup_alias_value_by_name - retreive the path referenced by a given alias
 * @fdt: pointer to the device tree blob
 * @name: name of the alias th look up
 *
 * fdt_lookup_alias_value_by_name() retrieves the value of a given alias.  That is, the
 * value of the property named 'name' in the node /aliases.
 *
 * returns:
 *	a pointer to the expansion of the alias named 'name', if it exists
 *	NULL, if the given alias or the /aliases node does not exist
 */
const char *fdt_lookup_alias_value_by_name(const void *fdt, const char *name)
{
    return fdt_lookup_alias_value_by_namelen(fdt, name, strlen(name));
}


/**
 * fdt_fetch_phandle - retrieve the phandle of a given node
 * @fdt: pointer to the device tree blob
 * @nodeoffset: structure block offset of the node
 *
 * fdt_fetch_phandle() retrieves the phandle of the device tree node at
 * structure block offset nodeoffset.
 *
 * returns:
 *	the phandle of the node at nodeoffset, on success (!= 0, != -1)
 *	0, if the node has no phandle, or another error occurs
 */
uint32_t fdt_fetch_phandle(const void *fdt, int nodeoffset)
{
	const fdt32_t *php;
	int len;

	/* FIXME: This is a bit sub-optimal, since we potentially scan
	 * over all the properties twice. */
	php = fdt_lookup_property_value_by_namelen(fdt, nodeoffset, "phandle", 7, &len);
	if (!php || (len != sizeof(*php))) {
		php = fdt_lookup_property_value_by_namelen(fdt, nodeoffset, "linux,phandle", 13, &len);
		if (!php || (len != sizeof(*php)))
			return 0;
	}

	return fdt32_to_cpu(*php);
}

/**
 * fdt_address_cells - retrieve address size for a bus represented in the tree
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node to find the address size for
 *
 * When the node has a valid #address-cells property, returns its value.
 *
 * returns:
 *	0 <= n < FDT_MAX_NCELLS, on success
 *      2, if the node has no #address-cells property
 *      -FDT_ERR_BADNCELLS, if the node has a badly formatted or invalid #address-cells property
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings
 */
int fdt_address_cells(const void *fdt, int nodeoffset);

/**
 * fdt_size_cells - retrieve address range size for a bus represented in the
 *                  tree
 * @fdt: pointer to the device tree blob
 * @nodeoffset: offset of the node to find the address range size for
 *
 * When the node has a valid #size-cells property, returns its value.
 *
 * returns:
 *	0 <= n < FDT_MAX_NCELLS, on success
 *      2, if the node has no #address-cells property
 *      -FDT_ERR_BADNCELLS, if the node has a badly formatted or invalid #size-cells property
 *	-FDT_ERR_BADMAGIC,
 *	-FDT_ERR_BADVERSION,
 *	-FDT_ERR_BADSTATE,
 *	-FDT_ERR_BADSTRUCTURE,
 *	-FDT_ERR_TRUNCATED, standard meanings
 */
int fdt_size_cells(const void *fdt, int nodeoffset);