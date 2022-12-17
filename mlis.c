#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <setjmp.h>
#include "yasalisp.h"

//
cell heap[HEAPSIZE];
int stack[STACKSIZE];
int argstk[STACKSIZE];
struct token stok = {GO,OTHER};
jmp_buf buf;

int main(){
    printf("MonoLis Ver0.01\n");
    initcell();
    initsubr();
    int ret = setjmp(buf);
 repl:
    if(ret == 0) {
        while(1) {
            printf("> ");
            fflush(stdout);
            fflush(stdin);
            print(eval(read()));
            printf("\n");
            fflush(stdout);
        }
    } else {
        if (ret == 1) {
            ret = 0;
            goto repl;
        } else {
            return 0;
        }
    }
}

void initcell() {
	for(int addr = 0; addr < HEAPSIZE; addr++) {
		heap[addr].flag = FRE;
		heap[addr].cdr = addr + 1;
	}
	hp = 0;
	fc = HEAPSIZE;

	ep = makesym("nil");
	assocsym(makesym("nil"), NIL);
	assocsym(makesym("t"), makesym("t"));
	sp = 0;
	ap = 0;
}

//変数の束縛
//ローカル変数の場合は以前の束縛に積み上げていく。
//したがって同じ変数名があったとしても書き変えない。
void assocsym(int sym, int val){
    ep = cons(cons(sym,val),ep);
}

int issymch(char c){
    switch(c){
    case '!':
    case '?':
    case '+':
    case '-':
    case '*':
    case '/':
    case '=':
    case '<':
    case '>': return(1);
    default:  return(0);
    }
}

int freshcell() {
    int res = hp;
    hp = heap[hp].cdr;
    SET_CDR(res, 0);
    fc--;
    return res;
}

int makenum(int num) {
    int addr;

    addr = freshcell();
    SET_TAG(addr, num);
    SET_NUMBER(addr, num);
    return addr;
}

int makesym(char *name) {
    int addr;

    addr = freshcell();
    SET_TAG(addr, SYM);
    SET_NAME(addr, name);
    return addr;

}

int car(int addr){
    return GET_CAR(addr);
}

int caar(int addr){
    return car(car(addr));
}

int cdar(int addr){
    return cdr(car(addr));
}

int cdr(int addr){
    return GET_CDR(addr);
}

int cadr(int addr){
    return car(cdr(addr));
}

int caddr(int addr){
    return car(cdr(cdr(addr)));
}

int cons(int car, int cdr) {
    int addr = freshcell();
    SET_TAG(addr, LIS);
    SET_CAR(addr, car);
    SET_CDR(addr, cdr);
    return addr;
}

int assoc(int sym, int lis){
    if (nullp(lis)) {
        return(0);
    }
    else if(eqp(sym, caar(lis))) {
        return(car(lis));
    }
    else {
        return(assoc(sym,cdr(lis)));
    }
}

int length(int addr){
    int len = 0;

    while(!(nullp(addr))){
        len++;
        addr = cdr(addr);
    }
    return(len);
}

int list(int arglist){
    if(nullp(arglist))
        return(NIL);
    else
        return(cons(car(arglist),list(cdr(arglist))));
}

int numbertoken(char buf[]){
    int i;
    char c;

    if(((buf[0] == '+') || (buf[0] == '-'))){
        if(buf[1] == NUL)
            return(0); // case {+,-} => symbol
        i = 1;
        while((c=buf[i]) != NUL)
            if(isdigit(c))
                i++;  // case {+123..., -123...}
            else
                return(0);
    }
    else {
        i = 0;    // {1234...}
        while((c=buf[i]) != NUL)
            if(isdigit(c))
                i++;
            else
                return(0);
    }
    return(1);
}

int symboltoken(char buf[]){

    if(isdigit(buf[0]))
        return(0);

    int i = 0;
    char c;
    while((c=buf[i]) != NUL)
        if((isalpha(c)) || (isdigit(c)) || (issymch(c)))
            i++;
        else
            return(0);

    return(1);
}

//-------read()--------

void gettoken(void){

    if(stok.flag == BACK){
        stok.flag = GO;
        return;
    }

    if(stok.ch == ')'){
        stok.type = RPAREN;
        stok.ch = NUL;
        return;
    }

    if(stok.ch == '('){
        stok.type = LPAREN;
        stok.ch = NUL;
        return;
    }

    char c = getchar();
    while((c == SPACE) || (c == EOL) || (c == TAB))
        c=getchar();

    int pos;
    switch(c){
        case '(':   stok.type = LPAREN; break;
        case ')':   stok.type = RPAREN; break;
        case '\'':  stok.type = QUOTE; break;
        case '.':   stok.type = DOT; break;
        default: {
            pos = 0; stok.buf[pos++] = c;
            while(((c=getchar()) != EOL) && (pos < BUFSIZE) &&
                    (c != SPACE) && (c != '(') && (c != ')'))
                stok.buf[pos++] = c;

            stok.buf[pos] = NUL;
            stok.ch = c;
            if(numbertoken(stok.buf)){
                stok.type = NUMBER;
                break;
            }
            if(symboltoken(stok.buf)){
                stok.type = SYMBOL;
                break;
            }
            stok.type = OTHER;
        }
    }
}

/* void gettoken(){ */
/* 	char c; */
/* 	int pos; */
/* 	if (stok.flag == BACK){ */

/* 	} */
/* } */

int read() {
    gettoken();
    switch(stok.type) {
    case NUMBER: return makenum(atoi(stok.buf));
    case SYMBOL: return makesym(stok.buf);
    case QUOTE: return cons(makesym("quote"), cons(read(), NIL));
    default: error(CANT_READ_ERR, "read", NIL);
    }
}

