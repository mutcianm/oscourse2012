#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

#define PAGE_SIZE 4096

typedef struct free_block_t {
    size_t size;
    struct free_block_t * ptr;
} free_block_t;

typedef struct occupied_block_t {
    size_t size;
    char content[];
} occupied_block_t;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX3(x, y, z) MAX((x), MAX((y), (z)))
#define MIN3(x, y, z) MIN((x), MIN((y), (z)))

static free_block_t * head = NULL;

size_t round_up(size_t n, size_t mult) {
    if (n == 0)
        return mult;
    if (n > mult)
        _exit(1);
    else
        return mult;
    //return ((n - 1) / mult + 1) * mult;
}

size_t block_size(size_t size) {
    return MAX(sizeof(free_block_t), sizeof(occupied_block_t) + size);
}

void * malloc(size_t size) {
    free_block_t * curr_ptr = head;
    while (curr_ptr != NULL) {
        if (curr_ptr->size >= size) {
            break;
        }
        curr_ptr = curr_ptr->ptr;
    }

    if (curr_ptr == NULL) {
        size_t need_size = block_size(size);
        size_t temp_size = round_up(need_size, PAGE_SIZE);
        curr_ptr = (free_block_t *) mmap(NULL, temp_size,
                PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (curr_ptr == MAP_FAILED)
            return NULL;
        if (temp_size - need_size < block_size(1)) {
            need_size = temp_size;
        } else {
            char * next_ptr = ((char *) curr_ptr) + need_size;
            free_block_t * next_block_ptr = (free_block_t *) next_ptr;
            next_block_ptr->size = (temp_size - need_size);
            next_block_ptr->ptr = head;
            head = next_block_ptr;
        }
        curr_ptr->size = need_size;
        return (void *) (((occupied_block_t *) curr_ptr)->content);
    } else {
        size_t need_size = block_size(size);
        size_t temp_size = curr_ptr->size;
        if (temp_size - need_size < block_size(1)) {
            need_size = temp_size;
        } else {
            char * next_ptr = ((char *) curr_ptr) + need_size;
            free_block_t * next_block_ptr = (free_block_t *) next_ptr;
            next_block_ptr->size = (temp_size - need_size);
            next_block_ptr->ptr = head;
            head = next_block_ptr;
        }
        curr_ptr->size = need_size;
        return (void *) (((occupied_block_t *) curr_ptr)->content);
    }
}

void free(void * ptr) {
/*    if (ptr == NULL)
        return;
    size_t * size_ptr = ((size_t *) ptr) - 1;
    munmap((void *) size_ptr, *size_ptr);*/
    return;
}

occupied_block_t * occ_block_ptr(void * ptr) {
    return (occupied_block_t *) (((char *) ptr) - sizeof(occupied_block_t));
}

void * realloc(void * ptr, size_t size) {
    if (ptr == NULL)
        return NULL;
    void * ret_ptr = malloc(size);
    memcpy(ret_ptr, ptr, MIN(occ_block_ptr(ptr)->size, size));
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
void *pvalloc(size_t size) { crash(); }
/*int main() {
    printf("%d\n", sizeof(another_block_t));
}
*/
int main() {
    //char * str = (char *) malloc(11);
    char * str2 = (char *) malloc(200);
    //strcpy(str, "testtest\0");
    //puts((const char *) str);
    int i = 0;
    //int n = strlen(str);
    //printf("length = %d\n", n);
    putchar('c');
    //for (i = 0; i < n; i++) {
     //   putchar(*(str + i));
    //}
    for (i = 0; i < 2 * PAGE_SIZE + 2100; i++) {
        str2[i] = 'c';
    }
    puts(str2);
    //free(str);
    free(str2);

    return 0;
}
