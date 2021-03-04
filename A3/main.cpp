#include <iostream>
#include <fstream>
#include <string>
#include <map>

using namespace std;

string file_path = "input.txt";

int pc = 0;
int memory[1048576];

string decimalToHexadecimal(int a) {

    string ans = "";
    map<int, string> hex{ {0,"0"},{1,"1"},{2,"2"},{3,"3"},{4,"4"},{5,"5"},{6,"6"},{7,"7"},{8,"8"},{9,"9"},{10,"a"},{11,"b"},{12,"c"},{13,"d"},{14,"e"},{15,"f"} };
    for(int i = 0; i < 8; i ++) {
        int dig = a % 16;
        a = a / 16;
        ans = hex[dig] + ans;
    }
    return ans;

}


struct REGI {
    //INSTRUCTION COUNT
    //int ins_cnt_add, ins_cnt_sub...
    // check lower case and upper case

    map<string, int> ins_map = { {"add", 0}, {"sub", 1}, {"mul", 2},{"beq", 3},{"bne", 4},{"slt", 5},{"j", 6},{"lw", 7},{"sw", 8},{"addi", 9} };
    map<string, int> reg_map = { {"$zero", 0}, {"$at", 1}, {"$v0", 2}, {"$v1", 3}, {"$a0", 4}, {"$a1", 5}, {"$a2",6}, {"$a3", 7}, {"$t0", 8}, {"$t1", 9}, {"$t2", 10}, {"$t3", 11}, {"$t4", 12}, {"$t5", 13}, {"$t6", 14},
                                 {"$t7", 15}, {"$s0", 16}, {"$s1", 17}, {"$s2", 18}, {"$s3", 19}, {"$s4", 20}, {"$s5", 21}, {"$s6", 22}, {"$s7", 23}, {"$t8", 24}, {"$t9", 25}, {"$k0", 26}, {"$k1", 27}, {"$gp", 28}, {"$ap", 29}, {"$fp", 30},
                                {"$ra", 31}};
    int ins_cnt[10], reg[32];
    int cycle_cnt = 0;
    REGI() {
        for (int i = 0; i < 32; i++)
            reg[i] = 0;
        for (int i = 0; i < 10; i++)
            ins_cnt[i] = 0;
    }
    void print_reg_file() {
        cout << "Register file contents after " << cycle_cnt << " clock cycles:\n";
        int j = 1;
        for (auto i = reg_map.begin(); i != reg_map.end(); i++) {
            cout << i->first << ": " << decimalToHexadecimal(reg[i->second]);
            if (j % 5 == 0 || j == 32) cout << "\n";
            else 
                cout << ", ";
            j++;
        }
    }
    
    void execution_stats() {
        cout << "Total clock cycles: " << cycle_cnt << "\n";
        for (auto i = ins_map.begin(); i != ins_map.end(); i++) {
            cout << i->first << ": " << ins_cnt[i->second] << "\n";
        }
    }
    int labelToAddr(string label) {
        // if label is integer return else hash
    }
    void add() { }
    void sub() {  }
    void mul() {  }
    void beq() { }
    void bne() {  }
    void slt() {  }
    void j() {  }
    void lw() { }
    void sw() {  }
    void addi() {  }
    void stat_update(string instruction) { cycle_cnt++; ins_cnt[ins_map[instruction]]++; }
};

//add line to memory
void addToMemory(string line) {
    cout << line << "\n"; // for debugging
}
int main(int argc, const char* argv[]) {
    ifstream infile(file_path);
    //File error
    if (!infile.is_open()) {
        cout << "ERROR: Unable to open file.\n";
        return 0;
    }
    string line;
    //read line by line
    while (getline(infile, line)) {
        //process and store each line in memory
        addToMemory(line);
    }
    infile.close();
    REGI rf;
    rf.print_reg_file();
    rf.execution_stats();
    return 0;
}
