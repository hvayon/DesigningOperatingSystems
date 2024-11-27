#include "slab.h"

#define ECHO(x) printf(x);printf("\n");fflush(0);

void print_cache_free(kmem_cache_t* cc)
{
	while (cc)
	{
		ECHO("********* After free *********");
		statistic(cc->id);
		//statistic(0);
		ECHO("");
		cc = cc->next;
	}
}
// занимаем два slabs (chunks = 6)
void test1() {
	void* p[MAX_CHUNKS + 1];
	int min_cache_size = 4;
	int cache_size = 8;


	int cache_id = cache_size/ min_cache_size;

	/* инициализация кэша начинается с 4, 4*2, 4*2^2, ..., 4*2^20 */
	kmem_cache_init(2, min_cache_size, 4*1024*1024); //4*1024*1024

	kmem_cache_t* cc = kmem_cache_create(cache_size);

	for (int i = 0; i < MAX_CHUNKS + 1; i++)
	{
		p[i] = kmem_cache_alloc(cc);
		statistic(cache_id);
	}
	ECHO("*********  start free ********* ");
	for (int i = 0; i < MAX_CHUNKS + 1; i++)
	{
		kmem_cache_free(cc, p[MAX_CHUNKS - i]);
		statistic(cache_id);
	}

	ECHO("********* After free *********");
	statistic(cache_id);
	ECHO("");;
}

// полное заполнение кеша
void test2()
{
	void* p[100];

	int min_cache_size = 4;
	/* инициализация кэша начинается с 4, 4*2, 4*2^2, ..., 4*2^20 */
	kmem_cache_init(2, min_cache_size, 8); //4*1024*1024

	int cache_size = 4;
	/* экземпляр кеша */
	kmem_cache_t* cc = kmem_cache_create(cache_size);

	kmem_cache_t *tmp = cc;

	int count = 0;
	int cache_id = 1;
	while(cc) {
		for (int i = 0; i < 10; i++) {
		// и все чанки внутри
			for (int j = 0; j < MAX_CHUNKS; j++)
			{
				p[count] = kmem_cache_alloc(cc);
				count++;
			}
			statistic(cache_id);
		}
		//void* res = kmem_cache_alloc(cc); // тут NULL
		cc = cc->next;
		cache_id++;
	}

	cc = tmp;
	count = 0;
	cache_id = 1;

	while(cc) {
		for (int i = 0; i < 10; i++) {
			// и все чанки внутри
			for (int j = 0; j < MAX_CHUNKS; j++)
			{
				kmem_cache_free(cc, p[count]);
				count++;
			}
			statistic(cache_id);
		}
		//void* res = kmem_cache_alloc(cc); // тут NULL
		cc = cc->next;
		cache_id++;
	}
	print_cache_free(tmp);
}

void test3()
{
		void* p[MAX_CHUNKS + 1];
		int min_cache_size = 4;
		int cache_size = 8;


		int cache_id = cache_size/ min_cache_size;

		/* инициализация кэша начинается с 4, 4*2, 4*2^2, ..., 4*2^20 */
		kmem_cache_init(2, min_cache_size, 4*1024*1024); //4*1024*1024

		kmem_cache_t* cc = kmem_cache_create(cache_size);

		for (int i = 0; i < MAX_CHUNKS + 1; i++)
		{
			p[i] = kmem_cache_alloc(cc);
			statistic(cache_id);
		}
		ECHO("*********  start free ********* ");
		for (int i = 0; i < MAX_CHUNKS + 1; i++)
		{
			kmem_cache_free(cc, p[MAX_CHUNKS - i]);
			statistic(cache_id);
		}

		ECHO("********* After free *********");
		statistic(cache_id);
		ECHO("");
}
int main()
{
	 test1();
	 test2();
	 test3();
	
	
	return 0;
}