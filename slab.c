#include "slab.h"

#include <stddef.h>

int kmem_cache_init(unsigned int growth_factor, unsigned int min_cache_size, unsigned int max_cache_size)
{
	unsigned int size = min_cache_size;
	kmem_cache_t *cc = NULL;
	int count = 1;
		
	if( min_cache_size < 0 
		|| max_cache_size < 0 
		|| min_cache_size > max_cache_size
	) return -1;

	cache_chain = (kmem_cache_t*)calloc(1, sizeof(kmem_cache_t));
	cc = cache_chain;
	
	/* инициализировать кеша */
	while(size <= max_cache_size) {
		
		/* инициализировать связанный список кеша */
		if(size != min_cache_size){
			cc->next = (kmem_cache_t*)calloc(1, sizeof(kmem_cache_t));
			cc = cc->next;
		}
		
		cc->size = size;
		cc->id = count++;
		cc->chunks = MAX_CHUNKS;
			
		cc->slabs_full_head    = (slabs_t *)calloc(1, sizeof(slabs_t));
		cc->slabs_partial_head = (slabs_t *)calloc(1, sizeof(slabs_t));
		cc->slabs_empty_head   = (slabs_t *)calloc(1, sizeof(slabs_t));

		cc->slabs = 10;
		for (int i = 0; i < 10; i++)
		{
			slab_queue_add(cc->slabs_empty_head, newslab_add(size));
		}

			
		size *= growth_factor;	
	}	
	
	return 0;
}

kmem_cache_t *kmem_cache_create(size_t size)
{
	kmem_cache_t *cc = cache_chain;
	
	while(cc->size < size){
		cc = cc->next;
	}
	
	return cc;
}

void *kmem_cache_alloc(kmem_cache_t *cp)
{
	slab_t* tmp_sl;

	if(cp == NULL) return ((void *)0);

	// Попытка найти свободный chunk в частично заполненных slab'ах
	tmp_sl = cp->slabs_partial_head->slab_head;

	while(tmp_sl)
	{
		// if (tmp_sl->free_chunks_begin == NULL)
		// 	tmp_sl = tmp_sl->next;
		{
			// Берём первый свободный chunk
			chunk_t *ret_chunk = tmp_sl->free_chunks_begin;

			// Обновляем указатель на следующий свободный chunk
			tmp_sl->free_chunks_begin = ret_chunk->next_chunk;
			// Добавляем выделенный chunk в начало списка занятых chunk'ов
			ret_chunk->next_chunk = tmp_sl->busy_chunks_begin;
			if (ret_chunk->next_chunk)
				ret_chunk->next_chunk->prev_chunk = ret_chunk; // вставляем в список

			// Устанавливаем prev_chunk выделенного chunk'а в NULL
			ret_chunk->prev_chunk = NULL;
			// Обновляем указатель на начало списка занятых chunk'ов
			tmp_sl->busy_chunks_begin = ret_chunk;

			// Если после выделения chunk'а slab стал полностью заполненным,
			// перемещаем его из списка частично заполненных в список полностью заполненных
			if (tmp_sl->free_chunks_begin == NULL)
			{
				slab_queue_remove(cp->slabs_partial_head, tmp_sl);
				slab_queue_add   (cp->slabs_full_head   , tmp_sl);
			}
			// Возвращаем данные выделенного chunk'а
			return ret_chunk->data;
		}
	}

	{
		tmp_sl = cp->slabs_empty_head->slab_head;

		if (tmp_sl == NULL)
			return NULL;

		chunk_t* ret_chunk = tmp_sl->free_chunks_begin;

		if (ret_chunk == NULL)
			return NULL;

		tmp_sl->free_chunks_begin = ret_chunk->next_chunk;
		ret_chunk->next_chunk = tmp_sl->busy_chunks_begin;
		if (ret_chunk->next_chunk)
			ret_chunk->next_chunk->prev_chunk = ret_chunk;

		ret_chunk->prev_chunk = NULL;

		tmp_sl->busy_chunks_begin = ret_chunk;

		slab_queue_remove(cp->slabs_empty_head  , tmp_sl);
		slab_queue_add   (cp->slabs_partial_head, tmp_sl);

		return ret_chunk->data;
	}

	/* нет свободного места */
	return NULL;

}

void *kmem_cache_free(kmem_cache_t *cp, void *buf)
{
	if (!buf)
		return NULL;


	chunk_t* chunk_ptr = (chunk_t*)((char*)buf - offsetof(chunk_t, data));

	slab_t* slab_ptr = chunk_ptr->slab_ptr;

	int was_full = slab_ptr->free_chunks_begin == NULL;

	if (chunk_ptr->prev_chunk)
		chunk_ptr->prev_chunk->next_chunk = chunk_ptr->next_chunk;
	else
	{
		slab_ptr->busy_chunks_begin = chunk_ptr->next_chunk;
		if (slab_ptr->busy_chunks_begin)
			slab_ptr->busy_chunks_begin->prev_chunk = NULL;
	}


	chunk_t* tmp = slab_ptr->free_chunks_begin;
	slab_ptr->free_chunks_begin = chunk_ptr;
	chunk_ptr->next_chunk = tmp;
	if (tmp)
	{
		tmp->prev_chunk = chunk_ptr;
	}

	if (was_full)
		slab_queue_remove(cp->slabs_full_head, slab_ptr);
	else
		slab_queue_remove(cp->slabs_partial_head, slab_ptr);

	if (slab_ptr->busy_chunks_begin == NULL)
		slab_queue_add(cp->slabs_empty_head, slab_ptr);
	else
		slab_queue_add(cp->slabs_partial_head, slab_ptr);
}

