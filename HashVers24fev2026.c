#include <stdio.h>
#include <stdlib.h>

/* necessary header file - PMDK */
#include <libpmemobj.h>

/*
 * The comments below the new sentences are the volatile equivalent.
 *
 */

#define MAX_SIZE 5
#define EMPTY 0
#define OCUPADO 1
#define DELETED 2

#define CLEAN 0
#define DIRTY 1

/* We usually put the root information inside a struct */
struct my_root {
  PMEMoid p_Hash;
};

struct Hash {
  int clean;
  int size;
  int valor [MAX_SIZE];
  int occupied [MAX_SIZE];
};

typedef struct Hash HASH;


#define LAYOUT_NAME "hash"

//funcao para imprimir hash
void display(PMEMoid p_Hash){
  printf ("Hash = ");

  for (int i = 0; i < MAX_SIZE; i++){
    
    if (((HASH *)pmemobj_direct(p_Hash))->occupied[i] == OCUPADO){
      printf ("%d ", ((HASH *)pmemobj_direct(p_Hash))->valor[i]);
    }

    else{
      printf ("* ");
    }
  
  }
  
    printf ("\n");
}

//funcao de calculo da posicao do hash
int funcaoHash (int dado){
  return (dado*dado) % MAX_SIZE;
}

//funcao para inicializar hash
void start_hash(PMEMobjpool *pop, PMEMoid *p_Hash){

    TX_BEGIN(pop){
      //TX_ADD(p_Hash); duvida

      PMEMoid p_newHash = pmemobj_tx_alloc(sizeof(HASH), 1);

      ((HASH *)pmemobj_direct(p_newHash))->size = 0;

      for (int i = 0; i < MAX_SIZE; i++){
        ((HASH *)pmemobj_direct(p_newHash))->valor[i] = -1;
        ((HASH *)pmemobj_direct(p_newHash))->occupied[i] = EMPTY;
      }

      *p_Hash = p_newHash;

  } TX_END

}

//funcao inserir
void insert (PMEMobjpool *pop, PMEMoid p_aux, int dado){

  if (((HASH *)pmemobj_direct(p_aux))->size >= MAX_SIZE) {
    printf("Hash cheio\n");
    return;
  }

  else{
      TX_BEGIN(pop) {

        int posicao = funcaoHash(dado);

        while (((HASH *)pmemobj_direct(p_aux))->occupied[posicao] == OCUPADO){
          posicao = posicao + 1;
          posicao = posicao % MAX_SIZE;
        }

        ((HASH *)pmemobj_direct(p_aux))->valor[posicao] = dado;
        ((HASH *)pmemobj_direct(p_aux))->occupied[posicao] = OCUPADO;

        ((HASH *)pmemobj_direct(p_aux))->size++;
    } TX_END
  }
  
}

//funcao busca
void search (PMEMoid p_aux, int dado){

  if (((HASH *)pmemobj_direct(p_aux))->size == 0) {
    printf("Hash vazio\n");
    return;
  } else {

    int posicao = funcaoHash(dado);

    for (int i = 0; i<MAX_SIZE && ((HASH *)pmemobj_direct(p_aux))->occupied[posicao] != EMPTY; i++){
        
      if (((HASH *)pmemobj_direct(p_aux))->valor[posicao]==dado && ((HASH *)pmemobj_direct(p_aux))->occupied[posicao]==OCUPADO){
        printf ("Valor encontrado na posicao %d\n", posicao);
        return;
      }
        
      posicao = posicao + 1;
      posicao = posicao % MAX_SIZE;
    }
  } 
  printf("Valor não encontrado\n");
}

void remove_position (PMEMobjpool *pop, PMEMoid p_aux, int posicao){
  if (((HASH *)pmemobj_direct(p_aux))->size == 0) {
    printf("Hash vazio\n");
    return;
  } else if (((HASH *)pmemobj_direct(p_aux))->occupied[posicao] != OCUPADO){
    printf("Posicao vazia\n");
  } else{
      TX_BEGIN(pop) {
        
        ((HASH *)pmemobj_direct(p_aux))->occupied[posicao] = DELETED;
        ((HASH *)pmemobj_direct(p_aux))->size--;
    
      } TX_END
  }  
}

void remove_value (PMEMobjpool *pop, PMEMoid p_aux, int dado){

  int flag = 0;
  
  if (((HASH *)pmemobj_direct(p_aux))->size == 0) {
    printf("Hash vazio\n");
    return;
  } else{
      TX_BEGIN(pop) {
        int i;
        int posicao = funcaoHash(dado);

        for (i = 0; i<MAX_SIZE && ((HASH *)pmemobj_direct(p_aux))->occupied[posicao] != EMPTY; i++){          
          if (((HASH *)pmemobj_direct(p_aux))->valor[posicao]==dado && ((HASH *)pmemobj_direct(p_aux))->occupied[posicao] != DELETED){
            ((HASH *)pmemobj_direct(p_aux))->occupied[posicao] = DELETED;
            ((HASH *)pmemobj_direct(p_aux))->size--;
            flag = 1;
          }
          posicao = posicao + 1;
          posicao = posicao % MAX_SIZE;      
        }
      } TX_END

      if (flag == 0){
        printf("Valor nao encontrado\n");
      }
  }  
}

int verifica_null(PMEMoid p_aux){

    HASH *h = pmemobj_direct(p_aux);

    if (h == NULL) {
      return CLEAN;
    } else {
      return DIRTY;
    }
}


int main(int argc, char *argv[]) {
/****
 * POOL MANEGEMENT CODE
 */

/* Open the pool and return a "pool object pointer" */
  PMEMobjpool *pop = pmemobj_open(argv[1], LAYOUT_NAME);
	if (pop == NULL) {
    perror("pmemobj_open");
    return 1;
  }

/* Retrieve a persistent pointer to the root object */  
  PMEMoid p_root = pmemobj_root(pop, sizeof(struct my_root));

/* Get a "conventional" pointer to the root object */  
  struct my_root *root = pmemobj_direct(p_root);

  if (verifica_null(root->p_Hash)==CLEAN){
    start_hash(pop, &root->p_Hash);
  }

  int option = 0;
  int dado;

  while (1){

    printf("\n\n");
    display(root->p_Hash);

    printf("Enter your choice:\n1. Insert\n2. Search by value\n3. Remove by position \n4. Remove by value\n5. Exit\n >> ");

    scanf("%d", &option);

    if (option < 1 || option > 6){
      
      printf("Valor invalido\n");
    
    } else if (option == 1) {
			
      printf("Enter data to be inserted: ");
      scanf("%d", &dado);
			insert(pop, root->p_Hash, dado);
      
	  } else if (option == 2) {
     
      printf("Enter value to be searched: ");
      scanf("%d", &dado);
      search(root->p_Hash, dado);
    
    } else if (option == 3) {
        
      printf("Enter position to be removed: ");
      scanf("%d", &dado);

      if (dado>=0 && dado<MAX_SIZE){
        remove_position(pop, root->p_Hash, dado);
      } else{
        printf ("Posicao invalida\n");
      }
      
    } else if (option == 4) {
        
      printf("Enter value to be removed: ");
      scanf("%d", &dado);
      remove_value(pop, root->p_Hash, dado);

    } else if (option == 5) {
        break;
    }
  
  }
  
  pmemobj_close(pop);
  return 0;
  
}

//duvida persistencia ponteiros apos funcoes
//https://pmem.io/pmdk/manpages/linux/v1.3/libpmemobj.3/#layout-declaration-1
//https://github.com/pmem/libpmemobj-cpp/tree/master/utils
//https://github.com/pmem/pmdk