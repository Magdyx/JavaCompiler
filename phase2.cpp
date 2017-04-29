#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stack>
#include <set>
#include <cstdlib>

#include "phase2.h"

#define PRODUCTION_SEPARATOR '='
#define NEW_PRODUCTION_INDICATOR '#'
#define OR_CHARACTER '|'
#define TERMINAL_INDICATOR '\''
#define TERMINAL_OPENING_INDICATOR '‘'
#define TERMINAL_CLOSING_INDICATOR '’'
#define BACKSLASH '\\'
#define CAPITAL_L 'L'
#define SYNC "SYNC"
#define ERROR "ERROR"
#define EPSILON "EPSILON"
#define EPSON "EPSON"
#define END_OF_TEXT "\$"

using namespace std;

/********DATA STRUCTURE******************/

struct Symbol {
    string value;
    bool terminal;
    bool operator == (const Symbol &x) const {
        /* your logic for comparison between "*this" and "rhs" */
        return (x.value == this->value && x.terminal == this->terminal);
    }
};

struct Production {
    // symbol accompanied with boolean either
    // it is terminal or non-terminal
    // true means terminal
    // false means non terminal
    vector<Symbol> symbols;
};


//key of map is the nonterminal on LHS and value
//is a vector of terminals and non terminals on RHS
// **this map is ready after reading grammar files
unordered_map<string, vector< Production > > productions;

vector<string> terminals;

//non terminal with either defined or not
unordered_map<string, bool> non_terminals;

unordered_map<string, int> terminals_index;
unordered_map<string, int> nonterminals_index;
vector<vector<string>> parsing_table;


//key of map is the non terminal
//vector of terminals that come first of the
//non terminal key and each terminal is
//accompanied by production that derivate it
//to this terminal
unordered_map<string,map<string,Production>> first; //production a struct?????????????

//
unordered_map<string,string> nonterminal_epsilon;

//key of map is the non terminal
//value of map is a vector of terminals who
//follow the non-terminal key
unordered_map<string,set<string>> follow;

//starting non terminal
string starting_lhs;

//to keep trace while recursion to find loop in first & follow
set<string> trace;
set<string> finished;


enum grammar_rules_errors {
    AMBIGUITY,
    EMPTY_LHS_OR_RHS,
    UNCLOSED_TERMINAL_QUOTATION,
    NON_TERMINAL_NOT_DEFINED,
    ERROR_READING_PARSING_TABLE,
    UNKNOWN_LINE,
    LEFT_RECURSION_ERROR,
    LEFT_FACTORING_ERROR
};

/****************************************/

/********Function prototypes*************/
string read_grammar_file(char* filename);
bool is_white_space(char c);
bool is_new_production(char c);
bool is_separator (char c);
bool is_or_char (char c);
bool is_epsilon (char c, char next_c);
bool is_terminal (char c);
bool is_terminal_closing (char c);
bool is_nonterminal (char c);
bool is_escape_char(char c);
bool is_reserved(char c);
bool is_nonterminal_first_char (char c);
bool is_in_string_bounds(int i, int length);
bool add_to_terminals(string terminal);
void add_to_non_terminal(string non_terminal);
void define_non_terminal(string lhs);
bool verify_nonterminals_valid(void);
string trim_string(string str);
string detect_left_recursion(string lhs, string lhs_iterator);
void add_production(string lhs, vector<Symbol> rhs);
void compute_table_entries();
void print_parsing_table();
void allocate_terminals();
void allocate_nonterminals();
void check_grammar_rules();
void error_grammar_file(int err_code);


/****************************************/


