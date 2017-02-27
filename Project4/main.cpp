//  main.cpp
//  Program_4
//
//  CS 280, Section 3, FALL 2016
//Adam Elshoubri
// Project #4

#include "p2lex.h"
#include <string>
#include <istream>
#include <fstream>
#include <map>
using namespace std;

/// Token Wrapper
Token saved;
bool isSaved = false;

Token GetAToken(istream *in) {
    if( isSaved ) {
        isSaved = false;
        return saved;
    }
    
    return getToken(in);
}

void PushbackToken(Token& t) {
    if( isSaved ) {
        cerr << "Can't push back more than one token!!!";
        exit(0);
    }
    
    saved = t;
    isSaved = true;
}

int linenum = 0;
int globalErrorCount = 0;

/// error handler
void error(string msg, bool showline=true)
{
    if( showline )
        cout << linenum << ": ";
    cout << msg << endl;
    ++globalErrorCount;
}

int RuntimeErrors = 0;

void error_runtime (string message)
{
    RuntimeErrors += 1;
    cout << "RUNTIME ERROR" << message << endl;
}

enum ValueType
{
    ValueStr, ValueInt
};

class Value
{
    ValueType type;
    string string_value;
    int int_value;

public:
    Value (ValueType type, int int_value, string string_value) : type(type), int_value(int_value), string_value(string_value) {}
        ValueType get_type()
            {return type;};
        Value *add (Value *right)
        {
            if(right -> get_type() == ValueStr && this -> get_type() == ValueStr)
            {
                return new Value (ValueStr, -1, this -> string_value + right -> string_value);
            
            } else if (right -> get_type() == ValueInt && this -> get_type() == ValueInt)
            {
                return new Value (ValueInt, this -> int_value + right -> int_value, "");
            }

            error_runtime ("Addition of two operands is not possible.");
            return NULL;
        };

	Value *mul(Value *right)
	{
        if (this->get_type() == ValueInt && right->get_type() == ValueInt)
        {
            return new Value(ValueInt, this->int_value * right->int_value, "");
        }
        
    	if (this->get_type() == ValueStr && right->get_type() == ValueStr)
        {
            error_runtime("Cannot multiply a string by a string");
            return NULL;
        }

        int t = this->get_type() == ValueInt ? this->int_value : right->int_value;
        string multiply = this -> get_type() == ValueStr ? this -> string_value: right->string_value;
        string value = "";
    
        for (int i = 0; i < t; i++)
            value += multiply;
        
        return new Value (ValueStr, -1, value);
    };

	Value *bracket(Value *right, Value *left)
    {
        if (left -> get_type() != ValueInt || (right && right -> get_type() != ValueInt))
        {
            error_runtime ("Bracket operator cannot be a non-int type");
            return NULL;
        }
    
        int from = left -> int_value;
        int to = right ? right -> int_value : from + 1;
    
    	string result = "";
    
        for (int i = from; i < to; i++)
        {
            if (i >= this -> string_value.length())
            {
                error_runtime("Cannot access out of string's bounds");
                return NULL;
            }
            result += this -> string_value [i];
        }
    
        return new Value(ValueStr, -1, result);
    };

	void print()
	{
    
    	if (this -> get_type() == ValueStr)
            cout << this -> string_value << endl;
        else
            cout << this -> int_value << endl;
    }
};


//// this class can be used to represent the parse result
//// the various different items that the parser might recognize can be
//// subclasses of ParseTree

class ParseTree {
private:
    ParseTree *leftChild;
    ParseTree *rightChild;
    
    int	whichLine;
    
public:
    ParseTree(ParseTree *left = 0, ParseTree *right = 0) : leftChild(left),rightChild(right) {
        whichLine = linenum;
    }
    
    int onWhichLine() { return whichLine; }
    
    int traverseAndCount(int (ParseTree::*f)()) {
        int cnt = 0;
        if( leftChild ) cnt += leftChild->traverseAndCount(f);
        if( rightChild ) cnt += rightChild->traverseAndCount(f);
        return cnt + (this->*f)();
    }
    
    int countUseBeforeSet( map<string,int>& symbols ) {
        int cnt = 0;
        if( leftChild ) cnt += leftChild->countUseBeforeSet(symbols);
        if( rightChild ) cnt += rightChild->countUseBeforeSet(symbols);
        return cnt + this->checkUseBeforeSet(symbols);
    }
    
    virtual int checkUseBeforeSet( map<string,int>& symbols ) {
        return 0;
    }
    
    virtual int isPlus() { return 0; }
    virtual int isStar() { return 0; }
    virtual int isBrack() { return 0; }
    virtual int isEmptyString() { return 0; }
    
    ParseTree *get_right() { return rightChild; };
    ParseTree *get_left() { return leftChild; };
    
