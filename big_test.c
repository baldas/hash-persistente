#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

#define TESTES 10

void task(int id, int lifetime) {

    char lifetime_buffer[64] = "";
    snprintf(lifetime_buffer, sizeof(char) * 64, "%d", lifetime);

    char id_buffer[64] = "";
    snprintf(id_buffer, sizeof(char) * 64, "%d", id);

    char *args[] = {"./hash_persistente", lifetime_buffer, id_buffer, NULL};
    
    if (execvp(args[0], args) == -1) {
        perror("Erro ao rodar o seu executável");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char* argv[]) {

    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    if (num_cores < 1) {
        perror("Não foi possível detectar o número de cores. Usando padrão 1.");
        num_cores = 1;
    }

    printf("[PAI] Detectados %ld processadores lógicos disponíveis.\n", num_cores);
    printf("[PAI] O tamanho do lote de processos paralelos será: %ld\n\n", num_cores);

    int total_tarefas = TESTES;

    if (argc == 2) {
        total_tarefas = atoi(argv[1]);
    }

    int tarefas_criadas = 0;
    int processos_ativos = 0;

    while (tarefas_criadas < total_tarefas || processos_ativos > 0) {
        
        while (processos_ativos < num_cores && tarefas_criadas < total_tarefas) {
            pid_t pid = fork();

            if (pid < 0) {
                perror("Erro no fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {

                int id_tarefa = tarefas_criadas + 1;
                printf("[Filho] Processando tarefa #%d (PID: %d) em um core...\n", id_tarefa, getpid());

                int arquivo_fd = open("test.txt", O_RDONLY);
                if (arquivo_fd < 0) {
                    perror("Erro ao abrir o arquivo test.txt");
                    exit(EXIT_FAILURE);
                }

                if (dup2(arquivo_fd, STDIN_FILENO) < 0) {
                    perror("Erro no dup2");
                    exit(EXIT_FAILURE);
                }

                close(arquivo_fd);
                
                task(id_tarefa, id_tarefa-1);
                
                printf("[Filho] Tarefa #%d CONCLUÍDA.\n", id_tarefa);
                exit(EXIT_SUCCESS);

            }

            tarefas_criadas++;
            processos_ativos++;
        }

        if (processos_ativos > 0) {
            wait(NULL); 
            processos_ativos--;
        }
    }

    printf("\n[PAI] Todas as %d tarefas foram processadas com sucesso em lotes de %ld!\n", total_tarefas, num_cores);
    return 0;
}