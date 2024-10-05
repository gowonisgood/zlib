#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../emscripten_build/include/zlib.h"

#define local static

/* print nastygram and leave */
local void quit(char *why)
{
    printf("Program aborted: %s\n", why);
    exit(1);
}

#define RAWLEN 4096    /* intermediate uncompressed buffer size */

/* compress from memory buffer to def until provided buffer is full or end of
   input reached; return last deflate() return value */
local int partcompress(unsigned char *input, size_t input_size, z_streamp def)
{
    int ret, flush;
    unsigned char raw[RAWLEN];
    size_t input_offset = 0;

    flush = Z_NO_FLUSH;
    do {
        size_t bytes_to_copy = (input_size - input_offset < RAWLEN) ? (input_size - input_offset) : RAWLEN;
        def->avail_in = bytes_to_copy;
        memcpy(raw, input + input_offset, bytes_to_copy);
        input_offset += bytes_to_copy;
        def->next_in = raw;

        if (input_offset >= input_size)
            flush = Z_FINISH;
        ret = deflate(def, flush);
        if (ret == Z_STREAM_ERROR)
            quit("Stream error during deflation");
    } while (def->avail_out != 0 && flush == Z_NO_FLUSH);
    return ret;
}

/* recompress from inf's input to def's output; the input for inf and
   the output for def are set in those structures before calling */
local int recompress(z_streamp inf, z_streamp def)
{
    int ret, flush;
    unsigned char raw[RAWLEN];

    flush = Z_NO_FLUSH;
    do {
        /* decompress */
        inf->avail_out = RAWLEN;
        inf->next_out = raw;
        ret = inflate(inf, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_NEED_DICT)
            quit("Stream or data error during inflation");
        if (ret == Z_MEM_ERROR)
            return ret;

        /* compress what was decompressed until done or no room */
        def->avail_in = RAWLEN - inf->avail_out;
        def->next_in = raw;
        if (inf->avail_out != 0)
            flush = Z_FINISH;
        ret = deflate(def, flush);
        if (ret == Z_STREAM_ERROR)
            quit("Stream error during recompression");
    } while (ret != Z_STREAM_END && def->avail_out != 0);
    return ret;
}

#define EXCESS 256      /* empirically determined stream overage */
#define MARGIN 8        /* amount to back off for completion */

/* compress from memory buffer to fixed-size block in another memory buffer */
int main(int argc, char **argv)
{
	printf("started\n");
    int ret;                /* return code */
    unsigned size;          /* requested fixed output block size */
    unsigned have;          /* bytes written by deflate() call */
    unsigned char *blk;     /* intermediate and final stream */
    unsigned char *tmp;     /* close to desired size stream */
    z_stream def, inf;      /* zlib deflate and inflate states */

    /* Example input data */
    unsigned char input_data[] = "This is an example of data that will be compressed in memory.";
    size_t input_size = sizeof(input_data) - 1; // Exclude null terminator

    /* get requested output size */
    if (argc != 2)
        quit("need one argument: size of output block");
    ret = strtol(argv[1], argv + 1, 10);
    if (argv[1][0] != 0)
        quit("argument must be a number");
    if (ret < 8)            /* 8 is minimum zlib stream size */
        quit("need positive size of 8 or greater");
    size = (unsigned)ret;

    /* allocate memory for buffers and compression engine */
    blk = malloc(size + EXCESS);
    if (blk == NULL)
        quit("out of memory");

    def.zalloc = Z_NULL;
    def.zfree = Z_NULL;
    def.opaque = Z_NULL;
    ret = deflateInit(&def, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
        quit("deflate initialization failed");

    /* compress from input_data until output full, or no more input */
    def.avail_out = size + EXCESS;
    def.next_out = blk;
    ret = partcompress(input_data, input_size, &def);

    /* if it all fit, then size was undersubscribed -- done! */
    if (ret == Z_STREAM_END && def.avail_out >= EXCESS) {
        /* compression done */
        have = size + EXCESS - def.avail_out;

        /* clean up */
        ret = deflateEnd(&def);
        if (ret == Z_STREAM_ERROR)
            quit("deflate end failed");
        free(blk);
        printf("1.have:%d",have);
        return have;  // return compressed size
    }

    /* it didn't all fit -- set up for recompression */
    inf.zalloc = Z_NULL;
    inf.zfree = Z_NULL;
    inf.opaque = Z_NULL;
    inf.avail_in = 0;
    inf.next_in = Z_NULL;
    ret = inflateInit(&inf);
    if (ret != Z_OK || blk == NULL)
        quit("inflate initialization failed or out of memory");

    tmp = malloc(size + EXCESS);
    if (tmp == NULL)
        quit("out of memory");
    ret = deflateReset(&def);
    if (ret == Z_STREAM_ERROR)
        quit("deflate reset failed");

    /* do first recompression close to the right amount */
    inf.avail_in = size + EXCESS;
    inf.next_in = blk;
    def.avail_out = size + EXCESS;
    def.next_out = tmp;
    ret = recompress(&inf, &def);
    if (ret == Z_MEM_ERROR)
        quit("out of memory");

    /* set up for next recompression */
    ret = inflateReset(&inf);
    if (ret == Z_STREAM_ERROR)
        quit("inflate reset failed");
    ret = deflateReset(&def);
    if (ret == Z_STREAM_ERROR)
        quit("deflate reset failed");

    /* do second and final recompression (third compression) */
    inf.avail_in = size - MARGIN;   /* assure stream will complete */
    inf.next_in = tmp;
    def.avail_out = size;
    def.next_out = blk;
    ret = recompress(&inf, &def);
    if (ret == Z_MEM_ERROR)
        quit("out of memory");
    if (ret != Z_STREAM_END)
        quit("stream did not end, MARGIN too small");

    /* compression done */
    have = size - def.avail_out;

    /* clean up */
    free(tmp);
    ret = inflateEnd(&inf);
    if (ret == Z_STREAM_ERROR)
        quit("inflate end failed");
    ret = deflateEnd(&def);
    if (ret == Z_STREAM_ERROR)
        quit("deflate end failed");
    free(blk);
    printf("have:%d",have); //debug
    return have;  // return compressed size
}

