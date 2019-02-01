/*
 * Copyright (C) 2010-2011 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 *
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_osk_memory.c
 * Implementation of the OS abstraction layer for the kernel device driver
 */

#include "mali_osk.h"
#include <linux/slab.h>
#include <linux/vmalloc.h>

__weak void *_mali_osk_calloc( u32 n, u32 size )
{
    return kcalloc(n, size, GFP_KERNEL);
}

__weak void *_mali_osk_malloc( u32 size )
{
    return kmalloc(size, GFP_KERNEL);
}

__weak void _mali_osk_free( void *ptr )
{
    kfree(ptr);
}

__weak void *_mali_osk_valloc( u32 size )
{
    return vmalloc(size);
}

__weak void _mali_osk_vfree( void *ptr )
{
    vfree(ptr);
}

__weak void *_mali_osk_memcpy( void *dst, const void *src, u32	len )
{
    return memcpy(dst, src, len);
}

__weak void *_mali_osk_memset( void *s, u32 c, u32 n )
{
    return memset(s, c, n);
}

__weak mali_bool _mali_osk_mem_check_allocated( u32 max_allocated )
{
	/* No need to prevent an out-of-memory dialogue appearing on Linux,
	 * so we always return MALI_TRUE.
	 */
	return MALI_TRUE;
}
