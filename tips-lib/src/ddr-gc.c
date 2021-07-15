#include "tips.h"

int update_index_safe(int index)
{	
	if (unlikely(index + 1 >= MAX_BUCKET)) {
		index = 0;
		return index;
	}
	return ++index;	
}

void collect_ddr_entries(void *thread, uint64_t bucket)
{	
	tips_gc_thread *th = (tips_gc_thread *)thread;
	ddr_cache *d = th->d;
	unsigned int *chain_length = &(d->arr[bucket].chain_length);
	ddr_entry *iterator, *prev, *local_head;
	ddr_entry **bucket_head;
	int current_epoch;
	void **epoch_array;
	int *index;
	int n_entries_coll = 0;

	bucket_head = &(d->arr[bucket].head);
	local_head = *bucket_head;
	if (!local_head) {
		return;
	}
	/* set GC flag*/
	smp_atomic_store(&(d->arr[bucket].is_gc_detected), 1);
	iterator = local_head->next;
	prev = local_head;
	current_epoch = get_current_epoch();
	epoch_array = get_epoch_array(current_epoch);
	index = get_epoch_array_index(current_epoch);
	while (iterator) {
		/* skip the entry if it is not yet replied*/
		if (unlikely(iterator->status == F_NOT_REPLIED)) {
			prev = iterator;
			iterator = iterator->next;
			continue;
		}
		smp_atomic_store(&(prev->next), iterator->next);	
		/* add the replied entry in the epoch array*/
		epoch_array[*index] = iterator;
		*index = inc_epoch_index_safe(*index, current_epoch);
		++n_entries_coll;
		if (((*chain_length) - n_entries_coll) <= MAX_CHAIN_LENGTH) {
			smp_fas(chain_length, n_entries_coll);
			smp_atomic_store(&(d->arr[bucket].is_gc_detected), 0);
			smp_atomic_store(&(d->arr[bucket].is_gc_requested), 0);
			return;
		}
		iterator = iterator->next;
	}

	/* local_head: head of the bucket when gc thread starts
	 * we handle that element in the last to reduce the CAS traffic
	 * at the head to preserve the writer thread latency
	 * because when all the entries in the bucket are to be reclaimed then
	 * we have to keep updating the bucket head for each and every entry 
	 * which might result in CAS failure for writer thread
	 * since GC is a async operation traversing the bucket list is not
	 * costly, considering the limited number of entries in a bucket*/
	if (local_head->status == F_NOT_REPLIED) {
		smp_fas(chain_length, n_entries_coll);
		th->n_reclaimed += n_entries_coll;
		smp_atomic_store(&(d->arr[bucket].is_gc_detected), 0);
		smp_atomic_store(&(d->arr[bucket].is_gc_requested), 0);
		return;
	}
	if (likely(local_head == *bucket_head)) {
		smp_atomic_store(bucket_head, NULL);
		epoch_array[*index] = local_head;
		*index = inc_epoch_index_safe(*index, current_epoch);
		++n_entries_coll;
		smp_fas(chain_length, n_entries_coll);
		th->n_reclaimed += n_entries_coll;
		smp_atomic_store(&(d->arr[bucket].is_gc_detected), 0);
		smp_atomic_store(&(d->arr[bucket].is_gc_requested), 0);
		return;
	}
	iterator = *bucket_head;
	while (iterator != local_head) {
		prev = iterator;
		iterator = iterator->next;
	}
	smp_atomic_store(&(prev->next), iterator->next);	
	epoch_array[*index] = iterator;
	*index = inc_epoch_index_safe(*index, current_epoch);
	++n_entries_coll;
	smp_fas(chain_length, n_entries_coll);
	th->n_reclaimed += n_entries_coll;
	smp_atomic_store(&(d->arr[bucket].is_gc_detected), 0);
	smp_atomic_store(&(d->arr[bucket].is_gc_requested), 0);
	return;
}
void free_ddr_entries(unsigned int current_epoch) 
{
	unsigned int reclaim_epoch;
	void **epoch_array;
	int *epoch_index;

		reclaim_epoch = get_reclaim_epoch();
		epoch_array = get_epoch_array(reclaim_epoch);
		epoch_index = get_epoch_array_index(reclaim_epoch);	
		if (*epoch_index) {
			for (int i = 0; i < (*epoch_index); ++i) {
				dram_free(epoch_array[i]);
			}
		}
		*epoch_index = 0;	
	return;
}

void try_ddr_reclaim(void *thread)
{
	tips_gc_thread *th = (tips_gc_thread *)thread;
	tips_thread **thread_list = get_thread_list_addr();
	unsigned int run_cnt;
	unsigned int current_epoch;
	
	current_epoch = get_current_epoch();
	for (int i = 0; i < th->n_readers; ++i) {
		if (unlikely(!thread_list[i])) {
			continue;
		}
		run_cnt = thread_list[i]->run_cnt;
		if (run_cnt && run_cnt % 2 != 0) {
			if (unlikely(thread_list[i]->local_epoch 
						!= current_epoch))
				return;
		}
		continue;		
	}
	current_epoch = increment_epoch_counter();
	free_ddr_entries(current_epoch);
	return;
}

void *reclaim_ddr_cache(void *arg)
{
	tips_gc_thread *th = (tips_gc_thread *)arg;
	work_q *wq = th->d->q;
	uint64_t bucket;

Retry_GC:
	while (th->should_stop) {
		while ( wq->head != wq->tail) {
			wq->head = update_index_safe(wq->head);
			bucket = wq->arr[wq->head];
			collect_ddr_entries(th, bucket);
			try_ddr_reclaim(th);		
		}
		goto Retry_GC;
	}
//	printf("$$$$$$$$$$$$$$$$$$$$$$ GC exited loop $$$$$$$$$$$$$$$$$$$$$$$\n");
	return NULL;
}