// Функция для создания нового slab'а (блока памяти) размером `size`
slab_t* newslab_add(unsigned int size)
{
	// Выделяем память под структуру slab_t и обнуляем её содержимое
	slab_t *sl = (slab_t *)calloc(1, sizeof(slab_t));

	if (!sl)
		return NULL; // Если выделение памяти не удалось, возвращаем NULL

	/* Инициализируем массив chunk'ов (кусков памяти), каждый из которых имеет размер
	   `sizeof(chunk_t)` плюс переданный размер `size` */
	sl->chunks = (char *)calloc(MAX_CHUNKS, sizeof(chunk_t) + size);

	// Указатель на начало свободных chunk'ов
	sl->free_chunks_begin = (chunk_t *)sl->chunks; // Все chunk'и пока свободны

	for (int i = 0; i < MAX_CHUNKS; i++) {
		// Получаем текущий chunk, смещая указатель на соответствующее количество байт
		chunk_t* cur_chunk = sl->chunks + (i) * (sizeof(chunk_t) + size);

		// Устанавливаем ссылку на предыдущий chunk, если он есть
		if (i >= 1)
			cur_chunk->prev_chunk = sl->chunks + (i - 1) * (sizeof(chunk_t) + size);

		// Устанавливаем ссылку на следующий chunk, если он есть
		if (i < MAX_CHUNKS - 1)
			cur_chunk->next_chunk = sl->chunks + (i + 1) * (sizeof(chunk_t) + size);

		// Сохраняем указатель на родительский slab
		cur_chunk->slab_ptr = sl;
	}

	// Возвращаем созданный slab
	return sl;
}

int slab_queue_add(slabs_t *sls, slab_t *sl)
{
	/* если очередь пустая */
	if(sls->slab_head == NULL){
		sls->slab_head = sl;
		sls->slab_tail = sl;
		sl->prev = NULL;
		sl->next = NULL;
		return 0;
	}
	
	/* иначе добавляем в хвост */ //check
	sls->slab_tail->next = sl;
	sl->prev = sls->slab_tail;
	sls->slab_tail = sl;
	sl->next = NULL;
	return 0;	
}

int slab_queue_remove(slabs_t *sls, slab_t *sl)
{
	if (sl->prev == NULL)
		sls->slab_head = sl->next;
	else
		sl->prev->next = sl->next;

	if (sl->next == NULL)
		sls->slab_tail = sl->prev;
	else
		sl->next->prev = sl->prev;

	return 0;
}

int slab_free(slabs_t *sls, slab_t *sl, kmem_cache_t *cc)
{
	int count;
	
	free(sl->chunks);
	
	free(sl);	
	cc->slabs--;
	
	return 0;	
}

void statistic(int id)
{
	const kmem_cache_t *cc = cache_chain;
	
	slab_t *sl;
	//int count;
	
	while(cc){
		if(cc->id != id && id != 0) {
			cc = cc->next;
			continue;		
		}
		
		printf("cache:%3d, size:%9d, slabs:%3d, chunks:%3d\n",cc->id,cc->size,cc->slabs,cc->chunks);
		
		// full
		printf("\t|Full slabs:\n");
		
		if(cc->slabs_full_head != NULL){
			sl = cc->slabs_full_head->slab_head;
			int i = 1;
			while(sl){
				printf("\t\t[%d]Slab:\n", i);
				//size_t real_chunk_size = sizeof(slab_t *) + cc->size;

				printf("\t\t\tFree chunk indexes: ");
				for (chunk_t *free_chunk = sl->free_chunks_begin; free_chunk; free_chunk = free_chunk->next_chunk)
					printf("%ld ", (char *)free_chunk - sl->chunks);

				printf("\n");

				printf("\t\t\tBusy chunk indexes: ");
				for (chunk_t* busy_chunk = sl->busy_chunks_begin; busy_chunk; busy_chunk = busy_chunk->next_chunk)
					printf("%ld ", (char *) busy_chunk - sl->chunks);

				printf("\n");
				//printf("%d chunks_used", sl->offset / real_chunk_size);
				printf("\n");
				sl = sl->next;
				i++;
			}
		}
		// partial
		printf("\t|Partial slabs:\n");
		
		if(cc->slabs_partial_head != NULL){ 
			sl = cc->slabs_partial_head->slab_head;
			int i = 1;
			while (sl) {
				printf("\t\t[%d]Slab:\n", i);
				//size_t real_chunk_size = sizeof(slab_t*) + cc->size;
				printf("\t\t\tFree chunk indexes: ");
				for (chunk_t* free_chunk = sl->free_chunks_begin; free_chunk; free_chunk = free_chunk->next_chunk)
					printf("%ld ", (char *) free_chunk - sl->chunks);

				printf("\n");

				printf("\t\t\tBusy chunk indexes: ");
				for (chunk_t* busy_chunk = sl->busy_chunks_begin; busy_chunk; busy_chunk = busy_chunk->next_chunk)
					printf("%ld ", (char *) busy_chunk - sl->chunks);

				printf("\n");
				printf("\n");
				sl = sl->next;
				i++;
			}
		}
		
		/* пустые не выводятся */
	
		cc = cc->next;
	}
}