#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_NOME 30
#define MAX_COR  10

/* Estrutura que representa um território */
typedef struct {
    char nome[MAX_NOME];
    char cor[MAX_COR];
    int tropas;
} Territorio;

/* ---------- Assinaturas das funções (modularização) ---------- */
void cadastrarTerritorios(Territorio* mapa, int qtd);
void exibirMapa(Territorio* mapa, int qtd);
void atribuirMissao(char* destino, char* missoes[], int totalMissoes, Territorio* mapa, int tamanho, int* idMissao);
void exibirMissao(const char* missao); /* passagem por valor para exibir */
int verificarMissao(const char* missao, Territorio* mapa, int tamanho, const char* corJogador);
void atacar(Territorio* atacante, Territorio* defensor);
void liberarMemoria(Territorio* mapa, char** missoesJogadores, int jogadores);

/* ---------------- Implementações ---------------- */

/* Cadastra 'qtd' territórios no vetor 'mapa'. Uso de ponteiros para manipular dados. */
void cadastrarTerritorios(Territorio* mapa, int qtd) {
    for (int i = 0; i < qtd; i++) {
        printf("\n--- Cadastro do território %d ---\n", i + 1);

        printf("Nome (sem espaços, max %d chars): ", MAX_NOME - 1);
        scanf("%29s", mapa[i].nome);

        printf("Cor do exército (ex: vermelho, azul) (max %d chars): ", MAX_COR - 1);
        scanf("%9s", mapa[i].cor);

        printf("Quantidade de tropas (inteiro >=0): ");
        scanf("%d", &mapa[i].tropas);

        if (mapa[i].tropas < 0) mapa[i].tropas = 0;
    }
}

/* Exibe o mapa atual com todos os territórios */
void exibirMapa(Territorio* mapa, int qtd) {
    printf("\n\n===== MAPA ATUAL =====\n");
    for (int i = 0; i < qtd; i++) {
        printf("Território %d: %s\n", i + 1, mapa[i].nome);
        printf("  Cor: %s\n", mapa[i].cor);
        printf("  Tropas: %d\n", mapa[i].tropas);
        printf("---------------------------\n");
    }
}

/*
    Atribui aleatoriamente uma missão:
    - 'destino' já deve apontar para um buffer alocado (por quem chamou)
    - escolhe um índice aleatório entre 0 e totalMissoes-1
    - copia a string da missão para destino com strcpy
    - se a missão for do tipo "Controlar o territorio {T}", substitui {T} por um nome aleatório do mapa
    - devolve o id da missão via 'idMissao' (passagem por referência)
*/
void atribuirMissao(char* destino, char* missoes[], int totalMissoes, Territorio* mapa, int tamanho, int* idMissao) {
    if (totalMissoes <= 0) {
        destino[0] = '\0';
        *idMissao = -1;
        return;
    }

    int idx = rand() % totalMissoes; /* sorteio */
    *idMissao = idx;

    /* Se missão template contém token {T}, substituímos por um nome de território aleatório do mapa */
    const char* token = "{T}";
    if (strstr(missoes[idx], token) != NULL && tamanho > 0) {
        /* escolher território aleatório do mapa para preencher o token */
        int idxTerr = rand() % tamanho;
        char buffer[200];
        buffer[0] = '\0';

        const char* part_before = missoes[idx];
        char* pos = strstr(part_before, token);
        if (pos) {
            /* concatena parte antes + nome + parte depois */
            size_t before_len = pos - part_before;
            strncat(buffer, part_before, before_len);
            strcat(buffer, mapa[idxTerr].nome);
            strcat(buffer, pos + strlen(token));
        } else {
            strcpy(buffer, missoes[idx]);
        }
        strcpy(destino, buffer);
    } else {
        /* cópia direta */
        strcpy(destino, missoes[idx]);
    }
}

/* Exibe a missão (passagem por valor - só leitura). Exibida apenas uma vez no início do jogo. */
void exibirMissao(const char* missao) {
    printf(">> Missão atribuída: %s\n", missao);
}

