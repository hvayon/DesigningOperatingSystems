#include "slab.h"

#define ECHO(x) printf(x);printf("\n");

int main()
{	
	/* адрес выделяемой вами памяти*/
	void *p;
	kmem_cache_t *cc;
	
	/* инициализация кэша начинается с 4, 4*2, 4*2^2, ..., 4*2^20 */
	kmem_cache_init(2, 4, 4*1024*1024);
	
	/* экземпляр кеша */
	cc = kmem_cache_create(4);
	
	/* выделение памяти */
	p = kmem_cache_alloc(cc);
	
	/*  
	* 1 для статистики 1-го кеша
	* 0 для статистики по всем остальным кешам
	 */
	ECHO("********* After allocation *********");
	statistic(1);
	//statistic(0);
	ECHO("");
	/* 
	 * тут использование памяти
	 */
	 
	
	/* освобождение */
	kmem_cache_free(cc,p);
	
	/* статистика кэша после освобождения памяти */
	ECHO("********* After free *********");
	statistic(1);
	//statistic(0);
	ECHO("");
	
	
	return 0;
}