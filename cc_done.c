#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

/**
 * http://www.felixcloutier.com/x86/
 *
 *
 * Document
 * 1. Virtual machine
 * +------------------+
 * |    stack   |     |      high address
 * |    ...     v     |
 * |                  |
 * |                  |
 * |                  |
 * |                  |
 * |    ...     ^     |
 * |    heap    |     |
 * +------------------+
 * | bss  segment     |
 * +------------------+
 * | data segment     |
 * +------------------+
 * | text segment     |      low address
 * +------------------+
 *
 *
 *
 * 2. instructions supported
 */
enum { LEA ,IMM ,JMP ,CALL,JZ ,JNZ ,ENT ,ADJ ,LEV ,LI ,LC ,SI ,SC , PUSH,
       OR ,XOR ,AND ,EQ ,NE ,LT ,GT ,LE ,GE ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV , MOD,
       OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT};

/**
 * 3. Operators supported(tokens and classes)
 */
enum { Num = 128, Fun, Sys, Glo, Loc, Id,
       Char, Else, Enum, If, Int, Return, Sizeof, While,
       Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak, RET};

enum { Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};
/**
 * 4. Types supported
 */
enum { CHAR, INT, PTR };

#include "testutil.c" // For test

/**
 * 5. Statement supported
 *
 * - if (...) <statement> [else <statement>]
 * - while (...) <statement>
 * - { <statement> }
 * - return <...>;
 * - <empty statement>;
 * - expression;
 */
int basetype;    // the type of a declaration, make it global for convenience
int expr_type;   // the type of an expression

// function frame
//
// 0: arg 1
// 1: arg 2
// 2: arg 3
// 3: return address
// 4: old bp pointer  <- index_of_bp
// 5: local var 1
// 6: local var 2
int index_of_bp;


// Variables
int  *text,          // text segment
     *old_text,      // for dump text segment
     *stack;         // stack pointer
char *data;          // data segment

int  *pc;            // program counter
int  *bp;            // base address in heap
int  *sp;
int  ax;             // execution res
int  cycle;          // virtual machine registers

int  token;          // current token
int token_val;       // value of current token (mainly for number)
char *src, *old_src; // pointer to source code string;
int  poolsize;       // default size of text/data/stack
int  line;           // line number

int *current_id,     // current parsed ID
    *symbols;        // symbol table
int *idmain;         // the `main` function

