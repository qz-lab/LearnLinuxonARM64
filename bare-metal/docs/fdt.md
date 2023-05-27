# Flatten Device Tree

For hwo to write a Device Tree file (DTS), please refer to the online document [Device Tree Usage](https://elinux.org/index.php?title=Device_Tree_Usage).

Device Tree is comprised of 4 parts:

1. Header: its version info and the locations of `Structure Block`, `Strings Block` and `Reserved Memories`.
2. Structure Block: Nodes and their Properties.
3. String Blocks: Property Name Strings. All the strings are catenated together, and each of them ends with a NULL character.
4. Reserved Memories: The address and size of each Reserved Memories are stored into an array. The last entry is filled with all 0s.

## Data Structure

In Linux implementation,


### Header

### Structure Block


## How to access Device Tree Structures

We need to provide the traversal and lookup APIs to access Nodes and Properties in the Structure Block.

### Low-level Procedure

* `int fdt_check_header(const void *fdt)` check the FDT magic number and its compatible versions.
* `int fdt_move(const void *fdt, void *buf, int bufsize)`

Convert offset to its pointer:

* `const void *fdt_offset_to_ptr(const void *fdt, int offset, unsigned int len)` convert an offset to the corresponding pointer of a tag, and make sure that the Node/Property locates in the FDT completely.
* `uint32_t fdt_tag_next_offset(const void *fdt, int offset, int *next_offset)` assure and return the tag value at the offset, calculate the next aligned address after this Node/Property.

### Node

Nodes are just containers of sub-Nodes and Properties. Node structure only contains its type (tag) and the name string. Thus, it is useless to return the pointer of Node structure. Instead, **all the Node-related APIs only use the offset to represent a Node**.

To access the Node name:

* `const char *fdt_node_name(const void *fdt, int offset, int *pnamelen)`
* `int fdt_nodename_equal(const void *fdt, int offset, const char *s, int len)`

To traverse Nodes, we provide the following APIs:

* `int fdt_next_node(const void *fdt, int offset, int *depth)`
* `int fdt_first_child_node(const void *fdt, int offset)`
* `int fdt_next_sibling_node(const void *fdt, int offset)`


To lookup a specific Node, we need to implement the following APIs:

1. lookup a child node with the specific name

  * `int fdt_lookup_child_node_by_namelen(const void *fdt, int offset, const char *name, int namelen)`
  * `int fdt_lookup_child_node_by_name(const void *fdt, int offset, const char *name)`

2. lookup a node with the specific path string

  * `int fdt_lookup_node_by_pathlen(const void *fdt, const char *path, int pathlen)`
  * `int fdt_lookup_node_by_path(const void *fdt, const char *path)`

3. lookup a node with the specific Property value

  * `int fdt_lookup_node_by_property_value(const void *fdt, int startoffset, const char *propname, const void *propval, int proplen)`
  * `int fdt_lookup_node_by_phandle(const void *fdt, uint32_t phandle)`
  * `int fdt_lookup_node_by_compatible(const void *fdt, int startoffset, const char *compatible)`
  * `int fdt_node_check_compatible(const void *fdt, int nodeoffset, const char *compatible)`


Advanced feature:

int fdt_get_path(const void *fdt, int nodeoffset, char *buf, int buflen);
int fdt_supernode_atdepth_offset(const void *fdt, int nodeoffset,
				 int supernodedepth, int *nodedepth);
int fdt_node_depth(const void *fdt, int nodeoffset);
int fdt_parent_offset(const void *fdt, int nodeoffset);


### Property


To traverse Properties, we provide the following APIs:

* `const struct fdt_property *fdt_property_entry(const void *fdt, int offset, int *pvaluelen)`
* `const void *fdt_property_value(const void *fdt, int offset, const char **pname, int *pvaluelen)`
* ` int fdt_first_property(const void *fdt, int nodeoffset)`
* ` int fdt_next_property(const void *fdt, int offset)`

The values of some Properties contains a list of strings, separated by comma:

* `int fdt_stringlist_contains(const char *strlist, int listlen, const char *str)`
* `int fdt_stringlist_count(const void *fdt, int nodeoffset, const char *property)`
* `int fdt_stringlist_search(const void *fdt, int nodeoffset, const char *property, const char *string)`
* `const char *fdt_stringlist_get(const void *fdt, int nodeoffset, const char *property, int index, int *lenp)`


To lookup a specific Property, we need to implement the following APIs:

1. lookup a Property Entry with the specific name

  * `const struct fdt_property *fdt_lookup_property_entry_by_namelen(const void *fdt, int nodeoffset, const char *name, int namelen, int *pvaluelen)`
  * `const struct fdt_property *fdt_lookup_property_entry_by_name( const void *fdt, int nodeoffset, const char *name, int *pvaluelen)`

2. lookup a Property value with the specific name

  * `const void *fdt_lookup_property_value_by_namelen(const void *fdt, int nodeoffset, const char *name, int namelen, int *pvaluelen)`
  * `const void *fdt_lookup_property_value_by_name(const void *fdt, int nodeoffset, const char *name, int *pvaluelen)`

3. lookup the special Property value in a Node

  * `const char *fdt_lookup_alias_value_by_namelen(const void *fdt, const char *name, int namelen)`
  * `const char *fdt_lookup_alias_value_by_name(const void *fdt, const char *name)`

  * `uint32_t fdt_fetch_phandle(const void *fdt, int nodeoffset)`
  * `int fdt_fetch_address_cells(const void *fdt, int nodeoffset)`
  * `int fdt_fetch_size_celss(const void *fdt, int nodeoffset)`

### String Block


### Reserved-Memory Regions