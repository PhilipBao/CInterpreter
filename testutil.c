/** 
 * Coverage
 * enum { LEA ,IMM ,JMP ,CALL,JZ ,JNZ ,ENT ,ADJ ,LEV ,LI ,LC ,SI ,SC , PUSH,
 *        OR ,XOR ,AND ,EQ ,NE ,LT ,GT ,LE ,GE ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV , MOD,
 *        OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT};
 * enum { Num = 64, Fun, Sys, Glo, Loc, Id,
 *        Char, Else, Enum, If, Int, Return, Sizeof, While,
 *        Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak};
 * enum { Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};
 * enum { CHAR, INT, PTR };
 */

const char* getEnumStr(int e) {
	switch (e) {
		case LEA   :  return "LEA";
		case IMM   :  return "IMM";
		case JMP   :  return "JMP";
		case CALL  :  return "CALL";
		case JZ    :  return "JZ";
		case JNZ   :  return "JNZ";
		case ENT   :  return "ENT";
		case ADJ   :  return "ADJ";
		case LEV   :  return "LEV";
		case LI    :  return "LI";
		case LC    :  return "LC";
		case SI    :  return "SI";
		case SC    :  return "SC";
		case PUSH  :  return "PUSH";
		case OR    :  return "OR";
		case XOR   :  return "XOR";
		case AND   :  return "AND";
		case EQ    :  return "EQ";
		case NE    :  return "NE";
		case LT    :  return "LT";
		case GT    :  return "GT";
		case LE    :  return "LE";
		case GE    :  return "GE";
		case SHL   :  return "SHL";
		case SHR   :  return "SHR";
		case ADD   :  return "ADD";
		case SUB   :  return "SUB";
		case MUL   :  return "MUL";
		case DIV   :  return "DIV";
		case MOD   :  return "MOD";
		case OPEN  :  return "OPEN";
		case READ  :  return "READ";
		case CLOS  :  return "CLOS";
		case PRTF  :  return "PRTF";
		case MALC  :  return "MALC";
		case MSET  :  return "MSET";
		case MCMP  :  return "MCMP";
		case EXIT  :  return "EXIT";
	}
	switch (e) {
		case Num   :  return "Num";
		case Fun   :  return "Fun";
		case Sys   :  return "Sys";
		case Glo   :  return "Glo";
		case Loc   :  return "Loc";
		case Id    :  return "Id";
		case Char  :  return "Char";
		case Else  :  return "Else";
		case Enum  :  return "Enum";
		case If    :  return "If";
		case Int   :  return "Int";
		case Return:  return "Return";
		case Sizeof:  return "Sizeof";
		case While :  return "While ";
		case Assign:  return "Assign";
		case Cond  :  return "Cond";
		case Lor   :  return "Lor";
		case Lan   :  return "Lan";
		case Or    :  return "Or";
		case Xor   :  return "Xor";
		case And   :  return "And";
		case Eq    :  return "Eq";
		case Ne    :  return "Ne";
		case Lt    :  return "Lt";
		case Gt    :  return "Gt";
		case Le    :  return "Le";
		case Ge    :  return "Ge";
		case Shl   :  return "Shl";
		case Shr   :  return "Shr";
		case Add   :  return "Add";
		case Sub   :  return "Sub";
		case Mul   :  return "Mul";
		case Div   :  return "Div";
		case Mod   :  return "Mod";
		case Inc   :  return "Inc";
		case Dec   :  return "Dec";
		case Brak  :  return "Brak";
	}
	switch (e) {
		case Token :  return "Token";
		case Hash  :  return "Hash";
		case Name  :  return "Name";
		case Type  :  return "Type";
		case Class :  return "Class";
		case Value :  return "Value";
		case BType :  return "BType";
		case BClass:  return "BClass";
		case BValue:  return "BValue";
		case IdSize:  return "IdSize";
	}
	switch (e) {
		case CHAR :  return "CHAR";
		case INT  :  return "INT";
		case PTR  :  return "PTR";
	}
	char *str = malloc(32);
	snprintf(str, 16, "%d", e);
	return str;
}