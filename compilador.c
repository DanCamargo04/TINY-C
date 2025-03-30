/*
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
#include <string.h> // strncpy
#include <stdlib.h> // atof

// definicoes dos atomos
typedef enum{
    ERROLEXICO,
    ERROSINTATICO,
    ENDOFFILE,
    COMENTARIO,
    ID,
    ADD,
    SUB,
    DIVI,
    MULTI,
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
    DIFERENTEDE,
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

// Declaracao de variaveis globais

char *entrada = NULL;
TAtomo lookahead;
int contaLinha = 1;

char *strAtomo[] = {
    "ERROLEXICO",
    "ERROSINTATICO",
    "ENDOFFILE",
    "COMENTARIO",
    "ID",
    "ADD",
    "SUB",
    "DIVI",
    "MULTI",
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
    "DIFERENTEDE",
    "NOT",
    "AND",
    "OR" 
};

// Declaracao das funcoes

TInfoAtomo verifica_igual(); // = ou ==
TInfoAtomo verifica_maior(); // > ou >=
TInfoAtomo verifica_menor(); // < ou <=
TInfoAtomo verifica_exclamacao(); // ! ou !=
TInfoAtomo verifica_ecomercial(); // &&
TInfoAtomo verifica_pipe(); // ||

TInfoAtomo obter_atomo();

TInfoAtomo reconhece_id();
TInfoAtomo reconhece_int();
TInfoAtomo reconhece_char();
TInfoAtomo reconhece_comentario_ou_divisao();

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

int main() {

    entrada = "void main ( void ) {int num_1, num_2, maior; readint(num_1); readint(num_2); if ( num_1 > num_2 ) maior = num_1; else maior = num_2; writeint(maior); // imprime o maior valor}";

    TInfoAtomo info_atomo = obter_atomo(); // primeira leitura
    lookahead = info_atomo.atomo;

    program(); // inicia o sintático

    if (lookahead != ENDOFFILE) {
        printf("Erro: tokens restantes após o fim do programa\n");
    } 
    else {
        printf("\nCódigo léxica e sintaticamente correto!\n\n");
    }

    return 0;
}

// implementacao da funcao
TInfoAtomo obter_atomo(){

    TInfoAtomo info_atomo;
    memset(&info_atomo, 0, sizeof(TInfoAtomo)); // limpa

    // eliminar espaços
    while (*entrada == ' ' || *entrada == '\n' || *entrada == '\r' || *entrada == '\t') {
        if (*entrada == '\n') contaLinha++;
        entrada++;
    }

    // fim de arquivo
    if (*entrada == '\0') {
        info_atomo.atomo = ENDOFFILE;
        return info_atomo;
    }

    if (*entrada == '/') {
        char *inicio = entrada;
        entrada++;
        if (*entrada == '/' || *entrada == '*') {
            entrada = inicio;
            return reconhece_comentario_ou_divisao(); // retorna COMENTARIO
        } else {
            entrada = inicio;
        }
    }

    // verifica cada caractere
    switch (*entrada) {
        case '\0':
            info_atomo.atomo = ENDOFFILE;
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
            entrada++;
            info_atomo = verifica_igual();
            break;
        case '>':
            entrada++;
            info_atomo = verifica_maior();
            break;
        case '<':
            entrada++;
            info_atomo = verifica_menor();
            break;
        case '!':
            entrada++;
            info_atomo = verifica_exclamacao();
            break;
        case '&':
            entrada++;
            info_atomo = verifica_ecomercial();
            break;
        case '|':
            entrada++;    
            info_atomo = verifica_pipe();
            break;
        case '+':
            entrada++;    
            info_atomo.atomo = ADD;
            break;
        case '-':
            entrada++;    
            info_atomo.atomo = SUB;
            break;
        case '*':
            entrada++;    
            info_atomo.atomo = MULTI;
            break;

        default: // ID / INT / CHAR / COMENTARIO

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
                info_atomo = reconhece_comentario_ou_divisao();
            }

            break;
    }    
    
    info_atomo.linha = contaLinha;
    
    return info_atomo;
}

TInfoAtomo verifica_igual(){

    TInfoAtomo info_igual;
    memset(&info_igual, 0, sizeof(TInfoAtomo)); // reseta
    info_igual.atomo = IGUALATRIBUICAO;

    if(*entrada == '='){
        entrada++;
        goto q1;
    }

    return info_igual;

q1:

    info_igual.atomo = IGUALCOMPARACAO;
    return info_igual;

}

TInfoAtomo verifica_maior(){

    TInfoAtomo info_maior;
    memset(&info_maior, 0, sizeof(TInfoAtomo)); // reseta
    info_maior.atomo = MAIOR;

    if(*entrada == '='){
        entrada++;
        goto q1;
    }

    return info_maior;

q1:

    info_maior.atomo = MAIORIGUAL;
    return info_maior;

}

TInfoAtomo verifica_menor(){

    TInfoAtomo info_menor;
    memset(&info_menor, 0, sizeof(TInfoAtomo)); // reseta
    info_menor.atomo = MENOR;
    
    if(*entrada == '='){
        entrada++;
        goto q1;
    }
    
    return info_menor;
    
q1:
    
    info_menor.atomo = MENORIGUAL;
    return info_menor;
    
}

TInfoAtomo verifica_exclamacao(){
    
    TInfoAtomo info_excl;
    memset(&info_excl, 0, sizeof(TInfoAtomo)); // reseta
    info_excl.atomo = NOT;
    
    if(*entrada == '='){
        entrada++;
        goto q1;
    }
    
    return info_excl;
    
q1:
    
    info_excl.atomo = DIFERENTEDE;
    return info_excl;
    
}

TInfoAtomo verifica_ecomercial(){

    TInfoAtomo info_ecom;
    memset(&info_ecom, 0, sizeof(TInfoAtomo)); // reseta
    info_ecom.atomo = ERROLEXICO;
    
    if(*entrada == '&'){
        entrada++;
        info_ecom.atomo = AND;
    }
    
    return info_ecom;

}

TInfoAtomo verifica_pipe(){
    
    TInfoAtomo info_pipe;
    memset(&info_pipe, 0, sizeof(TInfoAtomo)); // reseta
    info_pipe.atomo = ERROLEXICO;
    
    if(*entrada == '|'){
        entrada++;
        info_pipe.atomo = OR;
    }
    
    return info_pipe;

}

// IDENTIFICADOR -> (LETRA_MINUSCULA | LETRA_MAIUSCULA | _)+(LETRA_MINUSCULA | LETRA_MAIUSCULA | _ | DIGITO)* ||| Inserir o atributo
// PALAVRAS RESERVADAS: char, else, if, int, main, readint, void, while, writeint
TInfoAtomo reconhece_id() {

    TInfoAtomo info_id;
    char *ini_id = entrada;
    int count = 1;
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
            entrada++;
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

    if (info_id.atomo != ID) {
        strcpy(info_id.identificador, "");
    }
        
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

// COMENTARIO OU DIVISAO
TInfoAtomo reconhece_comentario_ou_divisao() {
    TInfoAtomo info_coment;
    memset(&info_coment, 0, sizeof(TInfoAtomo));
    info_coment.atomo = ERROLEXICO;

    if (*entrada == '/') {
        entrada++;
        if (*entrada == '/') {
            entrada++;
            while (*entrada != '\0' && *entrada != '\n') {
                entrada++;
            }
            info_coment.atomo = COMENTARIO;
            return info_coment;
        } else if (*entrada == '*') {
            entrada++;
            while (*entrada != '\0') {
                if (*entrada == '*' && *(entrada + 1) == '/') {
                    entrada += 2;
                    info_coment.atomo = COMENTARIO;
                    return info_coment;
                }
                if (*entrada == '\n') contaLinha++;
                entrada++;
            }
            info_coment.atomo = ERROLEXICO; // não fechou
            return info_coment;
        } else {
            info_coment.atomo = DIVI;
            return info_coment;
        }
    }

    return info_coment;
}


// SINTATICO ---------------------------------------------------------------------------------------------------

void consome(TAtomo atomo) {
    if (lookahead == atomo) {
        if (lookahead >= 0 && lookahead <= OR) {
            printf("Atomo: %s\n", strAtomo[lookahead]);
        } else {
            printf("Atomom: [Atomo inválido: %d]\n", lookahead);
        }
        TInfoAtomo info_atomo = obter_atomo();
        lookahead = info_atomo.atomo;
    } else {
        const char *esperado = (atomo >= 0 && atomo <= OR) ? strAtomo[atomo] : "DESCONHECIDO";
        const char *encontrado = (lookahead >= 0 && lookahead <= OR) ? strAtomo[lookahead] : "DESCONHECIDO";

        printf("\nErro sintatico na linha %d: esperado [%s] encontrado [%s]\n", contaLinha, esperado, encontrado);
        exit(1);
    }
}

// <program> ::= void main ‘(‘ void ‘)’ <compound_stmt>
void program(){

    consome(VOID);
    consome(MAIN);
    consome(ABRE_PAR);
    consome(VOID);
    consome(FECHA_PAR);
    compound_stmt();

}

// <compound_stmt> ::= '{' <var_decl> { <stmt> } '}'
void compound_stmt() {
    consome(ABRE_CHAVES);
    var_decl();

    while (lookahead == COMENTARIO) consome(COMENTARIO); // <-- aqui

    while (lookahead == ID || lookahead == IF || lookahead == WHILE ||
           lookahead == READINT || lookahead == WRITEINT || lookahead == ABRE_CHAVES || lookahead == COMENTARIO) {

        while (lookahead == COMENTARIO) consome(COMENTARIO); // <-- aqui

        stmt();
    }

    while (lookahead == COMENTARIO) consome(COMENTARIO); // <-- e aqui também

    consome(FECHA_CHAVES);
}


// <var_decl> ::= [ <type_specifier> <var_decl_list> ‘;’ ]
void var_decl(){

    if(lookahead == INT || lookahead == CHAR){
        type_specifier();
        var_decl_list();
        consome(PONTO_VIRGULA);
    }

}

// <type_specifier> ::= int | char
void type_specifier() {

    switch (lookahead) {
        case INT:
            consome(INT);
            break;
        case CHAR:
            consome(CHAR);
            break;
        default:
            printf("\nErro sintático na linha %d: esperado tipo INT ou CHAR, encontrado [%s]\n", contaLinha, strAtomo[lookahead]);
            exit(1);
    }
}


// <var_decl_list> ::= <variable_id> { ',' <variable_id> }
void var_decl_list() {

    variable_id();
    while (lookahead == VIRGULA) {
        consome(VIRGULA);
        variable_id();
    }
    
}

// <variable_id> ::= id [ ‘=’ <expr> ]
void variable_id(){

    consome(ID);
    
    if(lookahead == IGUALATRIBUICAO){
        consome(IGUALATRIBUICAO);
        expr();
    }

}

/*
<stmt> ::= <compound_stmt> |
        <assig_stmt> |
        <cond_stmt> |
        <while_stmt> |
        readint ‘(‘ id ‘)’ ‘;’ |
        writeint ‘(‘ <expr> ‘)’ ‘;’ 
*/
void stmt() {

    switch (lookahead) {
        case ABRE_CHAVES:
            compound_stmt(); 
            break;
        case ID:
            assig_stmt(); 
            break;
        case IF:
            cond_stmt(); 
            break;
        case WHILE:
            while_stmt();
            break;
        case READINT:
            consome(READINT);
            consome(ABRE_PAR);
            consome(ID);
            consome(FECHA_PAR);
            consome(PONTO_VIRGULA);
            break;
        case WRITEINT:
            consome(WRITEINT);
            consome(ABRE_PAR);
            expr();
            consome(FECHA_PAR);
            consome(PONTO_VIRGULA);
            break;
        default:
            printf("Erro sintático: início de comando inválido na linha %d\n", contaLinha);
            exit(1);
    }
}

