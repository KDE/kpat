void debug_malloc_init(void);
void debug_malloc_finish(void);
void * debug_malloc(int size);
void debug_free(void * block);
void * debug_realloc(void * memblock, int size);
const char * debug_strdup(const char * source);
