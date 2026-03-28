//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	WAD I/O functions.
//

#include <stdio.h>
#include <string.h>

#include "m_misc.h"
#include "w_file.h"
#include "z_zone.h"
#include "../doom_embedded_wad.h"

typedef struct
{
    wad_file_t wad;
    FILE *fstream;
} stdc_wad_file_t;

typedef struct
{
    wad_file_t wad;
    const byte *data;
} embedded_wad_file_t;

extern wad_file_class_t stdc_wad_file;
extern wad_file_class_t embedded_wad_file;

static wad_file_t *W_Embedded_OpenFile(void)
{
    embedded_wad_file_t *result;

    result = Z_Malloc(sizeof(embedded_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &embedded_wad_file;
    result->wad.mapped = (byte *) doom1_wad;
    result->wad.length = doom1_wad_len;
    result->data = doom1_wad;

    return &result->wad;
}

static wad_file_t *W_StdC_OpenFile(char *path)
{
    stdc_wad_file_t *result;
    FILE *fstream;

    if (M_StringEndsWith(path, "doom1.wad") && doom1_wad_len > 0)
    {
        return W_Embedded_OpenFile();
    }

    fstream = fopen(path, "rb");

    if (fstream == NULL)
    {
        return NULL;
    }

    // Create a new stdc_wad_file_t to hold the file handle.

    result = Z_Malloc(sizeof(stdc_wad_file_t), PU_STATIC, 0);
    result->wad.file_class = &stdc_wad_file;
    result->wad.mapped = NULL;
    result->wad.length = M_FileLength(fstream);
    result->fstream = fstream;

    return &result->wad;
}

static void W_StdC_CloseFile(wad_file_t *wad)
{
    stdc_wad_file_t *stdc_wad;

    stdc_wad = (stdc_wad_file_t *) wad;

    fclose(stdc_wad->fstream);
    Z_Free(stdc_wad);
}

static void W_Embedded_CloseFile(wad_file_t *wad)
{
    Z_Free(wad);
}

// Read data from the specified position in the file into the 
// provided buffer.  Returns the number of bytes read.

size_t W_StdC_Read(wad_file_t *wad, unsigned int offset,
                   void *buffer, size_t buffer_len)
{
    stdc_wad_file_t *stdc_wad;
    size_t result;

    stdc_wad = (stdc_wad_file_t *) wad;

    // Jump to the specified position in the file.

    fseek(stdc_wad->fstream, offset, SEEK_SET);

    // Read into the buffer.

    result = fread(buffer, 1, buffer_len, stdc_wad->fstream);

    return result;
}

static size_t W_Embedded_Read(wad_file_t *wad, unsigned int offset,
                              void *buffer, size_t buffer_len)
{
    embedded_wad_file_t *embedded_wad;
    size_t available;

    embedded_wad = (embedded_wad_file_t *) wad;

    if (offset >= embedded_wad->wad.length)
    {
        return 0;
    }

    available = embedded_wad->wad.length - offset;
    if (buffer_len > available)
    {
        buffer_len = available;
    }

    memcpy(buffer, embedded_wad->data + offset, buffer_len);
    return buffer_len;
}

wad_file_class_t stdc_wad_file = 
{
    W_StdC_OpenFile,
    W_StdC_CloseFile,
    W_StdC_Read,
};

wad_file_class_t embedded_wad_file =
{
    NULL,
    W_Embedded_CloseFile,
    W_Embedded_Read,
};