// <assig_stmt> ::= id ‘=’ <expr> ‘;’ 
void assig_stmt(){
    
    consome(ID);
    consome(IGUALATRIBUICAO);
    expr();
    consome(PONTO_VIRGULA);

}

// <cond_stmt> ::= if ‘(‘ <expr> ‘)’ <stmt> [ else <stmt> ]
void cond_stmt(){

    consome(IF);
    consome(ABRE_PAR);
    expr();
    consome(FECHA_PAR);
    stmt();
    
    if(lookahead == ELSE){
        consome(ELSE);
        stmt();
    }

}

// <while_stmt> ::= while ‘(‘ <expr> ‘)’ <stmt>
void while_stmt(){

    consome(WHILE);
    consome(ABRE_PAR);
    expr();
    consome(FECHA_PAR);
    stmt();

}

// <expr> ::= <conjunction> { '||' <conjunction> }
void expr() {

    conjunction();
    while (lookahead == OR) {
        consome(OR);
        conjunction();
    }
}

// <conjunction> ::= <comparison> { '&&' <comparison> }
void conjunction() {
    
    comparision();
    while (lookahead == AND) {
        consome(AND);
        comparision();
    }
}

// <comparison> ::= <sum> [ <relation> <sum> ]
void comparision(){

    sum();

    if(lookahead == MENOR || lookahead == MENORIGUAL || lookahead == IGUALCOMPARACAO || lookahead == DIFERENTEDE || lookahead == MAIOR || lookahead == MAIORIGUAL){
        relation();
        sum();
    }

}

