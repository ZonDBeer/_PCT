#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <strings.h>
#include <string.h>
#include <sys/time.h>

#define LIST_SIZE 1000
#define NUMBER_THREADS 4
#define SMTH 4

pthread_t threads[NUMBER_THREADS]; // переменная количества потоков
pthread_mutex_t hashlock = PTHREAD_MUTEX_INITIALIZER; // инициализируем мьютеус
pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER; // read write lock
pthread_rwlock_t wlock = PTHREAD_RWLOCK_INITIALIZER;	// only write lock

double p_time = 0;


// структура элемента
struct element {
	int value;
	struct element *next;
};


// структура данных
struct inf {
	int value;
	int thread_number;
};


struct element *head; // указатель на голову


// возвращает случайное число между min и max
int getrand(int min, int max)
{
    return (double)rand() / (RAND_MAX + 1.0) * (max - min) + min;
}

// замеряет время
double wtime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (double)t.tv_sec + (double)t.tv_usec * 1E-6;
}


// вставка элемента в начало
void *addfront(void *d)
{
	struct inf *elem = (struct inf *) d;
	int value = elem->value;
	
	struct element *p;
	p = malloc(sizeof(*p));
	
	pthread_rwlock_wrlock(&rwlock);
	if (p != NULL) {
		p->value = value;
		p->next = head;
	}
	head = p;
	pthread_rwlock_unlock(&rwlock);
	return (void *) 0;
}


// вставка элемента в конец
void *addback(void *d)
{
	struct inf *elem = (struct inf *) d;
	int value = elem->value;
	struct element *p;
	p = malloc(sizeof(*p));
	
	pthread_rwlock_wrlock(&wlock);
	if (p != NULL) {
		p->value = value;
		p->next = NULL;
	}
	
	struct element *tmp = head;
	
	while (tmp->next != NULL) {
		tmp = tmp->next;
	}
	tmp->next = p;
	pthread_rwlock_unlock(&wlock);
	return (void *) 0;
}

// удаление первого элемента
void *delfirst(void *d)
{
	if (head == NULL)
		return (void *) 0;
	pthread_rwlock_wrlock(&rwlock);
	struct element *tmp;
	tmp = malloc(sizeof(*tmp));
	tmp = head->next;
	head = tmp;
	pthread_rwlock_unlock(&rwlock);
	return (void *) 0;
}

// удаление последнего элемента
void *dellast(void *d)
{
	if (head == NULL)
		return (void *) 0;
	pthread_rwlock_wrlock(&rwlock);
	struct element *tmp = head;
	while (tmp->next->next != NULL) {
		tmp = tmp->next;
	}
	free(tmp->next->next);
	tmp->next = NULL;
	pthread_rwlock_unlock(&rwlock);
	return (void *) 0;
}

// мультипоточная рандомная функция
void *list_multithread(void *d)
{
	struct inf *elem = (struct inf *) d;
	int num = elem->thread_number;
	
	double t_time = wtime();
	int i;
	
	for (i = 0; i < LIST_SIZE / NUMBER_THREADS; ++i) {
		struct inf *d = malloc(sizeof(*d));
		d->value = i;

		int rd = getrand(0, SMTH);

		switch (rd) {
		case 0:
			addfront(d);
			break;
		case 1:
			addback(d);
			break;
		case 2:
			delfirst(d);
			break;
		case 3:
			dellast(d);
			break;
		}
		free(d);
	}
	if (num == 0)
		p_time = wtime() - p_time;
	t_time = wtime() - t_time;
	printf("Время работы потока |%d| : |%f|\n Скорость: %.0f oper/sec\n", num + 1, t_time, (LIST_SIZE/NUMBER_THREADS)/t_time);
	return (void *) 0;
}





int main()
{
	head = NULL;
	int i = 0, err = 0;
	printf(" Begin to |%d| threads:\n", NUMBER_THREADS);
	
	for (i = 0; i < 100; ++i) {
		struct inf *d = malloc(sizeof(*d));
		d->value = getrand(0, 100);
		addfront(d);	
		free(d);
	}
	
	
	double t = wtime();
	
	// создаем потоки
	for (i = 0; i < NUMBER_THREADS; ++i) {
		struct inf *d = malloc(sizeof(*d));
		d->thread_number = i;
		if ((err = pthread_create(&threads[i], NULL, list_multithread, d)) != 0)	// порождение потока
			exit(1);
		if (NUMBER_THREADS - i == 1)
			p_time = wtime();
		//printf(" Был создан поток |%d|\n", i + 1);
	}

	// собираем потоки
	for (i = 0; i < NUMBER_THREADS; ++i) {
		if ((err = pthread_join(threads[i], NULL)) != 0) // ожидаем завершения потока
			exit(1);
		//printf(" Был поднят поток |%d|\n", i + 1);
	}
	
	t = wtime() - t;
	printf(" Elapsed time: %.6f sec.\n", p_time);
	
	return 0;
}
