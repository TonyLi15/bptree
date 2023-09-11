/*
    B+tree implementation
    Template based on Professor Kawashima's code
    Extended by Haowen Li
*/

#include "bptree.h"
#include <vector>
#include <sys/time.h>
#include <thread>

#define DATA_SIZE 1000000
#define LEAF_MID ceil(N/2)
#define PARENT_MID ceil((N+1)/2)
vector<int> Data;

// Function declarations
struct timeval cur_time(void);
void print_performance(struct timeval begin, struct timeval end);
void init_vector(void);
void print_tree_core(NODE *n);
void print_tree(NODE *node);
NODE *alloc_leaf(NODE *parent);
NODE *alloc_internal(NODE *parent);
NODE *alloc_root(NODE *left, int rs_key, NODE *right);
NODE *find_leaf(NODE *node, int key);
NODE *insert_in_leaf(NODE *leaf, int key, DATA *data);
void insert_in_temp(TEMP *temp, int key, void *ptr);
void erase_entries(NODE *node);
void copy_from_leaf_to_temp(TEMP *temp, NODE *leaf);
void copy_leaf_left_half(TEMP *temp, NODE *leaf);
void copy_leaf_right_half(TEMP *temp, NODE *leaf);
void copy_parent_left_half(TEMP *temp, NODE *leaf);
void copy_parent_right_half(TEMP *temp, NODE *leaf);
void insert_to_parent(int key, NODE *parent, NODE *child, NODE *sibling);
void insert_to_temp_parent(int key, TEMP *parent, NODE *child, NODE *sibling);
void insert_in_parent(NODE *leaf, int key, NODE *new_leaf);
void insert(int key, DATA *data);
void init_root(void);
int interactive(void);
void search_core(const int key);
void search_single(void);
void search_range(int start, int end);
void *search_core(void* arg);
void search_multi_threaded(int thread_num);


// Function definitions
struct timeval cur_time(void){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv;
}

void print_performance(struct timeval begin, struct timeval end){
    double elapsed = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec) / 1000000.0;
    printf("Elapsed time: %.3lf sec\n", elapsed);

    long diff = (end.tv_sec - begin.tv_sec) * 1000 * 1000 + (end.tv_usec - begin.tv_usec);
	printf("%10.0f req/sec (lat:%7ld usec)\n", ((double)DATA_SIZE) / ((double)diff/1000.0/1000.0), diff);
}

void init_vector(void){
    Data.clear();
    for (int i = 0; i < DATA_SIZE; i++){
        int key = rand() % DATA_SIZE;
        Data.push_back(key);
    }
}

void print_tree_core(NODE *n){
    printf("[");
    for (int i = 0; i < n->nkey; i++){
        if (!n->isLeaf)
            print_tree_core(n->chi[i]);
        printf("%d", n->key[i]);
        if (i != n->nkey - 1 && n->isLeaf)
            putchar(' ');
    }
    if (!n->isLeaf)
        print_tree_core(n->chi[n->nkey]);
    printf("]");
}

void print_tree(NODE *node){
    if (node == NULL)
        cout << "Empty tree" << endl;
    else{
        print_tree_core(node);
        printf("\n");
        fflush(stdout);
    }
}

NODE *alloc_leaf(NODE *parent){
    NODE *leaf;
    if (!(leaf = (NODE *)calloc(1, sizeof(NODE)))) ERR;
    leaf->isLeaf = true;
    leaf->nkey = 0;
    leaf->parent = parent;
    return leaf;
}

NODE *alloc_internal(NODE *parent){
    NODE *internal;
    if (!(internal = (NODE *)calloc(1, sizeof(NODE)))) ERR;
    internal->isLeaf = false;
    internal->nkey = 0;
    internal->parent = parent;
    return internal;
}

NODE *alloc_root(NODE *left, int rs_key, NODE *right){
    NODE *root;
    if (!(root = (NODE *)calloc(1, sizeof(NODE)))) ERR;
    root->isLeaf = false;
    root->nkey = 1;
    root->parent = NULL;
    root->chi[0] = left;
    root->key[0] = rs_key;
    root->chi[1] = right;
    return root;
}

