/*
 * Copyright (C) 2006 Anders Brander <anders@brander.dk> and 
 * Anders Kvist <akv@lnxbx.dk>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <gtk/gtk.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "rawfile.h"

static int cpuorder;

void
raw_init()
{
	if (ntohs(0x1234) == 0x1234)
		cpuorder = 0x4D4D;
	else
		cpuorder = 0x4949;
	return;
}

gboolean
raw_get_uint(RAWFILE *rawfile, guint pos, guint *target)
{
	if((pos+4)>rawfile->size)
		return(FALSE);
	if (rawfile->byteorder == cpuorder)
		*target = *(guint *)(rawfile->map+pos);
	else
		*target = ENDIANSWAP4(*(guint *)(rawfile->map+pos));
	return(TRUE);
}

gboolean
raw_get_ushort(RAWFILE *rawfile, guint pos, gushort *target)
{
	if((pos+2)>rawfile->size)
		return(FALSE);
	if (rawfile->byteorder == cpuorder)
		*target = *(gushort *)(rawfile->map+pos);
	else
		*target = ENDIANSWAP2(*(gushort *)(rawfile->map+pos));
	return(TRUE);
}

gboolean
raw_get_float(RAWFILE *rawfile, guint pos, gfloat *target)
{
	if((pos+4)>rawfile->size)
		return(FALSE);

	if (rawfile->byteorder == cpuorder)
		*target = *(gfloat *)(rawfile->map+pos);
	else
		*target = (gfloat) (ENDIANSWAP4(*(gint *)(rawfile->map+pos)));
	return(TRUE);
}

gint
raw_strcmp(RAWFILE *rawfile, guint pos, const gchar *needle, gint len)
{
	if((pos+len) > rawfile->size)
		return(FALSE);
	return(g_ascii_strncasecmp(needle, rawfile->map+pos, len));
}

gboolean
raw_strcpy(RAWFILE *rawfile, guint pos, void *target, gint len)
{
	if((pos+len) > rawfile->size)
		return(FALSE);
	g_memmove(target, rawfile->map+pos, len);
	return(TRUE);
}

GdkPixbuf *
raw_get_pixbuf(RAWFILE *rawfile, guint pos, guint length)
{
	GdkPixbufLoader *pl;
	GdkPixbuf *pixbuf = NULL;
	if((pos+length)>rawfile->size)
		return(NULL);

	pl = gdk_pixbuf_loader_new();
	gdk_pixbuf_loader_write(pl, rawfile->map+pos, length, NULL);
	pixbuf = gdk_pixbuf_loader_get_pixbuf(pl);
	gdk_pixbuf_loader_close(pl, NULL);
	return(pixbuf);
}

RAWFILE *
raw_open_file(const gchar *filename)
{
	struct stat st;
	gint fd;
	RAWFILE *rawfile;

	if(stat(filename, &st))
		return(NULL);
	if ((fd = open(filename, O_RDONLY)) == -1)
		return(NULL);
	rawfile = g_malloc(sizeof(RAWFILE));
	rawfile->fd = fd;
	rawfile->size = st.st_size;
	rawfile->map = mmap(NULL, rawfile->size, PROT_READ, MAP_SHARED, fd, 0);
	if(rawfile->map == MAP_FAILED)
	{
		g_free(rawfile);
		return(NULL);
	}
	rawfile->byteorder = *((gushort *) rawfile->map);
	raw_get_uint(rawfile, 4, &rawfile->first_ifd_offset);
	return(rawfile);
}

void
raw_close_file(RAWFILE *rawfile)
{
	munmap(rawfile->map, rawfile->size);
	close(rawfile->fd);
	g_free(rawfile);
	return;
}
