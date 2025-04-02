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

// Declaracao de variaveis globais

// definicoes dos atomos
typedef enum{
    ERROLEXICO,
    ERROLEXICOEXCEDECARACTERES,
    ERROSINTATICO,
    ENDOFFILE,
    COMENTARIO,
    ID,
    ADICAO,
    SUBITRACAO,
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

char *entrada = NULL;
TAtomo lookahead;
int contaLinha = 1;

char *strAtomo[] = {
    "ERROLEXICO",
    "ERROLEXICOEXCEDECARACTERES",
    "ERROSINTATICO",
    "ENDOFFILE",
    "COMENTARIO",
    "ID",
    "ADICAO",
    "SUBITRACAO",
    "DIVISAO",
    "MULTIPLICACAO",
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

char *strSimbolo[] = {
    "ERROLEXICO",
    "ERROLEXICOEXCEDECARACTERES",
    "ERROSINTATICO",
    "ENDOFFILE",
    "COMENTARIO",
    "ID",
    "+",
    "-",
    "/",
    "*",
    "INT",
    "CHAR",
    "VOID",
    ";",
    "(",
    ")",
    "{",
    "}",
    "IF",
    "ELSE",
    "WHILE",
    "MAIN",
    "READINT",
    "WRITEINT",
    ",",
    "INTCONST",
    "CHARCONST",
    "=",
    "==",
    ">",
    ">=",
    "<",
    "<=",
    "!=",
    "!",
    "&&",
    "||" 
};

// Declaracao das funcoes do léxico

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

// Declaração das funções do sintático

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

int main(int num_argumentos, char **argumentos) {

    // verifica se o nome do arquivo foi passado como argumento
    if (num_argumentos < 2) {
        printf("Uso: %s <arquivo>\n", argumentos[0]);
        return 1;
    }

    FILE *arquivo = fopen(argumentos[1], "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo %s!\n", argumentos[1]);
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
    
    // lê o conteúdo do arquivo
    size_t lidos = fread(buffer, 1, tamanho, arquivo);
    buffer[lidos] = '\0'; // adiciona o terminador nulo
    fclose(arquivo);

    entrada = buffer;

    //entrada = "/*teste*/void main (void) {int num_1, num_2, maior; readint(num_1); readint(num_2); if (a && b) maior = num_1; else maior = num_2; num_1  = 0xA; p = 'c'; writeint(maior); // imprime o maior valor\n}";

    TInfoAtomo info_atomo = obter_atomo(); // primeira leitura

    printf("\n");

    // mostrar o primeiro comentário e a linha que ele acaba
    while (info_atomo.atomo == COMENTARIO) {
        printf("Atomo: %-15s | Linha: %2d\n", strAtomo[info_atomo.atomo], info_atomo.linha);
        info_atomo = obter_atomo();
    }    

    if (info_atomo.atomo == ERROLEXICO) {
        printf("\nErro léxico na linha %d\n\n", info_atomo.linha);
        exit(1);
    }

    lookahead = info_atomo.atomo;

    program();

    if (lookahead != ENDOFFILE) {
        printf("\nErro sintático na linha %d: esperado fim do arquivo, encontrado [%s]\n\n", contaLinha, strSimbolo[lookahead]);
        exit(1);
    }

    printf("\nAnálise léxica e sintática das %d linhas concluídas! Programa compilado com sucesso!\n\n", contaLinha);

    free(buffer);

    return 0;
}

// LÉXICO ---------------------------------------------------------------------------------------------------

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
        info_atomo.linha = contaLinha;
        return info_atomo;
    }

    // verifica cada caractere
    switch (*entrada) {
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
            info_atomo.atomo = ADICAO;
            break;
        case '-':
            entrada++;    
            info_atomo.atomo = SUBITRACAO;
            break;
        case '*':
            entrada++;    
            info_atomo.atomo = MULTIPLICACAO;
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

// = ou ==
TInfoAtomo verifica_igual(){

    TInfoAtomo info_igual;
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

// > ou >=
TInfoAtomo verifica_maior(){

    TInfoAtomo info_maior;
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

// < ou <=
TInfoAtomo verifica_menor(){

    TInfoAtomo info_menor;
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

// ! ou !=
TInfoAtomo verifica_exclamacao(){
    
    TInfoAtomo info_excl;
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

// &&
TInfoAtomo verifica_ecomercial(){

    TInfoAtomo info_ecom;
    info_ecom.atomo = ERROLEXICO;
    
    if(*entrada == '&'){
        entrada++;
        info_ecom.atomo = AND;
    }
    
    return info_ecom;

}

// ||
TInfoAtomo verifica_pipe(){
    
    TInfoAtomo info_pipe;
    info_pipe.atomo = ERROLEXICO;
    
    if(*entrada == '|'){
        entrada++;
        info_pipe.atomo = OR;
    }
    
    return info_pipe;

}

// IDENTIFICADOR -> (LETRA_MINUSCULA | LETRA_MAIUSCULA | _)+(LETRA_MINUSCULA | LETRA_MAIUSCULA | _ | DIGITO)* --- Inserir o atributo
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
        }
    }

    int tamanho = entrada - ini_id;
    if (tamanho > 14) { // excedeu o número de caracteres permitidos
        info_id.atomo = ERROLEXICOEXCEDECARACTERES;
        return info_id;
    }
    strncpy(info_id.identificador, ini_id, tamanho);
    info_id.identificador[tamanho] = '\0';

    // verifica se é palavra reservada
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

q4:

    if (isspace(*entrada) || *entrada == '\0' || *entrada == ';' || *entrada == ')' || *entrada == ',') {
        long valor = strtol(buffer, NULL, 16); // converte hexadecimal para inteiro
        sprintf(info_int.atributo, "%ld", valor); // salva como string em atributo
        info_int.atomo = INTCONST;
    }

    return info_int;
}

// CHAR
TInfoAtomo reconhece_char() {

    TInfoAtomo info_char;
    info_char.atomo = ERROLEXICO;

    goto q1;

q1:
    if (*entrada == '\0') return info_char; // erro

    // salva o caractere como atributo
    info_char.atributo[0] = *entrada;
    info_char.atributo[1] = '\0';

    entrada++;
    goto q2;

q2:
    if (*entrada == '\'') {
        entrada++; 
        info_char.atomo = CHARCONST;
        return info_char;
    }

    return info_char;
}

// COMENTARIO OU DIVISAOSAO
TInfoAtomo reconhece_comentario_ou_divisao() {

    TInfoAtomo info;
    info.atomo = ERROLEXICO;

    goto q1;

q1:
    if (*entrada == '/') {
        entrada++;
        goto q2; 
    }
    else if (*entrada == '*') {
        entrada++;
        goto q3; 
    }
    else {
        info.atomo = DIVISAO;
        info.linha = contaLinha;
        return info;
    }

q2:
    if (*entrada == '\0') {
        info.atomo = COMENTARIO;
        info.linha = contaLinha;
        return info;
    }
    else if (*entrada == '\n') {
        entrada++;
        contaLinha++;
        info.atomo = COMENTARIO;
        info.linha = contaLinha;
        return info;
    }
    else {
        entrada++;
        goto q2;
    }

q3:
    if (*entrada == '\0') {
        info.atomo = ERROLEXICO;
        info.linha = contaLinha;
        return info;
    }
    else if (*entrada == '*' && *(entrada + 1) == '/') {
        entrada += 2;
        info.atomo = COMENTARIO;
        info.linha = contaLinha;
        return info;
    }
    else {
        if (*entrada == '\n') contaLinha++;
        entrada++;
        goto q3;
    }
}

// SINTÁTICO ---------------------------------------------------------------------------------------------------

// consome e printa as informações
void consome(TAtomo atomo) {

    if (lookahead == atomo) {

        TInfoAtomo info_atomo = obter_atomo();   

        if(info_atomo.atomo == ERROLEXICOEXCEDECARACTERES){
            printf("\nErro léxico de excesso de caracteres na declaração de identificador na linha %d\n\n", info_atomo.linha);
            exit(1);
        }
        else if(info_atomo.atomo == ERROLEXICO) {
            printf("\nErro léxico na linha %d\n\n", info_atomo.linha);
            exit(1);
        }
        
        // printar informações do átomo
        printf("Atomo: %-15s | Linha: %2d", strAtomo[info_atomo.atomo], info_atomo.linha);
        if(info_atomo.atomo == ID){
            printf("  | ID: %4s", info_atomo.identificador);
        }
        else if(info_atomo.atomo == INTCONST){
            printf("  | Atributo: %1s", info_atomo.atributo);
        }
        else if(info_atomo.atomo == CHARCONST){
            printf("  | Atributo: \'%s\'", info_atomo.atributo);
        }
        printf("\n");
        
        lookahead = info_atomo.atomo;
    } 
    else {
        printf("\nErro sintático na linha %d: esperado [%s], encontrado [%s]\n\n", contaLinha, strSimbolo[atomo], strSimbolo[lookahead]);
        exit(1);
    }
}

// ignorar os comentários
void consome_comentarios(){
    while (lookahead == COMENTARIO) consome(COMENTARIO);
}

// <program> ::= void main ‘(‘ void ‘)’ <compound_stmt>
void program(){

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
void compound_stmt() {

    consome_comentarios();
    
    consome(ABRE_CHAVES);
    var_decl();

    while (lookahead == COMENTARIO) consome(COMENTARIO); 

    while (lookahead == ID || lookahead == IF || lookahead == WHILE ||
        lookahead == READINT || lookahead == WRITEINT || lookahead == ABRE_CHAVES) {
        stmt();
    } 

    consome_comentarios();
    consome(FECHA_CHAVES);
}

// <var_decl> ::= [ <type_specifier> <var_decl_list> ‘;’ ]
void var_decl(){

    consome_comentarios();

    if(lookahead == INT || lookahead == CHAR){
        type_specifier();
        var_decl_list();
        consome_comentarios();
        consome(PONTO_VIRGULA);
    }

}

// <type_specifier> ::= int | char
void type_specifier() {

    consome_comentarios();

    switch (lookahead) {
        case INT:
            consome(INT);
            break;
        case CHAR:
            consome(CHAR);
            break;
        default:
            printf("\nErro sintático na linha %d: esperado tipo [INT] ou [CHAR], encontrado [%s]\n\n", contaLinha, strSimbolo[lookahead]);
            exit(1);
    }
}

// <var_decl_list> ::= <variable_id> { ',' <variable_id> }
void var_decl_list() {

    consome_comentarios();

    variable_id();
    while (lookahead == VIRGULA) {
        consome(VIRGULA);
        variable_id();
    }
    
}

// <variable_id> ::= id [ ‘=’ <expr> ]
void variable_id(){

    consome_comentarios();

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

    consome_comentarios();

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
            consome_comentarios();
            consome(ABRE_PAR);
            consome_comentarios();
            consome(ID);
            consome_comentarios();
            consome(FECHA_PAR);
            consome_comentarios();
            consome(PONTO_VIRGULA);
            break;
        case WRITEINT:
            consome(WRITEINT);
            consome_comentarios();
            consome(ABRE_PAR);
            expr();
            consome_comentarios();
            consome(FECHA_PAR);
            consome_comentarios();
            consome(PONTO_VIRGULA);
            break;
        default:
            printf("\nErro sintático: início de comando inválido na linha %d\n\n", contaLinha);
            exit(1);
    }
}

// <assig_stmt> ::= id ‘=’ <expr> ‘;’ 
void assig_stmt(){

    consome_comentarios();
    
    consome(ID);
    consome_comentarios();
    consome(IGUALATRIBUICAO);
    expr();
    consome_comentarios();
    consome(PONTO_VIRGULA);

}

// <cond_stmt> ::= if ‘(‘ <expr> ‘)’ <stmt> [ else <stmt> ]
void cond_stmt(){

    consome_comentarios();

    consome(IF);
    consome_comentarios();
    consome(ABRE_PAR);
    expr();
    consome_comentarios();
    consome(FECHA_PAR);
    stmt();
    
    if(lookahead == ELSE){
        consome(ELSE);
        stmt();
    }

}

// <while_stmt> ::= while ‘(‘ <expr> ‘)’ <stmt>
void while_stmt(){

    consome_comentarios();

    consome(WHILE);
    consome_comentarios();
    consome(ABRE_PAR);
    expr();
    consome_comentarios();
    consome(FECHA_PAR);
    stmt();

}

// <expr> ::= <conjunction> { '||' <conjunction> }
void expr() {

    consome_comentarios();

    conjunction();
    while (lookahead == OR) {
        consome(OR);
        conjunction();
    }
}

// <conjunction> ::= <comparison> { '&&' <comparison> }
void conjunction() {

    consome_comentarios();
    
    comparision();
    while (lookahead == AND) {
        consome(AND);
        comparision();
    }
}

// <comparison> ::= <sum> [ <relation> <sum> ]
void comparision(){

    consome_comentarios();

    sum();

    if(lookahead == MENOR || lookahead == MENORIGUAL || lookahead == IGUALCOMPARACAO || lookahead == DIFERENTEDE || lookahead == MAIOR || lookahead == MAIORIGUAL){
        relation();
        sum();
    }

}

// <relation> ::= “<” | “<=” | “==” | “!=” | “>” | “>=”
void relation() {

    consome_comentarios();

    switch (lookahead){
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
            printf("\nErro sintático na linha %d: esperado algum operador relacional (<, <=, ==, !=, >, >=), encontrado [%s]\n\n", contaLinha, strSimbolo[lookahead]);
            exit(1);
    }
}

// <sum> ::= <term> { ('+' | '-') <term> }
void sum() {

    consome_comentarios();

    term();
    while (lookahead == ADICAO || lookahead == SUBITRACAO) {
        if (lookahead == ADICAO) {
            consome(ADICAO);
        } 
        else {
            consome(SUBITRACAO);
        }
        term();
    }
}

// <term> ::= <factor> { ('*' | '/') <factor> }
void term() {

    consome_comentarios();

    factor();
    while (lookahead == MULTIPLICACAO || lookahead == DIVISAO) {
        if (lookahead == MULTIPLICACAO) {
            consome(MULTIPLICACAO);
        } 
        else {
            consome(DIVISAO);
        }
        factor();
    }
}

// <factor> ::= intconst | charconst | id | ‘(‘ <expr> ‘)’
void factor() {

    consome_comentarios();

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
            consome_comentarios();
            consome(FECHA_PAR);
            break;
        default:
            printf("\nErro sintático na linha %d: esperado [INTCONST], [CHARCONST], [ID] ou ['(' expr ')'] — encontrado [%s]\n\n", contaLinha, strSimbolo[lookahead]);
            exit(1);
    }
}