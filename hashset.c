/*hashset.c - solution for EC project
 *
 *Hash implementation of a Set ADT
 *
 *Author: Eliot Martin
 */
#include "hashset.h"  /* the .h file does NOT reside in /usr/local/include/ADTs */
#include <stdlib.h>
/* any other includes needed by your code */
#define UNUSED __attribute__((unused))
#define DEFAULT_CAPACITY 16
#define MAX_CAPACITY 134217728L
#define DEFAULT_LOAD_FACTOR 0.75
#define TRIGGER 100

typedef struct node {
	struct node *next;
	void *value;
}Node;

typedef struct s_data {
	/* definitions of the data members of self */
	long (*hash)(void *, long n);
	int (*cmp)(void *, void *);
	long size;
	long capacity;
	long changes;
	double load;
	double loadFactor;
	double increment;
	Node **buckets;
	void (*freeV)(void *v);
} SData;

/*
 * important - remove UNUSED attributed in signatures when you flesh out the
 * methods
 */

static void purge(SData *sd, void (*freeV)(void *v)) {
	long i;

	for (i=0L; i < sd->capacity; i++) {
		Node *p, *q;
		p = sd->buckets[i];
		while (p != NULL) {
			if (freeV != NULL) {
				(*freeV)(p->value);
			}
			q = p->next;
			free(p);
			p = q;
		}
		sd->buckets[i] = NULL;
	}
}

static void s_destroy(const Set *s) {
	/* implement the destroy() method */
	SData *sd = (SData *)s->self;
	purge(sd, sd->freeV);
	free(sd->buckets);
	free(sd);
	free((void *)s);
}

static void s_clear(const Set *s) {
	/* implement the clear() method */
	SData *sd = (SData *)s->self;
	purge(sd, sd->freeV);
	sd->size = 0;
	sd->load = 0;
	sd->changes = 0;
}

static void resize(SData *sd) {
	int N;
	Node *p, *q, **array;
	long i, j;

	N = 2 * sd->capacity;
	if (N > MAX_CAPACITY)
		N = MAX_CAPACITY;
	if (N == sd->capacity)
		return;
	array = (Node **)malloc(N * sizeof(Node *));
	if (array == NULL)
		return;
	for (j = 0; j < N; j++)
		array[j] = NULL;
	for (i = 0; i < sd->capacity; i++) {
		for (p = sd->buckets[i]; p != NULL; p = q) {
			q = p->next;
			j = sd->hash(p->value, N);
			p->next = array[j];
			array[j] = p;
		}
	}
	free(sd->buckets);
	sd->buckets = array;
	sd->capacity = N;
	sd->load /= 2.0;
	sd->changes = 0;
	sd->increment = 1.0 / (double)N; 
}

static Node *findMember(SData *sd, void *member, long *bucket) {
	long i = sd->hash(member, sd->capacity);
	Node *p;
	*bucket = i;
	for (p = sd->buckets[i]; p != NULL; p = p->next) {
		if (sd->cmp(p->value, member) == 0) {
			break;
		}
	}
	return p;
}

static int insertNode(SData *sd, void *member, long i) {
	Node *p = (Node *)malloc(sizeof(Node));
	int ans = 0;

	if (p != NULL) {
		p->value = member;
		p->next = sd->buckets[i];
		sd->buckets[i] = p;
		sd->size++;
		sd->load += sd->increment;
		sd->changes++;
		ans = 1;
	}
	return ans;
}

static int s_add(const Set *s, void *member) {
	/* implement the add() method */
	SData *sd = (SData *)s->self;
	long i;
	Node *p;
	int ans = 0;

	if (sd->changes > TRIGGER) {
		sd->changes = 0;
		if (sd->load > sd->loadFactor)
			resize(sd);	
	}
	p = findMember(sd, member, &i);
	if (p == NULL){
		ans = insertNode(sd, member, i);
	}
	return ans;
}

static int s_contains(const Set *s, void *member) {
	/* implement the contains() method */
	SData *sd = (SData *)s->self;
	long l;
	return (findMember(sd, member, &l) != NULL);
}