/********Main functions******************/
/*
read grammar file, parse it and produce a map of productions
called "productions"

*/
void read_grammar(char* filename) {
    string lhs = "";
    bool is_rhs = true; //if true->reading in RHS else in LHS
    bool is_there_epsilon = false;
    int line_begining = 0;
    vector<Symbol> rhs;
    string grammar_buffer = read_grammar_file(filename);
    int grammar_buffer_length = grammar_buffer.length();

    for(int i = 0; is_in_string_bounds(i,grammar_buffer_length); i++) {
        char current_char = grammar_buffer.at(i);
        if ( is_white_space(current_char) ) {
            //skip
            //just not error
            //looking forward to get the newline character out of this
        } else if ( is_new_production(current_char) ) {
            //find a hash# ? begin new production rule and wrap up the previous one
            if(is_rhs == false) {
                error_grammar_file(EMPTY_LHS_OR_RHS);
                //error
                //must get to rhs first
            }
            if(is_there_epsilon && rhs.empty()) {
                nonterminal_epsilon[lhs] = lhs;
            } else {
                add_production(lhs,rhs);
            }
            lhs = "";
            vector<Symbol> temp;
            rhs = temp; //reinitialize rhs;
            line_begining = i+1;
            is_rhs = false;
        } else if ( is_separator(current_char) && !is_rhs) { //separator between LHS and RHS
            string lhs_untrimmed = grammar_buffer.substr(line_begining,i-line_begining);
            lhs = trim_string(lhs_untrimmed); //check if working
            define_non_terminal(lhs);
            is_rhs = true;
        } else if (is_in_string_bounds(i,grammar_buffer_length-1)
                   && is_epsilon(current_char, grammar_buffer.at(i+1)) && is_rhs ) { //epsilon handling
            //if the epsilon is alone then push it to rhs and add it as production then empty the rhs
            //if not empty then its presence is not wanted
            is_there_epsilon = true;
            i++;
        } else if ( is_or_char(current_char) && is_rhs ) { // or character
            if(is_there_epsilon && rhs.empty()) {
                nonterminal_epsilon[lhs] = lhs;
            } else {
                add_production(lhs,rhs);
            }
            vector<Symbol> temp;
            rhs = temp; //reinitialize rhs;
            is_there_epsilon = false;
        } else if ( is_terminal(current_char) && is_rhs ) { // terminal
            string terminal_buffer = "";
            while(true) {
                i++;
                if ( !is_in_string_bounds(i,grammar_buffer_length) ) {
                    //error
                    //open terminal quotation without close
                    //terminate
                    error_grammar_file(UNCLOSED_TERMINAL_QUOTATION);
                    break;
                }
                current_char = grammar_buffer.at(i);
                if( is_escape_char(current_char) ) {
                    i++;
                    if ( !is_in_string_bounds(i,grammar_buffer_length) ) {
                        //error
                        //open terminal quotation without close
                        //terminate
                        error_grammar_file(UNCLOSED_TERMINAL_QUOTATION);
                        break;
                    }
                    current_char = grammar_buffer.at(i);
                    if(is_reserved(current_char)) {
                        terminal_buffer += current_char;
                        continue;
                    }

                }
                if( !is_terminal_closing(grammar_buffer.at(i))  ) {
                    terminal_buffer += current_char;
                } else {
                    break;
                }
            }
            //loop done add to terminal
            add_to_terminals(terminal_buffer); //not acceptable terminal
            Symbol new_symbol;
            new_symbol.terminal = true;
            new_symbol.value = terminal_buffer;
            rhs.push_back(new_symbol);
        } else if ( is_nonterminal_first_char(current_char) ) { // non terminal
            string nonterminal_buffer = "";
            while (true) {
                nonterminal_buffer += current_char;
                i++;
                if ( !is_in_string_bounds(i,grammar_buffer_length) ) {
                    //not error
                    //end reading and add to non-terminal list
                    break;
                }
                current_char = grammar_buffer.at(i);
                if( is_escape_char(current_char) ) {
                    i++;
                    if ( !is_in_string_bounds(i,grammar_buffer_length) ) {
                        //error
                        //open terminal quotation without close
                        //terminate
                        break;
                    }
                    current_char = grammar_buffer.at(i);
                    if(is_reserved(current_char)) {
                        nonterminal_buffer += current_char;
                        continue;
                    }

                }
                if(!is_nonterminal(current_char)) { //current character is to be handled again through main loop
                    i--;
                    break;
                }
            }
            // add to non terminal list
            add_to_non_terminal(nonterminal_buffer);
            if(is_rhs) {
                Symbol new_symbol;
                new_symbol.terminal = false;
                new_symbol.value = nonterminal_buffer;
                rhs.push_back(new_symbol);
            }
        } else {
            //error
            error_grammar_file(UNKNOWN_LINE);
        }
    }
    if(is_rhs == false) {
        error_grammar_file(EMPTY_LHS_OR_RHS);
        //error
        //must get to rhs first
    }
    if(is_there_epsilon && rhs.empty()) {
        nonterminal_epsilon[lhs] = lhs;
    } else {
        add_production(lhs,rhs);
    }
//    check_grammar_rules();
    if(!verify_nonterminals_valid()) {
        error_grammar_file(0);
        //error
        // a non-terminal is not defined
        // terminate
    }

}

