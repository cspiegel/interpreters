
/* advexp.c - expression compiler for adventure games */
/*
	Copyright (c) 1986, by David Michael Betz
	All rights reserved
*/

#include "advcom.h"
#include "advdbs.h"

/* external routines */
extern SYMBOL *sfind();

/* external variables */
extern char t_token[];
extern int t_value;
extern int curobj;
extern char *code;
extern int cptr;

/* forward declarations */
int do_cond(),do_and(),do_or(),do_if(),do_while(),do_progn();
int do_setq(),do_return(),do_send(),do_sndsuper();

/* opcode tables */
static struct { char *nt_name; int nt_code,nt_args; } *nptr,ntab[] = {
	"not",		OP_NOT,		1,
	"+",		OP_ADD,		2,
	"-",		OP_SUB,		2,
	"*",		OP_MUL,		2,
	"/",		OP_DIV,		2,
	"%",		OP_REM,		2,
	"&",		OP_BAND,	2,
	"|",		OP_BOR,		2,
	"~",		OP_BNOT,	1,
	"<",		OP_LT,		2,
	"=",		OP_EQ,		2,
	">",		OP_GT,		2,
	"getp",		OP_GETP,	2,
	"setp",		OP_SETP,	3,
	"class",	OP_CLASS,	1,
	"match",	OP_MATCH,	2,
	"print",	OP_PRINT,	1,
	"print-number",	OP_PNUMBER,	1,
	"print-noun",	OP_PNOUN,	1,
	"terpri",	OP_TERPRI,	0,
	"finish",	OP_FINISH,	0,
	"chain",	OP_CHAIN,	0,
	"abort",	OP_ABORT,	0,
	"exit",		OP_EXIT,	0,
	"save",		OP_SAVE,	0,
	"restore",	OP_RESTORE,	0,
	"restart",	OP_RESTART,	0,
	"yes-or-no",	OP_YORN,	0,
	"rand",		OP_RAND,	1,
	"randomize",	OP_RNDMIZE,	0,
	0
};
static struct { char *ft_name; int (*ft_fcn)(); } *fptr,ftab[] = {
	"cond",		do_cond,
	"and",		do_and,
	"or",		do_or,
	"if",		do_if,
	"while",	do_while,
	"progn",	do_progn,
	"setq",		do_setq,
	"return",	do_return,
	"send",		do_send,
	"send-super",	do_sndsuper,
	0
};

/* do_expr - compile a subexpression */
do_expr()
{
    int tkn;

    switch (token()) {
    case T_OPEN:
	switch (tkn = token()) {
	case T_IDENTIFIER:
	    if (in_ntab() || in_ftab())
		break;
	default:
	    stoken(tkn);
	    do_call();
	}
	break;
    case T_NUMBER:
	do_literal();
	break;
    case T_STRING:
	do_literal();
	break;
    case T_IDENTIFIER:
	do_identifier();
	break;
    default:
	error("Expecting expression");
    }
}

/* in_ntab - check for a function in ntab */
int in_ntab()
{
    for (nptr = ntab; nptr->nt_name; ++nptr)
	if (strcmp(t_token,nptr->nt_name) == 0) {
	    do_nary(nptr->nt_code,nptr->nt_args);
	    return (TRUE);
	}
    return (FALSE);
}

/* in_ftab - check for a function in ftab */
int in_ftab()
{
    for (fptr = ftab; fptr->ft_name; ++fptr)
	if (strcmp(t_token,fptr->ft_name) == 0) {
	    (*fptr->ft_fcn)();
	    return (TRUE);
	}
    return (FALSE);
}

/* do_cond - compile the (COND ... ) expression */
do_cond()
{
    int tkn,nxt,end;

    /* initialize the fixup chain */
    end = NIL;

    /* compile each COND clause */
    while ((tkn = token()) != T_CLOSE) {
	require(tkn,T_OPEN);
	do_expr();
	putcbyte(OP_BRF);
	nxt = putcword(NIL);
	while ((tkn = token()) != T_CLOSE) {
	    stoken(tkn);
	    do_expr();
	}
	putcbyte(OP_BR);
	end = putcword(end);
	fixup(nxt,cptr);
    }

    /* fixup references to the end of statement */
    if (end)
	fixup(end,cptr);
    else
	putcbyte(OP_NIL);
}