// next function will load in the next token from src domain
void next() {
    char *last_pos;
    int hash;

    while (token = *src) {
        ++src;

        if (token == '\n') {
            // change line request
            ++line;
        } else if (token == '#') {
            // skip macro
            while (*src != 0 && *src != '\n') {
                src++;
            }
        } else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {
            // parse identifier
            last_pos = src - 1;
            hash = token;

            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * 147 + *src;
                src++;
            }

            // look for existing identifier
            current_id = symbols;
            while (current_id[Token]) {
                if (current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos)) {
                    //found and return
                    token = current_id[Token];
                    return;
                }
                current_id = current_id + IdSize;
            }

            // create new ID
            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;
            return;
        } else if (token >= '0' && token <= '9') {
            // parse number: dec(123), hex(0x123) or oct(017)
            token_val = token - '0';
            if (token_val > 0) {
                // dec, starts with [1-9]
                while (*src >= '0' && *src <= '9') {
                    token_val = token_val*10 + *src++ - '0';
                }
            } else {
                // starts with 0
                if (*src == 'x' || *src == 'X') {
                    //hex
                    token = *++src;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')) {
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                } else {
                    // oct
                    while (*src >= '0' && *src <= '7') {
                        token_val = token_val*8 + *src++ - '0';
                    }
                }
            }
            token = Num;
            return;

        } else if (token == '"' || token == '\'') {
            // String (\n)
            last_pos = data;
            while (*src != 0 && *src != token) {
                token_val = *src++;
                if (token_val == '\\') {
                    // escape character
                    token_val = *src++;
                    if (token_val == 'n') {
                        token_val = '\n';
                    }
                }
                if (token == '"') {
                    *data++ = token_val;
                }
            }

            src++;
            // if it is a single character, return Num ? TODO
            if (token == '"') {
                token_val = (int)last_pos;
            } else {
                token = Num;
            }
            return;
        } else if (token == '/') {
            if (*src == '/') {
                // Comment
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else {
                token = Div;
                return;
            }
        } else if (token == '=') {
            if (*src == '=') {
                src ++;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        } else if (token == '+') {
            if (*src == '+') {
                src ++;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        } else if (token == '-') {
            if (*src == '-') {
                src ++;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        } else if (token == '!') {
            if (*src == '=') {
                src++;
                token = Ne;
            }
            return;
        } else if (token == '<') {
            if (*src == '=') {
                src ++;
                token = Le;
            } else if (*src == '<') {
                src ++;
                token = Shl;
            } else {
                token = Lt;
            }
            return;
        } else if (token == '>') {
            if (*src == '=') {
                src ++;
                token = Ge;
            } else if (*src == '>') {
                src ++;
                token = Shr;
            } else {
                token = Gt;
            }
            return;
        } else if (token == '|') {
            if (*src == '|') {
                src ++;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        } else if (token == '&') {
            if (*src == '&') {
                src ++;
                token = Lan;
            } else {
                token = And;
            }
            return;
        } else if (token == '^') {
            token = Xor;
            return;
        } else if (token == '%') {
            token = Mod;
            return;
        } else if (token == '*') {
            token = Mul;
            return;
        } else if (token == '[') {
            token = Brak;
            return;
        } else if (token == '?') {
            token = Cond;
            return;
        } else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // directly return the character as token;
            return;
        }
    }
    return;
}

// Expression will transfer the token to x86 assembly according to its precedence
// http://en.cppreference.com/w/c/language/operator_precedence

void expression(int level) {
    int *id;
    int tmp;
    int *addr;
    {
        if (!token) {
            printf("%d: unexpected token EOF of expression\n", line);
            exit(-1);
        }
        if (token == Num) {
            // imm
            match(Num);
            *++ text = IMM;
            *++ text = token_val;
            expr_type = INT;
        } else if (token == '"') {
            *++ text = IMM;
            *++ text = token_val;
            match('"');
            while (token == '"') {
                match('"');
            }
            // append the end of string character '\0'
            data = (char *)(((int)data + sizeof(int)) & (-sizeof(int)));
            expr_type = PTR;
        } else if (token == Sizeof) {
            // sizeof(int), sizeof(char) and sizeof(*...)
            match(Sizeof);
            match('(');
            expr_type = INT;
            if (token == Int) {
                match(Int);
            } else if (token == Char) {
                match(Char);
                expr_type = CHAR;
            }
            while (token == Mul) {
                match(Mul);
                expr_type = expr_type + PTR;
            }
            match(')');
            // emit code
            *++text = IMM;
            *++text = (expr_type == CHAR) ? sizeof(char) : sizeof(int);
            expr_type = INT;
        } else if (token == Id) {
            // 1. function call
            // 2. Enum variable
            // 3. global/local variable

            match(Id);
            id = current_id;
            if (token == '(') {
                // function call
                match('(');
                // 1. pass in arguments
                tmp = 0; // number of arguments
                while (token != ')') {
                    expression(Assign);
                    *++text = PUSH;
                    tmp ++;
                    if (token == ',') {
                        match(',');
                    }
                }
                match(')');
                // 2. emit code
                if (id[Class] == Sys) {
                    // system functions
                    *++text = id[Value];
                } else if (id[Class] == Fun) {
                    // function call
                    *++text = CALL;
                    *++text = id[Value];
                } else {
                    printf("%d: bad function call\n", line);
                    exit(-1);
                }
                // 3. clean the stack for arguments
                if (tmp > 0) {
                    *++text = ADJ;
                    *++text = tmp;
                }
                expr_type = id[Type];

            } else if (id[Class] == Num) {
                // enum variable
                *++text = IMM;
                *++text = id[Value];
                expr_type = INT;
            } else {
                // variable
                if (id[Class] == Loc) {
                    *++text = LEA;
                    *++text = index_of_bp - id[Value];
                } else if (id[Class] == Glo) {
                    *++text = IMM;
                    *++text = id[Value];
                } else {
                    printf("%d: undefined variable\n", line);
                    exit(-1);
                }
                // emit code, default behaviour is to load the value of the
                // address stored in `ax`
                expr_type = id[Type];
                *++text = (expr_type == Char) ? LC : LI;
            }
        } else if (token == '(') {
            // cast or parenthesis
            match('(');
            if (token == Int || token == Char) {
                tmp = (token == Char) ? CHAR : INT; // cast type
                match(token);
                while (token == Mul) {
                    match(Mul);
                    tmp = tmp + PTR;
                }
                match(')');
                expression(Inc); // cast has precedence as Inc(++)
                expr_type  = tmp;
            } else {
                // normal parenthesis
                expression(Assign);
                match(')');
            }
        } else if (token == Mul) {
            // dereference *<addr>
            match(Mul);
            expression(Inc); // dereference has the same precedence as Inc(++)

            if (expr_type >= PTR) {
                expr_type = expr_type - PTR;
            } else {
                printf("%d: bad dereference\n", line);
                exit(-1);
            }

            *++text = (expr_type == CHAR) ? LC : LI;
        } else if (token == And) {
            // get the address of
            match(And);
            expression(Inc); // get the address of
            if (*text == LC || *text == LI) {
                text --;
            } else {
                printf("%d: bad address of\n", line);
                exit(-1);
            }

            expr_type = expr_type + PTR;
        } else if (token == '!') {
            // not
            match('!');
            expression(Inc);

            *++text = PUSH;
            *++text = IMM;
            *++text = 0;
            *++text = EQ;

            expr_type = INT;
        } else if (token == '~') {
            // bitwise not
            match('~');
            expression(Inc);

            // emit code, use <expr> XOR -1
            *++text = PUSH;
            *++text = IMM;
            *++text = -1;
            *++text = XOR;

            expr_type = INT;

        } else if (token == Add) {
            // +var, do nothing
            match(Add);
            expression(Inc);

            expr_type = INT;
        } else if (token == Sub) {
            // -var
            match(Sub);

            if (token == Num) {
                *++text = IMM;
                *++text = -token_val;
                match(Num);
            } else {

                *++text = IMM;
                *++text = -1;
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
            }

            expr_type = INT;
        } else if (token == Inc || token == Dec) {
            tmp = token;
            match(token);
            expression(Inc);
            if (*text == LC) {
                *text = PUSH;  // to duplicate the address
                *++text = LC;
            } else if (*text == LI) {
                *text = PUSH;
                *++text = LI;
            } else {
                printf("%d: bad lvalue of pre-increment\n", line);
                exit(-1);
            }
            *++text = PUSH;
            *++text = IMM;
            *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
            *++text = (tmp == Inc) ? ADD : SUB;
            *++text = (expr_type == CHAR) ? SC : SI;
        } else {
            printf("%d: bad expression token=", line);
            printf(getEnumStr(token));
            printf("\n");
            exit(-1);
        }
    }

    // binary operator and postfix operators.
    {
        while (token >= level) {
            // handle according to current operator's precedence
            tmp = expr_type;
            if (token == Assign) {
                // var = expr;
                match(Assign);
                if (*text == LC || *text == LI) {
                    *text = PUSH; // save the lvalue's pointer
                } else {
                    printf("%d: bad lvalue in assignment\n", line);
                    exit(-1);
                }
                expression(Assign);

                expr_type = tmp;
                *++text = (expr_type == CHAR) ? SC : SI;
            } else if (token == Cond) {
                // expr ? a : b;
                match(Cond);
                *++text = JZ;
                addr = ++text;
                expression(Assign);
                if (token == ':') {
                    match(':');
                } else {
                    printf("%d: missing colon in conditional\n", line);
                    exit(-1);
                }
                *addr = (int)(text + 3);
                *++text = JMP;
                addr = ++text;
                expression(Cond);
                *addr = (int)(text + 1);
            } else if (token == Lor) {
                // logic or
                match(Lor);
                *++text = JNZ;
                addr = ++text;
                expression(Lan);
                *addr = (int)(text + 1);
                expr_type = INT;
            } else if (token == Lan) {
                // logic and
                match(Lan);
                *++text = JZ;
                addr = ++text;
                expression(Or);
                *addr = (int)(text + 1);
                expr_type = INT;
            } else if (token == Or) {
                // bitwise or
                match(Or);
                *++text = PUSH;
                expression(Xor);
                *++text = OR;
                expr_type = INT;
            } else if (token == Xor) {
                // bitwise xor
                match(Xor);
                *++text = PUSH;
                expression(And);
                *++text = XOR;
                expr_type = INT;
            } else if (token == And) {
                // bitwise and
                match(And);
                *++text = PUSH;
                expression(Eq);
                *++text = AND;
                expr_type = INT;
            } else if (token == Eq) {
                // equal ==
                match(Eq);
                *++text = PUSH;
                expression(Ne);
                *++text = EQ;
                expr_type = INT;
            } else if (token == Ne) {
                // not equal !=
                match(Ne);
                *++text = PUSH;
                expression(Lt);
                *++text = NE;
                expr_type = INT;
            } else if (token == Lt) {
                // less than
                match(Lt);
                *++text = PUSH;
                expression(Shl);
                *++text = LT;
                expr_type = INT;
            } else if (token == Gt) {
                // greater than
                match(Gt);
                *++text = PUSH;
                expression(Shl);
                *++text = GT;
                expr_type = INT;
            } else if (token == Le) {
                // less than or equal to
                match(Le);
                *++text = PUSH;
                expression(Shl);
                *++text = LE;
                expr_type = INT;
            } else if (token == Ge) {
                // greater than or equal to
                match(Ge);
                *++text = PUSH;
                expression(Shl);
                *++text = GE;
                expr_type = INT;
            } else if (token == Shl) {
                // shift left
                match(Shl);
                *++text = PUSH;
                expression(Add);
                *++text = SHL;
                expr_type = INT;
            } else if (token == Shr) {
                // shift right
                match(Shr);
                *++text = PUSH;
                expression(Add);
                *++text = SHR;
                expr_type = INT;
            } else if (token == Add) {
                // add
                match(Add);
                *++text = PUSH;
                expression(Mul);

                expr_type = tmp;
                if (expr_type > PTR) {
                    // pointer type, and not `char *`
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                }
                *++text = ADD;
            } else if (token == Sub) {
                // sub
                match(Sub);
                *++text = PUSH;
                expression(Mul);
                if (tmp > PTR && tmp == expr_type) {
                    // pointer subtraction
                    *++text = SUB;
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = DIV;
                    expr_type = INT;
                } else if (tmp > PTR) {
                    // pointer movement
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                    *++text = SUB;
                    expr_type = tmp;
                } else {
                    // numeral subtraction
                    *++text = SUB;
                    expr_type = tmp;
                }
            } else if (token == Mul) {
                // multiply
                match(Mul);
                *++text = PUSH;
                expression(Inc);
                *++text = MUL;
                expr_type = tmp;
            } else if (token == Div) {
                // divide
                match(Div);
                *++text = PUSH;
                expression(Inc);
                *++text = DIV;
                expr_type = tmp;
            } else if (token == Mod) {
                // Modulo
                match(Mod);
                *++text = PUSH;
                expression(Inc);
                *++text = MOD;
                expr_type = tmp;
            } else if (token == Inc || token == Dec) {
                // postfix inc(++) and dec(--)
                if (*text == LI) {
                    *text = PUSH;
                    *++text = LI;
                }
                else if (*text == LC) {
                    *text = PUSH;
                    *++text = LC;
                }
                else {
                    printf("%d: bad value in increment\n", line);
                    exit(-1);
                }

                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? ADD : SUB;
                *++text = (expr_type == CHAR) ? SC : SI;
                *++text = PUSH;
                *++text = IMM;
                *++text = (expr_type > PTR) ? sizeof(int) : sizeof(char);
                *++text = (token == Inc) ? SUB : ADD;
                match(token);

            } else if (token == Brak) {
                // array access var[xx]
                match(Brak);
                *++text = PUSH;
                expression(Assign);
                match(']');

                if (tmp > PTR) {
                    // pointer
                    *++text = PUSH;
                    *++text = IMM;
                    *++text = sizeof(int);
                    *++text = MUL;
                } else if (tmp < PTR) {
                    printf("%d: pointer type expected\n", line);
                    exit(-1);
                }
                expr_type = tmp - PTR;
                *++text = ADD;
                *++text = (expr_type == CHAR) ? LC : LI;
            } else {
                printf("%d: compiler error, token = %d\n", line, token);
                exit(-1);
            }
        }
    }
}

// Assert the current token
void match(int tk) {
    if (token == tk) {
        next();
    } else {
        printf("%d: Syntax Error! Expected token: %d\n", line, tk);
        exit(-1);
    }
}

// Analysis statement and break it down to expression
void statement() {
    int *a, *b; // bess for branch control

    if (token == If) {
        ////////////////////////////////////////
        //   if (...) {            <cond>
        //                         JZ a
        //     <statement>         <statement>
        //   }
        //                  ==>     
        // //a                  a:
        //
        ////////////////////////////////////////
        //   if (...) {            <cond>
        //                         JZ a
        //     <statement>         <statement>
        //   }
        //   else {        ==>     JMP b
        // //a                  a:
        //     <statement>         <statement>
        // //b                  b:
        //   }
        /////////////////////////////////////////
        match(If);
        match('(');
        expression(Assign);
        match(')');
        *++text = JZ;
        b = ++text;
        statement();

        if (token == Else) {
            match(Else);

            // emit code for JMP B
            *b = (int)(text + 3);
            *++text = JMP;
            b = ++text;
            statement();
        }

        *b = (int)(text + 1);
    } else if (token == While) {
        ////////////////////////////////////////
        // a:                     a:
        //    while (<cond>) {      <cond>
        //                          JZ b
        //       <statement>  ==>   <statement>
        //    }                     JMP a
        // b:                     b:
        ////////////////////////////////////////
        match(While);

        a = text + 1;
        match('(');
        expression(Assign);
        match(')');

        *++text = JZ;
        b = ++text;
        statement();

        *++text = JMP;
        *++text = (int)a;
        *b = (int)(text + 1);
    } else if (token == '{') {
        // { <statement> ... }
        match('{');

        while (token != '}') {
            statement();
        }
        match('}');

    } else if (token == Return) {
        // return [expression];
        match(Return);
        if (token != ';') {
            expression(Assign);
        }

        match(';');
        *++text = LEV;

    } else if (token == ';') {
        match(';');

    } else {
        // a = b; or function_call();
        expression(Assign);
        match(';');
    }
}

void function_declaration() {
    // type func_name (...) {...}
    match('(');
    function_parameter();
    match(')');


    match('{');
    function_body();

    // unwind local variable declarations for all local variables. TODO
    current_id = symbols;
    while (current_id[Token]) {
        if (current_id[Class] == Loc) {
            current_id[Class] = current_id[BClass];
            current_id[Type]  = current_id[BType];
            current_id[Value] = current_id[BValue];
        }
        current_id = current_id + IdSize;
    }
}

void function_parameter() {
    // type func_name (...) {...}
    //             -->|   |<--
    int type;
    int params;
    params = 0;
    while (token != ')') {
        type = INT;
        if (token == Int) {
            match(Int);
        } else if (token == Char) {
            type = CHAR;
            match(Char);
        }

        // pointer type
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }
        // parameter name
        if (token != Id) {
            printf("%d: bad parameter declaration\n", line);
            exit(-1);
        }
        if (current_id[Class] == Loc) {
            printf("%d: duplicate parameter declaration\n", line);
            exit(-1);
        }
        match(Id);

        // store the local variable
        current_id[BClass] = current_id[Class];
        current_id[Class]  = Loc;
        current_id[BType]  = current_id[Type];
        current_id[Type]   = type;
        current_id[BValue] = current_id[Value];
        current_id[Value]  = params++;   // index of current parameter
        if (token == ',') {
            match(',');
        }
    }
    index_of_bp = params+1;
}
void function_body() {
    // type func_name (...) {...}
    //                   -->|   |<--

    int pos_local; // position of local variables on the stack.
    int type;
    pos_local = index_of_bp;
    while (token == Int || token == Char) {
        // local variable declaration, just like global ones.
        basetype = (token == Int) ? INT : CHAR;
        match(token);
        while (token != ';') {
            type = basetype;
            while (token == Mul) {
                match(Mul);
                type = type + PTR;
            }
            if (token != Id) {
                // invalid declaration
                printf("%d: bad local declaration\n", line);
                exit(-1);
            }
            if (current_id[Class] == Loc) {
                // identifier exists
                printf("%d: duplicate local declaration\n", line);
                exit(-1);
            }
            match(Id);
            // store the local variable
            current_id[BClass] = current_id[Class];
            current_id[Class]  = Loc;
            current_id[BType]  = current_id[Type];
            current_id[Type]   = type;
            current_id[BValue] = current_id[Value];
            current_id[Value]  = ++pos_local;   // index of current parameter
            if (token == ',') {
                match(',');
            }
        }
        match(';');
    }
    // save the stack size for local variables
    *++text = ENT;
    *++text = pos_local - index_of_bp;
    // statements
    while (token != '}') {
        statement();
    }
    // emit code for leaving the sub function
    *++text = LEV;
}

// Top level for all global declaration
void global_declaration() {
    int type; // tmp, actual type for variable
    int i; // tmp
    basetype = INT;
    // parse enum, this should be treated alone.
    if (token == Enum) {
        // enum [id] { a = 10, b = 20, ...  }
        //           |<-enum_declaration()->|
        match(Enum);
        if (token != '{') {
            match(Id); // skip the [id] part
        }
        if (token == '{') {
            // parse the assign part
            match('{');
            enum_declaration();
            match('}');
        }
        match(';');
        return;
    }
    // parse type information
    if (token == Int) {
        match(Int);
    }
    else if (token == Char) {
        match(Char);
        basetype = CHAR;
    }
    // parse the comma seperated variable declaration.
    while (token != ';' && token != '}') {
        type = basetype;
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }
        if (token != Id) {
            // invalid declaration
            printf("%d: Bad global declaration\n", line);
            exit(-1);
        }
        if (current_id[Class]) {
            // identifier exists
            printf("%d: Duplicate global declaration\n", line);
            exit(-1);
        }
        match(Id);
        current_id[Type] = type;
        if (token == '(') {
            current_id[Class] = Fun;
            current_id[Value] = (int)(text + 1); // the memory address of function
            function_declaration();
        } else {
            // variable declaration
            current_id[Class] = Glo; // global variable
            current_id[Value] = (int)data; // assign memory address
            data = data + sizeof(int);
        }
        if (token == ',') {
            match(',');
        }
    }
    next();
}