int readlist() {
    gettoken();

    if (stok.type == RPAREN) {
        return NIL;
    } else if (stok.type == DOT) {
        int cdr = read();
        if (atomp(cdr)) {
            gettoken();
        }
        return cdr;
    } else {
        stok.flag = BACK;
        int car = read();
        int cdr = readlist();
        return cons(car, cdr);
    }
}

void print(int addr) {
    switch(GET_TAG(addr)) {
    case NUM: printf("%d", GET_NUMBER(addr)); break;
    case SYM: printf("%s", GET_NAME(addr)); break;
    case SUBR: printf("<subr>"); break;
    case FSUBR: printf("<fsubr>"); break;
    case FUNC: printf("function"); break;
    case LIS: {
        printf("(");
        printlist(addr);
        break;
    }
    }
}

void printlist(int addr) {
    if (IS_NIL(addr)) {
        printf(")");
    } else if ((!(listp(cdr(addr)))) && (! (nullp(cdr(addr))))) {
        print(car(addr));
        printf(" . ");
        print(cdr(addr));
        printf(")");
    } else {
        print(GET_CAR(addr));
        if (! (IS_NIL(GET_CDR(addr)))) {
            printf(" ");
        }
        printlist(GET_CDR(addr));
    }
}

int eval(int addr) {
    if (atomp(addr)) {
        if (numberp(addr)) {
            return addr;
        }

        if (symbolp(addr)) {
            int res = findsym(addr);
            if (res == 0) {
                error(CANT_FIND_ERR, "eval", addr);
            } else {
                return res;
            }
        }
    } else if (listp(addr)) {
        if (listp(addr)) {
            if (symbolp(car(addr)) && HAS_NAME(car(addr), "quote")) {
                return cadr(addr);
            }
            if (numberp(car(addr))) {
                error(ARG_SYM_ERR, "eval", addr);
            }
            if (subrp(car(addr))) {
                return apply(car(addr), evlis(cdr(addr)));
            }
            if (fsubrp(car(addr))) {
                return apply(car(addr), cdr(cdr(addr)));
            }
            if (functionp(car(addr))) {
                return apply(car(addr), cdr(cdr(addr)));
            }

        }
    }
    error(CANT_FIND_ERR, "eval", addr);
}

int atomp(int addr) {
    if (IS_NUMBER(addr) || IS_SYMBOL(addr)) {
        return 1;
    } else {
        return 0;
    }
}

int numberp(int addr){
    if(IS_NUMBER(addr))
        return(1);
    else
        return(0);
}

int symbolp(int addr){
    if(IS_SYMBOL(addr))
        return(1);
    else
        return(0);
}

int listp(int addr){
    if(IS_LIST(addr))
        return(1);
    else
        return(0);
}

int nullp(int addr){
    if(IS_NIL(addr))
        return(1);
    else
        return(0);
}

int eqp(int addr1, int addr2) {
    if ((numberp(addr1) && numberp(addr2))
        && (GET_NUMBER(addr1) == GET_NUMBER(addr2))) {
        return 1;
    } else if ((symbolp(addr1) && symbolp(addr2))
               && (SAME_NAME(addr1, addr2))) {
        return 1;
    } else {
        return 0;
    }
}

int subrp(int addr){
    int val;

    val = findsym(addr);
    if(val != -1)
        return(IS_SUBR(val));
    else
        return(0);
}

int fsubrp(int addr){
    int val;

    val = findsym(addr);
    if(val != -1)
        return(IS_FSUBR(val));
    else
        return(0);
}

int functionp(int addr){
    int val;

    val = findsym(addr);
    if(val != -1)
        return(IS_FUNC(val));
    else
        return(0);
}

//環境は次のように連想リストになっている。
// env = ((sym1 . val1) (sym2 . val2) ...(t . t)(nil . 0))
// assocでシンボルに対応する値を探す。
//見つからなかった場合には-1を返す。
int findsym(int sym){
	int addr;

    addr = assoc(sym,ep);

    if(addr == 0)
    	return(-1);
    else
    	return(cdr(addr));
}

//-------エラー処理------
void error(int errnum, char *fun, int arg){
    switch(errnum){
        case CANT_FIND_ERR:{printf("%s can't find difinition of ", fun);
                            print(arg); break; }

        case CANT_READ_ERR:{printf("%s can't read of ", fun);
                            break; }

        case ILLEGAL_OBJ_ERR:{printf("%s got an illegal object ", fun);
                            print(arg); break; }

        case ARG_SYM_ERR:  {printf("%s require symbol but got ", fun);
                            print(arg); break; }

        case ARG_NUM_ERR:  {printf("%s require number but got ", fun);
                            print(arg); break; }

        case ARG_LIS_ERR:  {printf("%s require list but got ", fun);
                            print(arg); break; }

        case ARG_LEN0_ERR: {printf("%s require 0 arg but got %d", fun, length(arg));
                            break; }

        case ARG_LEN1_ERR: {printf("%s require 1 arg but got %d", fun, length(arg));
                            break; }

        case ARG_LEN2_ERR: {printf("%s require 2 args but got %d", fun, length(arg));
                            break; }

        case ARG_LEN3_ERR: {printf("%s require 3 args but got %d", fun, length(arg));
                            break; }

        case MALFORM_ERR:  {printf("%s got malformed args " ,fun);
                            print(arg); break; }
    }
    printf("\n");
    longjmp(buf,1);
}

void initsubr() {

}
