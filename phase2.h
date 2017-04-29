#ifndef PHASE2_H_INCLUDED
#define PHASE2_H_INCLUDED

void read_grammar(char* filename);
void make_parsing_table();
void print_production_map();
void print_epsilon_nonterminals();
void initialize_first();
void initialize_follow();
void print_first();
void print_follow();
void make_parsing_table();

#endif // PHASE2_H_INCLUDED
