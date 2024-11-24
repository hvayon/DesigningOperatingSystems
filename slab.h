#ifndef SLAB_H
#define SLAB_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
//#include <sys/queue.h>

#define MAX_CHUNKS 5

typedef struct slab slab_t;

typedef struct chunk
{
	slab_t *slab_ptr;
	struct chunk *prev_chunk;
	struct chunk *next_chunk;
	char data[];
} chunk_t;

/* из определённого количества кусков получается slab */
typedef struct slab {
	chunk_t *busy_chunks_begin;
	chunk_t *free_chunks_begin;
	char* chunks;
	struct slab *next;
	struct slab *prev;
}				slab_t;

/* структура управления плитами */
typedef struct slabs {
	slab_t *slab_head;
	slab_t *slab_tail;
	struct slabs *next;
} 				slabs_t;

/* структура управления кешем */
typedef struct kmem_cache_s {
	unsigned int id;			
    unsigned int size;
    unsigned int chunks;   		/* сколько кусков в одной плите */
	unsigned int slabs;     	/* сколько плит в данный момент выделено (full + partial) */
		
    slabs_t *slabs_full_head;	/* указатель на список структур */
	slabs_t *slabs_partial_head;
	slabs_t *slabs_empty_head;
	
	struct kmem_cache_s *next;
	
}kmem_cache_t;


/* Очередь на кэширование, все начинается здесь. */
kmem_cache_t *cache_chain;

/** 
 * Инициализаация кеша памяти.
 * 
 * 1-й аргумент - коэффициент роста. Первый кэш будет содержать
 * блоки min_cahce_size (2-й аргумент); в каждом кэше будет использоваться блок размером, равным 
 * к размеру блока предыдущего кэша, умноженному на коэффициент роста; до тех пор, пока размер блоков 
 * не достигнет max_cache_size (3-й аргумент). 
 * Ожидаемый коэффициент должен быть как минимум больше 1.
 * Единица измерения: байт.
 * Возврат: 0 - успех, -1 - сбой
 */
int kmem_cache_init(unsigned int growth_factor, unsigned int min_cache_size, unsigned int max_cache_size);

/**
 * Задается размер объекта, возвращается адрес кеша
 * мб добавить выравнивание
*/
kmem_cache_t *kmem_cache_create(size_t size);

/* выделите память для объекта, возвращаемого с помощью kmem_Cache_create(...) 
 * Возвращаемое значение: доступный адрес или NULL. 
 */
void *kmem_cache_alloc(kmem_cache_t *cp);

/* свободная память объекта, возвращаемая kmem_Cache_create(...) */
void *kmem_cache_free(kmem_cache_t *cp, void *buf);

/* добавление нового slab */
slab_t* newslab_add(unsigned int size);

/* добавление/удаление из очереди
 * 1-й аргумент указывает на адрес начала очереди. Эта очередь может быть любой 
 * slabs_full_queue, или slabs_partial_queue, или slabs_empty_queue
 * 2-й аргумент указывает адрес элемента, ожидающего добавления/удаления.
 *  */
int slab_queue_add(slabs_t *sls, slab_t *sl);
int slab_queue_remove(slabs_t *sls, slab_t *sl);

int slab_free(slabs_t *sls, slab_t *sl,kmem_cache_t *cc);

void statistic(int id);

#endif
