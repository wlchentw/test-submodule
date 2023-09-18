/*
 * Copyright (c) 2017-2018, Linaro Limited
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef __ATTRIBUTES_H
#define __ATTRIBUTES_H

#include <assert.h>
#include <sks_internal_abi.h>
#include <stdint.h>
#include <stddef.h>

#include "sks_helpers.h"

#ifdef SKS_SHEAD_WITH_BOOLPROPS
static inline void set_attributes_in_head(struct sks_attrs_head *head)
{
	head->boolproph |= SKS_BOOLPROPH_FLAG;
}

static inline bool head_contains_boolprops(struct sks_attrs_head __unused *head)
{
	return head->boolproph & SKS_BOOLPROPH_FLAG;
}
#endif

/*
 * Allocation a reference for a serialized attributes.
 * Can be freed from a simple TEE_Free(reference);
 *
 * Return a SKS_OK on success or a SKS return code.
 */
uint32_t init_attributes_head(struct sks_attrs_head **head);

/*
 * Update serialized attributes to add an entry. Can relocate the attribute
 * list buffer.
 *
 * Return a SKS_OK on success or a SKS return code.
 */
uint32_t add_attribute(struct sks_attrs_head **head,
			uint32_t attribute, void *data, size_t size);

/*
 * Update serialized attributes to remove an entry. Can relocate the attribute
 * list buffer. Only 1 instance of the entry is expected (TODO factory with _check)
 *
 * Return a SKS_OK on success or a SKS return code.
 */
uint32_t remove_attribute(struct sks_attrs_head **head, uint32_t attrib);

/*
 * Update serialized attributes to remove an entry. Can relocate the attribute
 * list buffer. If attribute ID is find several times, remove all of them.
 *
 * Return a SKS_OK on success or a SKS return code.
 */
uint32_t remove_attribute_check(struct sks_attrs_head **head, uint32_t attribute,
				size_t max_check);

/*
 * If *count == 0, count and return in *count the number of attributes matching
 * the input attribute ID.
 *
 * If *count != 0, return the address and size of the attributes found, up to
 * the occurrence number *count. attr and attr_size and expected large
 * enough. attr is the output array of the values found. attr_size is the
 * output array of the size of each values found.
 *
 * If attr_size != NULL, return in in *attr_size attribute value size.
 * If attr != NULL return in *attr the address in memory of the attribute value.
 */
void get_attribute_ptrs(struct sks_attrs_head *head, uint32_t attribute,
			void **attr, uint32_t *attr_size, size_t *count);

/*
 * If attributes is not found return SKS_NOT_FOUND.
 * If attr_size != NULL, return in in *attr_size attribute value size.
 * If attr != NULL return in *attr the address in memory of the attribute value.
 *
 * Return a SKS_OK or SKS_NOT_FOUND on success, or a SKS return code.
 */
uint32_t get_attribute_ptr(struct sks_attrs_head *head, uint32_t attribute,
			   void **attr_ptr, uint32_t *attr_size);
/*
 * If attribute is not found, return SKS_NOT_FOUND.
 * If attr_size != NULL, check *attr_size matches attributes size of return
 * SKS_SHORT_BUFFER with expected size in *attr_size.
 * If attr != NULL and attr_size is NULL or gives expected buffer size,
 * copy attribute value into attr.
 *
 * Return a SKS_OK or SKS_NOT_FOUND on success, or a SKS return code.
 */
uint32_t get_attribute(struct sks_attrs_head *head, uint32_t attribute,
			void *attr, uint32_t *attr_size);

static inline uint32_t get_u32_attribute(struct sks_attrs_head *head,
					 uint32_t attribute, uint32_t *attr)
{
	uint32_t size = sizeof(uint32_t);
	uint32_t rv = get_attribute(head, attribute, attr, &size);

	if (size != sizeof(uint32_t))
		return SKS_ERROR;

	return rv;
}

/*
 * Return true all attributes from the reference are found and match value
 * in the candidate attribute list.
 *
 * Return a SKS_OK on success, or a SKS return code.
 */
bool attributes_match_reference(struct sks_attrs_head *ref,
				struct sks_attrs_head *candidate);

/*
 * Some helpers
 */
static inline size_t attributes_size(struct sks_attrs_head *head)
{
	return sizeof(struct sks_attrs_head) + head->attrs_size;
}

#ifdef SKS_SHEAD_WITH_TYPE
static inline uint32_t get_class(struct sks_attrs_head *head)
{
	return head->class;
}

static inline uint32_t get_type(struct sks_attrs_head *head)
{
	return head->type;
}
#else
static inline uint32_t get_class(struct sks_attrs_head *head)
{
	uint32_t class;
	uint32_t size = sizeof(class);

	if (get_attribute(head, SKS_CKA_CLASS, &class, &size))
		return SKS_UNDEFINED_ID;

	return class;
}
static inline uint32_t get_type(struct sks_attrs_head *head)
{
	uint32_t type;
	uint32_t size = sizeof(type);

	if (get_attribute(head, SKS_CKA_KEY_TYPE, &type, &size))
		return SKS_UNDEFINED_ID;

	return type;
}
#endif

bool get_bool(struct sks_attrs_head *head, uint32_t attribute);

/* Debug: dump object attributes to IMSG() trace console */
uint32_t trace_attributes(const char *prefix, void *ref);

#endif /*__ATTRIBUTES_H*/