void make_parsing_table() {
    allocate_terminals();
    allocate_nonterminals();
    compute_table_entries();
    print_parsing_table();
}


/*
First and Follow functions
*/
void get_first(string non_terminal) {
    int index;
    map<string,Production> first_terminals;
    if(nonterminal_epsilon.count(non_terminal)) {
        Production p;
        first_terminals.insert(make_pair(EPSON,p));
    }
    if(trace.count(non_terminal)) {
        //loop in rules
        //infinite recursion
        //error
        cout<<"ERROR : Infinite Recursion"<<endl;
    }
    ////iterates on production with or
    for(vector<Production>::iterator it = productions[non_terminal].begin();
            it != productions[non_terminal].end() ; it++) {
        index = 0;
        //iterates on concatenated symbols while EPSON found
        while(index < it->symbols.size()) {
            if(!(it->symbols[index].terminal)) {
                //non terminal at first
                //check if i have its first before or not
                if(!first.count(it->symbols[index].value)) {
                    //not found & must get it then continue
                    trace.insert(non_terminal);
                    get_first(it->symbols[index].value);
                    trace.erase(non_terminal);
                }
                //add all firsts of the non-terminal that in first
                for(map<string,Production>::iterator firsts_it = first[it->symbols[index].value].begin();
                        firsts_it != first[it->symbols[index].value].end(); firsts_it++) {
                    if(first_terminals.count(firsts_it->first) && first_terminals[firsts_it->first].symbols !=  it->symbols) {
                        //have same symbol form different production
                        //so it is not left factorized
                        cout<<"ERROR : Not left factorized"<<endl;
                    }
                    first_terminals.insert(make_pair(firsts_it->first,*it));
                }
                //if it has an epson in first set then must continue
                if(!first[it->symbols[index].value].count(EPSON)) {
                    //not founded
                    break;
                }

            } else {
                //terminal at first
                first_terminals.insert(make_pair(it->symbols[index].value, *it));
                break;
            }
            index++;
        }
    }
    first[non_terminal] = first_terminals;
}

void initialize_first() {
    for(unordered_map<string, vector< Production > >::iterator map_it = productions.begin();
            map_it != productions.end() ; map_it++) {
        //clear trace
        //clear finished
        trace.clear();
        if(!first.count(map_it->first))
            get_first(map_it->first);
    }
}

//Follow functions
void get_follow(string non_ter) {
    //iteration to get follow of terminal and non-terminals in vector
    //unordered_map<string,vector<Symbol>> follow_non_ter;
    trace.insert(non_ter);
    //insert non-terminal if it is not in follow map
    if(!follow.count(non_ter)) {
        set<string> temp;
        follow[non_ter]= temp;
    }
    for(unordered_map<string, vector< Production > >::iterator map_it = productions.begin();
            map_it != productions.end() ; map_it++) {
        //vector<Symbol> temp;
        //follow_non_ter[map_it->first] = temp;
        //vector iteration of productions
        for(int pro_index = 0; pro_index < productions[map_it->first].size(); pro_index++) {
            //vector symbols iteration
            for(int symbol_index = 0; symbol_index < productions[map_it->first][pro_index].symbols.size();
                    symbol_index++) {
                //iterate on every symbol in the rules
                if(productions[map_it->first][pro_index].symbols[symbol_index].value == non_ter) {
                    //not terminal symbol
                    if(symbol_index == productions[map_it->first][pro_index].symbols.size()-1) {
                        //it is right most symbol
                        if(trace.count(map_it->first)) {
                            continue;
                        }
                        if(!finished.count(map_it->first))
                            get_follow(map_it->first);
                        follow[non_ter].insert(follow[map_it->first].begin(),follow[map_it->first].end());
                        //add its follow to me
                    } else {
                        //not right most symbol
                        int i = 1;
                        while(true) {
                            Symbol s = productions[map_it->first][pro_index].
                                       symbols[symbol_index+i];
                            if(s.terminal) {
                                //terminal follows it
                                follow[non_ter].insert(s.value);
                                break;
                            } else {
                                //not terminal follows it
                                //put first set into  follow
                                for(map<string,Production>::iterator it = first[s.value].begin();
                                        it != first[s.value].end(); it++) {
                                    if(it->first != EPSON)
                                        follow[non_ter].insert(it->first);
                                }
                                //check if there is an EPSON
                                if(!first[s.value].count(EPSON)) {
                                    break;
                                }
                                //EPSON found must continue
                                if(symbol_index + i == productions[map_it->first][pro_index].symbols.size()-1) {
                                    //it is right most symbol
                                    if(trace.count(map_it->first)) {
                                        continue;
                                    }
                                    if(!finished.count(map_it->first))
                                        get_follow(map_it->first);
                                    follow[non_ter].insert(follow[map_it->first].begin(),follow[map_it->first].end());
                                    break;
                                    //add its follow to me
                                } else if(symbol_index + i < productions[map_it->first][pro_index].symbols.size()-1) {
                                    //it is not right most
                                    i++;
                                } else {
                                    break;
                                }
                            }
                        }
                    }

                }
            }
        }
    }
    finished.insert(non_ter);
    trace.erase(non_ter);
}

