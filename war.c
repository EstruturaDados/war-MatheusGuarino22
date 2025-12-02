#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NOME 30
#define MAX_COR  16
#define MISSao_BUFFER 200

typedef struct {
    char nome[MAX_NOME];
    char cor[MAX_COR];
    int tropas;
} Territorio;

/* --- Assinaturas --- */
void cadastrarTerritorios(Territorio* mapa, int qtd);
void exibirMapa(const Territorio* mapa, int qtd);
void atribuirMissao(char* destino, char* missoes[], int totalMissoes, Territorio* mapa, int tamanho, int* idMissao);
void exibirMissao(const char* missao);
int verificarMissao(const char* missao, Territorio* mapa, int tamanho, const char* corJogador);
void atacar(Territorio* atacante, Territorio* defensor);
void liberarMemoria(Territorio* mapa, char** missoesJogadores, int jogadores);

/* --- Implementações --- */

void cadastrarTerritorios(Territorio* mapa, int qtd) {
    for (int i = 0; i < qtd; i++) {
        printf("\n--- Cadastro do território %d ---\n", i + 1);

        printf("Nome (sem espaços, max %d chars): ", MAX_NOME - 1);
        scanf("%29s", mapa[i].nome);

        printf("Cor do exército (ex: vermelho, azul) (max %d chars): ", MAX_COR - 1);
        scanf("%15s", mapa[i].cor);

        printf("Quantidade de tropas (inteiro >=0): ");
        scanf("%d", &mapa[i].tropas);
        if (mapa[i].tropas < 0) mapa[i].tropas = 0;
    }
}

void exibirMapa(const Territorio* mapa, int qtd) {
    printf("\n===== MAPA ATUAL =====\n");
    for (int i = 0; i < qtd; i++) {
        printf("Território %d: %s | Cor: %s | Tropas: %d\n",
               i + 1, mapa[i].nome, mapa[i].cor, mapa[i].tropas);
    }
    printf("======================\n");
}

/* atribuirMissao: sorteia e copia a missão para 'destino' */
/* Se a missão conter token {T}, substitui por um nome de território aleatório do mapa */
void atribuirMissao(char* destino, char* missoes[], int totalMissoes, Territorio* mapa, int tamanho, int* idMissao) {
    if (totalMissoes <= 0) {
        destino[0] = '\0';
        if (idMissao) *idMissao = -1;
        return;
    }
    int idx = rand() % totalMissoes;
    if (idMissao) *idMissao = idx;

    const char* token = "{T}";
    const char* template = missoes[idx];

    if (strstr(template, token) != NULL && tamanho > 0) {
        int idxTerr = rand() % tamanho;
        char buffer[MISSao_BUFFER];
        buffer[0] = '\0';

        const char* pos = strstr(template, token);
        size_t before_len = pos - template;
        if (before_len > 0) strncat(buffer, template, before_len);
        strncat(buffer, mapa[idxTerr].nome, sizeof(buffer) - strlen(buffer) - 1);
        strncat(buffer, pos + strlen(token), sizeof(buffer) - strlen(buffer) - 1);

        strncpy(destino, buffer, MISSao_BUFFER - 1);
        destino[MISSao_BUFFER - 1] = '\0';
    } else {
        strncpy(destino, template, MISSao_BUFFER - 1);
        destino[MISSao_BUFFER - 1] = '\0';
    }
}

void exibirMissao(const char* missao) {
    printf(">> MISSÃO: %s\n", missao);
}