NODE *find_leaf(NODE *node, int key){
    int kid;

    if (node->isLeaf)
        return node;

    for (kid = 0; kid < node->nkey; kid++){
        if (key < node->key[kid])
            break;
    }

    return find_leaf(node->chi[kid], key);
}

NODE *insert_in_leaf(NODE *leaf, int key, DATA *data){
    int i;
    //if the key is smaller than the first key in the leaf node then insert the key at the beginning of the leaf node
    if (key < leaf->key[0]){
        //shift all the keys and pointers to the right
        for (i = leaf->nkey; i > 0; i--) {
			leaf->chi[i] = leaf->chi[i-1] ;
			leaf->key[i] = leaf->key[i-1] ;
		} 
        //insert the key at the beginning of the leaf node
        leaf->key[0] = key;
		leaf->chi[0] = (NODE *)data;
    }
    //else let i be the index of the key in the leaf node such that the key is smaller than the key at index i but larger than the key at index i-1
    else {
        //find the index i
        for(i = 0; i < leaf->nkey; i++){
            if (key < leaf->key[i])
                break;
        }
        //shift all the keys and pointers from index i to the right
        for (int j = leaf->nkey; j > i; j--){
            leaf->chi[j] = leaf->chi[j-1];
            leaf->key[j] = leaf->key[j-1];
        }
        //insert the key at index i
        leaf->key[i] = key;
        leaf->chi[i] = (NODE *)data;
    }
    //increment the number of keys in the leaf node
    leaf->nkey++;
    return leaf;
}

void insert_in_temp(TEMP *temp, int key, void *ptr){
    int i;
	if (key < temp->key[0]) {
		for (i = temp->nkey; i > 0; i--) {
			temp->chi[i] = temp->chi[i-1] ;
			temp->key[i] = temp->key[i-1] ;
		}
		temp->key[0] = key;
		temp->chi[0] = (NODE *)ptr;
	}
	else {
		for (i = 0; i < temp->nkey; i++) {
			if (key < temp->key[i]) 
                break;
		}
		for (int j = temp->nkey; j > i; j--) {
			temp->chi[j] = temp->chi[j-1] ;
			temp->key[j] = temp->key[j-1] ;
		}
		temp->key[i] = key;
		temp->chi[i] = (NODE *)ptr;
	}
	temp->nkey++;
}

void erase_entries(NODE *node){
    for (int i = 0; i < N; i++){
        node->chi[i] = NULL;
        if (i < N - 1)
            node->key[i] = 0;
    }
    node->nkey = 0;
}

void copy_from_leaf_to_temp(TEMP *temp, NODE *leaf){
    int i;
    for (i = 0; i < (N-1); i++) {
		temp->chi[i] = leaf->chi[i];
		temp->key[i] = leaf->key[i];
	} 
    temp->nkey = N-1;
	temp->chi[i] = leaf->chi[i];
}

void copy_leaf_left_half(TEMP *temp, NODE *leaf){
    for (int i = 0; i < (int)LEAF_MID; i++){
        leaf->chi[i] = temp->chi[i];
        leaf->key[i] = temp->key[i];
        leaf->nkey++;
    }
}

void copy_leaf_right_half(TEMP *temp, NODE *leaf){
    for (int i = (int)LEAF_MID; i < N; i++){
        leaf->chi[i - (int)LEAF_MID] = temp->chi[i];
        leaf->key[i - (int)LEAF_MID] = temp->key[i];
        leaf->nkey++;
    }
}

void copy_parent_left_half(TEMP *temp, NODE *leaf){
    for (int i = 0; i <= (int)PARENT_MID; i++){
        leaf->chi[i] = temp->chi[i];
        leaf->key[i] = temp->key[i];
        if (i < (int)PARENT_MID)
            leaf->nkey++;
    }
}

void copy_parent_right_half(TEMP *temp, NODE *leaf){
    int i;
    for (i = (int)PARENT_MID + 1; i <= N; i++){
        leaf->chi[i - ((int)PARENT_MID + 1)] = temp->chi[i];
        leaf->key[i - ((int)PARENT_MID + 1)] = temp->key[i];
        if (i < N)
            leaf->nkey++;
    }

    for (int i = 0; i <= leaf->nkey; i++) 
        leaf->chi[i]->parent = leaf;
}

