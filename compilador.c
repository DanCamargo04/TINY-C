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
    char atributo[15];
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

// declaracao das funcoes

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
            entrada++;
            break;
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
            info_atomo.atomo = IGUAL;
            entrada++;
            break;
        case '>':
            info_atomo.atomo = MAIOR;
            entrada++;
            break;
        case '<':
            info_atomo.atomo = MENOR;
            entrada++;
            break;
        case '!':
            info_atomo.atomo = EXCLAMACAO;
            entrada++;
            break;
        case '&':
            info_atomo.atomo = ECOMERCIAL;
            entrada++;
            break;
        case '|':
            info_atomo.atomo = PIPE;
            entrada++;           
            break;
        default:
            if(isdigit(*entrada)){
                info_atomo = reconhece_int();
            }
            else if(*entrada == '\''){
                entrada++;
                info_atomo = reconhece_char();
            } 
            else if(isalpha(*entrada) || *entrada == '_'){
                info_atomo = reconhece_id();
            }
            else if(*entrada == '/' && *(entrada + 1) == '/'){
                entrada+=2;
                info_atomo = reconhece_comentario_simples();
            }
            else if(*entrada == '/' && *(entrada + 1) == '*'){
                entrada+=2;
                info_atomo = reconhece_comentario_composto();
            }
            break;
    }    
    
    info_atomo.linha = contaLinha;
    
    return info_atomo;
}

// IDENTIFICADOR -> (LETRA_MINUSCULA | LETRA_MAIUSCULA | _)+(LETRA_MINUSCULA | LETRA_MAIUSCULA | _ | DIGITO)* ||| Inserir o atributo
// PALAVRAS RESERVADAS: char, else, if, int, main, readint, void, while, writeint
TInfoAtomo reconhece_id(){
    TInfoAtomo info_id;
    char *ini_id = entrada;
    int count = 0;
    info_id.atomo = ERRO;

    if(isalpha(*entrada) || *entrada == '_') {
        entrada++;
        goto q1;
    }
    return info_id;

q1:
    if (count > 14) { 
        return info_id;
    }

    if(isalnum(*entrada) || *entrada == '_') {
        count++;
        entrada++;
        goto q1;
    }

    strncpy(info_id.identificador, ini_id, entrada - ini_id);
    info_id.identificador[entrada - ini_id] = '\0';

    // Verifica se há palavras reservadas
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
        info_id.atomo = ID;

    return info_id;
}

// INT -> DIGITO+ (HEXADECIMAL)
TInfoAtomo reconhece_int(){
    TInfoAtomo info_int;
    info_int.atomo = ERRO;
    memset(info_int.atributo, 0, sizeof(info_int.atributo));

    char *inicio = entrada;

    if (entrada[0] == '0' && (entrada[1] == 'x' || entrada[1] == 'X')) {
        entrada += 2;
        if (!isxdigit(*entrada)) {
            return info_int; // precisa ter ao menos 1 caractere após 0x
        }

        while (isxdigit(*entrada)) {
            entrada++;
        }

        int len = entrada - inicio;
        if (len < sizeof(info_int.atributo)) {
            strncpy(info_int.atributo, inicio, len);
            info_int.atributo[len] = '\0';
        } else {
            return info_int; // valor hexadecimal muito grande
        }

        info_int.atomo = INTCONST;
        return info_int;
    }

    return info_int;
}

// CHAR
TInfoAtomo reconhece_char(){ // ok
    TInfoAtomo info_char;
    info_char.atomo = ERRO;

    info_char.atributo[0] = *entrada;
    entrada++;
        
    if(*entrada != '\''){
        return info_char;
    }

    entrada++;

    if(!isspace(*entrada)){ // verfica se há um espaço após o fechamento das aspas
        return info_char;
    }

    entrada++;

    info_char.atomo = CHAR;
    return info_char;
}

// COMENTARIO SIMPLES
TInfoAtomo reconhece_comentario_simples(){ // ok
    TInfoAtomo info_coment;
    info_coment.atomo = COMENTARIO;

    while (*entrada != '\n' && *entrada != '\0') {
        entrada++; 
    }

    return info_coment;
}

// COMENTARIO COMPOSTO
TInfoAtomo reconhece_comentario_composto(){ // ok
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