void enum_declaration() {
    // parse enum [id] { a = 1, b = 3, ...}
    int i;
    i = 0;
    while (token != '}') {
        if (token != Id) {
            printf("%d: bad enum identifier %d\n", line, token);
            exit(-1);
        }
        next();
        if (token == Assign) {
            // like {a=10}
            next();
            if (token != Num) {
                printf("%d: bad enum initializer\n", line);
                exit(-1);
            }
            i = token_val;
            next();
        }
        current_id[Class] = Num;
        current_id[Type] = INT;
        current_id[Value] = i++;
        if (token == ',') {
            next();
        }
    }
}


void program() {
    next();
    while (token > 0) {
        global_declaration();
        //printf("token is: %c\n", token);
    }
}

// run the assembly code be generated
int eval() {
    int op, *tmp;
    while (1) {
        op = *pc++;
        
        // Common functions
        if (op == IMM)       {ax = *pc++;}                                     // load immediate value to ax
        else if (op == LC)   {ax = *(char *)ax;}                               // load character to ax, address in ax
        else if (op == LI)   {ax = *(int *)ax;}                                // load integer to ax, address in ax
        else if (op == SC)   {ax = *(char *)*sp++ = ax;}                       // save character to address, value in ax, address on stack
        else if (op == SI)   {*(int *)*sp++ = ax;}                             // save integer to address, value in ax, address on stack
        else if (op == PUSH) {*--sp = ax;}                                     // push the value of ax onto the stack
        else if (op == JMP)  {pc = (int *)*pc;}                                // jump to the address
        else if (op == JZ)   {pc = ax ? pc + 1 : (int *)*pc;}                  // jump if ax is zero
        else if (op == JNZ)  {pc = ax ? (int *)*pc : pc + 1;}                  // jump if ax is zero
        else if (op == CALL) {*--sp = (int)(pc+1); pc = (int *)*pc;}           // call subroutine
        else if (op == RET)  {pc = (int *)*sp++;}                              // return from subroutine
        else if (op == ENT)  {                                                 // make new stack frame
            *--sp = (int)bp; 
            bp = sp; 
            sp = sp - *pc++;
        }                                                                      
        else if (op == ADJ)  {sp = sp + *pc++;}                                // add esp, <size>
        else if (op == LEV)  {                                                 // restore call frame and PC
            sp = bp; 
            bp = (int *)*sp++; 
            pc = (int *)*sp++;
        }                                                                      
        else if (op == LEA)  {ax = (int)(bp + *pc++);}                         // load address for arguments.
        
        // Default Operator:
        else if (op == OR)  ax = *sp++ |  ax;
        else if (op == XOR) ax = *sp++ ^  ax;
        else if (op == AND) ax = *sp++ &  ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ <  ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ +  ax;
        else if (op == SUB) ax = *sp++ -  ax;
        else if (op == MUL) ax = *sp++ *  ax;
        else if (op == DIV) ax = *sp++ /  ax;
        else if (op == MOD) ax = *sp++ %  ax;
        
        // System functions:
        else if (op == EXIT) { 
            printf("exit(%d)", *sp); 
            return *sp;
        }
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
        else if (op == PRTF) { 
            tmp = sp + pc[1]; 
            ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); 
        }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
        // Exception:
        else {
            printf("Invalid instruction:%d \n", op);
            return -1;
        }

    }
    return 0;
}

