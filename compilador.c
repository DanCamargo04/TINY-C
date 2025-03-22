/*
Para compilar no vscode use:
gcc compilador.c -Wall -Og -g -o compilador

// teste de memoria
https://diveintosystems.org/book/C3-C_debug/valgrind.html

Rode o Valgrind com 
valgrind --leak-check=yes ./miniLexico 

caso não esteja instalado use
sudo apt update
sudo apt install valgrind
sudo apt upgrade
*/

#include <stdio.h>
#include <ctype.h>
#include <string.h> // strncpy
#include <stdlib.h> // atof

// definicoes dos atomos
typedef enum{
    ERRO,
    ENDOFFILE,
    ENDOFSTRING,
    COMENTARIO,
    ID,
    INT,
    CHAR,
    VOID,
    PONTO_VIRGULA,
    ABRE_PAR,
    FECHA_PAR,
    ABRE_CHAVES,
    FECHA_CHAVES,
    ASPAS,
    IF,
    ELSE,
    WHILE,
    MAIN,
    READINT,
    WRITEINT,
    VIRGULA,
    INTCONST,
    CHARCONST,
    IGUAL,
    MAIOR,
    MENOR,
    EXCLAMACAO,
    ECOMERCIAL,
    PIPE // fim de buffer
}TAtomo;

typedef struct{
    TAtomo atomo;
    int linha;
    char identificador[15];
}TInfoAtomo;

// declaracao de variaveis globais

char *entrada = NULL;

char *strAtomo[] = {
    "ERRO",
    "ENDOFFILE",
    "ENDOFSTRING",
    "COMENTARIO",
    "ID",
    "INT",
    "CHAR",
    "VOID",
    "PONTO_VIRGULA",
    "ABRE_PAR",
    "FECHA_PAR",
    "ABRE_CHAVES",
    "FECHA_CHAVES",
    "ASPAS",
    "IF",
    "ELSE",
    "WHILE",
    "MAIN",
    "READINT",
    "WRITEINT",
    "VIRGULA",
    "INTCONST",
    "CHARCONST",
    "IGUAL",
    "MAIOR",
    "MENOR",
    "EXCLAMACAO",
    "ECOMERCIAL",
    "PIPE"
};

int contaLinha = 1;

// declaracao da funcao

TInfoAtomo obter_atomo();
TInfoAtomo reconhece_id();
TInfoAtomo reconhece_int();
TInfoAtomo reconhece_char();
TInfoAtomo reconhece_comentario_simples();
TInfoAtomo reconhece_comentario_composto();

int main(int argc, char *argv[]){

    // verifica se o nome do arquivo foi passado como argumento
    if (argc != 2) {
        printf("%s\n", argv[0]);
        return 1;
    }

    FILE *arquivo;

    arquivo = fopen(argv[1], "r");

    if (arquivo == NULL) {
        printf("Erro na abertura do arquivo!\n");
        return 1;
    }

    // determina o tamanho do arquivo
    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);
        
    // aloca memória para o conteúdo do arquivo
    char *buffer = (char *)malloc(tamanho + 1);
    if (buffer == NULL) {
        printf("Erro de alocação de memória!\n");
        fclose(arquivo);
        return 1;
    }
        
    // lê o conteúdo de um arquivo e armazena os dados em um buffer
    size_t lidos = fread(buffer, 1, tamanho, arquivo);
    buffer[lidos] = '\0';  // Adiciona o terminador nulo
    entrada = buffer;

    fclose(arquivo);

    TInfoAtomo info_atm;

    do{
        info_atm = obter_atomo();

        // FUNCAO DO SINTATICO

        printf("%03d# %s", info_atm.linha,strAtomo[info_atm.atomo]);
        if(info_atm.atomo == ID){
            printf(" | %s", info_atm.identificador);
        }
        printf("\n");

    }while(info_atm.atomo != ERRO && info_atm.atomo != ENDOFFILE);

    free(buffer);

    printf("Fim de analise lexica e sintatica!\n");
}

// LEXICO ---------------------------------------------------------------------------------------------------

