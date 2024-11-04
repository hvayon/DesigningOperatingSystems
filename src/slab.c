#include "slab.h"

int kmem_cache_init(double growth_factor, unsigned int min_cache_size, unsigned int max_cache_size)
{
	unsigned int size = min_cache_size;
	kmem_cache_t *cc;
	int count = 1;
		
	if( min_cache_size < 0 
		|| max_cache_size < 0 
		|| min_cache_size > max_cache_size
	) return -1;
	
	/* инициализировать кеша */
	while(size <= max_cache_size) {
		
		/* инициализировать связанный список кеша */
		if(size == min_cache_size){
			cache_chain = (kmem_cache_t*)malloc(sizeof(kmem_cache_t));
			memset(cache_chain,0 ,sizeof(kmem_cache_t));
			
			cc = cache_chain;
			cc->size = size;
			cc->id = count++;
			cc->chunks = MAX_CHUNKS;
			
			cc->slabs_full_head= (slabs_t *)malloc(sizeof(slabs_t));
			cc->slabs_partial_head = (slabs_t *)malloc(sizeof(slabs_t));
			cc->slabs_empty_head = (slabs_t *)malloc(sizeof(slabs_t));
			
			size *= growth_factor;
			continue;
		}		
		
		
		cc->next = (kmem_cache_t *)malloc(sizeof(kmem_cache_t));
		cc = cc->next;
		memset(cc,0,sizeof(kmem_cache_t));
		cc->size = size;
		cc->id = count++;
		cc->chunks = MAX_CHUNKS;
		
		cc->slabs_full_head= (slabs_t *)malloc(sizeof(slabs_t));
		cc->slabs_partial_head = (slabs_t *)malloc(sizeof(slabs_t));
		cc->slabs_empty_head = (slabs_t *)malloc(sizeof(slabs_t));

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
	slab_t *sl;
	int count;
	slab_t* tmp_sl;

	if(cp == NULL) return ((void *)0);
	
	/* значение slab равно null, что означает, что это первый объект, сохраненный в этом slab */
	if(cp->slabs_partial_head->slab_head == NULL){
		sl = newslab_add(cp->size);
		
		/* нет доступного места */
		if(sl == NULL){
			return NULL;
		} 
		
		cp->slabs++;
		cp->slabs_partial_head->slab_head = sl;
		cp->slabs_partial_head->slab_tail = sl;
	}	
	
	tmp_sl = cp->slabs_partial_head->slab_head;
	
	while(tmp_sl){
		for(count = 0; count < MAX_CHUNKS; count++){
			if(tmp_sl->chunks[count].flag == false){
				memset(tmp_sl->chunks[count].value, 0, sizeof(cp->size));
				tmp_sl->chunks[count].flag = true;
				
				/* проверяем заполненность */
				if(slab_check_full(tmp_sl)){
					slab_queue_remove(cp->slabs_partial_head, tmp_sl);
					slab_queue_add(cp->slabs_full_head, tmp_sl);
				}
				
				return (void*)tmp_sl->chunks[count].value;
			}
		}
		tmp_sl = tmp_sl->next;
	}
	
	/* нет свободного места */
	return NULL;
	
}

void *kmem_cache_free(kmem_cache_t *cp, void *buf)
{	
	int count;
	slab_t *sl;
	
	/* проверяем slab_full_queue */
	if(cp->slabs_full_head != NULL){
		sl = cp->slabs_full_head->slab_head;
		while(sl){
			for(count = 0; count < MAX_CHUNKS; count++){
				if(buf == sl->chunks[count].value){
					sl->chunks[count].flag = false;
					
					/* переносим slab из full queue в partial queue */
					slab_queue_remove(cp->slabs_full_head,sl);
					slab_queue_add(cp->slabs_partial_head,sl);
					
					return ((void *)0);		
				}
			}
			sl = sl->next;
		}
	}
	
	/* проверка slab_partial_queue */
	if(cp->slabs_partial_head != NULL){
		sl = cp->slabs_partial_head->slab_head;
		while(sl){
			for(count = 0; count < MAX_CHUNKS; count++){
				if(buf == sl->chunks[count].value){
					sl->chunks[count].flag = false;
					
					/* 
					 * удаление slab из очереди
					 */
					if(slab_check_empty(sl)){
						
						slab_queue_remove(cp->slabs_partial_head,sl);
						slab_free(cp->slabs_partial_head,sl,cp);
					}					
					
					return ((void *)0);					
				}
			}
			sl = sl->next;
		}
	}
	
	return ((void *)0);
	
}

slab_t* newslab_add(unsigned int size)
{
	int count;
	
	slab_t *sl;
	
	sl = (slab_t*)malloc(sizeof(slab_t));
	if (!sl)
		return (NULL);
	
	/* инициализация кусков chunks */
	for(count = 0; count < MAX_CHUNKS; count++){
		sl->chunks[count].flag = false;
		sl->chunks[count].value = (char *)malloc(sizeof(size));
		memset(sl->chunks[count].value, 0, sizeof(size));
	}
	
	return sl;	
}

int slab_queue_add(slabs_t *sls, slab_t *sl)
{
	/* если очередь пустая */
	if(sls->slab_head == NULL){
		sls->slab_head = sl;
		sls->slab_tail = sl;
		sl->next = NULL;
		return 0;
	}
	
	/* иначе добавляем в хвост */ //check
	sls->slab_tail->next = sl;
	sls->slab_tail = sl;
	sl->next = NULL;
	return 0;	
}

int slab_queue_remove(slabs_t *sls, slab_t *sl)
{
	slab_t* tmp_sl;
	slab_t* tmp_sl2;
	
	/* только один slab */
	if(sls->slab_head == sls->slab_tail){
		sls->slab_head = NULL;
		sls->slab_tail = NULL;
		sl->next = NULL;
		return 0;
	}
	
	/* удаляем из головы */
	if(sls->slab_head == sl){
		sls->slab_head = sls->slab_head->next;
			
		sl->next = NULL;
		return 0;
	}
	
	tmp_sl = sls->slab_head;
	tmp_sl2 = tmp_sl;
	
	/* поиск элемента */
	while(tmp_sl != sl){
		tmp_sl2 = tmp_sl;
		tmp_sl = tmp_sl->next;
	}	
	
	/* sl это хвост */
	if(sl == sls->slab_tail){
		tmp_sl2->next = NULL;
		//sl->next = NULL;
		return 0;
	}
	
	/* иначе */
	tmp_sl2->next = sl->next;
	sl->next = NULL;
	return 0;
}

int slab_free(slabs_t *sls, slab_t *sl, kmem_cache_t *cc)
{
	int count;
	
	for(count = 0; count < MAX_CHUNKS; count++){
		free(sl->chunks[count].value);
	}
	
	free(sl);	
	cc->slabs--;
	
	return 0;	
}

bool slab_check_full(slab_t *sl){
	int count;
	bool res = true;
	
	for(count = 0; count < MAX_CHUNKS; count++){
		if(sl->chunks[count].flag == false){
			res = false;
		}
	}
	
	return res;
}

bool slab_check_empty(slab_t *sl){
	int count;
	bool res = true;
	
	for(count = 0; count < MAX_CHUNKS; count++){
		if(sl->chunks[count].flag == true){
			res = false;
		}
	}
	
	return res;
}

void statistic(int id)
{		
	kmem_cache_t *cc = cache_chain;
	
	slab_t *sl;
	int count;
	
	while(cc){
		if(cc->id != id && id != 0) {
			cc = cc->next;
			continue;		
		}
		
		printf("cache:%3d, size:%9d, slabs:%3d, chunks:%3d\n",cc->id,cc->size,cc->slabs,cc->chunks);
		
		// full
		printf("  |Full slabs:\n");
		
		if(cc->slabs_full_head != NULL){
			sl = cc->slabs_full_head->slab_head;
			while(sl){
				printf("    ||Slab:\n      chunk");
				for(count = 0; count < MAX_CHUNKS; count++){
					printf("[%d]:%s  ",count,sl->chunks[count].flag ? "used":"free");
				}
				printf("\n");
				sl = sl->next;
			}
		}
		// partial
		printf("  |Partial slabs:\n");
		
		if(cc->slabs_partial_head != NULL){ 
			sl = cc->slabs_partial_head->slab_head;
			while(sl){
				printf("    ||Slab:\n      chunk");
				for(count = 0; count < MAX_CHUNKS; count++){
					printf("[%d]:%s  ",count,sl->chunks[count].flag ? "used":"free");
				}
				printf("\n");
				sl = sl->next;
			}
		}
		
		/* пустые не выводятся */
	
		cc = cc->next;
	}
}