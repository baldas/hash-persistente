#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include <libpmemobj.h>


#define EMPTY 0.0
#define FULL 1.0
#define false 0
#define true 1

#define BUFFER_SIZE 64
#define DEFAULT INT_MIN

#define INITIAL_SIZE 8
#define SIZE_RATE 2
#define EXPAND_RATE 0.75
#define REDUCTION_RATE (EXPAND_RATE / SIZE_RATE)

int lifetime = DEFAULT;

#define LAYOUT_NAME "HASH"
#define POOL_SIZE ((size_t) (10 * 1024 * 1024)) //10 MB
#define POOL_NAME "hash_pool.obj"

POBJ_LAYOUT_BEGIN(HASH);
  POBJ_LAYOUT_ROOT(HASH, struct my_root);
  POBJ_LAYOUT_TOID(HASH, int);
  POBJ_LAYOUT_TOID(HASH, char);
  POBJ_LAYOUT_TOID(HASH, struct hash);
POBJ_LAYOUT_END(HASH);

struct hash {
  int size;
  int max_size;
  TOID(int) data;
  TOID(char) occupied;
};

struct my_root {
  TOID(struct hash) p_hash;
};

// Imprime o hash na tela, para monitoramento do usuário
void display(TOID(struct hash) p_aux) {

  // Percorre as linhas de 8 itens do hash
  for (int i = 0; i < (D_RO(p_aux)->max_size / INITIAL_SIZE); i++) {

    // Caso seja a primeira linha, imprime o cabeçalho do hash
    if (i != 0)
      printf("       ");
    else
      printf("\n HASH =");

    // Percorre os itens do hash daquela linha
    for (int j = 0; j < INITIAL_SIZE; j++) {

      // Se tiver um conteúdo naquele espaço, verifica se está ocupado
      if (D_RO(D_RO(p_aux)->data)[i*INITIAL_SIZE+j] != DEFAULT) {

        // Caso não esteja ocupado, usa a flag de remoção na resposta
        if (D_RO(D_RO(p_aux)->occupied)[i*INITIAL_SIZE+j])
            printf(" [%d] ", D_RO(D_RO(p_aux)->data)[i*INITIAL_SIZE+j]);
        else
            printf(" [!%d] ", D_RO(D_RO(p_aux)->data)[i*INITIAL_SIZE+j]);

      } else
        printf(" [*] ");
    }

    printf(" (%d)\n", (i+1)*INITIAL_SIZE);
  }
}

// Pode ser trocada por qualquer função de espalhamento (hash)
int hash_function(int dado, int max_size) {
  int res = (dado * dado) % max_size;
  return res < 0 ? res + max_size : res; //Caso dê overflow, mantém o valor por meio da rotação no hash
}

// Taxa de ocupação do Hash
double hash_rate(TOID(struct hash) p_aux) {
  return (((double) D_RO(p_aux)->size) / ((double) D_RO(p_aux)->max_size)) * FULL;
}

//funcao para inicializar hash
void start_hash(PMEMobjpool *pop, TOID(struct hash) *p_hash){

  TX_BEGIN(pop){
    TX_ADD_DIRECT(p_hash);

    *p_hash = TX_NEW(struct hash);

    D_RW(*p_hash)->size = 0;
    D_RW(*p_hash)->max_size = INITIAL_SIZE;

    if (lifetime != DEFAULT) {
      if (lifetime == 0)
        exit(0);
      lifetime--;
    }

    D_RW(*p_hash)->data = TX_ALLOC(int, sizeof(int) * D_RO(*p_hash)->max_size);
    D_RW(*p_hash)->occupied = TX_ALLOC(char, sizeof(char) * D_RO(*p_hash)->max_size);

    for (int i = 0; i < D_RO(*p_hash)->max_size; i++){
      D_RW(D_RW(*p_hash)->data)[i] = DEFAULT;
      D_RW(D_RW(*p_hash)->occupied)[i] = false;
    }
  } TX_END

}