void initialize_follow() {
    set<string> f;
    f.insert("$");
    follow[starting_lhs] = f;
    for(unordered_map<string, vector< Production > >::iterator map_it = productions.begin();
            map_it != productions.end() ; map_it++) {
        //clear trace
        //clear finished
        finished.clear();
        trace.clear();
        get_follow(map_it->first);
    }
}

void print_first() {
    cout<<"size : "<<first.size()<<endl;
    for(unordered_map<string,map<string, Production>>::iterator it = first.begin();
            it != first.end(); it++) {
        cout<<it->first<<" : ";
        for(map<string, Production>::iterator it2 = it->second.begin();
                it2 != it->second.end(); it2++) {
            cout<<"\t"<<it2->first <<"-> ";
            for(Symbol sy :it2->second.symbols ) {
                cout<<sy.value;
            }
            /*for(vector<Symbol>::iterator it3 = (it2->second).symbols.begin();                    it3 = (it2->second).symbols.end() ; it3++){
                cout<<it->value;
            }*/
            cout<<endl;
        }
        cout<<endl;
    }
}

void print_follow() {
    for(unordered_map<string,set<string>>::iterator it = follow.begin();
            it != follow.end(); it++) {
        cout<<it->first<<endl;
        cout<<"\t";
        for(string s : it->second) {
            cout<<", "<<s;
        }
        cout<<endl;
        cout<<endl;
    }
}

/********Auxiliary functions******************/
bool is_white_space(char c) {
    return (c == ' ' || c == '\t' || c == '\n')? true:false;
}

bool is_new_production(char c) {
    return (c == NEW_PRODUCTION_INDICATOR)? true:false;
}

bool is_separator (char c) {
    return (c == PRODUCTION_SEPARATOR) ? true:false;
}

bool is_or_char (char c) {
    return (c == OR_CHARACTER) ? true:false;
}

bool is_escape_char(char c) {
    return (c == BACKSLASH) ? true:false;
}

bool is_epsilon (char c, char next_c) {
    return (c == BACKSLASH && next_c == CAPITAL_L) ? true:false;
}

bool is_reserved(char c) {
    return (c == TERMINAL_INDICATOR || c == TERMINAL_OPENING_INDICATOR || c == TERMINAL_CLOSING_INDICATOR
            || c == PRODUCTION_SEPARATOR || c == BACKSLASH || c == NEW_PRODUCTION_INDICATOR) ? true:false;
}

bool is_terminal (char c) {
    return (c == TERMINAL_INDICATOR || c == TERMINAL_OPENING_INDICATOR) ? true:false;
}

bool is_terminal_closing (char c) {
    return (c == TERMINAL_INDICATOR || c == TERMINAL_CLOSING_INDICATOR) ? true:false;
}

bool is_nonterminal_first_char (char c) {
    return (c >= 65 && c <= 90) ? true:false;
}

