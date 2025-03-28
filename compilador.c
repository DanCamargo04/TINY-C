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
    ERROLEXICO,
    ERROSINTATICO,
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
    IF,
    ELSE,
    WHILE,
    MAIN,
    READINT,
    WRITEINT,
    VIRGULA,
    INTCONST,
    CHARCONST,
    IGUALATRIBUICAO,
    IGUALCOMPARACAO,
    MAIOR,
    MAIORIGUAL,
    MENOR,
    MENORIGUAL,
    NOT,
    AND,
    OR // fim de buffer
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
    "ERROLEXICO",
    "ERROSINTATICO",
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
    "IF",
    "ELSE",
    "WHILE",
    "MAIN",
    "READINT",
    "WRITEINT",
    "VIRGULA",
    "INTCONST",
    "CHARCONST",
    "IGUALATRIBUICAO",
    "IGUALCOMPARACAO",
    "MAIOR",
    "MAIORIGUAL",
    "MENOR",
    "MENORIGUAL",
    "NOT",
    "AND",
    "OR" 
};

int contaLinha = 1;

// declaracao das funcoes

TInfoAtomo obter_atomo();
TInfoAtomo reconhece_id();
TInfoAtomo reconhece_int();
TInfoAtomo reconhece_char();
TInfoAtomo reconhece_comentario();

int main(int argc, char *argv[]){

    // verifica se o nome do arquivo foi passado como argumento
    if (argc != 2) {
        printf("%s\n", argv[0]);
        return 1;
    }

    FILE *arquivo;

    arquivo = fopen(argv[1], "r");

    if (arquivo == NULL) {
        printf("ERROLEXICO na abertura do arquivo!\n");
        return 1;
    }

    // determina o tamanho do arquivo
    fseek(arquivo, 0, SEEK_END);
    long tamanho = ftell(arquivo);
    fseek(arquivo, 0, SEEK_SET);
        
    // aloca memória para o conteúdo do arquivo
    char *buffer = (char *)malloc(tamanho + 1);
    if (buffer == NULL) {
        printf("ERROLEXICO de alocação de memória!\n");
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

    }while(info_atm.atomo != ERROLEXICO && info_atm.atomo != ENDOFFILE);

    free(buffer);

    printf("Fim de analise lexica e sintatica!\n");
}

// LEXICO ---------------------------------------------------------------------------------------------------

// implementacao da funcao
TInfoAtomo obter_atomo(){
    TInfoAtomo info_atomo;
    info_atomo.atomo = ERROLEXICO;

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
        default:

            // CRIAR AS FUNÇÕES PARA OPERADORES RELACIONAIS E LÓGICOS

            if(isdigit(*entrada)){ // q0
                if(*entrada != '0'){
                    info_atomo.atomo = ERROLEXICO;
                    break;
                }
                info_atomo = reconhece_int();
            }
            else if(*entrada == '\''){ // q0
                entrada++;
                info_atomo = reconhece_char();
            } 
            else if(isalpha(*entrada) || *entrada == '_'){ // q0
                info_atomo = reconhece_id();
            }
            else if(*entrada == '/'){ // q0
                entrada++;
                info_atomo = reconhece_comentario();
            }
            
            break;
    }    
    
    info_atomo.linha = contaLinha;
    
    return info_atomo;
}

// IDENTIFICADOR -> (LETRA_MINUSCULA | LETRA_MAIUSCULA | _)+(LETRA_MINUSCULA | LETRA_MAIUSCULA | _ | DIGITO)* ||| Inserir o atributo
// PALAVRAS RESERVADAS: char, else, if, int, main, readint, void, while, writeint
TInfoAtomo reconhece_id() {

    TInfoAtomo info_id;
    char *ini_id = entrada;
    int count = 1; // já vai consumir o primeiro caractere
    info_id.atomo = ERROLEXICO;

    if (isalpha(*entrada) || *entrada == '_') {
        entrada++;
        goto q1;
    }

    return info_id;

q1:

    if (isalpha(*entrada) || isdigit(*entrada) || *entrada == '_') {
        if (count < 14) {
            count++;
            entrada++;
            goto q1;
        } else {
            entrada++; // consome o último, mas ignora se ultrapassar
            goto q1;
        }
    }

    strncpy(info_id.identificador, ini_id, entrada - ini_id);
    info_id.identificador[entrada - ini_id] = '\0';

    // Verifica se é palavra reservada

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
TInfoAtomo reconhece_int() {

    TInfoAtomo info_int;
    info_int.atomo = ERROLEXICO;

    char buffer[15]; // para guardar os dígitos hexadecimais
    int idx = 0;

    if (*entrada == '0') {
        entrada++;
        goto q1;
    }

    return info_int;

q1:
    if (*entrada == 'x' || *entrada == 'X') {
        entrada++;
        goto q2;
    }

    return info_int;

q2:
    if (isxdigit(*entrada)) {
        buffer[idx++] = *entrada;
        entrada++;
        goto q3;
    }

    return info_int;

q3:
    if (isxdigit(*entrada)) {
        if (idx < 14) buffer[idx++] = *entrada;
        entrada++;
        goto q3;
    }

    buffer[idx] = '\0'; // finaliza a string com '\0'
    goto q4;

q4: // final

    if (isspace(*entrada) || *entrada == '\0' || *entrada == ';' || *entrada == ')' || *entrada == ',') {
        long valor = strtol(buffer, NULL, 16); // converte hexadecimal para inteiro
        sprintf(info_int.atributo, "%ld", valor); // salva como string em atributo
        info_int.atomo = INTCONST;
    }

    return info_int;
}

// CHAR
TInfoAtomo reconhece_char(){ 

    TInfoAtomo info_char;
    info_char.atomo = ERROLEXICO;

    info_char.atributo[0] = *entrada;
    entrada++; // q1
        
    if(*entrada != '\''){
        entrada++;
        goto q2;
    }

q2:

    if(isspace(*entrada) || *entrada == '\0' || *entrada == ';' || *entrada == ')' || *entrada == ','){ // verfica se há um espaço após o fechamento das aspas
        entrada++;
        info_char.atomo = CHARCONST;
    }

    return info_char;
}

// COMENTARIO
TInfoAtomo reconhece_comentario(){

    TInfoAtomo info_coment;
    info_coment.atomo = ERROLEXICO;

    if(*entrada == '/'){
        entrada++;
        goto q1;
    }
    else if(*entrada == '*'){
        entrada++;
        goto q2;
    }
    else{
        return info_coment;
    }

q1:
    
    if(*entrada != '\0' || *entrada != '\n'){
        entrada++;
        goto q1;
    }
    entrada++;
    goto q3;

q2:

    if(*entrada == '*'){
        entrada++;
        goto q4;
    }
    entrada++;
    goto q2;

q3: // final

    info_coment.atomo = COMENTARIO;
    return info_coment;

q4: 

    if(*entrada == '/'){
        entrada++;
        goto q3;
    }
    entrada++;
    goto q2;

}

// SINTATICO ---------------------------------------------------------------------------------------------------