void insert_to_parent(int key, NODE *parent, NODE *child, NODE *sibling){
    int i;
    for (i = 0; i <= parent->nkey; i++){
        if (parent->chi[i] == sibling){
            for (int j = parent->nkey; j > i; j--){
                parent->chi[j + 1] = parent->chi[j];
                parent->key[j] = parent->key[j - 1];
            }
            parent->key[i] = key;
            parent->chi[i + 1] = child;
        }
    }
    parent->nkey++;
}

void insert_to_temp_parent(int key, TEMP *parent, NODE *child, NODE *sibling){
    int i;
    for (i = 0; i <= parent->nkey; i++){
        if (parent->chi[i] == sibling){
            for (int j = parent->nkey; j > i; j--){
                parent->chi[j + 1] = parent->chi[j];
                parent->key[j] = parent->key[j - 1];
            }
            parent->key[i] = key;
            parent->chi[i + 1] = child;
        }
    }
    parent->nkey++;
}

void insert_in_parent(NODE *leaf, int key, NODE *new_leaf){
    //if the leaf node is the root then create a new root node and set the leaf node and new leaf node as the children of the new root node
    if(leaf == Root){
        Root = alloc_root(leaf, key, new_leaf);
        leaf->parent = new_leaf->parent = Root;
        return;
    }
    //let parent be the parent of the leaf node
    NODE *parent = leaf->parent;
    //if the parent is not full then insert the key into the parent node
    if (parent->nkey < N - 1){
        insert_to_parent(key, parent, new_leaf, leaf);
        new_leaf->parent = parent;
    }
    //If the parent is full then conduct internal split
    else {
        //create a temporary node and copy the keys and pointers from the parent node to the temporary node
        TEMP *parent_copy = (TEMP *)calloc(1, sizeof(TEMP));
        copy_from_leaf_to_temp(parent_copy, parent);
        //insert the key into the temporary node
        insert_to_temp_parent(key, parent_copy, new_leaf, leaf);
        //erase all the keys and pointers from the parent node
        erase_entries(parent);
        //let parent be the left which contains the first half of the keys and pointers in the temporary node
        copy_parent_left_half(parent_copy, parent);
        //create a new leaf node and copy the second half of the keys and pointers in the temporary node to the new leaf node
        NODE *new_parent = alloc_internal(parent->parent);
        copy_parent_right_half(parent_copy, new_parent);
        //insert the middle key of the temporary node into the parent node
        insert_in_parent(parent, parent_copy->key[(int)PARENT_MID], new_parent);
    }
}


void insert(int key, DATA *data){
    NODE *leaf;
    //if tree is empty then create an empty leaf node which is also the root
    if (Root == NULL){
        leaf = alloc_leaf(NULL);
		Root = leaf;
    }
    //else find the leaf node that should contain the key
    else
        leaf = find_leaf(Root, key);
    
    //if the leaf node is not full then insert the key into the leaf node
    if (leaf->nkey < N - 1)
        insert_in_leaf(leaf, key, data);
    //else split the leaf node and insert the key into the new leaf node
    else {
        //create a new leaf node that will contain the second half of the keys and pointers in the leaf node
        NODE *new_leaf = alloc_leaf(leaf->parent);
        
        //create a temporary node and copy the keys and pointers from the leaf node to the temporary node
        TEMP temp;
        copy_from_leaf_to_temp(&temp, leaf);

        //insert the key into the temporary node
        insert_in_temp(&temp, key, data);

        //set pointers of the leaf node and new leaf node
        new_leaf->chi[N - 1] = leaf->chi[N - 1];
        leaf->chi[N - 1] = new_leaf;

        //erase all the keys and pointers from the leaf node
        erase_entries(leaf);

        //let leaf be the left which contains the first half of the keys and pointers in the temporary node
        copy_leaf_left_half(&temp, leaf);

        //let thee new leaf node contain the second half of the keys and pointers in the temporary node
        copy_leaf_right_half(&temp, new_leaf);

        //insert the middle key of the temporary node into the parent node
        insert_in_parent(leaf, temp.key[(int)LEAF_MID], new_leaf);

    }
}

void init_root(void){
    Root = NULL;
}

int interactive(){
    int key;
    cout << "Enter key: ";
    cin >> key;
    return key;
}