/* do_and - compile the (AND ... ) expression */
do_and()
{
    int tkn,end;

    /* initialize the fixup chain */
    end = NIL;

    /* compile each expression */
    while ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
	putcbyte(OP_BRF);
	end = putcword(end);
    }

    /* fixup references to the end of statement */
    if (end)
	fixup(end,cptr);
    else
	putcbyte(OP_NIL);
}

/* do_or - compile the (OR ... ) expression */
do_or()
{
    int tkn,end;

    /* initialize the fixup chain */
    end = NIL;

    /* compile each expression */
    while ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
	putcbyte(OP_BRT);
	end = putcword(end);
    }

    /* fixup references to the end of statement */
    if (end)
	fixup(end,cptr);
    else
	putcbyte(OP_T);
}

/* do_if - compile the (IF ... ) expression */
do_if()
{
    int tkn,nxt,end;

    /* compile the test expression */
    do_expr();

    /* skip around the 'then' clause if the expression is false */
    putcbyte(OP_BRF);
    nxt = putcword(NIL);

    /* compile the 'then' clause */
    do_expr();

    /* compile the 'else' clause */
    if ((tkn = token()) != T_CLOSE) {
	putcbyte(OP_BR);
	end = putcword(NIL);
	fixup(nxt,cptr);
	stoken(tkn);
	do_expr();
	frequire(T_CLOSE);
	nxt = end;
    }

    /* handle the end of the statement */
    fixup(nxt,cptr);
}

/* do_while - compile the (WHILE ... ) expression */
do_while()
{
    int tkn,nxt,end;

    /* compile the test expression */
    nxt = cptr;
    do_expr();

    /* skip around the 'then' clause if the expression is false */
    putcbyte(OP_BRF);
    end = putcword(NIL);

    /* compile the loop body */
    while ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
    }

    /* branch back to the start of the loop */
    putcbyte(OP_BR);
    putcword(nxt);

    /* handle the end of the statement */
    fixup(end,cptr);
}

/* do_progn - compile the (PROGN ... ) expression */
do_progn()
{
    int tkn,n;

    /* compile each expression */
    for (n = 0; (tkn = token()) != T_CLOSE; ++n) {
	stoken(tkn);
	do_expr();
    }

    /* check for an empty statement list */
    if (n == 0)
	putcbyte(OP_NIL);
}

/* do_setq - compile the (SETQ v x) expression */
do_setq()
{
    char name[TKNSIZE+1];
    int n;

    /* get the symbol name */
    frequire(T_IDENTIFIER);
    strcpy(name,t_token);

    /* compile the value expression */
    do_expr();

    /* check for this being a local symbol */
    if ((n = findarg(name)) >= 0)
	code_setargument(n);
    else if ((n = findtmp(name)) >= 0)
	code_settemporary(n);
    else {
	n = venter(name);
	code_setvariable(n);
    }
    frequire(T_CLOSE);
}

/* do_return - handle the (RETURN [expr]) expression */
do_return()
{
    int tkn;

    /* look for a result expression */
    if ((tkn = token()) != T_CLOSE) {
	stoken(tkn);
	do_expr();
	frequire(T_CLOSE);
    }

    /* otherwise, default the result to nil */
    else
	putcbyte(OP_NIL);

    /* insert the return opcode */
    putcbyte(OP_RETURN);
}

/* do_send - handle the (SEND obj msg [expr]...) expression */
do_send()
{
    /* start searching for the method at the object itself */
    putcbyte(OP_NIL);

    /* compile the object expression */
    putcbyte(OP_PUSH);
    do_expr();

    /* call the general message sender */
    sender();
}

/* do_sndsuper - handle the (SEND-SUPER msg [expr]...) expression */
do_sndsuper()
{
    /* start searching for the method at the current class object */
    code_literal(curobj);

    /* pass the message to "self" */
    putcbyte(OP_PUSH);
    code_argument(findarg("self"));

    /* call the general message sender */
    sender();
}

/* sender - compile an expression to send a message to an object */
sender()
{
    int tkn,n;
    
    /* compile the selector expression */
    putcbyte(OP_PUSH);
    do_expr();

    /* compile each argument expression */
    for (n = 2; (tkn = token()) != T_CLOSE; ++n) {
	stoken(tkn);
	putcbyte(OP_PUSH);
	do_expr();
    }
    putcbyte(OP_SEND);
    putcbyte(n);
}

