/*
INTEGRANTES:
- Cláudio Dias Alves (101403569)
- Daniel Rubio Camargo (10408823)

Para compilar no vscode use:
gcc compilador.c -Wall -Og -g -o compilador

// teste de memoria
https://diveintosystems.org/book/C3-C_debug/valgrind.html

Rode o Valgrind com
valgrind --leak-check=yes ./compilador

caso não esteja instalado use
sudo apt update
sudo apt install valgrind
sudo apt upgrade
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

// Enumeração dos possíveis átomos do analisador léxico
typedef enum
{
    ERROLEXICO,
    ERROLEXICOEXCEDECARACTERES,
    ERROSINTATICO,
    ENDOFFILE,
    COMENTARIO,
    ID,
    ADICAO,
    SUBTRACAO,
    DIVISAO,
    MULTIPLICACAO,
    INT,
    CHAR,
    VOID,
    PONTO_VIRGULA,
    ABRE_PAR,
    FECHA_PAR,
    ABRE_CHAVES,
    FECHA_CHAVES,
    IF,
    ELSE,
    WHILE,
    MAIN,
    READINT,
    WRITEINT,
    VIRGULA,
    INTCONST,
    IGUALATRIBUICAO,
    IGUALCOMPARACAO,
    MAIOR,
    MAIORIGUAL,
    MENOR,
    MENORIGUAL,
    DIFERENTEDE,
    NOT,
    AND,
    OR
} TAtomo;

// Definição da tebela de símbolos
typedef struct _TNo
{
    char ID[16];
    int endereco;
    struct _TNo *prox;
} TNo;

// Estrutura para armazenar as informações de um átomo
typedef struct
{
    TAtomo atomo;           // tipo do átomo
    int linha;              // linha onde foi encontrado
    char identificador[15]; // identificador (se for ID)
    char atributo[15];      // atributof (ex: valor inteiro convertido)
} TInfoAtomo;

char *entrada = NULL;  // ponteiro para o conteúdo do código-fonte
TInfoAtomo info_atomo; // variável global que mantém o átomo atual
TAtomo lookahead;      // próximo átomo a ser analisado
int contaLinha = 1;    // contador de linhas para mensagens de erro
int qtd_variaveis = 0; // Contador do número total de variáveis declaradas no escopo atual
int escopo_global = 0; // 1 quando estamos no bloco principal
char buffer_inicializacoes[1000] = "";

// Nome dos átomos e símbolos para exibição
char *strAtomo[] = {
    "errolexico", "errolexicoexcedecaracteres", "errosintatico", "endoffile", "comentario",
    "id", "adicao", "subtracao", "divisao", "multiplicacao", "int", "char", "void", "ponto_virgula",
    "abre_par", "fecha_par", "abre_chaves", "fecha_chaves", "if", "else", "while", "main",
    "readint", "writeint", "virgula", "intconst", "igualatribuicao", "igualcomparacao",
    "maior", "maiorigual", "menor", "menorigual", "diferentede", "not", "and", "or"};
char *strSimbolo[] = {
    "errolexico", "errolexicoexcedecaracteres", "errosintatico", "endoffile", "comentario", "id",
    "+", "-", "/", "*", "int", "char", "void", ";", "(", ")", "{", "}", "if", "else", "while", "main", "readint",
    "writeint", ",", "intconst", "=", "==", ">", ">=", "<", "<=", "!=", "!", "&&", "||"};

// Funções do analisador léxico
TInfoAtomo obter_atomo();
TInfoAtomo verifica_igual();
TInfoAtomo verifica_maior();
TInfoAtomo verifica_menor();
TInfoAtomo verifica_exclamacao();
TInfoAtomo verifica_ecomercial();
TInfoAtomo verifica_pipe();
TInfoAtomo reconhece_id();
TInfoAtomo reconhece_int();
TInfoAtomo reconhece_comentario_ou_divisao();

// Funções do analisador sintático
void program();
void compound_stmt();
void var_decl();
void type_specifier();
void var_decl_list();
void variable_id();
void stmt();
void assig_stmt();
void cond_stmt();
void while_stmt();
void expr();
void conjunction();
void comparision();
void relation();
void sum();
void term();
void factor();
void consome_comentarios();

TNo *tabela = NULL;
int proxEndereco = 0;
int contador_rotulos = 1;

// ---------- SEMÂNTICO ----------
// Verifica se identificador já está na tabela
int verificar_declaracao_simbolo(const char *id)
{
    TNo *atual = tabela;
    while (atual != NULL)
    {
        if (strcmp(atual->ID, id) == 0)
        {
            return 1; // Já foi declarado
        }
        atual = atual->prox;
    }
    return 0; // Ainda não foi declarado
}

// Insere identificador novo na tabela
void inserir_simbolo_tabela(const char *id)
{
    TNo *novo = (TNo *)malloc(sizeof(TNo));
    if (novo == NULL)
    {
        printf("Erro de alocação na tabela de símbolos\n");
        exit(1);
    }

    strcpy(novo->ID, id);
    novo->endereco = proxEndereco++;
    novo->prox = tabela;
    tabela = novo;

    qtd_variaveis++;
}

// Busca uma variável na tabela (para uso)
int verificar_existencia_simbolo(const char *id)
{
    TNo *atual = tabela;
    while (atual != NULL)
    {
        if (strcmp(atual->ID, id) == 0)
        {
            return 1; // Existe na tabela
        }
        atual = atual->prox;
    }
    return 0; // Não encontrado
}

// Retorna um novo rótulo numérico único para geração de código (ex: L0, L1, ...).
int proximo_rotulo()
{
    return contador_rotulos++;
}

// Busca o endereço de uma variável na tabela de símbolos.
int busca_tabela_simbolos(const char *id)
{
    TNo *atual = tabela;
    while (atual != NULL)
    {
        if (strcmp(atual->ID, id) == 0)
        {
            return atual->endereco;
        }
        atual = atual->prox;
    }

    printf("# %d:erro semântico: variável [%s] não declarada\n", contaLinha, id);
    exit(1);
}

// Retorna o número total de variáveis declaradas (equivale ao próximo endereço disponível).
int obter_total_variaveis()
{
    return proxEndereco;
}

// Imprime a tabela de símbolos na ordem de declaração.
void imprimir_tabela_simbolos()
{
    printf("\nTABELA DE SIMBOLOS\n");

    TNo *vetor[100];
    int i = 0;

    TNo *atual = tabela;
    while (atual != NULL && i < 100)
    {
        vetor[i++] = atual;
        atual = atual->prox;
    }

    // Imprime em ordem inversa (do fim para o início da lista encadeada)
    for (int j = i - 1; j >= 0; j--)
    {
        printf("%-4s | Endereco: %d\n", vetor[j]->ID, vetor[j]->endereco);
    }
}

int main(int num_argumentos, char **argumentos)
{
    // Verifica se um arquivo foi passado como argumento
    if (num_argumentos < 2)
    {
        printf("Uso: %s <arquivo>\n", argumentos[0]);
        return 1;
    }

    // Abre o arquivo fonte
    FILE *arquivo = fopen(argumentos[1], "r");
    if (arquivo == NULL)
    {
        printf("Erro ao abrir o arquivo %s!\n", argumentos[1]);
        return 1;
    }

    // Lê o conteúdo do arquivo para memória
    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);

    // Aloca memória para o conteúdo do arquivo
    char *buffer = (char *)malloc(tamanho + 1);
    if (buffer == NULL)
    {
        printf("Erro de alocação de memória!\n");
        fclose(arquivo);
        return 1;
    }

    // Lê o conteúdo do arquivo
    size_t lidos = fread(buffer, 1, tamanho, arquivo);
    buffer[lidos] = '\0';
    fclose(arquivo);

    entrada = buffer;

    // Imprime o primeiro token
    do
    {
        info_atomo = obter_atomo();
        lookahead = info_atomo.atomo;
    } while (lookahead == COMENTARIO);

    // Consome todos os comentários antes de iniciar a análise
    consome_comentarios();

    // Verifica erro léxico de início
    if (info_atomo.atomo == ERROLEXICOEXCEDECARACTERES)
    {
        printf("# %d:erro léxico de excesso de caracteres na declaração de identificador\n", info_atomo.linha);
        free(buffer);
        exit(1);
    }
    else if (info_atomo.atomo == ERROLEXICO)
    {
        printf("# %d:erro léxico\n", info_atomo.linha);
        free(buffer);
        exit(1);
    }

    lookahead = info_atomo.atomo;

    // Inicia o analisador sintático
    program();

    // Após análise sintática, verifica se chegou ao fim do arquivo
    if (lookahead != ENDOFFILE)
    {
        printf("# %d:erro sintático, esperado fim do arquivo, encontrado [%s]\n", contaLinha, strSimbolo[lookahead]);
        free(buffer);
        exit(1);
    }

    free(buffer);

    return 0;
}

// ---------- LÉXICO ----------
// Função principal do analisador léxico: responsável por retornar o próximo átomo do código-fonte
TInfoAtomo obter_atomo()
{
    TInfoAtomo info_atomo;
    memset(&info_atomo, 0, sizeof(TInfoAtomo)); // Inicializa a estrutura zerando todos os campos

    // Ignora espaços em branco, quebras de linha e tabulações
    while (*entrada == ' ' || *entrada == '\n' || *entrada == '\r' || *entrada == '\t')
    {
        if (*entrada == '\n')
            contaLinha++;
        entrada++;
    }

    // Verifica se chegou ao fim do código-fonte
    if (*entrada == '\0')
    {
        info_atomo.atomo = ENDOFFILE; // Marca fim do arquivo
        info_atomo.linha = contaLinha;
        return info_atomo;
    }

    // Verificação do caractere atual para identificar o tipo de átomo
    switch (*entrada)
    {
    case '{':
        info_atomo.atomo = ABRE_CHAVES;
        entrada++;
        break;
    case '}':
        info_atomo.atomo = FECHA_CHAVES;
        entrada++;
        break;
    case '(':
        info_atomo.atomo = ABRE_PAR;
        entrada++;
        break;
    case ')':
        info_atomo.atomo = FECHA_PAR;
        entrada++;
        break;
    case ';':
        info_atomo.atomo = PONTO_VIRGULA;
        entrada++;
        break;
    case ',':
        info_atomo.atomo = VIRGULA;
        entrada++;
        break;
    case '=':
        entrada++;
        info_atomo = verifica_igual(); // Pode ser '=' ou '=='
        break;
    case '>':
        entrada++;
        info_atomo = verifica_maior(); // Pode ser '>' ou '>='
        break;
    case '<':
        entrada++;
        info_atomo = verifica_menor(); // Pode ser '<' ou '<='
        break;
    case '!':
        entrada++;
        info_atomo = verifica_exclamacao(); // Pode ser erro ou '!='
        break;
    case '&':
        entrada++;
        info_atomo = verifica_ecomercial(); // Pode ser erro ou '&&'
        break;
    case '|':
        entrada++;
        info_atomo = verifica_pipe(); // Pode ser erro ou '||'
        break;
    case '+':
        entrada++;
        info_atomo.atomo = ADICAO;
        break;
    case '-':
        entrada++;
        info_atomo.atomo = SUBTRACAO;
        break;
    case '*':
        entrada++;
        info_atomo.atomo = MULTIPLICACAO;
        break;

    default:
        // Reconhece tokens complexos: literais, identificadores, comentários ou gera erro léxico
        if (isdigit(*entrada))
        {
            if (*entrada != '0')
            { // Apenas hexadecimais são aceitos
                info_atomo.atomo = ERROLEXICO;
                break;
            }
            info_atomo = reconhece_int();
        }
        else if (isalpha(*entrada) || *entrada == '_')
        {
            info_atomo = reconhece_id();
        }
        else if (*entrada == '/')
        {
            entrada++;
            info_atomo = reconhece_comentario_ou_divisao();
        }
        break;
    }

    // Associa a linha atual ao átomo reconhecido
    info_atomo.linha = contaLinha;

    return info_atomo;
}

// Função para verificar "=" ou "=="
TInfoAtomo verifica_igual()
{
    TInfoAtomo info_igual;
    // Assume que é uma atribuição "="
    info_igual.atomo = IGUALATRIBUICAO;

    if (*entrada == '=')
    {
        entrada++;
        goto q1; // se houver outro "=", então é uma comparação "=="
    }

    return info_igual;

q1:
    // Atribui que é um "==" (igualdade)
    info_igual.atomo = IGUALCOMPARACAO;
    return info_igual;
}

// Função para verificar ">" ou ">="
TInfoAtomo verifica_maior()
{
    TInfoAtomo info_maior;
    // Assume que é um ">"
    info_maior.atomo = MAIOR;

    if (*entrada == '=')
    {
        entrada++;
        goto q1; // se houver outro "=", então é ">="
    }

    return info_maior;

q1:
    // Atribui ">="
    info_maior.atomo = MAIORIGUAL;
    return info_maior;
}

// Função para verificar "<" ou "<="
TInfoAtomo verifica_menor()
{
    TInfoAtomo info_menor;
    // Assume que é um "<"
    info_menor.atomo = MENOR;

    if (*entrada == '=')
    {
        entrada++;
        goto q1; // se houver "=", então é "<="
    }

    return info_menor;

q1:
    // Atribui "<="
    info_menor.atomo = MENORIGUAL;
    return info_menor;
}

// Função para verificar "!" ou "!="
TInfoAtomo verifica_exclamacao()
{
    TInfoAtomo info_excl;
    // Assume que é um operador NOT (!)
    info_excl.atomo = NOT;

    if (*entrada == '=')
    {
        entrada++;
        goto q1; // se houver "=", então é uma desigualdade "!="
    }

    return info_excl;

q1:
    // Atribui "!="
    info_excl.atomo = DIFERENTEDE;
    return info_excl;
}

// Função para verificar "&&"
TInfoAtomo verifica_ecomercial()
{
    TInfoAtomo info_ecom;
    info_ecom.atomo = ERROLEXICO; // assume erro se não for "&&"

    if (*entrada == '&')
    {
        entrada++;
        info_ecom.atomo = AND; // reconhece operador lógico AND
    }

    return info_ecom;
}

// Função para verificar "||"
TInfoAtomo verifica_pipe()
{
    TInfoAtomo info_pipe;
    info_pipe.atomo = ERROLEXICO; // assume erro se não for "||"

    if (*entrada == '|')
    {
        entrada++;
        info_pipe.atomo = OR; // reconhece operador lógico OR
    }

    return info_pipe;
}

// Reconhece identificadores e palavras-chave
TInfoAtomo reconhece_id()
{
    TInfoAtomo info_id;
    char *ini_id = entrada; // início do identificador
    int count = 1;
    info_id.atomo = ERROLEXICO;

    // Verifica se o primeiro caractere é letra ou "_"
    if (isalpha(*entrada) || *entrada == '_')
    {
        entrada++;
        goto q1;
    }

    return info_id;

q1:
    // Continua lendo letras, dígitos ou "_" (identificadores válidos)
    if (isalpha(*entrada) || isdigit(*entrada) || *entrada == '_')
    {
        if (count < 14)
        { // verifica se não excedeu limite de caracteres
            count++;
            entrada++;
            goto q1;
        }
        else
        {
            entrada++; // permite um último caractere além do limite
        }
    }

    int tamanho = entrada - ini_id;
    if (tamanho > 14)
    {
        // Se exceder o limite, retorna erro específico
        info_id.atomo = ERROLEXICOEXCEDECARACTERES;
        return info_id;
    }

    // Copia o identificador para o atributo do token
    strncpy(info_id.identificador, ini_id, tamanho);
    info_id.identificador[tamanho] = '\0';

    // Verifica se o identificador é uma palavra-chave
    if (strcmp(info_id.identificador, "int") == 0)
        info_id.atomo = INT;
    else if (strcmp(info_id.identificador, "char") == 0)
        info_id.atomo = CHAR;
    else if (strcmp(info_id.identificador, "if") == 0)
        info_id.atomo = IF;
    else if (strcmp(info_id.identificador, "else") == 0)
        info_id.atomo = ELSE;
    else if (strcmp(info_id.identificador, "while") == 0)
        info_id.atomo = WHILE;
    else if (strcmp(info_id.identificador, "main") == 0)
        info_id.atomo = MAIN;
    else if (strcmp(info_id.identificador, "readint") == 0)
        info_id.atomo = READINT;
    else if (strcmp(info_id.identificador, "writeint") == 0)
        info_id.atomo = WRITEINT;
    else if (strcmp(info_id.identificador, "void") == 0)
        info_id.atomo = VOID;
    else
        info_id.atomo = ID; // caso contrário, é um identificador comum

    // Limpa atributo se for palavra reservada (não precisa armazenar nome)
    if (info_id.atomo != ID)
    {
        strcpy(info_id.identificador, "");
    }

    return info_id;
}

// Reconhece constantes inteiras no formato hexadecimal (ex: 0x1A3F)
TInfoAtomo reconhece_int()
{
    TInfoAtomo info_int;
    info_int.atomo = ERROLEXICO;
    char buffer[15]; // Armazena os dígitos hexadecimais
    int idx = 0;

    if (*entrada == '0')
    { // Início padrão de hexadecimal
        entrada++;
        goto q1;
    }

    return info_int;

q1:
    if (*entrada == 'x' || *entrada == 'X')
    {
        entrada++;
        goto q2;
    }

    return info_int;

q2:
    // Verifica se há ao menos um dígito hexadecimal
    if (isxdigit(*entrada))
    {
        buffer[idx++] = *entrada;
        entrada++;
        goto q3;
    }

    return info_int;

q3:
    // Continua lendo os próximos dígitos hexadecimais
    if (isxdigit(*entrada))
    {
        if (idx < 14)
            buffer[idx++] = *entrada;
        entrada++;
        goto q3;
    }

    buffer[idx] = '\0'; // Termina a string
    goto q4;

q4:
    // Verifica se o próximo caractere delimita fim de número (ex: espaço, ponto e vírgula, etc)
    if (isspace(*entrada) || *entrada == '\0' || *entrada == ';' || *entrada == ')' || *entrada == ',')
    {
        long valor = strtol(buffer, NULL, 16);    // Converte para decimal
        sprintf(info_int.atributo, "%ld", valor); // Salva o valor como string
        info_int.atomo = INTCONST;                // Define o tipo de átomo como constante inteira
    }

    return info_int;
}

// COMENTARIO OU DIVISAO
TInfoAtomo reconhece_comentario_ou_divisao()
{

    TInfoAtomo info;
    info.atomo = ERROLEXICO;

    goto q1;

q1:
    if (*entrada == '/')
    {
        entrada++;
        goto q2;
    }
    else if (*entrada == '*')
    {
        entrada++;
        goto q3;
    }
    else
    {
        info.atomo = DIVISAO;
        info.linha = contaLinha;
        return info;
    }

q2:
    if (*entrada == '\0')
    {
        info.atomo = COMENTARIO;
        info.linha = contaLinha;
        return info;
    }
    else if (*entrada == '\n')
    {
        entrada++;
        contaLinha++;
        info.atomo = COMENTARIO;
        info.linha = contaLinha;
        return info;
    }
    else
    {
        entrada++;
        goto q2;
    }

q3:
    if (*entrada == '\0')
    {
        info.atomo = ERROLEXICO;
        info.linha = contaLinha;
        return info;
    }
    else if (*entrada == '*' && *(entrada + 1) == '/')
    {
        entrada += 2;
        info.atomo = COMENTARIO;
        info.linha = contaLinha;
        return info;
    }
    else
    {
        if (*entrada == '\n')
            contaLinha++;
        entrada++;
        goto q3;
    }
}

// ---------- SINTÁTICO ----------
// consome e printa as informações
void consome(TAtomo atomo)
{
    if (lookahead == atomo)
    {
        // Agora avança para o próximo token
        info_atomo = obter_atomo();

        // Verifica erros léxicos
        if (info_atomo.atomo == ERROLEXICOEXCEDECARACTERES)
        {
            printf("# %d:erro léxico de excesso de caracteres na declaração de identificador\n", info_atomo.linha);
            exit(1);
        }
        else if (info_atomo.atomo == ERROLEXICO)
        {
            printf("# %d:erro léxico\n", info_atomo.linha);
            exit(1);
        }

        lookahead = info_atomo.atomo;
    }
    else
    {
        printf("# %d:erro sintático, esperado [%s], encontrado [%s]\n", contaLinha, strSimbolo[atomo], strSimbolo[lookahead]);
        exit(1);
    }
}

// ignorar os comentários
void consome_comentarios()
{
    while (lookahead == COMENTARIO)
        consome(COMENTARIO);
}

// ---------- GERAÇÃO DE CÓDIGO INTERMEDIÁRIO (MEPA) ----------
// <program> ::= void main ‘(‘ void ‘)’ <compound_stmt>
void program()
{
    escopo_global = 1; // indica que estamos no escopo global (main)

    consome_comentarios();
    consome(VOID);
    consome_comentarios();
    consome(MAIN);
    consome_comentarios();
    consome(ABRE_PAR);
    consome_comentarios();
    consome(VOID);
    consome_comentarios();
    consome(FECHA_PAR);
    compound_stmt();
}

// <compound_stmt> ::= '{' <var_decl> { <stmt> } '}'
void compound_stmt()
{
    consome_comentarios();
    consome(ABRE_CHAVES);
    consome_comentarios();

    int escopo_era_global = escopo_global;
    int adiar_amem = 0; // flag
    int total_amem = 0; // valor a ser impresso depois

    if (escopo_global)
    {
        printf("    INPP\n");
        escopo_global = 0;
        adiar_amem = 1; // marca para adiar
    }

    // processa as declarações de variáveis
    while (lookahead == INT || lookahead == CHAR)
    {
        var_decl();
        consome_comentarios();
    }

    // só agora que as variáveis foram declaradas, obtemos o valor correto
    if (adiar_amem)
    {
        total_amem = obter_total_variaveis();
        printf("    AMEM %d\n", total_amem);
        printf("%s", buffer_inicializacoes); // agora imprime os ARMZ salvos
    }

    // comandos do bloco
    while (lookahead == ID || lookahead == IF || lookahead == WHILE ||
           lookahead == READINT || lookahead == WRITEINT || lookahead == ABRE_CHAVES || lookahead == COMENTARIO)
    {
        consome_comentarios();
        stmt();
        consome_comentarios();
    }

    consome_comentarios();
    consome(FECHA_CHAVES);

    if (escopo_era_global)
    {
        printf("    PARA\n");
        imprimir_tabela_simbolos();
    }
}

// <var_decl> ::= [ <type_specifier> <var_decl_list> ‘;’ ]
void var_decl()
{
    consome_comentarios();

    if (lookahead == INT || lookahead == CHAR)
    {
        type_specifier();
        var_decl_list();
        consome_comentarios();
        consome(PONTO_VIRGULA);
    }
}

// <type_specifier> ::= int | char
void type_specifier()
{
    consome_comentarios();

    switch (lookahead)
    {
    case INT:
        consome(INT);
        break;
    case CHAR:
        consome(CHAR);
        break;
    default:
        printf("# %d:erro sintático, esperado tipo [INT] ou [CHAR], encontrado [%s]\n", contaLinha, strSimbolo[lookahead]);
        exit(1);
    }
}

// <var_decl_list> ::= <variable_id> { ',' <variable_id> }
void var_decl_list()
{
    consome_comentarios();

    variable_id();
    while (lookahead == VIRGULA)
    {
        consome(VIRGULA);
        variable_id();
    }
}

// <variable_id> ::= id [ ‘=’ <expr> ]
void variable_id()
{
    consome_comentarios();

    if (lookahead != ID)
    {
        printf("# %d:erro sintático, esperado identificador\n", contaLinha);
        exit(1);
    }

    char nome_id[16];
    strcpy(nome_id, info_atomo.identificador);
    consome(ID);

    if (verificar_declaracao_simbolo(nome_id))
    {
        printf("# %d:erro semântico, identificador [%s] já declarado\n", info_atomo.linha, nome_id);
        exit(1);
    }

    inserir_simbolo_tabela(nome_id);

    // Verifica se tem uma inicialização
    if (lookahead == IGUALATRIBUICAO)
    {
        consome_comentarios();
        consome(IGUALATRIBUICAO);

        // Captura o valor da constante (suporte apenas para CRCT simples)
        if (lookahead == INTCONST)
        {
            int valor = atoi(info_atomo.atributo);
            consome(INTCONST);

            int endereco = busca_tabela_simbolos(nome_id);

            char linha[128];
            sprintf(linha, "    CRCT %d\n    ARMZ %d\n", valor, endereco);
            strcat(buffer_inicializacoes, linha);
        }
        else
        {
            // Se não for constante direta, processa normalmente (expr + ARMZ direto)
            expr();

            int endereco = busca_tabela_simbolos(nome_id);

            char linha[64];
            sprintf(linha, "    ARMZ %d\n", endereco);
            strcat(buffer_inicializacoes, linha);
        }
    }
}

// <stmt> ::= <compound_stmt> | <assig_stmt> | <cond_stmt> | <while_stmt> | readint ‘(‘ id ‘)’ ‘;’ | writeint ‘(‘ <expr> ‘)’ ‘;’
void stmt()
{
    consome_comentarios();

    switch (lookahead)
    {
    case ABRE_CHAVES:
        compound_stmt();
        break;

    case ID:
        assig_stmt();
        consome_comentarios();
        consome(PONTO_VIRGULA);
        break;

    case IF:
        cond_stmt();
        break;

    case WHILE:
        while_stmt();
        break;

    case READINT:
        consome(READINT);
        consome_comentarios();
        consome(ABRE_PAR);

        char nome_lido[16];
        strcpy(nome_lido, info_atomo.identificador);
        consome_comentarios();
        consome(ID);

        // Verifica se a variável foi declarada antes do uso
        if (!verificar_existencia_simbolo(nome_lido))
        {
            printf("# %d:erro semântico, identificador [%s] não declarado\n", info_atomo.linha, nome_lido);
            exit(1);
        }

        int end_lido = busca_tabela_simbolos(nome_lido);

        printf("    LEIT\n");              // leitura da entrada padrão
        printf("    ARMZ %d\n", end_lido); // armazena o valor na variável lida

        consome_comentarios();
        consome(FECHA_PAR);
        consome_comentarios();
        consome(PONTO_VIRGULA);
        break;

    case WRITEINT:
        consome(WRITEINT);
        consome_comentarios();
        consome(ABRE_PAR);

        expr();               // avalia e empilha o valor a ser impresso
        printf("    IMPR\n"); // imprime o valor no topo da pilha

        consome_comentarios();
        consome(FECHA_PAR);
        consome_comentarios();
        consome(PONTO_VIRGULA);
        break;

    default:
        printf("# %d:erro sintático: início de comando inválido\n", contaLinha);
        exit(1);
    }
}

// <assig_stmt> ::= id ‘=’ <expr> ‘;’
void assig_stmt()
{
    char nome_id[16];
    strcpy(nome_id, info_atomo.identificador);
    consome_comentarios();
    consome(ID);

    if (!verificar_existencia_simbolo(nome_id))
    {
        printf("# %d:erro semântico, identificador [%s] não declarado\n", contaLinha, nome_id);
        exit(1);
    }

    consome_comentarios();
    consome(IGUALATRIBUICAO);
    expr();
    int end_dest = busca_tabela_simbolos(nome_id);
    printf("    ARMZ %d\n", end_dest);
    consome_comentarios();
}

// <cond_stmt> ::= if ‘(‘ <expr> ‘)’ <stmt> [ else <stmt> ]
void cond_stmt()
{
    consome_comentarios();

    int L1 = proximo_rotulo();
    int L2 = proximo_rotulo();

    consome(IF);
    consome_comentarios();
    consome(ABRE_PAR);
    expr(); // vai gerar código da expressão condicional
    consome_comentarios();
    consome(FECHA_PAR);

    printf("    DSVF L%d\n", L1); // desvia se falso
    stmt();                       // bloco do if
    printf("    DSVS L%d\n", L2); // desvia sempre para o fim (pula o else)
    printf("L%d: NADA\n", L1);    // rótulo do else

    if (lookahead == ELSE)
    {
        consome(ELSE);
        stmt(); // bloco do else
    }

    printf("L%d: NADA\n", L2); // rótulo final
}

// <while_stmt> ::= while ‘(‘ <expr> ‘)’ <stmt>
void while_stmt()
{
    consome(WHILE);
    int L1 = proximo_rotulo();
    int L2 = proximo_rotulo();

    printf("L%d: NADA\n", L1);

    consome_comentarios();
    consome(ABRE_PAR);
    expr();
    printf("    DVSF L%d\n", L2);

    consome_comentarios();
    consome(FECHA_PAR);

    stmt();

    printf("    DSVS L%d\n", L1);
    printf("L%d: NADA\n", L2);
}

// <expr> ::= <conjunction> { '||' <conjunction> }
void expr()
{
    consome_comentarios();

    conjunction();
    while (lookahead == OR)
    {
        consome(OR);
        printf("    DISJ\n");
        conjunction();
    }
}

// <conjunction> ::= <comparison> { '&&' <comparison> }
void conjunction()
{
    consome_comentarios();

    comparision();
    while (lookahead == AND)
    {
        consome(AND);
        printf("    CONJ\n");
        comparision();
    }
}

// <comparison> ::= <sum> [ <relation> <sum> ]
void comparision()
{
    consome_comentarios();

    sum();

    if (lookahead == MENOR || lookahead == MENORIGUAL || lookahead == IGUALCOMPARACAO || lookahead == DIFERENTEDE || lookahead == MAIOR || lookahead == MAIORIGUAL)
    {
        int operador = lookahead;
        relation();
        sum();

        switch (operador)
        {
        case MENOR:
            printf("    CMME\n");
            break;
        case MENORIGUAL:
            printf("    CMEG\n");
            break;
        case IGUALCOMPARACAO:
            printf("    CMIG\n");
            break;
        case DIFERENTEDE:
            printf("    CMDG\n");
            break;
        case MAIOR:
            printf("    CMMA\n");
            break;
        case MAIORIGUAL:
            printf("    CMAG\n");
            break;
        }
    }
}

// <relation> ::= “<” | “<=” | “==” | “!=” | “>” | “>=”
void relation()
{
    consome_comentarios();

    switch (lookahead)
    {
    case MENOR:
        consome(MENOR);
        break;
    case MENORIGUAL:
        consome(MENORIGUAL);
        break;
    case IGUALCOMPARACAO:
        consome(IGUALCOMPARACAO);
        break;
    case DIFERENTEDE:
        consome(DIFERENTEDE);
        break;
    case MAIOR:
        consome(MAIOR);
        break;
    case MAIORIGUAL:
        consome(MAIORIGUAL);
        break;
    default:
        printf("# %d:erro sintático, esperado algum operador relacional (<, <=, ==, !=, >, >=), encontrado [%s]\n", contaLinha, strSimbolo[lookahead]);
        exit(1);
    }
}

// <sum> ::= <term> { ('+' | '-') <term> }
void sum()
{
    consome_comentarios();

    term();
    while (lookahead == ADICAO || lookahead == SUBTRACAO)
    {
        if (lookahead == ADICAO)
        {
            consome(ADICAO);
            term();
            printf("    SOMA\n");
        }
        else
        {
            consome(SUBTRACAO);
            term();
            printf("    SUBT\n");
        }
    }
}

// <term> ::= <factor> { ('*' | '/') <factor> }
void term()
{
    consome_comentarios();

    factor();
    while (lookahead == MULTIPLICACAO || lookahead == DIVISAO)
    {
        if (lookahead == MULTIPLICACAO)
        {
            consome(MULTIPLICACAO);
            factor();
            printf("    MULT\n");
        }
        else
        {
            consome(DIVISAO);
            factor();
            printf("    DIVI\n");
        }
    }
}

// <factor> ::= intconst | id | ‘(‘ <expr> ‘)’
void factor()
{
    consome_comentarios();

    switch (lookahead)
    {
    case INTCONST:
    {
        int valor = atoi(info_atomo.atributo);
        printf("    CRCT %d\n", valor);
        consome(INTCONST);
        break;
    }

    case ID:
    {
        char nome[16];
        strcpy(nome, info_atomo.identificador);
        int endereco = busca_tabela_simbolos(nome);
        printf("    CRVL %d\n", endereco);
        consome(ID);
        break;
    }

    case ABRE_PAR:
        consome(ABRE_PAR);
        expr();
        consome_comentarios();
        consome(FECHA_PAR);
        break;

    default:
        printf("# %d:erro sintático, esperado fator válido — encontrado [%s]\n", contaLinha, strSimbolo[lookahead]);
        exit(1);
    }
}