/*
    Verifica se a missão foi cumprida.
    Retorna 1 se cumprida, 0 caso contrário.
    A função interpreta algumas missões predefinidas:
    - "Conquistar 3 territorios"            -> jogador controla >= 3 territórios
    - "Eliminar todas as tropas da cor X"   -> total de tropas da cor X == 0
    - "Controlar o territorio NAME"         -> território NAME tem cor igual à corJogador
    - "Acumular 10 tropas no total"         -> soma de tropas do jogador >= 10
    - "Conquistar 2 territorios seguidos"  -> existe i tal que mapa[i] e mapa[i+1] têm cor igual à corJogador
*/
int verificarMissao(const char* missao, Territorio* mapa, int tamanho, const char* corJogador) {
    if (missao == NULL || mapa == NULL || corJogador == NULL) return 0;

    /* 1) Conquistar 3 territorios */
    if (strstr(missao, "Conquistar 3 territorios") != NULL) {
        int conta = 0;
        for (int i = 0; i < tamanho; i++)
            if (strcmp(mapa[i].cor, corJogador) == 0) conta++;
        return (conta >= 3) ? 1 : 0;
    }

    /* 2) Eliminar todas as tropas da cor <cor>  -> exemplo: "Eliminar todas as tropas da cor vermelha" */
    if (strstr(missao, "Eliminar todas as tropas da cor") != NULL) {
        /* extrair a cor alvo (última palavra) */
        char copia[200];
        strncpy(copia, missao, sizeof(copia)-1);
        copia[sizeof(copia)-1] = '\0';
        char* token = strrchr(copia, ' ');
        if (token) {
            char corAlvo[MAX_COR];
            strncpy(corAlvo, token + 1, MAX_COR-1);
            corAlvo[MAX_COR-1] = '\0';
            int total = 0;
            for (int i = 0; i < tamanho; i++)
                if (strcmp(mapa[i].cor, corAlvo) == 0) total += mapa[i].tropas;
            return (total == 0) ? 1 : 0;
        }
        return 0;
    }

    /* 3) Controlar o territorio NAME -> missão contém "Controlar o territorio " seguido do nome */
    if (strstr(missao, "Controlar o territorio") != NULL) {
        /* extrair nome após a frase */
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

    /* 4) Acumular N tropas no total -> formato "Acumular 10 tropas no total" (valor N variável) */
    if (strstr(missao, "Acumular") != NULL && strstr(missao, "tropas no total") != NULL) {
        int N = 0;
        if (sscanf(missao, "Acumular %d tropas no total", &N) == 1 && N > 0) {
            int soma = 0;
            for (int i = 0; i < tamanho; i++)
                if (strcmp(mapa[i].cor, corJogador) == 0) soma += mapa[i].tropas;
            return (soma >= N) ? 1 : 0;
        }
        return 0;
    }

    /* 5) Conquistar 2 territorios seguidos -> busca por dois índices consecutivos com a cor do jogador */
    if (strstr(missao, "Conquistar 2 territorios seguidos") != NULL) {
        for (int i = 0; i < tamanho - 1; i++) {
            if (strcmp(mapa[i].cor, corJogador) == 0 && strcmp(mapa[i+1].cor, corJogador) == 0)
                return 1;
        }
        return 0;
    }

    /* Caso missão não reconhecida -> false (0) */
    return 0;
}

/*
    Simula um ataque entre dois territórios (ponteiros).
    - rola dados 1..6 para cada lado
    - se atacante vencer: defensor passa a ter a cor do atacante e perde metade das tropas (integer division)
    - se atacante perder (ou empatar): atacante perde 1 tropa (se tiver)
*/
void atacar(Territorio* atacante, Territorio* defensor) {
    if (atacante == NULL || defensor == NULL) return;

    printf("\n--- Ataque: %s (cor %s, tropas %d) -> %s (cor %s, tropas %d) ---\n",
           atacante->nome, atacante->cor, atacante->tropas,
           defensor->nome, defensor->cor, defensor->tropas);

    int dadoA = (rand() % 6) + 1;
    int dadoD = (rand() % 6) + 1;

    printf("Rolagem: atacante %d x defensor %d\n", dadoA, dadoD);

    if (dadoA > dadoD) {
        printf("Atacante venceu! %s agora pertence à cor %s.\n", defensor->nome, atacante->cor);
        strcpy(defensor->cor, atacante->cor);
        defensor->tropas = defensor->tropas / 2;
        if (defensor->tropas < 0) defensor->tropas = 0;
    } else {
        printf("Defensor resistiu! Atacante perde 1 tropa.\n");
        if (atacante->tropas > 0) atacante->tropas -= 1;
    }
}

/* Libera memória alocada dinamicamente (mapa já alocado, e missões dos jogadores) */
void liberarMemoria(Territorio* mapa, char** missoesJogadores, int jogadores) {
    if (mapa) free(mapa);
    if (missoesJogadores) {
        for (int i = 0; i < jogadores; i++) {
            if (missoesJogadores[i]) free(missoesJogadores[i]);
        }
        free(missoesJogadores);
    }
}

/* ---------------------- main: orquestra o jogo ---------------------- */
int main() {
    srand((unsigned int) time(NULL));

    int qtdTerritorios;
    printf("Quantos territórios deseja cadastrar? ");
    scanf("%d", &qtdTerritorios);
    if (qtdTerritorios <= 0) {
        printf("Número de territórios inválido.\n");
        return 1;
    }

    /* 1) alocação dinâmica do mapa */
    Territorio* mapa = (Territorio*) calloc(qtdTerritorios, sizeof(Territorio));
    if (!mapa) {
        printf("Falha ao alocar memória para mapa.\n");
        return 1;
    }

    /* cadastro */
    cadastrarTerritorios(mapa, qtdTerritorios);
    exibirMapa(mapa, qtdTerritorios);

    /* 2) definições de missão (templates). Pelo menos 5 descrições. */
    /* Notas:
       - Pode haver tokens {T} que serão substituídos pelo nome de um território aleatório.
       - As missões foram pensadas para serem verificáveis com as regras implementadas em verificarMissao.
    */
    char* missoesTemplates[] = {
        "Conquistar 3 territorios",
        "Eliminar todas as tropas da cor vermelha",
        "Controlar o territorio {T}",
        "Acumular 10 tropas no total",
        "Conquistar 2 territorios seguidos"
    };
    int totalMissoes = sizeof(missoesTemplates) / sizeof(missoesTemplates[0]);

    /* 3) jogadores (2 jogadores) - pedir cor de cada um */
    int jogadores = 2;
    char coresJogadores[2][MAX_COR];
    for (int p = 0; p < jogadores; p++) {
        printf("Digite a cor que representa o jogador %d (ex: vermelho, azul): ", p + 1);
        scanf("%9s", coresJogadores[p]);
    }

    /* 4) alocar dinamicamente espaço para guardar a missão de cada jogador (string) */
    char** missoesJogadores = (char**) malloc(sizeof(char*) * jogadores);
    if (!missoesJogadores) {
        printf("Erro ao alocar memória para missões dos jogadores.\n");
        free(mapa);
        return 1;
    }
    for (int p = 0; p < jogadores; p++) {
        missoesJogadores[p] = (char*) malloc(200); /* 200 bytes por missão (suficiente) */
        if (!missoesJogadores[p]) {
            printf("Erro ao alocar memória para missão do jogador %d\n", p+1);
            /* liberar já alocado */
            for (int k = 0; k < p; k++) free(missoesJogadores[k]);
            free(missoesJogadores);
            free(mapa);
            return 1;
        }
        missoesJogadores[p][0] = '\0';
    }

    /* também guardamos o id da missão sorteada (útil para debug ou lógica futura) */
    int* idsMissoes = (int*) malloc(sizeof(int) * jogadores);
    if (!idsMissoes) {
        printf("Erro ao alocar memória.\n");
        liberarMemoria(mapa, missoesJogadores, jogadores);
        return 1;
    }

    /* 5) atribuir missão para cada jogador (com substituição de {T} se necessário) */
    for (int p = 0; p < jogadores; p++) {
        atribuirMissao(missoesJogadores[p], missoesTemplates, totalMissoes, mapa, qtdTerritorios, &idsMissoes[p]);
        printf("\nJogador %d (cor %s) recebeu sua missão:\n", p + 1, coresJogadores[p]);
        exibirMissao(missoesJogadores[p]); /* exibida apenas uma vez */
    }

    /* 6) loop de jogo - alterna entre jogadores para realizar ataques; após cada ataque, verifica missões */
    int turno = 0; /* jogador 0 inicia */
    while (1) {
        printf("\n--- Turno do jogador %d (cor %s) ---\n", turno + 1, coresJogadores[turno]);
        exibirMapa(mapa, qtdTerritorios);

        int idxAtacante, idxDefensor;
        printf("Escolha número do território ATACANTE (1 a %d): ", qtdTerritorios);
        scanf("%d", &idxAtacante);
        printf("Escolha número do território DEFENSOR (1 a %d): ", qtdTerritorios);
        scanf("%d", &idxDefensor);

        /* converter para índices base 0 e validar */
        idxAtacante--; idxDefensor--;
        if (idxAtacante < 0 || idxAtacante >= qtdTerritorios || idxDefensor < 0 || idxDefensor >= qtdTerritorios) {
            printf("Índices inválidos. Tente novamente.\n");
            continue;
        }

        /* validação: não atacar território da mesma cor */
        if (strcmp(mapa[idxAtacante].cor, mapa[idxDefensor].cor) == 0) {
            printf("Não é permitido atacar território da mesma cor. Turno perdido.\n");
        } else {
            /* ataque */
            atacar(&mapa[idxAtacante], &mapa[idxDefensor]);
        }

        /* após ação, verificamos silenciosamente se alguma missão foi cumprida */
        for (int p = 0; p < jogadores; p++) {
            int ok = verificarMissao(missoesJogadores[p], mapa, qtdTerritorios, coresJogadores[p]);
            if (ok) {
                printf("\n########################################\n");
                printf("Jogador %d (cor %s) CUMPRIU A MISSÃO: %s\n", p + 1, coresJogadores[p], missoesJogadores[p]);
                printf("######### JOGO ENCERRADO - VENCEDOR #########\n");
                printf("Parabéns!\n");
                printf("########################################\n");

                /* limpa memória e encerra */
                liberarMemoria(mapa, missoesJogadores, jogadores);
                free(idsMissoes);
                return 0;
            }
        }

        /* alterna turno */
        turno = (turno + 1) % jogadores;

        /* opcional: controle para sair do loop (evitar loop infinito de testes) */
        char cont;
        printf("Deseja continuar o jogo? (s/n): ");
        scanf(" %c", &cont);
        if (cont == 'n' || cont == 'N') {
            printf("Jogo encerrado pelo usuário.\n");
            break;
        }
    }

    /* libera memória antes de sair */
    liberarMemoria(mapa, missoesJogadores, jogadores);
    free(idsMissoes);

    return 0;
}
