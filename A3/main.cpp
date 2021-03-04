#include <iostream>
#include <fstream>
#include <string>
#include <map>
using namespace std;

string file_path = "/Users/aparahuja/Desktop/IITD/COL216 - Arch/COL216_Lab/A3/A3/input.txt";
struct register_file{
    //INSTRUCTION COUNT
    //int ins_cnt_add, ins_cnt_sub...
    //int ins_cnt[10] ... add is 0, sub is 1, etc ...
    //map<string, int> ins_cnt;
    int cycle_cnt = 0;
    void print_reg_file(){}
    void execution_stats(){}
    void add(){cycle_cnt++;}
    void sub(){cycle_cnt++;}
    void mul(){cycle_cnt++;}
    void beq(){cycle_cnt++;}
    void bne(){cycle_cnt++;}
    void slt(){cycle_cnt++;}
    void j(){cycle_cnt++;}
    void lw(){cycle_cnt++;}
    void sw(){cycle_cnt++;}
    void addi(){cycle_cnt++;}
};

//add line to memory
void addToMemory(string line){
    cout<<line<<"\n"; // for debugging
}
int main(int argc, const char * argv[]) {
    ifstream infile(file_path);
    //File error
    if(!infile.is_open()){
        cout<<"ERROR: Unable to open file.\n";
        return 0;
    }
    string line;
    //read line by line
    while(getline(infile, line)){
        //process and store each line in memory
        addToMemory(line);
    }
    infile.close();
    return 0;
}