bool is_nonterminal (char c) {
    return (c == '=' || c == '|' || is_white_space(c) || is_terminal(c)
            || is_terminal_closing(c) || is_separator(c)) ? false:true;
}

bool is_in_string_bounds(int i, int length) {
    return i < length? true:false;
}

/*
add to terminals vector if not empty nor repeated
//TODO: is acceptable terminal
return true if addition is successful (acceptable terminal)
*/
bool add_to_terminals(string terminal) {
    if(terminal == "")
        return true;
    int length = terminals.size();
    for(int i = 0; i < length; i++) {
        if(terminal == terminals[i]) {
            return true;
        }
    }
    //new terminal -> check in acceptable tokens from phase 1
    //not found -> return false
    terminals.push_back(terminal);
    return true;
}

/*
adds to non-terminal vector if not repeated
"can't think of a case where it is not valid"
so returns nothing
*/
void add_to_non_terminal(string non_terminal) {
    if(non_terminals.empty())
        starting_lhs = non_terminal;
    int length = non_terminals.size();
    if(non_terminals.find(non_terminal) == non_terminals.end()) {
        non_terminals[non_terminal] = false;
    }

}

void define_non_terminal(string lhs) {
    non_terminals[lhs] = true;
}

bool verify_nonterminals_valid(void) {
    for(unordered_map<string, bool>::iterator it = non_terminals.begin(); it !=non_terminals.end(); it++) {
        bool defined = it->second;
        if(!defined) {
            return false;
        }
    }
    return true;
}

//need update
void print_production_map() {
    for(unordered_map<string, vector<Production> >::iterator it = productions.begin(); it != productions.end(); it++) {
        string lhs = it->first;
        vector<Production> cur_productions = it->second;

        cout << lhs << " ::= ";

        for(int i=0; i<cur_productions.size(); i++) {
            vector<Symbol> cur_expr = cur_productions[i].symbols;
            for(int j = 0 ; j < cur_expr.size(); j++) {
                if(cur_expr[j].terminal)
                    cout << "^" << cur_expr[j].value << " ";
                else
                    cout << cur_expr[j].value << " ";
            }
            if(i != cur_productions.size()-1)
                cout << "| ";
        }
        cout << endl;
    }
}

void print_epsilon_nonterminals() {
    cout << "****************epsilon non-terminals**********" <<endl;
    for(unordered_map<string,string>::iterator it = nonterminal_epsilon.begin();
            it != nonterminal_epsilon.end(); it++) {
        string nonterminal = it->first;
        cout << nonterminal << endl;
    }
}

void add_production(string lhs, vector<Symbol> rhs) {
    if(lhs == "" || rhs.empty()) //lhs empty -> start of file
        return;
    Production new_prod;
    new_prod.symbols = rhs;
    vector<Production> prod_tmp = productions[lhs];
    prod_tmp.push_back(new_prod);
    productions[lhs] = prod_tmp;
}

