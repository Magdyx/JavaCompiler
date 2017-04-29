#include <iostream>

#include "phase2.h"

using namespace std;

void test_grammar();
void test_printing_parsing_table();

int main()
{
    test_printing_parsing_table();
    return 0;
}


void test_grammar()
{
    read_grammar("Grammar_file.txt");
    print_production_map();
    print_epsilon_nonterminals();
}

void test_printing_parsing_table()
{
    read_grammar("Grammar_file.txt");
    print_production_map();
    print_epsilon_nonterminals();
    initialize_first();
    initialize_follow();
    print_first();
    print_follow();
    make_parsing_table();
}
