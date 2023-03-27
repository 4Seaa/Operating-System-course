#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t* q) {
	return (q->size == 0);
}

void enqueue(struct queue_t* q, struct pcb_t* proc) {
	/* TODO: put a new process to queue [q] */
	if (empty(q)) {//Empty
		q->proc[0] = proc;
		q->size += 1;
	}
	else {//Not empty - Note: Đang phân vân ko biết có trường hợp nào queue full mà vẫn còn proc cần enqueue ko, tạm thời chưa làm case này
		q->proc[q->size] = proc;
		q->size += 1;
	}
}

struct pcb_t* dequeue(struct queue_t* q) {
	/* TODO: return a pcb whose prioprity is the highest
	 * in the queue [q] and remember to remove it from q
	 * */
	if (empty(q)) {//Empty
		return NULL;
	}
	else {//Not empty
		uint32_t max = 0;
		struct pcb_t* temp;
		for (int i = 0; i < q->size; i++) {//Gán temp = proc có highest priority
			if (q->proc[i]->priority >= max) {
				max = q->proc[i]->priority;
				temp = q->proc[i];
			}
		}
		if (temp == q->proc[q->size - 1]) {//Proc có highest priority nằm ở cuối queue
			q->proc[q->size - 1] = NULL;
			q->size -= 1;
			return temp;
		}
		else {//Proc có highest priority ko nằm ở cuối queue
			int reach = 0;
			for (int i = 0; i < q->size; i++) {//Di chuyển tất cả các phần tử nằm sau proc có highest priority lùi lại một đơn vị vị trí
				if (q->proc[i] == temp) {
					q->proc[i] = NULL;
					reach = 1;
				}
				if (reach == 1) {
					q->proc[i] = q->proc[i + 1];
				}
			}
			q->size -= 1;
			return temp;
		}
	}

}