string trim_string(string str) {
    size_t first = str.find_first_not_of(" \t\n");
    if (first == string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t\n");
    return str.substr(first, (last-first+1));
}

string read_grammar_file(char* filename) {
    ifstream in(filename);
    stringstream stream;
    stream << in.rdbuf();
    string grammar_buffer = stream.str();
    in.close();
    return grammar_buffer;
}

/*
give each terminal a number for the use of
2d array in parsing table
*/
void allocate_terminals() {
    int i;
    for(i = 0; i < terminals.size(); i++) {
        terminals_index[terminals[i]] = i;
    }
    terminals_index[END_OF_TEXT] = i;
}

/*
give each  non terminal a number for the use of
2d array in partition table
*/
void allocate_nonterminals() {
    int given_index = 0;
    for(unordered_map<string,bool>::iterator it = non_terminals.begin(); it != non_terminals.end(); it++) {
        string nonterminal = it->first;
        nonterminals_index[nonterminal] = given_index;
        given_index++;
    }
}

string concat_production(Production prod) {
    string result = "";
    vector<Symbol> prod_tmp = prod.symbols;
    int i;
    for(i = 0; i < prod_tmp.size()-1; i++) {
        Symbol s = prod_tmp[i];
//        if(s.terminal)
//            result += "^";
        result += s.value;
        result += " ";
    }
    result += prod_tmp[i].value;
    return result;
}

void initialize_parsing_table() {
    int terminals_size = terminals.size()+1;
    int nonterminal_size = non_terminals.size();
    /*
    parsing_table = new string*[nonterminal_size];
    for(int i = 0; i < nonterminal_size; ++i) {
        parsing_table[i] = new string[terminals_size];
    }
    */
    parsing_table.resize(nonterminal_size);
    for(int i = 0; i < nonterminal_size; ++i) {
        parsing_table[i].resize(terminals_size);
    }

    for(int i = 0; i < nonterminal_size; i++)
        for(int j = 0; j < terminals_size; j++)
            parsing_table[i][j] = "";
}

void destroy_parsing_table() {
    /*
    int parsing_table_size = sizeof(parsing_table[0])/sizeof(parsing_table[0][0]);
    for(int i = 0; i < parsing_table_size; i++) {
        delete [] parsing_table[i];
    }
    delete [] parsing_table;
    */
}

void add_to_parsing_table(string terminal, string nonterminal, string table_entry) {
    int terminal_index = terminals_index[terminal];
    int nonterminal_index = nonterminals_index[nonterminal];
    if(parsing_table[nonterminal_index][terminal_index] != "") {
        if(table_entry == SYNC)
            return;
        cout << "Filling " << nonterminal<< " " << terminal << " Found : " << " " << parsing_table[nonterminal_index][terminal_index]<< endl;
        cout << "while trying to fill it with " << table_entry << endl;
        error_grammar_file(AMBIGUITY);
        //error
        //not a LL(1) grammar
    }
    parsing_table[nonterminal_index][terminal_index] = table_entry;
}

bool epsilon_in_first(string non_terminal) {
    map<string, Production> first_set = first[non_terminal];
    if(first_set.find(EPSON) != first_set.end())
        return true;
    return false;
}

bool is_in_epsilon(string nonterminal) {
    if(nonterminal_epsilon.find(nonterminal) != nonterminal_epsilon.end())
        if(epsilon_in_first(nonterminal))
            return true;
    return false;
}

void fill_errors_parsing_table() {
    int rows = parsing_table.size();
    int cols = parsing_table[0].size();
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            if(parsing_table[i][j] == "")
                parsing_table[i][j] = ERROR;
        }
    }
}

void compute_table_entries() {
    initialize_parsing_table();
    //fill the first of each non terminal
    for(unordered_map<string,map<string,Production>>::iterator it = first.begin(); it != first.end(); it++) {
        string nonterminal = it->first;
        string terminal;
        map<string,Production> first_set = it->second;

        for(map<string,Production>::iterator it2 = first_set.begin(); it2 != first_set.end(); it2++) {
            terminal = it2->first;
            if(terminal == EPSON)
                continue;
            add_to_parsing_table(terminal,nonterminal,concat_production(it2->second));
        }
    }
    //fill the follow of each non terminal with epsilon transition or sync
    for(unordered_map<string,set<string>>::iterator it = follow.begin();
            it != follow.end(); it++) {
        string nonterminal = it->first;
        string terminal;
        set<string> follow_set = it->second;
        bool epsilon = is_in_epsilon(nonterminal);
        for(set<string>::iterator it = follow_set.begin(); it != follow_set.end(); it++) {
            terminal = *it;
            if(epsilon) {
                add_to_parsing_table(terminal,nonterminal,EPSILON);
            } else {
                add_to_parsing_table(terminal,nonterminal,SYNC);
            }
        }
    }
    //fill the empty cells with error
    fill_errors_parsing_table();
}

string get_terminal_of_index(int index) {
    for(unordered_map<string,int>::iterator it = terminals_index.begin(); it != terminals_index.end(); it++) {
        if(it->second == index)
            return it->first;
    }
}

string get_nonterminal_of_index(int index) {
    for(unordered_map<string,int>::iterator it = nonterminals_index.begin(); it != nonterminals_index.end(); it++) {
        if(it->second == index)
            return it->first;
    }
}


