#include <stdlib.h>
#include <string.h>
#include <stdio.h>

FILE * debug;

static char szPad[4] = "\0\0\0\0";
static char szMalloc[4] = "m\0\0\0";
static char szFree[4] = "f\0\0\0";
static char szRealloc[4] = "r\0\0\0";

void debug_malloc_init(void)
{
    debug = fopen("malloc.debug.bin", "wb");
}

void debug_malloc_finish(void)
{
    fflush(debug);
    fclose(debug);
}

void * debug_malloc(size_t size)
{
    void * ret = malloc(size);
    fwrite(szMalloc, 4, 1, debug);
    fwrite(&ret, 4, 1, debug);
    fwrite(szPad,4,1,debug);

    return ret;
}

void debug_free(void * block)
{
    fwrite(szFree,4,1,debug);
    fwrite(&block,4,1,debug);
    fwrite(szPad,4,1,debug);

    free(block);
}

void * debug_realloc(void * memblock, size_t size)
{
    void * ret;
    fwrite(szRealloc,4,1,debug);
    fwrite(&memblock,4,1,debug);

    ret = realloc(memblock, size);
    fwrite(&ret,4,1,debug);
    return ret;
}

const char * debug_strdup(const char * source)
{    
    const char * ret;
    ret = strdup(source);
    fwrite(szMalloc,4,1,debug);
    fwrite(&ret,4,1,debug);
    fwrite(szPad,4,1,debug);

    return ret;
}