//funcao inserir
char insert (PMEMobjpool *pop, TOID(struct hash) p_aux, int dado){

  if (hash_rate(p_aux) >= FULL) {
    printf("Hash cheio.\n");
    return false;
  }

  TX_BEGIN(pop) {

    TX_ADD(p_aux);
    pmemobj_tx_add_range_direct(D_RW(D_RW(p_aux)->data), sizeof(int) * D_RO(p_aux)->max_size);
    pmemobj_tx_add_range_direct(D_RW(D_RW(p_aux)->occupied), sizeof(char) * D_RO(p_aux)->max_size);

    int posicao = hash_function(dado, D_RO(p_aux)->max_size);

    while (D_RO(D_RO(p_aux)->occupied)[posicao]){
      posicao++;
      posicao = posicao % D_RO(p_aux)->max_size;
    }

    if (lifetime != DEFAULT) {
      if (lifetime == 0)
        exit(0);
      lifetime--;
    }

    D_RW(D_RW(p_aux)->data)[posicao] = dado;
    D_RW(D_RW(p_aux)->occupied)[posicao] = true;
    D_RW(p_aux)->size++;
  } TX_END

  return true;
}

//funcao busca
int search_value (TOID(struct hash) p_aux, int dado){

  if (hash_rate(p_aux) <= EMPTY) {
    printf("Hash vazio.\n");
    return DEFAULT;
  }

  int posicao = hash_function(dado, D_RO(p_aux)->max_size);

  for (int i = 0; D_RO(D_RO(p_aux)->data)[posicao] != DEFAULT; i++){
      
    if (D_RO(D_RO(p_aux)->data)[posicao] == dado && D_RO(D_RO(p_aux)->occupied)[posicao]){
      return posicao;
    }

    posicao++;
    posicao = posicao % D_RO(p_aux)->max_size;

    if (posicao == hash_function(dado, D_RO(p_aux)->max_size))
      break;
  }

  return DEFAULT;
}

char remove_position (PMEMobjpool *pop, TOID(struct hash) p_aux, int posicao){

  if (hash_rate(p_aux) <= EMPTY) {
    printf("Hash vazio.\n");
    return false;
  }

  if (posicao < 1 || posicao > D_RO(p_aux)->max_size){
    printf("Posição invalida.\n");
    return false;
  }

  posicao--;

  if (! D_RO(D_RO(p_aux)->occupied)[posicao]){
    printf("Posição vazia.\n");
    return false;
  }
  

  TX_BEGIN(pop) {
    TX_ADD(p_aux);
    pmemobj_tx_add_range_direct(D_RW(D_RW(p_aux)->occupied), sizeof(char) * D_RO(p_aux)->max_size);
    D_RW(D_RW(p_aux)->occupied)[posicao] = false;

    if (lifetime != DEFAULT) {
      if (lifetime == 0)
        exit(0);
      lifetime--;
    }

    D_RW(p_aux)->size--;
  } TX_END

  return true;
}

char remove_value (PMEMobjpool *pop, TOID(struct hash) p_aux, int dado){

  char removed = false;
  
  if (hash_rate(p_aux) <= EMPTY) {
    printf("Hash vazio.\n");
    return removed;
  }
  
  TX_BEGIN(pop) {

    TX_ADD(p_aux);
    pmemobj_tx_add_range_direct(D_RW(D_RW(p_aux)->occupied), sizeof(char) * D_RO(p_aux)->max_size);

    int posicao = hash_function(dado, D_RO(p_aux)->max_size);
    while (D_RO(D_RO(p_aux)->data)[posicao] != DEFAULT){

      if (D_RO(D_RO(p_aux)->data)[posicao] == dado && D_RO(D_RO(p_aux)->occupied)[posicao]){
        D_RW(D_RW(p_aux)->occupied)[posicao] = false;

        if (lifetime != DEFAULT) {
          if (lifetime == 0)
            exit(0);
          lifetime--;
        }

        D_RW(p_aux)->size--;
        removed = true;
      }
      posicao++;
      posicao = posicao % D_RO(p_aux)->max_size;
      
      if (posicao == hash_function(dado, D_RO(p_aux)->max_size))
        break;
    }
  } TX_END

  return removed;
}

char restore_position (PMEMobjpool *pop, TOID(struct hash) p_aux, int posicao){

  if (posicao < 1 || posicao > D_RO(p_aux)->max_size){
    printf("Posição invalida.\n");
    return false;
  }

  posicao--;

  if (D_RO(D_RO(p_aux)->data)[posicao] == DEFAULT){
    printf("Posição vazia.\n");
    return false;
  }
  

  TX_BEGIN(pop) {
    TX_ADD(p_aux);
    pmemobj_tx_add_range_direct(D_RW(D_RW(p_aux)->occupied), sizeof(char) * D_RO(p_aux)->max_size);
    D_RW(D_RW(p_aux)->occupied)[posicao] = true;

    if (lifetime != DEFAULT) {
      if (lifetime == 0)
        exit(0);
      lifetime--;
    }

    D_RW(p_aux)->size++;
  } TX_END

  return true;
}

