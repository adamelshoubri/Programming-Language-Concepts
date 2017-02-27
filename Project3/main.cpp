//  main.cpp
//  Program_3
//
//  CS 280, Section 3, FALL 2016
// Adam Elshoubri
// Project #3
//

#include "p2lex.h"
#include <map>
#include <list>
#include <fstream>

#define IN_DEBUG false

#define DEBUG(dbug)  if (IN_DEBUG) {dbug;}

using namespace std;


class PTError
{
private:
    int line_num;
public:
    PTError(int line_num) : line_num(line_num)
    {}
    
    int getLineNum()
    { return line_num; }
    
    virtual void print()
    {}
};

class PTUsageError : public PTError
{
private:
    string symbol;
public:
    PTUsageError(int line_num, string symbol) : PTError(line_num), symbol(symbol)
    {}
    
    string getSymbol()
    { return symbol; }
    
    void print()
    {
        
        cout << "Symbol " << getSymbol() << " used without being set at line " << getLineNum() << endl;
        
    }
};

class PTStringError : public PTError
{
public:
    PTStringError(int line_num) : PTError(line_num)
    {}
    
    void print()
    {
        cout << "Empty string not permitted on line " << getLineNum() << endl;
    }
    
};


enum PTType
{
    PT_OPERATION,
    PT_STRING,
    PT_INT,
    PT_IDENTIFIER,
    PT_OTHER
};

class PTNode
{
    
    int line;
    Token token;
    PTType type;
public:
    PTNode(int line, Token token, PTType type) : line(line), token(token), type(type)
    {};
    
    int getLine()
    { return line; }
    
    bool is(PTType check) { return check == this->type; }
    
    void setLine(int line)
    { this->line = line; }
    
    Token getToken()
    { return token; }
    
    void setToken(Token token)
    { this->token = token; }
    
    virtual void
    stats(map<string, bool> &state, map<string, int> &operator_count, list<PTError *> &errors)
    {};
};

class PTPrimary : public PTNode
{
public:
    PTPrimary(int line, Token token, PTType type) : PTNode(line, token, type)
    {};
    PTPrimary(int line, Token token) :  PTNode(line, token, PT_OTHER)
    {};
};

class PTIdentifier : public PTPrimary
{
private:
    string identifier;
public:
    PTIdentifier(int line, Token token, string id) : identifier(id), PTPrimary(line, token, PT_IDENTIFIER)
    {}
    
    string getIdentifier()
    { return identifier; }
    
    void
    stats(map<string, bool> &state, map<string, int> &operator_count, list<PTError *> &errors)
    {
        if (!state[getIdentifier()])
        {
            errors.push_back(new PTUsageError(getLine(), getIdentifier()));
        }
    }
};

class PTString : public PTPrimary
{
private:
    string str;
public:
    PTString(int line, Token token, string str) : str(str), PTPrimary(line, token, PT_STRING)
    {}
    
    string getString()
    { return str; }
    
    void
    stats(map<string, bool> &state, map<string, int> &operator_count, list<PTError *> &errors)
    {
        
        DEBUG(cout << "Checking for " << str << endl);
        if (getString().length() == 2)
            errors.push_back(new PTStringError(getLine()));
        
        PTPrimary::stats(state, operator_count, errors);
    }
};

class PTInt : public PTPrimary
{
private:
    int value;
public:
    PTInt(int line, Token token, int value) : value(value), PTPrimary(line, token, PT_INT)
    {}
    
    int getValue()
    { return value; }
};

class PTOperation : public PTPrimary
{
private:
    PTPrimary *left;
    PTPrimary *right;
public:
    PTOperation(int line, Token token, PTPrimary *left, PTPrimary *right)
    : PTPrimary(line, token, PT_OPERATION)
    {
        this->setLeft(left);
        this->setRight(right);
    }
    
    void
    stats(map<string, bool> &state, map<string, int> &operator_count, list<PTError *> &errors)
    {
        
        static unsigned long deep = 0;
        
        DEBUG(cout << string(deep++, '\t') << "Entering Operation ( " << this->getToken().getLexeme() << " ) "
              << endl);
        operator_count[this->getToken().getLexeme()]++;
        
        
        PTPrimary *l = this->getLeft(), *r = this->getRight();
        
        DEBUG(cout << string(deep, '\t') << "Left: " << (l ? l->getToken().getLexeme() : "NA") << endl);
        if (l)
            l->stats(state, operator_count, errors);
        
        DEBUG(cout << string(deep, '\t') << "Right: " << (r ? r->getToken().getLexeme() : "NA") << endl);
        if (r)
            r->stats(state, operator_count, errors);
        
        DEBUG(cout << string(--deep, '\t') << "Exiting Operation" << endl);
    }
    