static int s_isEmpty(const Set *s) {
	/* implement the isEmpty() method */
	SData *sd = (SData *)s->self;
	return (sd->size == 0L);
}

static int s_remove(const Set *s, void *member) {
	/* implement the remove() method */
	SData *sd = (SData *)s->self;
	long i;
	Node *remove;
	int ans = 0;
	
	remove = findMember(sd, member, &i);
	if (remove != NULL) {
		Node *p, *c;
		for (p = NULL, c = sd->buckets[i]; c != remove; p = c, c = c->next)
		       ;
		if (p == NULL)
			sd->buckets[i] = remove->next;
		else 
			p->next = remove->next;
		sd->size--;
		sd->load-=sd->increment;
		sd->changes++;
		if (sd->freeV != NULL)
			sd->freeV(remove->value);
		free(remove);
		ans = 1;
	}
	return ans;
}

static long s_size(const Set *s) {
	/* implement the size() method */
	SData *sd = (SData *)s->self;
	return (sd->size);
}

static void **valueArray(SData *sd) {
	void **tmp = NULL;
	if (sd->size > 0L) {
		size_t nbytes = sd->size * sizeof(void *);
		tmp = (void *)malloc(nbytes);
		if (tmp != NULL) {
			long i, n = 0L;
			for (i = 0l; i < sd->capacity; i++) {
				Node *p = sd->buckets[i];
				while (p != NULL){
					tmp[n++] = p->value;
					p = p->next;
				}
			}
		}
	}
	return tmp;
}

static void **s_toArray(const Set *s, long *len) {
	/* implement the toArray() method */
	SData *sd = (SData *)s->self;
	void **tmp = valueArray(sd);
	if (tmp != NULL)
		*len = sd->size;
	return tmp;
}

static const Iterator *s_itCreate(const Set *s) {
	/* implement the itCreate() method */
	SData *sd = (SData *)s->self;
	const Iterator *it = NULL;
	void **tmp = (void **)valueArray(sd);
	if (tmp != NULL) {
		it = Iterator_create(sd->size, tmp);
		if (it == NULL)
			free(tmp);
	}
	return it;
}

static Set template = {
	NULL, s_destroy, s_clear, s_add, s_contains, s_isEmpty, s_remove,
	s_size, s_toArray, s_itCreate
};

static const Set *newSet(long capacity, double loadFactor, long (*hash)(void*, long), int (*cmp)(void*, void*), void (*freeV)(void *)) {
	Set *s = (Set *)malloc(sizeof(Set));
	long N;
	double lf;
	Node **array;
	long i;
	if (s != NULL) {
		SData *sd = (SData *)malloc(sizeof(SData));
		if (sd != NULL){
			N = ((capacity > 0)  ? capacity : DEFAULT_CAPACITY);
			if (N > MAX_CAPACITY)
				N = MAX_CAPACITY;
			lf = ((loadFactor > 0.000001) ? loadFactor : DEFAULT_LOAD_FACTOR);
			array = (Node **)malloc(N * sizeof(Node *));
			if (array != NULL) {
				sd->capacity = N;
				sd->loadFactor = lf;
				sd->size = 0L;
				sd->load = 0.0;
				sd->changes=0L;
				sd->increment = 1.0/(double)N;
				sd->hash = hash;
				sd->cmp = cmp;
				sd->freeV = freeV;
				sd->buckets = array;
				for (i = 0; i < N; i++)
					array[i] = NULL;
				*s = template;
				s->self = sd;
			}
			else {
				free(sd);
				free(s);
				s = NULL;
			}
		}
		else {
			free(s);
			s = NULL;
		}
	}
	return s;
}

const Set *HashSet(void (*freeValue)(void*), int (*cmpFxn)(void*, void*),
                   long capacity, double loadFactor,
                   long (*hashFxn)(void *m, long N)
                  ) {
	/* construct a Set instance and return to the caller */
	return newSet(capacity, loadFactor, hashFxn, cmpFxn, freeValue);
}
