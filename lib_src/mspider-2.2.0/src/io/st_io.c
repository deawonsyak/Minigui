/* * File: st_io.c *
 *
 * Copyright (C) 2005-2006 Feynman Software
 *
 * This program is g_free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mgdconfig.h"
#ifdef _NOUNIX_
#define NR_OPEN 1024
#else
#include <linux/limits.h>
#endif

#include "glib.h"

#define USE_LIST

static inline void  _DUMMY_FPRINTF (FILE* fp, const char * fmt, ...)
{ return; }
#define fprintf _DUMMY_FPRINTF

typedef struct _strnode strnode;
struct _strnode {
	int fd;
	int off;
	char *str;
};
static unsigned char appendmap[(NR_OPEN-1)/8+1];
static unsigned char bittab[] 
	= { 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
static unsigned char rbittab[] 
	= { 0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
#define setb(fd) do {appendmap[fd>>3] |= bittab[fd&0x07];} while(0)
#define unsetb(fd) do {appendmap[fd>>3] &= rbittab[fd&0x07];} while(0)
#define getb(fd) (appendmap[fd>>3]&bittab[fd&0x07])

#ifdef USE_LIST
static GList *appendlist;
static strnode* st_getappendnode(int fd);
#else
static void *pttab[NR_OPEN];
#endif

ssize_t st_read (int __fd, void *__buf, size_t __nbytes)
{
	int ret;
	int slen;
	char *str = NULL;
	strnode *node = NULL;

	fprintf(stderr, "call st_read fd=%d\n", __fd);
	if ( getb(__fd) ) {
		/* find and add some header info */
		fprintf(stderr, "st_read: find append node\n");
#ifdef USE_LIST
		node =  st_getappendnode(__fd);
#else
		node = pttab[__fd];
#endif
		str = node->str + node->off;
		ret = slen = strlen(str);
		if ( slen > __nbytes ) {
			fprintf(stderr, "st_read: slen>__nbytes\n");
			node->off += __nbytes; 
			strncpy(__buf, str, __nbytes);
			return __nbytes;
		}

		strcpy(__buf, str);
		__buf = (char*)__buf + slen; 
		__nbytes -= slen;

		unsetb(__fd);
#ifdef USE_LIST
		appendlist = g_list_remove(appendlist, node);
#endif
		g_free(node->str);
		g_free(node);

		slen += read(__fd, __buf, __nbytes);
		fprintf(stderr, "st_read: furthur call system read, readlen=%d\n", slen);
		return slen;
	}
	return read(__fd, __buf, __nbytes);
}

void st_addappendstr(int fd, char *str)
{
	strnode *node = NULL;

	if ( fd < 0 || str == NULL) {
		return;
	}

	node = g_malloc(sizeof(strnode));
	if ( node ) {
		node->fd = fd;
		node->off = 0;
		node->str = (gchar*)g_strdup(str);
		setb(fd);
#ifdef USE_LIST
		appendlist = g_list_append(appendlist, node);
#else
		pttab[fd] = node;
#endif
	}
}

#ifdef USE_LIST
static strnode* st_getappendnode(int fd)
{
	strnode *node = NULL;
	GList *list = NULL;
	
	list = appendlist;
	while ( list ) {
		node = (strnode*)list->data;
		if ( node->fd == fd ) {
			return node;
		}

		list = list->next;
	}

	/* never to here */
	return NULL;
}
#endif
