#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

void * malloc(size_t size) {
    void * ptr = mmap(NULL, size + sizeof(size_t), PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
        return NULL;
    size_t * size_ptr = (size_t *) ptr;
    *(size_ptr) = size + sizeof(size_t);
    return (void *) (size_ptr + 1);
}

void free(void * ptr) {
    if (ptr == NULL)
        return;
    size_t * size_ptr = ((size_t *) ptr) - 1;
    munmap((void *) size_ptr, *size_ptr);
}

size_t ptr_size(void * ptr) {
    size_t * size_ptr = ((size_t *) ptr) - 1;
    return *(size_ptr);
}

void * ptr_start(void * ptr) {
    size_t * size_ptr = ((size_t *) ptr) - 1;
    return (void *) size_ptr;
}

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void * realloc(void * ptr, size_t size) {
    if (ptr == NULL)
        return NULL;
    void * ret_ptr = malloc(size);
    memcpy(ret_ptr, ptr, MIN(ptr_size(ptr), size));
    free(ptr);
    return ret_ptr;
}

void * calloc(size_t nmemb, size_t size) {
    void * ptr = malloc(nmemb * size);
    if (ptr == NULL)
        return NULL;
    return memset(ptr, 0, nmemb * size);
}

void crash() {
    puts("Crashed in allocator library\n");
    _exit(1);
}

int posix_memalign(void **memptr, size_t alignment, size_t size) { crash(); }
void *aligned_alloc(size_t alignment, size_t size) { crash(); }
void *valloc(size_t size) { crash(); }
void *memalign(size_t alignment, size_t size) { crash(); }
void *pvalloc(size_t size) {crash(); }

/*
int main() {
    char * str = (char *) malloc(11);
    strcpy(str, "testtest\0");
    puts((const char *) str);
    int i = 0;
    int n = strlen(str);
    printf("length = %d\n", n);
    putchar('c');
    for (i = 0; i < n; i++) {
        putchar(*(str + i));
    }
    free(str);

    return 0;
}
*/