// implementacao da funcao
TInfoAtomo obter_atomo(){
    TInfoAtomo info_atomo;
    info_atomo.atomo = ERRO;

    // eliminar delimitadores
    while(*entrada == ' '|| 
          *entrada == '\n'||
          *entrada == '\r'||
          *entrada == '\t'){
        if(*entrada == '\n')
            contaLinha++;
        entrada++;
    }

    switch (*entrada) {
        case '\0':
            info_atomo.atomo = ENDOFSTRING;
            break;
        case '{':
            info_atomo.atomo = ABRE_CHAVES;
            break;
        case '}':
            info_atomo.atomo = FECHA_CHAVES;
            break;
        case '(':
            info_atomo.atomo = ABRE_PAR;
            break;
        case ')':
            info_atomo.atomo = FECHA_PAR;
            break;
        case '"':
        case '\'':
            info_atomo.atomo = ASPAS;
            break;
        case ';':
            info_atomo.atomo = PONTO_VIRGULA;
            break;
        case ',':
            info_atomo.atomo = VIRGULA;
            break;
        case '=':
            info_atomo.atomo = IGUAL;
            break;
        case '>':
            info_atomo.atomo = MAIOR;
            break;
        case '<':
            info_atomo.atomo = MENOR;
            break;
        case '!':
            info_atomo.atomo = EXCLAMACAO;
            break;
        case '&':
            info_atomo.atomo = ECOMERCIAL;
            break;
        case '|':
            info_atomo.atomo = PIPE;
            break;
        default:
            if(isdigit(*entrada)){
                info_atomo = reconhece_int();
            } 
            else if(isalpha(*entrada) || *entrada == '_'){
                info_atomo = reconhece_id();
            }
            else if(*entrada == '/' && *(entrada + 1) == '/'){
                info_atomo = reconhece_comentario_simples();
            }
            else if(*entrada == '/' && *(entrada + 1) == '*'){
                info_atomo = reconhece_comentario_composto();
            }
            break;
    }    
    
    info_atomo.linha = contaLinha;
    
    return info_atomo;
}

// IDENTIFICADOR -> (LETRA_MINUSCULA | LETRA_MAIUSCULA | _)+(LETRA_MINUSCULA | LETRA_MAIUSCULA | _ | DIGITO)*
TInfoAtomo reconhece_id(){
    TInfoAtomo info_id;
    char *ini_id = entrada;
    int count = 1;
    info_id.atomo = ERRO;

    if(isalpha(*entrada) || *entrada == '_') {
        entrada ++;// consome letra maiuscula, minuscula ou _
        goto q1;
    }
    return info_id;
q1:
    if (count > 15) { // Excedeu 15 caracteres
        return info_id;
    }
    if(isalpha(*entrada) || *entrada == '_' || isdigit(*entrada)){
        count++;
        entrada ++;// consome letra maiuscula, minuscula ou _
        goto q1;
    }
    
    info_id.atomo = ID;

    strncpy(info_id.identificador, ini_id, entrada - ini_id);
    info_id.identificador[entrada - ini_id] = '\0';

    return info_id;
}

// INT -> DIGITO+
TInfoAtomo reconhece_int(){
    TInfoAtomo info_int;
    info_int.atomo = ERRO;

    if(isdigit(*entrada)){
        entrada++;
        goto q1;
    }

    return info_int;

q1:
    if(isdigit(*entrada)){
        entrada++;
        goto q1;
    }

    info_int.atomo = INT;
    return info_int;
}

// CHAR
TInfoAtomo reconhece_char(){
    TInfoAtomo info_char;
    info_char.atomo = ERRO;

    if(isalpha(*entrada) || isdigit(*entrada) || *entrada == "_"){
        entrada++;
        info_char.atomo = CHAR;
    }

    return info_char;
}

// COMENTARIO SIMPLES
TInfoAtomo reconhece_comentario_simples(){
    TInfoAtomo info_coment;
    info_coment.atomo = COMENTARIO;

    while (*entrada != '\n' && *entrada != '\0') {
        entrada++; 
    }

    return info_coment;
}

// COMENTARIO COMPOSTO
TInfoAtomo reconhece_comentario_composto(){
    TInfoAtomo info_coment;
    info_coment.atomo = COMENTARIO;

    entrada++;
    while (*entrada != '\0' && !(*entrada == '*' && *(entrada + 1) == '/')) {
        if (*entrada == '\n') contaLinha++; 
        entrada++;
    }

    if (*entrada == '*') entrada += 2;

    return info_coment;
}


// SINTATICO ---------------------------------------------------------------------------------------------------