/* do_call - compile a function call */
do_call()
{
    int tkn,n;
    
    /* compile the function itself */
    do_expr();

    /* compile each argument expression */
    for (n = 0; (tkn = token()) != T_CLOSE; ++n) {
	stoken(tkn);
	putcbyte(OP_PUSH);
	do_expr();
    }
    putcbyte(OP_CALL);
    putcbyte(n);
}

/* do_nary - compile nary operator expressions */
do_nary(op,n)
  int op,n;
{
    while (n--) {
	do_expr();
	if (n) putcbyte(OP_PUSH);
    }
    putcbyte(op);
    frequire(T_CLOSE);
}

/* do_literal - compile a literal */
do_literal()
{
    code_literal(t_value);
}

/* do_identifier - compile an identifier */
do_identifier()
{
    SYMBOL *sym;
    int n;
    
    if (match("t"))
	putcbyte(OP_T);
    else if (match("nil"))
	putcbyte(OP_NIL);
    else if ((n = findarg(t_token)) >= 0)
	code_argument(n);
    else if ((n = findtmp(t_token)) >= 0)
	code_temporary(n);
    else if (sym = sfind(t_token)) {
	if (sym->s_type == ST_VARIABLE)
	    code_variable(sym->s_value);
	else
	    code_literal(sym->s_value);
    }
    else
	code_literal(oenter(t_token));
}

/* code_argument - compile an argument reference */
code_argument(n)
  int n;
{
    putcbyte(OP_ARG);
    putcbyte(n);
}

/* code_setargument - compile a set argument reference */
code_setargument(n)
  int n;
{
    putcbyte(OP_ASET);
    putcbyte(n);
}

/* code_temporary - compile an temporary reference */
code_temporary(n)
  int n;
{
    putcbyte(OP_TMP);
    putcbyte(n);
}

/* code_settemporary - compile a set temporary reference */
code_settemporary(n)
  int n;
{
    putcbyte(OP_TSET);
    putcbyte(n);
}

/* code_variable - compile a variable reference */
code_variable(n)
  int n;
{
    if (n < 32)
	putcbyte(OP_XVAR+n);
    else if (n < 256)
	{ putcbyte(OP_SVAR); putcbyte(n); }
    else
	{ putcbyte(OP_VAR); putcword(n); }
}

/* code_setvariable - compile a set variable reference */
code_setvariable(n)
  int n;
{
    if (n < 32)
	putcbyte(OP_XSET+n);
    else if (n < 256)
	{ putcbyte(OP_SSET); putcbyte(n); }
    else
	{ putcbyte(OP_SET); putcword(n); }
}

/* code_literal - compile a literal reference */
code_literal(n)
  int n;
{
    if (n >= 0 && n < 64)
	putcbyte(OP_XPLIT+n);
    else if (n < 0 && n > -64)
	putcbyte(OP_XNLIT-n);
    else if (n >= 64 && n < 256)
	{ putcbyte(OP_SPLIT); putcbyte(n); }
    else if (n <= -64 && n > -256)
	{ putcbyte(OP_SNLIT); putcbyte(-n); }
    else
	{ putcbyte(OP_LIT); putcword(n); }
}

/* do_op - insert an opcode and look for closing paren */
do_op(op)
  int op;
{
    putcbyte(op);
    frequire(T_CLOSE);
}

/* putcbyte - put a code byte into data space */
int putcbyte(b)
  int b;
{
    if (cptr < CMAX)
	code[cptr++] = b;
    else
	error("insufficient code space");
    return (cptr-1);
}

/* putcword - put a code word into data space */
int putcword(w)
  int w;
{
    putcbyte(w);
    putcbyte(w >> 8);
    return (cptr-2);
}

/* fixup - fixup a reference chain */
fixup(chn,val)
  int chn,val;
{
    int hval,nxt;

    /* store the value into each location in the chain */
    for (hval = val >> 8; chn != NIL; chn = nxt) {
	if (chn < 0 || chn > CMAX-2)
	    return;
	nxt = (code[chn] & 0xFF) | (code[chn+1] << 8);
	code[chn] = val;
	code[chn+1] = hval;
    }
}