int main(int argc, char **argv) {

    int i, fd;

    argc--;
    argv++;

    poolsize = 127 * 1024; // arbitrary size, TODO: (> 128 will crash the program)
    line = 1;

    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }
    
    // allocate memory for virtual machine
    if (!(text = old_text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }
    if (!(symbols = malloc(poolsize))) {
        printf("could not malloc(%d) for symbol table\n", poolsize);
        return -1;
    }
    bp = sp = (int *)((int)stack + poolsize);
    ax = 0;
    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);
    memset(symbols, 0, poolsize);
    bp = sp = (int *)((int)stack + poolsize);
    ax = 0;
    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit "
          "void main";

    // add keywords to symbol table
    i = Char;
    while (i <= While) {
        next();
        current_id[Token] = i++;
    }

    // add library to symbol table
    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
    }

    next();
    current_id[Token] = Char; // handle void type
    next();
    idmain = current_id; // keep track of main

    if (!(src = old_src = malloc(poolsize))) {
        printf("could not malloc(%d) for source area\n", poolsize);
        return -1;
    }
    // read the source file
    if ((i = read(fd, src, poolsize-1)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // add EOF character
    close(fd);
    // Test:
    //i = 0;
    //text[i++] = IMM;
    //text[i++] = 10;
    //text[i++] = PUSH;
    //text[i++] = IMM;
    //text[i++] = 20;
    //text[i++] = ADD;
    //text[i++] = PUSH;
    //text[i++] = EXIT;
    //pc = text;
    
    program();
    int *tmp;
    
    if (!(pc = (int *)idmain[Value])) {
        printf("main() not defined\n");
        return -1;
    }

    // setup stack
    sp = (int *)((int)stack + poolsize);
    *--sp = EXIT; // call exit if main returns
    *--sp = PUSH;
    tmp = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)tmp;

    return eval();
}