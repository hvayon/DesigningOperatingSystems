#include "slab.h"

#define ECHO(x) printf(x);printf("\n");fflush(0);

int main()
{	
	/* адрес выделяемой вами памяти*/
	void* p[MAX_CHUNKS + 1];
	kmem_cache_t *cc;
	
	/* инициализация кэша начинается с 4, 4*2, 4*2^2, ..., 4*2^20 */
	kmem_cache_init(2, 4, 4*1024*1024);
	
	/* экземпляр кеша */
	cc = kmem_cache_create(8);
	
	/* выделение памяти */
	statistic(2);
	p[0] = kmem_cache_alloc(cc);
	statistic(2);
	p[1] = kmem_cache_alloc(cc);
	statistic(2);
	p[2] = kmem_cache_alloc(cc);
	statistic(2);
	p[3] = kmem_cache_alloc(cc);
	statistic(2);
	p[4] = kmem_cache_alloc(cc);
	statistic(2);
	p[5] = kmem_cache_alloc(cc);
	statistic(2);
	
	//statistic(0);
	ECHO("");
	/* 
	 * тут использование памяти
	 */
	 
	
	/* освобождение */
	kmem_cache_free(cc, p[0]);
	statistic(2);
	kmem_cache_free(cc, p[1]);
	statistic(2);
	kmem_cache_free(cc, p[2]);
	statistic(2);
	kmem_cache_free(cc, p[3]);
	statistic(2);
	kmem_cache_free(cc, p[4]);
	statistic(2);
	kmem_cache_free(cc, p[5]);
	
	/* статистика кэша после освобождения памяти */
	ECHO("********* After free *********");
	statistic(2);
	//statistic(0);
	ECHO("");
	
	
	return 0;
}