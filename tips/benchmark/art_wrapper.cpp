#include "art_wrapper.h"

void loadKey(TID tid, Key &key){
  return ;
}


 ART_ROWEX::Tree *art_tree;
 Key *end;
void *init_art_tree(){
  art_tree= new ART_ROWEX::Tree(loadKey);

  end = end->make_key(UINT64_MAX, sizeof(uint64_t), 0);
  return (void *)art_tree;
}

int tips_art_scan(void *p_root, uint64_t start_key, int range, uint64_t *buff){
	Key *start = start->make_key(start_key, sizeof(uint64_t), 0);

	Key *continueKey = NULL;
	Key *results[200];
	size_t resultsSize = range;
	size_t resultsFound =0;
	int i;
	uint64_t tmp;
	auto t = art_tree->getThreadInfo();
        art_tree->lookupRange(start, end, continueKey, results, resultsSize, 
			resultsFound, t);       	
	return resultsFound;

}
char *tips_art_read(void *p_head, uint64_t target_key){

    Key *key = key->make_key(target_key,sizeof(uint64_t),(uint64_t)0);
    auto t = art_tree->getThreadInfo();
    char *ret  =(char *) art_tree->art_lookup(p_head,key,t);
    return ret;
}

int tips_art_insert(void *p_head, uint64_t target_key,char *value){
    Key *key = key->make_leaf(target_key,sizeof(uint64_t),(uint64_t)value);
    auto t = art_tree->getThreadInfo();
    art_tree->art_insert(p_head,key,t);
    return 0;
}

int tips_art_remove(void *p_head, uint64_t target_key){
    Key *key = key->make_key(target_key,sizeof(uint64_t),(uint64_t)0);
    auto t = art_tree->getThreadInfo();
    art_tree->art_remove(p_head,key,t);
    return 0;
}