    virtual void eval (map < string, Value* > &state)
    {
        if (this -> get_right())
            this -> get_right() -> eval(state);
        if (this -> get_left())
            this -> get_left() -> eval(state);
    }
    
    virtual Value *get_value(map < string, Value* > &state)
    { return NULL; };
};

class Slist : public ParseTree {
public:
    Slist(ParseTree *left, ParseTree *right) : ParseTree(left,right) {}
};

class PrintStmt : public ParseTree {
public:
    PrintStmt(ParseTree *expr) : ParseTree(expr) {}
    void eval(map < string, Value* > &state)
    {
        this -> get_left() -> get_value(state) -> print();
    };
};

class SetStmt : public ParseTree {
private:
    string	ident;
    
public:
    SetStmt(string id, ParseTree *expr) : ParseTree(expr), ident(id) {}
    
    int checkUseBeforeSet( map<string,int>& symbols ) {
        symbols[ident]++;
        return 0;
    }
    
    void eval(map < string, Value* > &state)
    {
        state[ident] = this -> get_left() -> get_value(state);
    };
};

class PlusOp : public ParseTree {
public:
    PlusOp(ParseTree *left, ParseTree *right) : ParseTree(left,right) {}
    int isPlus() { return 1; }
    
    Value *get_value(map < string, Value* > &state)
    {
        return this -> get_left() -> get_value(state) -> add(this -> get_right() -> get_value(state));
    };
};

class StarOp : public ParseTree {
public:
    StarOp(ParseTree *left, ParseTree *right) : ParseTree(left,right) {}
    int isStar() { return 1; }
    
    Value *get_value(map < string, Value* > &state)
    {
        return this -> get_left() -> get_value(state) -> mul(this -> get_right() -> get_value(state));
    };
};

class BracketOp : public ParseTree {
private:
    Token sTok;
    
public:
    BracketOp(const Token& sTok, ParseTree *left, ParseTree *right = 0) : ParseTree(left,right), sTok(sTok) {}
    int isBrack() { return 1; }
    
    Value *get_value(map < string, Value* > &state)
    {
        string lexeme = sTok.getLexeme();
        Value *s = new Value(ValueStr, -1, lexeme.substr(1, lexeme.length()));
        
        Value *left = this -> get_left() -> get_left() -> get_value(state);
        Value *right = this -> get_right() ? this -> get_right() -> get_value(state) : NULL;
        
        return s -> bracket(left, right);
    };
};

class StringConst : public ParseTree {
private:
    Token sTok;
    
public:
    StringConst(const Token& sTok) : ParseTree(), sTok(sTok) {}
    
    string	getString() { return sTok.getLexeme(); }
    int isEmptyString() {
        if( sTok.getLexeme().length() == 2 ) {
            error("Empty string not permitted on line " + to_string(onWhichLine()), false );
            return 1;
        }
        return 0;
    }
    
    Value *get_value(map < string, Value* > &state)
    {
        string lexeme = sTok.getLexeme();
        return new Value(ValueStr, -1, lexeme.substr(1, lexeme.length() - 2));
    }
};

//// for example, an integer...
class Integer : public ParseTree {
private:
    Token	iTok;
    
public:
    Integer(const Token& iTok) : ParseTree(), iTok(iTok) {}
    
    int	getInteger() { return stoi( iTok.getLexeme() ); }
    
    Value *get_value(map < string, Value* > &state)
    {
        string lexeme = iTok.getLexeme();
        return new Value(ValueInt, stoi(lexeme), "");
    };
};

class Identifier : public ParseTree {
private:
    Token	iTok;
    
public:
    Identifier(const Token& iTok) : ParseTree(), iTok(iTok) {}
    
    int checkUseBeforeSet( map<string,int>& symbols ) {
        if( symbols.find( iTok.getLexeme() ) == symbols.end() ) {
            error("Symbol " + iTok.getLexeme() + " used without being set at line " + to_string(onWhichLine()), false);
            return 1;
        }
        return 0;
    }
    
    Value *get_value(map < string, Value* > &state)
    {
        return state[iTok.getLexeme()];
    }
};

/// function prototypes
ParseTree *Program(istream *in);
ParseTree *StmtList(istream *in);
ParseTree *Stmt(istream *in);
ParseTree *Expr(istream *in);
ParseTree *Term(istream *in);
ParseTree *Primary(istream *in);
ParseTree *String(istream *in);


ParseTree *Program(istream *in)
{
    ParseTree *result = StmtList(in);
    
    // make sure there are no more tokens...
    if( GetAToken(in).getTok() != DONE )
        return 0;
    
    return result;
}


ParseTree *StmtList(istream *in)
{
    ParseTree *stmt = Stmt(in);
    
    if( stmt == 0 )
        return 0;
    
    return new Slist(stmt, StmtList(in));
}