    void setLeft(PTPrimary *left)
    { this->left = left; }
    
    void setRight(PTPrimary *right)
    { this->right = right; }
    
    
    PTPrimary *getLeft()
    { return left; }
    
    PTPrimary *getRight()
    { return right; }
};


class PTSet : public PTNode
{
private:
    PTIdentifier *identifier;
    PTOperation *operation;
public:
    PTSet(int line, Token token, PTIdentifier *identifier, PTOperation *operation)
    : identifier(identifier), operation(operation), PTNode(line, token, PT_OTHER)
    {
        
    }
    
    void
    stats(map<string, bool> &state, map<string, int> &operator_count, list<PTError *> &errors)
    {
        
        DEBUG(cout << "Stat: Set(" << identifier->getIdentifier() << ", " << operation->getToken().getLexeme() << ")"
              << endl);
        operation->stats(state, operator_count, errors);
        state[identifier->getIdentifier()] = true;
        
    }
};


class PTPrint : public PTNode
{
private:
    PTOperation *operation;
public:
    PTPrint(int line, Token token, PTOperation *operation) : operation(operation), PTNode(line, token, PT_OTHER)
    {
        
    }
    
    void
    stats(map<string, bool> &state, map<string, int> &operator_count, list<PTError *> &errors)
    {
        DEBUG(cout << "Entering print" << endl);
        operation->stats(state, operator_count, errors);
    }
};

class PTStatementList : public PTNode
{
private:
    list<PTNode *> nodes;
public:
    
    PTStatementList() : PTNode(-1, Token(), PT_OTHER)
    {};
    
    void add(PTNode *node)
    {
        nodes.push_back(node);
    }
    
    void
    stats(map<string, bool> &state, map<string, int> &operator_count, list<PTError *> &errors)
    {
        list<PTNode *>::const_iterator iterator;
        for (iterator = nodes.begin(); iterator != nodes.end(); ++iterator)
        {
            PTNode *node = *iterator;
            if (!node)
                continue;
            node->stats(state, operator_count, errors);
        }
        
    }
};


int linenum = 0;
int globalErrorCount = 0;

void error(string msg)
{
    cout << linenum << ": " << msg << endl;
    ++globalErrorCount;
}

PTStatementList *StmtList(istream &in);

PTNode *Stmt(istream &in);

PTOperation *Expr(istream &in);

PTOperation *Term(istream &in);

PTPrimary *Primary(istream &in);

PTPrimary *String(istream &in);

PTIdentifier *Identifier(istream &in);

list<Token> buffer;

void add_operation(PTOperation **operation, PTOperation *to_add)
{
    if (!*operation)
    {
        *operation = to_add;
        return;
    }
    
    to_add->setLeft(*operation);
    
    *operation = to_add;
}

Token read(istream &in)
{
    Token t;
    if (!buffer.empty())
    {
        t = buffer.front();
        buffer.pop_front();
        
        DEBUG(cout << "Read " << t << " from stream\n");
        return t;
    }
    t = getToken(&in);
    DEBUG(cout << "Read " << t << " from stream\n");
    return t;
}

void push_back(Token t)
{
    buffer.push_back(t);
    DEBUG(cout << "Pushed " << t << " back\n");
}

bool is_next(istream &in, TokenType type)
{
    if (!buffer.empty())
    {
        Token t = buffer.front();
        buffer.pop_front();
        buffer.push_back(t);
        return t.getTok() == type;
    }
    
    Token t = getToken(&in);
    buffer.push_back(t);
    return t.getTok() == type;
}


bool has_more_tokens(istream &in)
{
    return !is_next(in, DONE) && !is_next(in, ERR) && !globalErrorCount;
}


PTStatementList *StmtList(istream &in)
{
    PTStatementList *statementList = new PTStatementList;
    statementList->add(Stmt(in));
    
    while (has_more_tokens(in))
    {
        statementList->add(Stmt(in));
    }
    
    return statementList;
}