void print_parsing_table() {
    ofstream outputFile("parsing_table.txt");
    int rows = parsing_table.size();
    int cols = parsing_table[0].size();
    outputFile << starting_lhs << endl;
    outputFile << rows << " " << cols << endl;
    for(int i = 0; i < rows; i++) {
        string terminal;
        string expression;
        string nonterminal = get_nonterminal_of_index(i);
        outputFile << nonterminal << endl;
        for(int j = 0; j < cols; j++) {
            terminal   = get_terminal_of_index(j);
            expression = parsing_table[i][j];
            outputFile << "-> " << terminal;
            outputFile << " with " << expression;
            outputFile << endl;
        }
    }
    outputFile.close();
}

/*
nonterminal
-> terminal
with expression
*/
void read_parsing_table() {
    int rows, cols;
    string terminal, nonterminal, expression, temp;
    vector<vector<string>> parsing_table;
    vector<pair<string,string>> terminals_read; // pair of terminals and expressions
    unordered_map<string, int> terminals_no;
    unordered_map<string, int> nonterminals_no;

    ifstream inputFile("parsing_table.txt");
    inputFile >> starting_lhs;
    inputFile >> rows >> cols;
    parsing_table.resize(rows);
    for(int i = 0; i < rows; i++) {
        parsing_table[i].resize(cols);
    }

    for(int i = 0; i < rows; i++) {
        inputFile >> nonterminal;
        nonterminals_no[nonterminal] = i;
        for(int j = 0; j < cols; j++) {
            inputFile >> temp;
            if(temp != "->") {
                //error
                error_grammar_file(ERROR_READING_PARSING_TABLE);
            }
            inputFile >> terminal;
            if(i == 0) { //allocate in map
                terminals_no[terminal] = j;
            }
            inputFile >> temp;
            if(temp != "with") {
                //error
                error_grammar_file(ERROR_READING_PARSING_TABLE);
            }
            getline(inputFile, expression);
            expression = trim_string(expression);
            parsing_table[i][j] = expression;
        }
    }
    inputFile.close();
}

void check_grammar_rules() {
    for(unordered_map<string,vector<Production>>::iterator it = productions.begin();
            it != productions.end(); it++) {
        string lhs = it->first;
        //check left recursion
        vector<Production> or_prods = productions[lhs];
        string result_left_recursion = detect_left_recursion(lhs,lhs);
        if(result_left_recursion != "") {
            cout << "LEFT LECURSION" << endl;
            //solve
        }


    }
}

string detect_left_recursion(string lhs, string lhs_iterator) {
    vector<Production> or_prods = productions[lhs_iterator];
    vector<string> returns(or_prods.size());
    int i = 0;
    for(vector<Production>::iterator it = or_prods.begin(); it != or_prods.end(); it++) {

        vector<Symbol> symbols = it->symbols;
        Symbol first_symbol = symbols[0];
        if(!first_symbol.terminal) {
            if(lhs == lhs_iterator)
                returns[i] = lhs;
            else if(lhs == first_symbol.value)
                return lhs_iterator;
            else
                returns[i] = detect_left_recursion(lhs, first_symbol.value);
        }
        i++;
    }
    for(int j = 0; j < returns.size(); j++) {
        if(returns[j] != "")
            return returns[j];
    }
    return "";
}

void error_grammar_file(int err_code) {
    /*
    AMBIGUITY,
    EMPTY_LHS_OR_RHS,
    UNCLOSED_TERMINAL_QUOTATION,
    NON_TERMINAL_NOT_DEFINED,
    ERROR_READING_PARSING_TABLE,
    UNKNOWN_LINE
    */
    switch(err_code) {
    case AMBIGUITY: {
        cout << "Error: Ambiguity while filling table" << endl;
        break;
    }
    case EMPTY_LHS_OR_RHS: {
        cout << "Error: a grammar line is messed up may be empty lhs or rhs" << endl;
        break;
    }
    case UNCLOSED_TERMINAL_QUOTATION: {
        cout << "Error: unclosed terminal quotation" << endl;
        break;
    }
    case NON_TERMINAL_NOT_DEFINED: {
        cout << "Error: A non-terminal is not defined" << endl;
        break;
    }
    case ERROR_READING_PARSING_TABLE: {
        cout << "Error while reading parsing table" << endl;
        break;
    }
    case UNKNOWN_LINE: {
        cout << "Error: A line is not understandable" << endl;
        break;
    }
    default:
        cout << "Error reading grammar files" << endl;
    }
    exit(EXIT_FAILURE);
}