ParseTree *Stmt(istream *in)
{
    Token t;
    
    t = GetAToken(in);
    
    if( t.getTok() == ERR ) {
        error("Invalid token");
        return 0;
    }
    
    if( t.getTok() == DONE )
        return 0;
    
    if( t.getTok() == PRINT ) {
        // process PRINT
        ParseTree *ex = Expr(in);
        
        if( ex == 0 ) {
            error("Expecting expression after print");
            return 0;
        }
        
        if( GetAToken(in).getTok() != SC ) {
            error("Missing semicolon");
            return 0;
        }
        
        return new PrintStmt(ex);
    }
    else if( t.getTok() == SET ) {
        // process SET
        Token tid = GetAToken(in);
        
        if( tid.getTok() != ID ) {
            error("Expecting identifier after set");
            return 0;
        }
        
        ParseTree *ex = Expr(in);
        
        if( ex == 0 ) {
            error("Expecting expression after identifier");
            return 0;
        }
        
        if( GetAToken(in).getTok() != SC ) {
            error("Missing semicolon");
            return 0;
        }
        
        return new SetStmt(tid.getLexeme(), ex);
    }
    else {
        error("Syntax error, invalid statement");
    }
    
    return 0;
}


ParseTree *Expr(istream *in)
{
    ParseTree *exp = Term( in );
    
    if( exp == 0 ) return 0;
    
    while( true ) {
        
        Token t = GetAToken(in);
        
        if( t.getTok() != PLUS ) {
            PushbackToken(t);
            break;
        }
        
        ParseTree *exp2 = Term( in );
        if( exp2 == 0 ) {
            error("missing operand after +");
            return 0;
        }
        
        exp = new PlusOp(exp, exp2);
    }
    
    return exp;
}


ParseTree *Term(istream *in)
{
    ParseTree *pri = Primary( in );
    
    if( pri == 0 ) return 0;
    
    while( true ) {
        
        Token t = GetAToken(in);
        
        if( t.getTok() != STAR ) {
            PushbackToken(t);
            break;
        }
        
        ParseTree *pri2 = Primary( in );
        if( pri2 == 0 ) {
            error("missing operand after *");
            return 0;
        }
        
        pri = new StarOp(pri, pri2);
    }
    
    return pri;
}


ParseTree *Primary(istream *in)
{
    Token t = GetAToken(in);
    
    if( t.getTok() == ID ) {
        return new Identifier(t);
    }
    else if( t.getTok() == INT ) {
        return new Integer(t);
    }
    else if( t.getTok() == STR ) {
        PushbackToken(t);
        return String(in);
    }
    else if( t.getTok() == LPAREN ) {
        
        ParseTree *ex = Expr(in);
        if( ex == 0 )
            return 0;
        t = GetAToken(in);
        if( t.getTok() != RPAREN ) {
            error("expected right parens");
            return 0;
        }
        
        return ex;
    }
    
    return 0;
}


ParseTree *String(istream *in)
{
    Token t = GetAToken(in); // I know it's a string!
    ParseTree *lexpr, *rexpr;
    
    Token lb = GetAToken(in);
    if( lb.getTok() != LEFTSQ ) {
        PushbackToken(lb);
        return new StringConst(t);
    }
    
    lexpr = Expr(in);
    if( lexpr == 0 ) {
        error("missing expression after [");
        return 0;
    }
    
    lb = GetAToken(in);
    if( lb.getTok() == RIGHTSQ ) {
        return new BracketOp(t, lexpr);
    }
    else if( lb.getTok() != SC ) {
        error("expected ; after first expression in []");
        return 0;
    }
    
    rexpr = Expr(in);
    if( rexpr == 0 ) {
        error("missing expression after ;");
        return 0;
    }
    
    lb = GetAToken(in);
    if( lb.getTok() == RIGHTSQ ) {
        return new BracketOp(t, lexpr, rexpr);
    }
    
    error("expected ]");
    return 0;
}


int
main(int argc, char *argv[])
{
    istream *in = &cin;
    ifstream infile;
    
    for( int i = 1; i < argc; i++ ) {
        if( in != &cin ) {
            cerr << "Cannot specify more than one file" << endl;
            return 1;
        }
        
        infile.open(argv[i]);
        if( infile.is_open() == false ) {
            cerr << "Cannot open file " << argv[i] << endl;
            return 1;
        }
        
        in = &infile;
    }
    
    ParseTree *prog = Program(in);
    
    if( prog == 0 || globalErrorCount != 0 ) {
        return 0;
    }
    
    map<string,int> symbols;
    int useBeforeSetCount = prog -> countUseBeforeSet( symbols );
    int emptyStrings = prog -> traverseAndCount(&ParseTree::isEmptyString);
    
    if(useBeforeSetCount)
    {
        cout << emptyStrings << " empty strings " << endl;
        return 0;
    }
    
    map<string , Value* > state;
    prog -> eval(state);
    
    return 0;
}