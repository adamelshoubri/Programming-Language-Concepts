//
//  main.cpp
//  project2
//
//  Created by Adam Elshoubri on 10/27/16.
//  Copyright © 2016 Adam Elshoubri. All rights reserved.
//

// test version of main

#include "p2lex.h"
using namespace std;

// defines the linenum declared as extern in p2lex.h
int	linenum = 0;

// defines the getToken function; this needs to be fleshed out

Token
getToken(std::istream* instream)
{
    return Token();
}

int main()
{
    Token	t1;
    Token	t2;
    Token	t3(ID, "help");
    Token	t4(ID, "me");
    Token	t5(INT, "27");
    Token	t6(LPAREN, "(");
    
    cout << (t1 == t2) << endl;
    cout << (t1 == t3) << endl;
    cout << (t3 == t4) << endl;
    cout << (t3 == t5) << endl;
    
    cout << t3.getTok() << endl;
    cout << t4.getLexeme() << endl;
    cout << t6.getTok() << ":" << t6.getLexeme() << endl;
    
    cout << "Done!" << endl;
}