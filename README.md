# TINY-C

O objetivo desse trabalho é implementar as fases de análise léxica e sintática de um compilador para
linguagem TINY-C (baseada na Linguagem C). O compilador para a linguagem TINY-C restringe a
Linguagem C para ter apenas tipos inteiros (int) e caractere (char), comandos condicionais (if) e
repetição (while) e não implementa a declaração e chamadas de funções, a exceção se faz para as funções
de entrada (readint) e saída (writeint).

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