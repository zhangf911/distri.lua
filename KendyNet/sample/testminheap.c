#include "minheap.h"
#include "kn_time.h"

struct ele{
	struct heapele base;
	int value;
};

static int8_t less(struct heapele*l,struct heapele*r){
	return ((struct ele*)l)->value < ((struct ele*)r)->value;
}
int main(){
	
	minheap_t m = minheap_create(8192,less);
	struct ele eles[11];
	int i = 0;
	srand(kn_systemms());
	for(; i < 10; ++i){
		eles[i].value = rand();
		eles[i].base.index = 0;
		minheap_insert(m,(struct heapele*)&eles[i]);
	}
	minheap_remove(m,(struct heapele*)&eles[7]);
	eles[10].value = rand();
	eles[10].base.index = 0;
	minheap_insert(m,(struct heapele*)&eles[10]);
	struct ele *e;
	while((e = (struct ele*)minheap_popmin(m))){
		printf("%d\n",e->value);
	}	

	/*struct ele *e = calloc(1,sizeof(*e));
	e->value = 1;
	minheap_insert(m,(struct heapele*)e);
	minheap_remove(m,(struct heapele*)e);
	e = (struct ele*)minheap_popmin(m);*/


	/*for(;;){
		srand(kn_systemms());
		int i = 0;
		for(; i < 1000; i++){
			struct ele *e = calloc(1,sizeof(*e));
			e->value = rand();
			minheap_insert(m,(struct heapele*)e);
		}
		int value = 0;
		for(i = 0; i < 1000; ++i){
			struct ele *e = (struct ele*)minheap_popmin(m);
			if(value <= e->value){
				value = e->value;
			}else{
				printf("error:%d,%d,%d\n",value,e->value,i);
				exit(0);
			}
			free(e);
		}
		if(m->size != 0){
			printf("error1\n");
			exit(0);
		}
		kn_sleepms(1);
	}*/
	return 0;
}