PTNode *Stmt(istream &in)
{
    Token t = read(in);
    
    switch (t.getTok())
    {
        case PRINT:
        {
            PTPrint *print = new PTPrint(linenum, t, Expr(in));
            if (!is_next(in, SC))
                error("Missing semicolon after print.");
            read(in);
            return print;
        }
        case SET:
        {
            
            
            if (!is_next(in, ID))
            {
                error("Expected an ID, got something else.");
                return NULL;
            }
            
            PTIdentifier *identifier = Identifier(in);
            PTOperation *operation = Expr(in);
            
            if (!operation)
            {
                error("Attempted to parse expression that wasn't found.");
                return NULL;
            }
            PTSet *set = new PTSet(linenum, t, identifier, operation);
            if (!is_next(in, SC))
            {
                error("Expected semicolon.");
                return NULL;
            }
            read(in);
            return set;
        }
        default:
        {
            error("Syntax error, invalid statement");
            return NULL;
        }
    }
}


PTOperation *Expr(istream &in)
{
    
    PTOperation *left = Term(in);
    
    while (is_next(in, PLUS))
    {
        Token t = read(in);
        left = new PTOperation(linenum, t, left, Term(in));
    }
    
    
    return left;
}


PTOperation *Term(istream &in)
{
    
    
    PTPrimary *p = Primary(in);
    
    if (!p) return NULL;
    
    PTOperation *left = p->is(PT_OPERATION) ? (PTOperation*) p : new PTOperation(linenum, Token(), p, NULL);
    
    
    while (is_next(in, STAR))
    {
        Token t = read(in);
        left = new PTOperation(linenum, t, left, Primary(in));
    }
    
    
    return left;
}


PTPrimary *Primary(istream &in)
{
    Token t = read(in);
    
    switch (t.getTok())
    {
        case ID:
        {
            return new PTIdentifier(linenum, t, t.getLexeme());
        }
        case INT:
        {
            return new PTInt(linenum, t, stoi(t.getLexeme().c_str()));
            
        }
        case STR:
        {
            push_back(t);
            return String(in);
        }
        case LPAREN:
        {
            PTOperation *expr = Expr(in);
            if (expr == NULL)
                return NULL;
            t = read(in);
            if (t.getTok() != RPAREN)
            {
                error("expected right parens");
                return NULL;
            }
            
            return expr;
        }
        default:
        {
            error("Attempted to parse Primary, found another token type instead.");
            return NULL;
        }
    }
    
}


PTIdentifier *Identifier(istream &in)
{
    if (!is_next(in, ID))
        return NULL;
    
    Token token = read(in);
    return new PTIdentifier(linenum, token, token.getLexeme());
}

PTPrimary *String(istream &in)
{
    int line = linenum;
    Token main_string = read(in);
    PTString *astString = new PTString(line, main_string, main_string.getLexeme());;
    
    if (is_next(in, SC) || !is_next(in, LEFTSQ))
    {
        return astString;
    }
    
    Token bracket = read(in);
    PTOperation *expr = Expr(in);
    PTOperation *bracket_operation = new PTOperation(line, bracket, astString, expr);
    
    if (is_next(in, SC))
    {
        Token semi = read(in);
        PTOperation *second_expr = Expr(in);
        PTOperation *expanded_bracket = new PTOperation(line, semi, bracket_operation->getRight(), second_expr);
        bracket_operation->setRight(expanded_bracket);
    }
    
    if (!is_next(in, RIGHTSQ))
    {
        error("Expected closing bracket.");
        return NULL;
    }
    
    read(in);
    
    return bracket_operation;
    
}


int main(int argc, char *argv[])
{
    PTStatementList *prog;
    
    if (argc == 1)
    {
        prog = StmtList(cin);
    }
    else if (argc == 2)
    {
        ifstream file(argv[1]);
        if (!file.good())
        {
            cout << "Error opening file." << endl;
            return 1;
        }
        prog = StmtList(file);
    }
    else
    {
        cout << "Wrong number of args" << endl;
        return 1;
    }
    
    if (globalErrorCount)
        return 1;
    
    map<string, bool> state;
    map<string, int> operator_count;
    list<PTError *> errors;
    prog->stats(state, operator_count, errors);
    
    cout << "Count of + operators: " << operator_count["+"] << endl;
    cout << "Count of * operators: " << operator_count["*"] << endl;
    cout << "Count of [] operators: " << operator_count["["] << endl;
    
    list<PTError *>::const_iterator iterator;
    for (iterator = errors.begin(); iterator != errors.end(); ++iterator)
    {
        PTError *error = *iterator;
        if (!error)
            continue;
        error->print();
    }
    
    return 0;
}