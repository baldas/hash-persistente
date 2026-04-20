#include <stdio.h>
#include <stdlib.h>

#include <libpmemobj.h>

#define MAX_SIZE 5
#define EMPTY 0
#define OCUPADO 1
#define DELETED 2

#define LAYOUT_NAME "hash"

POBJ_LAYOUT_BEGIN(hash);
  POBJ_LAYOUT_ROOT(hash, struct my_root);
  POBJ_LAYOUT_TOID(hash, struct Hash);
POBJ_LAYOUT_END(hash);

struct Hash {
  int clean;
  int size;
  int valor [MAX_SIZE];
  int occupied [MAX_SIZE];
};

struct my_root {
  TOID(struct Hash) p_Hash;
};

//funcao para imprimir hash
void display(TOID(struct Hash) p_Hash){
  printf ("Hash = ");

  for (int i = 0; i < MAX_SIZE; i++){
    if (D_RO(p_Hash)->occupied[i] == OCUPADO)
      printf ("%d ", D_RO(p_Hash)->valor[i]);
    else
      printf ("* ");
  }
  printf ("\n");
}

//funcao de calculo da posicao do hash
int funcaoHash (int dado){
  return (dado*dado) % MAX_SIZE;
}

//funcao para inicializar hash
void start_hash(PMEMobjpool *pop, TOID(struct Hash) *p_Hash){

  TX_BEGIN(pop){
    TX_ADD_DIRECT(p_Hash);

    *p_Hash = TX_NEW(struct Hash);

    D_RW(*p_Hash)->size = 0;

    for (int i = 0; i < MAX_SIZE; i++){
      D_RW(*p_Hash)->valor[i] = -1;
      D_RW(*p_Hash)->occupied[i] = EMPTY;
    }
  } TX_END

}

//funcao inserir
void insert (PMEMobjpool *pop, TOID(struct Hash) p_aux, int dado){

  if (D_RO(p_aux)->size >= MAX_SIZE) {
    printf("Hash cheio\n");
    return;
  }

  TX_BEGIN(pop) {

    TX_ADD(p_aux);

    int posicao = funcaoHash(dado);

    while (D_RO(p_aux)->occupied[posicao] == OCUPADO){
      posicao++;
      posicao = posicao % MAX_SIZE;
    }

    D_RW(p_aux)->valor[posicao] = dado;
    D_RW(p_aux)->occupied[posicao] = OCUPADO;
    D_RW(p_aux)->size++;
  } TX_END
  
}

//funcao busca
void search (TOID(struct Hash) p_aux, int dado){

  if (D_RO(p_aux)->size == 0) {
    printf("Hash vazio\n");
    return;
  }

  int posicao = funcaoHash(dado);

  for (int i = 0; i<MAX_SIZE && D_RO(p_aux)->occupied[posicao] != EMPTY; i++){
      
    if (D_RO(p_aux)->valor[posicao]==dado && D_RO(p_aux)->occupied[posicao]==OCUPADO){
      printf ("Valor encontrado na posicao %d\n", posicao);
      return;
    }

    posicao++;
    posicao = posicao % MAX_SIZE;
  }

  printf("Valor não encontrado\n");
}

void remove_position (PMEMobjpool *pop, TOID(struct Hash) p_aux, int posicao){

  if (D_RO(p_aux)->size == 0) {
    printf("Hash vazio\n");
    return;
  }

  if (D_RO(p_aux)->occupied[posicao] != OCUPADO){
    printf("Posicao vazia\n");
  } else {
    TX_BEGIN(pop) {
      TX_ADD(p_aux);
      D_RW(p_aux)->occupied[posicao] = DELETED;
      D_RW(p_aux)->size--;
    } TX_END
  } 
}

void remove_value (PMEMobjpool *pop, TOID(struct Hash) p_aux, int dado){

  int flag = 0;
  
  if (D_RO(p_aux)->size == 0) {
    printf("Hash vazio\n");
    return;
  }

  int i;
  int posicao = funcaoHash(dado);
  
  TX_BEGIN(pop) {

    TX_ADD(p_aux);

    for (i = 0; i<MAX_SIZE && D_RO(p_aux)->occupied[posicao] != EMPTY; i++){          
      if (D_RO(p_aux)->valor[posicao]==dado && D_RO(p_aux)->occupied[posicao] != DELETED){
        D_RW(p_aux)->occupied[posicao] = DELETED;
        D_RW(p_aux)->size--;
        flag = 1;
      }
      posicao++;
      posicao = posicao % MAX_SIZE;      
    }
  } TX_END

  if (flag == 0){
    printf("Valor nao encontrado\n");
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

  TX_BEGIN(pop) {
    if (OID_IS_NULL(root->p_Hash)){
      TX_ADD_DIRECT(&root->p_Hash);
      start_hash(pop, &root->p_Hash);
    }
  } TX_END

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