// <relation> ::= “<” | “<=” | “==” | “!=” | “>” | “>=”
void relation() {

    switch (lookahead) {
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
            printf("\nErro sintático na linha %d: esperado operador relacional (<, <=, ==, !=, >, >=), encontrado [%s]\n", contaLinha, strAtomo[lookahead]);
            exit(1);
    }
}

// <sum> ::= <term> { ('+' | '-') <term> }
void sum() {
    term();
    while (lookahead == ADD || lookahead == SUB) {
        if (lookahead == ADD) {
            consome(ADD);
        } else {
            consome(SUB);
        }
        term();
    }
}

// <term> ::= <factor> { ('*' | '/') <factor> }
void term() {
    factor();
    while (lookahead == MULTI || lookahead == DIVI) {
        if (lookahead == MULTI) {
            consome(MULTI);
        } else {
            consome(DIVI);
        }
        factor();
    }
}

// <factor> ::= intconst | charconst | id | ‘(‘ <expr> ‘)’
void factor() {

    switch (lookahead) {
        case INTCONST:
            consome(INTCONST);
            break;
        case CHARCONST:
            consome(CHARCONST);
            break;
        case ID:
            consome(ID);
            break;
        case ABRE_PAR:
            consome(ABRE_PAR);
            expr();
            consome(FECHA_PAR);
            break;
        default:
            printf("\nErro sintático na linha %d: esperado INTCONST, CHARCONST, ID ou '(' expr ')' — encontrado [%s]\n",
                   contaLinha, strAtomo[lookahead]);
            exit(1);
    }
}