void search_core(const int key){
	NODE *leaf = find_leaf(Root, key);
	for (int i = 0; i < leaf->nkey + 1; i++) {
		if (leaf->key[i] == key)
            return;
	}
	cout << "Key not found: " << key << endl;
}

void search_single(void){
    for (int i = 0; i < DATA_SIZE; i++){
        search_core(Data[i]);
    }
}

void search_range(int min, int max){
    for (int i = min; i < max; i++){
        search_core(Data[i]);
    }
}

void* search_core(void* arg) {
    int* indices = (int*)arg;
    int start = indices[0];
    int end = indices[1];
    
    for (int i = start; i < end; i++) {
        NODE *leaf = find_leaf(Root, Data[i]);
        bool found = false;
        for (int j = 0; j < leaf->nkey + 1; j++) {
            if (leaf->key[j] == Data[i]) {
                found = true;
                break;
            }
        }
        if (!found) {
            cout << "Key not found: " << Data[i] << endl;
        }
    }
    pthread_exit(NULL);
}

void search_multi_threaded(int thread_num) {
    pthread_t threads[thread_num];
    int indices[thread_num][2]; 

    int dataPerThread = DATA_SIZE / thread_num;

    for (int i = 0; i < thread_num; i++) {
        indices[i][0] = i * dataPerThread;
        indices[i][1] = (i == thread_num - 1) ? DATA_SIZE : indices[i][0] + dataPerThread;
        int rc = pthread_create(&threads[i], NULL, search_core, (void *)&indices[i]);
        if (rc) {
            cerr << "Error: Unable to create thread: " << rc << endl;
            exit(-1);
        }
    }

    for (int i = 0; i < thread_num; i++) {
        pthread_join(threads[i], NULL);
    }
}



// Main function
int main(int argc, char *argv[]){
    char cmd;
    init_root();
    
    /*
    //interactive mode
    while (true){
        cout << "Interactive Mode: [i]nsert, [s]earch, [d]elete, [p]rint, [q]uit" << endl;
        cin >> cmd;
        if (cmd == 'i'){
            printf("-----Insert-----\n");
            insert(interactive(), NULL);
        }   
        else if (cmd == 's'){
            printf("-----Search-----\n");
            search_core(interactive());
        }
        else if (cmd == 'd'){
            printf("-----Delete-----\n");
            //will complete later
        }
        else if (cmd == 'p'){
            print_tree(Root);
        }
        else if (cmd == 'q'){
            break;
        }
        else
            cout << "Invalid command" << endl;
    }
    */
    
    ///*
    //performance measurement mode
    init_vector();
    struct timeval begin, end;
    
    //if (argc == 2) 
    //    DATA_SIZE = atoi(argv[1]);
    
    
    while(true){
        cout << "Performance Measurement Mode: [i]nsert_dataset, [s]ingle_thread_search, [r]ange_search, [m]ulti_thread_search, [p]rint" << endl;
        cin >> cmd;
        if (cmd == 'i'){
            printf("-----Insert Dataset-----\n");
            begin = cur_time();
            for (int i = 0; i < DATA_SIZE; i++){
                insert(Data[i], NULL);
            }
            end = cur_time();
            print_performance(begin, end);
        }   
        else if (cmd == 's'){
            printf("-----Single thread search-----\n");
            begin = cur_time();
            for (int i = 0; i < DATA_SIZE; i++){
                search_core(Data[i]);
            }
            end = cur_time();
            print_performance(begin, end);
        }
        else if (cmd == 'r'){
            printf("-----Range search-----\n");
            int min, max;
            cout << "Enter start key: ";
            cin >> min;
            cout << "Enter end key: ";
            cin >> max;
            begin = cur_time();
            search_range(min, max);
            end = cur_time();
            print_performance(begin, end);
        }
        else if (cmd == 'm'){
            printf("-----Multi thread search-----\n");
            int thread_num;
            cout << "Enter number of threads: ";
            cin >> thread_num;
            begin = cur_time();
            search_multi_threaded(thread_num);
            end = cur_time();
            print_performance(begin, end);
            
        }
        else if (cmd == 'p'){
            cout << "Are you sure? Type [Y] to continue" << endl;
            cin >> cmd;
            if (cmd == 'Y')
                print_tree(Root);
        }
        else
            cout << "Invalid command" << endl;
    }
    //*/
    
    return 0;
}