void reset_hash(PMEMobjpool *pop, struct my_root * root) {

  TX_BEGIN (pop) {

    TX_FREE(D_RW(root->p_hash)->occupied);
    TX_FREE(D_RW(root->p_hash)->data);

    if (lifetime != DEFAULT) {
      if (lifetime == 0)
        exit(0);
      lifetime--;
    }
  
    TX_FREE(root->p_hash);
    root->p_hash = TOID_NULL(struct hash);
    start_hash(pop, &root->p_hash);

  } TX_END

}

int main(int argc, char *argv[]) {
/****
 * POOL MANEGEMENT CODE
 */

  if (argc > 1) {
    lifetime = atoi(argv[1]);
  }

  PMEMobjpool *pop = pmemobj_create(POOL_NAME, LAYOUT_NAME, POOL_SIZE, 0666);
  if (pop == NULL) {
    /* Open the pool and return a "pool object pointer" */
      pop = pmemobj_open(POOL_NAME, LAYOUT_NAME);
      if (pop == NULL) {
        perror("pmemobj_open\n");
        return 1;
      }
  }

/* Get a "conventional" pointer to the root object */  
  struct my_root *root = D_RW(POBJ_ROOT(pop, struct my_root));

  if (TOID_IS_NULL(root->p_hash)){
    start_hash(pop, &root->p_hash);
  }

  int option, data;
  char buffer[BUFFER_SIZE];

    // Menu de interação
  while (true) {
    display(root->p_hash);

    if (lifetime <0) {
      printf("\nEnter your choice:\n1. Insert data\n2. Remove by position\n3. Remove by value\n4. Search by value\n5. Restore by position\n6. Reset Hash\n7. Exit\n >> ");
    } else {
      printf("\nEnter your choice: (");
      switch (lifetime) {
        case 0: printf("- - -"); break;
        case 1: printf("█ - -"); break;
        case 2: printf("█ █ -"); break;
        case 3: printf("█ █ █"); break;
        default: printf("%dx █",lifetime);
      }
      printf(")\n1. Insert data\n2. Remove by position\n3. Remove by value\n4. Search by value\n5. Restore by position\n6. Reset Hash\n7. Exit\n >> ");
    }
    fgets(buffer, BUFFER_SIZE-1, stdin);
    option = atoi(buffer);

    // Alterna entre as possíveis opções do menu
    switch (option) {

      case 1: // Caso da inserção
        printf("Enter data to be inserted: ");
        fgets(buffer, BUFFER_SIZE-1, stdin);
        data = atoi(buffer);

        if (insert(pop, root->p_hash, data)) {
          printf("Data inserted!\n");
        } else {
          printf("Data not inserted.\n");
        }
        break;

      case 2: // Caso da remoção posicional
        printf("Enter position to be removed: ");
        fgets(buffer, BUFFER_SIZE-1, stdin);
        data = atoi(buffer);

        if (remove_position(pop, root->p_hash, data)) {
          printf("Position removed!\n");
        } else {
          printf("Position not removed.\n");
        }
        break;

      case 3: // Caso da remoção de itens
        printf("Enter value to be removed: ");
        fgets(buffer, BUFFER_SIZE-1, stdin);
        data = atoi(buffer);

        if (remove_value(pop, root->p_hash, data)) {
          printf("Data removed!\n");
        } else {
          printf("Data not removed.\n");
        }
        break;

      case 4: // Caso da busca (para ver se há um dado item)
        printf("Enter value to be searched: ");
        fgets(buffer, BUFFER_SIZE-1, stdin);
        data = atoi(buffer);

        if ((data = search_value(root->p_hash, data)) >= 0) {
          printf("Data found at position %d.\n", data+1);
        } else {
          printf("No data found.\n");
        }
        break;

      case 5: // Caso de restauração de itens recém-deletados
        printf("Enter position to be restored: ");
        fgets(buffer, BUFFER_SIZE-1, stdin);
        data = atoi(buffer);

        if (restore_position(pop, root->p_hash, data)) {
          printf("Position restored!\n");
        } else {
          printf("Position not restored.\n");
        }
        break;

      case 6: // Caso de reset da hash
        reset_hash(pop, root);
        printf("Hash reseted!\n");
        break;

      case 7: // Caso de saída do programa
        pmemobj_close(pop);
        exit(0);
      
      default:
        printf("No command found.\n");
    }
  }
  pmemobj_close(pop);
  return 0;
  
}
