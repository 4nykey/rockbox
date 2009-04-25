#include "os_types.h"

#if defined(CPU_ARM) || defined(CPU_COLDFIRE)
#include <setjmp.h>
extern jmp_buf rb_jump_buf;
#endif

static size_t tmp_ptr;

void ogg_malloc_init(void)
{
    mallocbuf = ci->codec_get_buffer(&bufsize);
    tmp_ptr = bufsize & ~3;
    mem_ptr = 0;
}

void *ogg_malloc(size_t size)
{
    void* x;

    size = (size + 3) & ~3;

    if (mem_ptr + size > tmp_ptr)
#if defined(CPU_ARM) || defined(CPU_COLDFIRE)
        longjmp(rb_jump_buf, 1);
#else
        return NULL;
#endif
    
    x = &mallocbuf[mem_ptr];
    mem_ptr += size; /* Keep memory 32-bit aligned */

    return x;
}

void *ogg_tmpmalloc(size_t size)
{
    size = (size + 3) & ~3;

    if (mem_ptr + size > tmp_ptr)
        return NULL;
    
    tmp_ptr -= size;
    return &mallocbuf[tmp_ptr];
}

void *ogg_calloc(size_t nmemb, size_t size)
{
    void *x;
    x = ogg_malloc(nmemb * size);
    if (x == NULL)
        return NULL;
    ci->memset(x, 0, nmemb * size);
    return x;
}

void *ogg_tmpcalloc(size_t nmemb, size_t size)
{
    void *x;
    x = ogg_tmpmalloc(nmemb * size);
    if (x == NULL)
        return NULL;
    ci->memset(x, 0, nmemb * size);
    return x;
}

void *ogg_realloc(void *ptr, size_t size)
{
    void *x;
    (void)ptr;
    x = ogg_malloc(size);
    return x;
}

long ogg_tmpmalloc_pos(void)
{
    return tmp_ptr;
}

void ogg_tmpmalloc_free(long pos)
{
    tmp_ptr = pos;
}

/* Allocate IRAM buffer */
static unsigned char iram_buff[IRAM_IBSS_SIZE] IBSS_ATTR __attribute__ ((aligned (16)));
static size_t iram_remain;

void iram_malloc_init(void){
    iram_remain=IRAM_IBSS_SIZE;
}

void *iram_malloc(size_t size){
    void* x;
    
    /* always ensure 16-byte aligned */
    if(size&0x0f)
      size=(size-(size&0x0f))+16;
      
    if(size>iram_remain)
      return NULL;
    
    x = &iram_buff[IRAM_IBSS_SIZE-iram_remain];
    iram_remain-=size;

    return x;
}
