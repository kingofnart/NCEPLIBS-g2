/**
 * @file
 * @brief This program reads PNG stream data from memory.
 * @author Stephen Gilbert
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

typedef int g2int; /**< Integer type. */

/** Location to write PNG stream. */
struct png_stream
{
    unsigned char *stream_ptr;     /**< location to write PNG stream */
    g2int stream_len;              /**< number of bytes written */
};

typedef struct png_stream png_stream; /**< location to write PNG stream */

/**
 * Custom read function used so that libpng will read a PNG stream
 * from memory instead of a file on disk.
 *
 * @param png_ptr Pointer to PNG.
 * @param data Pointer to data.
 * @param length Length.
 *
 * @author Stephen Gilbert
 */
static void
user_read_data(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
    char *ptr;
    g2int offset;
    png_stream *mem;

    mem = (png_stream *)png_get_io_ptr(png_ptr);
    ptr = (void *)mem->stream_ptr;
    offset = mem->stream_len;
    memcpy(data, ptr + offset, length);
    mem->stream_len += length;
}

/**
 * Decode some PNG compressed data.
 *
 * @param pngbuf Pointer to the PNG buffer to be uncompressed.
 * @param width Pointer to the width.
 * @param height Pointer to the height.
 * @param cout Pointer that gets the unpacked data.
 *
 * @return 
 * - 0 Success
 * - -1 Error initializing PNG structs.
 * - -2 Error initializing PNG info struct.
 * - -3 Error initializing PNG error callbacks, or stream not valid PNG.
 *
 * @author Stephen Gilbert
 */
int
dec_png(unsigned char *pngbuf, g2int *width, g2int *height, char *cout)
{
    int interlace, color, compres, filter, bit_depth;
    g2int j, k, n, bytes, clen;
    png_structp png_ptr;
    png_infop info_ptr, end_info;
    png_bytepp row_pointers;
    png_stream read_io_ptr;
    png_uint_32 h32, w32;

    /* Check if stream is a valid PNG format. */
    if (png_sig_cmp(pngbuf, 0, 8) != 0)
        return -3;

    /* Create and initialize png_structs. */
    if (!(png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,
                                           NULL, NULL)))
        return -1;

    if (!(info_ptr = png_create_info_struct(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
        return -2;
    }

    if (!(end_info = png_create_info_struct(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr, (png_infopp)info_ptr, (png_infopp)NULL);
        return -2;
    }

    /* Set Error callback. */
    if (setjmp(png_jmpbuf(png_ptr)))
    {
        png_destroy_read_struct(&png_ptr,  &info_ptr, &end_info);
        return -3;
    }

    /* Initialize info for reading PNG stream from memory. */
    read_io_ptr.stream_ptr = (png_voidp)pngbuf;
    read_io_ptr.stream_len = 0;

    /* Set new custom read function. */
    png_set_read_fn(png_ptr, (png_voidp)&read_io_ptr, (png_rw_ptr)user_read_data);

    /* Read and decode PNG stream. */
    png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    /* Get pointer to each row of image data. */
    row_pointers = png_get_rows(png_ptr, info_ptr);

    /* Get image info, such as size, depth, colortype, etc...  */
    (void)png_get_IHDR(png_ptr, info_ptr, &w32, &h32,
                       &bit_depth, &color, &interlace, &compres, &filter);

    *height = h32;
    *width = w32;

    /* Check if image was grayscale. */
    if ( color == PNG_COLOR_TYPE_RGB ) {
        bit_depth = 24;
    }
    else if ( color == PNG_COLOR_TYPE_RGB_ALPHA ) {
        bit_depth = 32;
    }

    /* Copy image data to output string */
    n = 0;
    bytes = bit_depth / 8;
    clen = *width * bytes;
    for (j = 0; j < *height; j++) {
        for (k = 0; k < clen; k++) {
            cout[n] = *(row_pointers[j]+k);
            n++;
        }
    }

    /* Clean up. */
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return 0;
}