/*
  verificarMissao:
  Retorna 1 se a missão foi cumprida para o jogador identificado por corJogador.
  Implementa reconhecimento simples das missões definidas no template.
*/
int verificarMissao(const char* missao, Territorio* mapa, int tamanho, const char* corJogador) {
    if (!missao || !mapa || !corJogador) return 0;

    /* 1) Conquistar 3 territorios */
    if (strstr(missao, "Conquistar 3 territorios") != NULL) {
        int conta = 0;
        for (int i = 0; i < tamanho; i++) if (strcmp(mapa[i].cor, corJogador) == 0) conta++;
        return (conta >= 3) ? 1 : 0;
    }

    /* 2) Eliminar todas as tropas da cor X -> ex: "Eliminar todas as tropas da cor vermelha" */
    if (strstr(missao, "Eliminar todas as tropas da cor") != NULL) {
        /* pega a última palavra como cor alvo */
        char copia[MISSao_BUFFER];
        strncpy(copia, missao, sizeof(copia) - 1);
        copia[sizeof(copia)-1] = '\0';
        char* tok = strrchr(copia, ' ');
        if (tok) {
            char corAlvo[MAX_COR];
            strncpy(corAlvo, tok + 1, MAX_COR - 1);
            corAlvo[MAX_COR - 1] = '\0';
            int total = 0;
            for (int i = 0; i < tamanho; i++) if (strcmp(mapa[i].cor, corAlvo) == 0) total += mapa[i].tropas;
            return (total == 0) ? 1 : 0;
        }
        return 0;
    }

    /* 3) Controlar o territorio NAME -> "Controlar o territorio NAME" */
    if (strstr(missao, "Controlar o territorio") != NULL) {
        const char* prefix = "Controlar o territorio ";
        char nomeBusca[MAX_NOME];
        if (sscanf(missao + strlen(prefix), "%29s", nomeBusca) == 1) {
            for (int i = 0; i < tamanho; i++) {
                if (strcmp(mapa[i].nome, nomeBusca) == 0) {
                    return (strcmp(mapa[i].cor, corJogador) == 0) ? 1 : 0;
                }
            }
        }
        return 0;
    }

    /* 4) Acumular N tropas no total -> "Acumular 10 tropas no total" */
    if (strstr(missao, "Acumular") != NULL && strstr(missao, "tropas no total") != NULL) {
        int N = 0;
        if (sscanf(missao, "Acumular %d tropas no total", &N) == 1 && N > 0) {
            int soma = 0;
            for (int i = 0; i < tamanho; i++) if (strcmp(mapa[i].cor, corJogador) == 0) soma += mapa[i].tropas;
            return (soma >= N) ? 1 : 0;
        }
        return 0;
    }

    /* 5) Conquistar 2 territorios seguidos */
    if (strstr(missao, "Conquistar 2 territorios seguidos") != NULL) {
        for (int i = 0; i < tamanho - 1; i++) {
            if (strcmp(mapa[i].cor, corJogador) == 0 && strcmp(mapa[i+1].cor, corJogador) == 0) return 1;
        }
        return 0;
    }

    /* Missão não reconhecida */
    return 0;
}

/* Função de ataque: usa rand() para rolagem 1..6 para cada lado */
void atacar(Territorio* atacante, Territorio* defensor) {
    if (!atacante || !defensor) return;

    printf("\nAtaque: %s (cor %s, tropas %d) -> %s (cor %s, tropas %d)\n",
           atacante->nome, atacante->cor, atacante->tropas,
           defensor->nome, defensor->cor, defensor->tropas);

    int dadoA = (rand() % 6) + 1;
    int dadoD = (rand() % 6) + 1;
    printf("Rolagens: atacante %d x defensor %d\n", dadoA, dadoD);

    if (dadoA > dadoD) {
        printf("Atacante venceu! %s agora pertence à cor %s.\n", defensor->nome, atacante->cor);
        strncpy(defensor->cor, atacante->cor, MAX_COR - 1);
        defensor->cor[MAX_COR - 1] = '\0';
        defensor->tropas = defensor->tropas / 2;
        if (defensor->tropas < 0) defensor->tropas = 0;
    } else {
        printf("Defensor resistiu! Atacante perde 1 tropa.\n");
        if (atacante->tropas > 0) atacante->tropas -= 1;
    }
}

/* libera mapa e array de strings de missões dos jogadores */
void liberarMemoria(Territorio* mapa, char** missoesJogadores, int jogadores) {
    if (mapa) free(mapa);
    if (missoesJogadores) {
        for (int i = 0; i < jogadores; i++) {
            if (missoesJogadores[i]) free(missoesJogadores[i]);
        }
        free(missoesJogadores);
    }
}

