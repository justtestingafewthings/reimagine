/*
 * Copyright 2013, winocm. <winocm@icloud.com>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 * 
 *   Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 *   Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 * 
 *   If you are going to use this software in any form that does not involve
 *   releasing the source to this project or improving it, let me know beforehand.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <err.h>
#include <stdbool.h>
#include <assert.h>
#include "util.h"
#include "image3.h"
#include "helper.h"

#define image3_off(head, off) ((uint32_t)((uintptr_t)off - (uintptr_t)head))

int image3_fast_get_type(void *buffer, uint32_t * type)
{
    Image3RootHeader *head = (Image3RootHeader *) buffer;
    if (head->header.magic != kImage3Magic) {
        printf("image3_fast_get_type: invalid header magic 0x%08x\n",
               (unsigned int)head->header.magic);
        return false;
    }

    *type = head->shshExtension.imageType;

    return true;
}

Image3Header *image3_find_tag(void *image, uint32_t find_tag)
{
    Image3Header *root = (Image3Header *) image;
    Image3Header *tag = (Image3Header *) ((Image3RootHeader *) image + 1);

    while (1) {
        if (tag->magic == find_tag)
            return tag;
        tag = (Image3Header *) add_ptr2(tag, tag->size);
        if (image3_off(root, tag) > root->size)
            break;
    }

    return NULL;
}

int image3_get_tag_data(void *image, uint32_t tag, void **out_data,
                        uint32_t * out_size)
{
    Image3RootHeader *head = (Image3RootHeader *) image;
    Image3Header *imageTag = image3_find_tag(image, tag);

    if (imageTag) {
        assert((out_data != NULL) && (out_size != NULL));

        printf
            ("image3_get_tag_data: data at 0x%08x of size 0x%08x for image type '%c%c%c%c'\n",
             (unsigned int)(imageTag + 1),
             (unsigned int)imageTag->dataSize,
             (char)(head->shshExtension.imageType >> 24),
             (char)(head->shshExtension.imageType >> 16),
             (char)(head->shshExtension.imageType >> 8),
             (char)(head->shshExtension.imageType)
            );

        *out_size = imageTag->dataSize;
        *out_data = (void *)(imageTag + 1);
        return true;
    }

    return false;
}

int image3_iterate_tags(void *image, tag_callback_t callback)
{
    Image3Header *root = (Image3Header *) image;
    if (root->magic != kImage3Magic) {
        printf("image3_iterate_tags: this is not an Img3 file 0x%08x\n",
               (unsigned int)root->magic);
        return false;
    }

    Image3Header *tag = (Image3Header *) ((Image3RootHeader *) image + 1);

    while (image3_off(root, tag) < root->size) {

        callback(tag, (Image3RootHeader *) root);

        tag = (Image3Header *) add_ptr2(tag, tag->size);        
    }

    /*printf("size: 0x%x\nend of last tag is 0x%x\n", root->size, (uint32_t) tag + tag->size - (uint32_t) image);*/

    return true;    
}

int image3_map_file(const char* filename, void** buffer)
{
    struct stat buf;

    FILE *fp = fopen (filename, "rb");

    if (!fp)
        return -ENOENT;
    if (stat (filename, &buf) > 0)
        return -ENOENT;

    *buffer = (uint8_t *) _xmalloc (buf.st_size);

    printf("buffer size:0x%x\n", (uint32_t) buf.st_size);

    fread ((void *) *buffer, buf.st_size, 1, fp);
    fclose (fp);

    return 0;
}