/* --- main: orquestra o jogo --- */
int main(void) {
    srand((unsigned int) time(NULL));

    int qtdTerritorios = 0;
    printf("Quantos territórios deseja cadastrar? ");
    if (scanf("%d", &qtdTerritorios) != 1 || qtdTerritorios <= 0) {
        printf("Entrada inválida.\n");
        return 1;
    }

    /* aloca mapa dinamicamente */
    Territorio* mapa = (Territorio*) calloc(qtdTerritorios, sizeof(Territorio));
    if (!mapa) {
        printf("Erro ao alocar memória para mapa.\n");
        return 1;
    }

    cadastrarTerritorios(mapa, qtdTerritorios);
    exibirMapa(mapa, qtdTerritorios);

    /* templates de missões (pelo menos 5) */
    char* missoesTemplates[] = {
        "Conquistar 3 territorios",
        "Eliminar todas as tropas da cor vermelha",
        "Controlar o territorio {T}",
        "Acumular 10 tropas no total",
        "Conquistar 2 territorios seguidos"
    };
    int totalMissoes = sizeof(missoesTemplates) / sizeof(missoesTemplates[0]);

    int jogadores = 2;
    char coresJogadores[2][MAX_COR];

    for (int p = 0; p < jogadores; p++) {
        printf("Digite a cor que representa o jogador %d (ex: vermelho, azul): ", p + 1);
        scanf("%15s", coresJogadores[p]);
    }

    /* aloca missões dos jogadores dinamicamente */
    char** missoesJogadores = (char**) malloc(sizeof(char*) * jogadores);
    if (!missoesJogadores) {
        printf("Erro ao alocar missões.\n");
        free(mapa);
        return 1;
    }
    for (int p = 0; p < jogadores; p++) {
        missoesJogadores[p] = (char*) malloc(MISSao_BUFFER);
        if (!missoesJogadores[p]) {
            printf("Erro ao alocar missão para jogador %d\n", p+1);
            for (int k = 0; k < p; k++) free(missoesJogadores[k]);
            free(missoesJogadores);
            free(mapa);
            return 1;
        }
        missoesJogadores[p][0] = '\0';
    }

    int* idsMissoes = (int*) malloc(sizeof(int) * jogadores);
    if (!idsMissoes) {
        printf("Erro ao alocar ids.\n");
        liberarMemoria(mapa, missoesJogadores, jogadores);
        return 1;
    }

    /* atribui missão a cada jogador e exibe apenas uma vez */
    for (int p = 0; p < jogadores; p++) {
        atribuirMissao(missoesJogadores[p], missoesTemplates, totalMissoes, mapa, qtdTerritorios, &idsMissoes[p]);
        printf("\nJogador %d (cor %s) recebeu sua missão:\n", p + 1, coresJogadores[p]);
        exibirMissao(missoesJogadores[p]);
    }

    int turno = 0; /* jogador 0 inicia */

    while (1) {
        printf("\n--- Turno do jogador %d (cor %s) ---\n", turno + 1, coresJogadores[turno]);
        exibirMapa(mapa, qtdTerritorios);

        int idxAtacante, idxDefensor;
        printf("Escolha número do território ATACANTE (1 a %d): ", qtdTerritorios);
        if (scanf("%d", &idxAtacante) != 1) { printf("Entrada inválida. Término.\n"); break; }
        printf("Escolha número do território DEFENSOR (1 a %d): ", qtdTerritorios);
        if (scanf("%d", &idxDefensor) != 1) { printf("Entrada inválida. Término.\n"); break; }

        idxAtacante--; idxDefensor--;
        if (idxAtacante < 0 || idxAtacante >= qtdTerritorios || idxDefensor < 0 || idxDefensor >= qtdTerritorios) {
            printf("Índices inválidos. Tente novamente.\n");
            continue;
        }

        /* validacao: atacante deve pertencer ao jogador da vez */
        if (strcmp(mapa[idxAtacante].cor, coresJogadores[turno]) != 0) {
            printf("Erro: o território atacante não pertence ao jogador %d. Turno perdido.\n", turno + 1);
        } else if (strcmp(mapa[idxAtacante].cor, mapa[idxDefensor].cor) == 0) {
            printf("Erro: não é permitido atacar território da mesma cor. Turno perdido.\n");
        } else {
            atacar(&mapa[idxAtacante], &mapa[idxDefensor]);
        }

        /* após ação, verificar missões silenciosamente */
        for (int p = 0; p < jogadores; p++) {
            if (verificarMissao(missoesJogadores[p], mapa, qtdTerritorios, coresJogadores[p])) {
                printf("\n=====================================\n");
                printf("Jogador %d (cor %s) CUMPRIU A MISSÃO: %s\n", p + 1, coresJogadores[p], missoesJogadores[p]);
                printf("==== JOGO ENCERRADO - VENCEDOR ======\n");
                printf("=====================================\n");

                liberarMemoria(mapa, missoesJogadores, jogadores);
                free(idsMissoes);
                return 0;
            }
        }

        /* alterna turno */
        turno = (turno + 1) % jogadores;

        /* opção de continuar ou terminar */
        char cont;
        printf("Deseja continuar o jogo? (s/n): ");
        scanf(" %c", &cont);
        if (cont == 'n' || cont == 'N') {
            printf("Jogo encerrado pelo usuário.\n");
            break;
        }
    }

    liberarMemoria(mapa, missoesJogadores, jogadores);
    free(idsMissoes);
    return 0;
}
