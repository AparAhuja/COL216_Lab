#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

using namespace std;

string file_path = "/Users/aparahuja/Desktop/IITD/COL216 - Arch/COL216_Lab/A3/A3/input.txt";

int PC = 0;
int PARTITION = 0;
int memory[1048576];

string decimalToHexadecimal(long long a) {
    
    string ans = "";
    map<int, string> hex{ {0,"0"},{1,"1"},{2,"2"},{3,"3"},{4,"4"},{5,"5"},{6,"6"},{7,"7"},{8,"8"},{9,"9"},{10,"a"},{11,"b"},{12,"c"},{13,"d"},{14,"e"},{15,"f"} };

    if(a < 0) { a += 4294967296; }

    for(int i = 0; i < 8; i ++) {
        int dig = a % 16;
        a /= 16;
        ans = hex[dig] + ans;
    }

    return ans;
}

bool isInteger(string s) {

    int len = s.length();

    if (len == 0) return false;
    for (int i = 0; i < len; i++) {
        if (s.at(i) < '0' || s.at(i) > '9') { return false; }
    }

    return true;
}


struct REGI {
    //INSTRUCTION COUNT
    //int ins_cnt_add, ins_cnt_sub...
    // check lower case and upper case

    map<string, int> ins_map = { {"add", 0}, {"sub", 1}, {"mul", 2},{"beq", 3},{"bne", 4},{"slt", 5},{"j", 6},{"lw", 7},{"sw", 8},{"addi", 9} };

    map<string, int> reg_map = { {"$zero", 0}, {"$at", 1}, {"$v0", 2}, {"$v1", 3}, {"$a0", 4}, {"$a1", 5}, {"$a2",6}, {"$a3", 7}, {"$t0", 8}, {"$t1", 9}, {"$t2", 10}, 
                                 {"$t3", 11}, {"$t4", 12}, {"$t5", 13}, {"$t6", 14}, {"$t7", 15}, {"$s0", 16}, {"$s1", 17}, {"$s2", 18}, {"$s3", 19}, {"$s4", 20}, {"$s5", 21}, 
                                 {"$s6", 22}, {"$s7", 23}, {"$t8", 24}, {"$t9", 25}, {"$k0", 26}, {"$k1", 27}, {"$gp", 28}, {"$ap", 29}, {"$fp", 30}, {"$ra", 31} };
    int ins_cnt[10], reg[32];
    int cycle_cnt = 0;

    // constructor for initialization
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
            cout << (i->first) << ": " << decimalToHexadecimal(reg[(i->second)]);
            if (j % 5 == 0 || j == 32) cout << "\n";
            else
                cout << ", ";
            j++;
        }
    }
    
    void execution_stats() {

        cout << "Total clock cycles: " << cycle_cnt << "\n\n";
        cout << "Number of instructions executed for each type are given below:-\n"
        for (auto i = ins_map.begin(); i != ins_map.end(); i++) {
            cout << (i->first) << ": " << ins_cnt[(i->second)] << "\n";
        }
    }

    int labelToAddr(string label) {

        // if label is integer return else hash
        if (isInteger(label))
            return stoi(label);
        else
            return reg_map[label];
    }

    bool add(int dest, int src1, int src2) { 
        long long sum = reg[src1] + reg[src2];
        
        if (sum > 2147483647 || sum < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            return false;
        }
        else {
            reg[dest] = sum;
            stat_update(0);
            return true;
        }
    }

    bool sub(int dest, int src1, int src2) { 
        long long diff = reg[src1] - reg[src2];

        if (diff > 2147483647 || diff < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            return false;
        }
        else {
            reg[dest] = diff;
            stat_update(1);
            return true;
        }
    }

    bool mul(int dest, int src1, int src2) { 
        long long prod = reg[src1] * reg[src2];

        if (prod > 2147483647 || prod < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            return false;
        }
        else {
            reg[dest] = prod;
            stat_update(2);
            return true;
        }
    }

    void beq(int src1, int src2, int jumpby) {
        if (src1 == src2) {
            PC += 4 + 4 * jumpby;
        }
        else PC += 4;
        stat_update(3);
    }

    void bne(int src1, int src2, int jumpby) {
        if (src1 != src2) {
            PC += 4 + 4 * jumpby;
        }
        else PC += 4;
        stat_update(4);
    }

    void slt(int dest, int src1, int src2) { 
        reg[dest] = (reg[src1] < reg[src2]) ? 1 : 0;
        stat_update(5);
    }

    void j(int jumpto) {
        PC =  4 * jumpto;
        stat_update(6);
    }

    void lw(int dest, int offset, int src) { 
        // check if address is valid
        reg[dest] = memory[src + 4*offset];
        stat_update(7);
    }

    void sw(int src, int offset, int dest) { 
        // check if address is valid
        memory[dest + 4*offset] = reg[src];
        stat_update(8);
    }

    bool addi(int dest, int src, int adds) { 
        long long sum = reg[src] + adds;

        if (sum > 2147483647 || sum < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            return false;
        }
        else {
            reg[dest] = sum;
            stat_update(9);
            return true;
        }
    }

    void stat_update(int ins_code) { 
        cycle_cnt++; 
        ins_cnt[ins_code]++;
    }
};


bool simulator(REGI rf) {

    int ins_code;
    bool flag;

    while (PC < PARTITION) {
        //{"add", 0}, { "sub", 1 }, { "mul", 2 }, { "beq", 3 }, { "bne", 4 }, { "slt", 5 }, { "j", 6 }, { "lw", 7 }, { "sw", 8 }, { "addi", 9 }
        ins_code = memory[PC];
        switch (ins_code) {
            
            case 0: 
                flag = rf.add(memory[PC + 1], memory[PC + 2], memory[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;

            case 1: 
                flag = rf.sub(memory[PC + 1], memory[PC + 2], memory[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;

            case 2: 
                flag = rf.mul(memory[PC + 1], memory[PC + 2], memory[PC + 3]); 
                if (flag) PC += 4;
                else return flag;
                break;

            case 3: 
                rf.beq(memory[PC + 1], memory[PC + 2], memory[PC + 3]); 
                break;

            case 4: 
                rf.bne(memory[PC + 1], memory[PC + 2], memory[PC + 3]); 
                break;

            case 5: 
                rf.slt(memory[PC + 1], memory[PC + 2], memory[PC + 3]); 
                PC += 4;
                break;

            case 6: 
                rf.j(memory[PC + 1]); 
                break;

            case 7: 
                rf.lw(memory[PC + 1], memory[PC + 2], memory[PC + 3]); 
                PC += 4;
                break;

            case 8: 
                rf.sw(memory[PC + 1], memory[PC + 2], memory[PC + 3]); 
                PC += 4;
                break;

            case 9: 
                flag = rf.addi(memory[PC + 1], memory[PC + 2], memory[PC + 3]); 
                if (flag) PC += 4;
                else return flag;
                break;
        }

        rf.print_reg_file():
    }
    
    // PC >= PARTITION, so execution is complete
    return true;
}

pair<vector<string>, bool> parseInstruction(string instruction) {
    
    vector<string> args;
    int len = instruction.length();
    int arg_count = 0, i = 0;
    bool isInstruction = false;
    string code = "", arg1 = "", arg2 = "", arg3 = "";
    
    while (i < len && instruction.at(i) = ' ') i++;
    if (i == len)
        return make_pair(args, true);
    else {
        while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',') {
            code = code + instruction[i];
            i++;
        }
        if (code.length() == 0) {
            return make_pair(args, false);
        }
        else {
            args.push_back(code);
            while (i < len && instruction.at(i) = ' ') i++;
            if (i == len)
                return make_pair(args, false);
            else {
                arg_count++;
                while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',') {
                    arg1 = arg1 + instruction[i];
                    i++;
                }
                if (arg1.length() == 0)
                    return make_pair(args, false);
                else {
                    args.push_back(arg1);
                    while (i < len && instruction.at(i) = ' ') i++;
                    if (i == len)
                        return make_pair(args, true);
                    else {
                        if (instruction.at(i) = ',') i++;
                        arg_count++;
                        while (i < len && instruction.at(i) = ' ') i++;
                        if (i == len)
                            return make_pair(args, false);
                        while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',' && instruction.at(i) != '(') {
                            arg2 = arg2 + instruction[i];
                            i++;
                        }
                        if (arg2.length() == 0)
                            return make_pair(args, false);
                        else {
                            args.push_back(arg2);
                            while (i < len && instruction.at(i) = ' ') i++;
                            if (i == len)
                                return make_pair(args, true);
                            else {
                                if (instruction.at(i) = ',') {
                                    i++;
                                    arg_count++;
                                    while (i < len && instruction.at(i) = ' ') i++;
                                    if (i == len)
                                        return make_pair(args, false);
                                    while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',') {
                                        arg3 = arg3 + instruction[i];
                                        i++;
                                    }
                                    if (arg3.length() == 0)
                                        return make_pair(args, false);
                                    else {
                                        args.push_back(arg3);
                                        while (i < len && instruction.at(i) = ' ') i++;
                                        if (i == len)
                                            return make_pair(args, true);
                                        else return make_pair(args, false);
                                    }
                                }
                                else {
                                    if (instruction.at(i) == '(') {
                                        i++;
                                        arg_count++;
                                        while (i < len && instruction.at(i) = ' ') i++;
                                        if (i == len)
                                            return make_pair(args, false);
                                        while (i < len && instruction.at(i) != ')') {
                                            arg3 = arg3 + instruction[i];
                                            i++;
                                        }
                                        if (arg3.length() == 0)
                                            return make_pair(args, false);
                                        else {
                                            args.push_back(arg3);
                                            while (i < len && instruction.at(i) = ' ') i++;
                                            if (i == len)
                                                return make_pair(args, true);
                                            else return make_pair(args, false);
                                        }
                                    }
                                    else {
                                        while (i < len && instruction.at(i) != ' ') {
                                            arg3 = arg3 + instruction[i];
                                            i++;
                                        }
                                        if (arg3.length() == 0)
                                            return make_pair(args, false);
                                        else {
                                            args.push_back(arg3);
                                            while (i < len && instruction.at(i) = ' ') i++;
                                            if (i == len)
                                                return make_pair(args, true);
                                            else return make_pair(args, false);
                                        }
                                    }
                                }
                            }
                        }

                    }

                }
            }
        }
    }
}

//add line to memory
bool addToMemory(string line, REGI rf, int line_number) {
    
    vector<string> args;
    bool flag;
    long i = 0, len = line.length();
    
    /*while(true){

        string x = "";
        while(i < len && (line.at(i) == ' ' || line.at(i) == ',' || line.at(i) == '(')) { i++; }
        while(i < len && line.at(i) != ' ' && line.at(i) != ',' && line.at(i) != ')' && line.at(i) != '(') { x += line[i]; i++; }
        while(i < len && (line.at(i) == ' ' || line.at(i) == ',' || line.at(i) == ')')) { i++; }

        if(x.length() != 0) args.push_back(x);
        if(i == len){break;}

    }*/
    pair<vector<string>, bool> Pair = parseInstruction(line);
    args = Pair.first;
    flag = Pair.second;
    if (!flag) {
        cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
        return false;
    }
    else {
        if (args.size() == 0)
            return true;
        else {
            
            if (len == 0) { return true; }

            if (rf.ins_map.find(args[0]) == rf.ins_map.end()) { cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n"; return false; }
            int ins = rf.ins_map[args[0]];
            memory[PC] = ins;
            len = args.size();
            
            //jump
            if (ins == 6) {
                if (len != 2) {
                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    if (!isInteger(args[1])) { cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n"; return false; }
                    else {
                        int addr = stoi(args[1]);
                        if (addr >= PARTITION / 4 || addr < 0) { cout << "Invalid Address ERROR: At line number: " << line_number << ": " << "\n"; return false; }
                        memory[PC + 1] = stoi(args[1]);
                    }
                }
            }

            //add sub mul slt
            else if (ins == 0 || ins == 1 || ins == 2 || ins == 5) {
                if (len != 4) {
                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    if (rf.reg_map.find(args[1]) == rf.reg_map.end() || rf.reg_map.find(args[2]) == rf.reg_map.end() || rf.reg_map.find(args[3]) == rf.reg_map.end()) {
                        cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                        return false;
                    }
                    else {
                        memory[PC + 1] = rf.reg_map[args[1]];
                        memory[PC + 2] = rf.reg_map[args[2]];
                        memory[PC + 3] = rf.reg_map[args[3]];
                    }
                }
            }
            //beq bne addi
            else if (ins == 3 || ins == 4 || ins == 9) {
                if (len != 4) {
                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                }
                else {
                    if (rf.reg_map.find(args[1]) == rf.reg_map.end() || rf.reg_map.find(args[2]) == rf.reg_map.end() || !isInteger(args[3])) {
                        cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                    }
                    else {
                        memory[pc + 1] = rf.reg_map[args[1]];
                        memory[pc + 2] = rf.reg_map[args[2]];
                        memory[pc + 3] = stoi(args[3]);
                    }
                }
            }
            //sw lw
            else {
                if (len != 4) {
                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                }
                else {
                    if (rf.reg_map.find(args[1]) == rf.reg_map.end() || !isInteger(args[2]) || rf.reg_map.find(args[3]) == rf.reg_map.end()) {
                        cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                    }
                    else {
                        memory[pc + 1] = rf.reg_map[args[1]];
                        memory[pc + 2] = stoi(args[2]);
                        memory[pc + 3] = rf.reg_map[args[3]];
                    }
                }
            }
        }
    }

    PC += 4;
}


int main(int argc, const char* argv[]) {
    
    ifstream infile(file_path);

    //File error
    if (!infile.is_open()) {
        cout << "ERROR: Unable to open file. Program terminating!\n";
        return 0;
    }

    REGI rf;
    string line;
    int line_number = 1;
    bool flag;

    //read line by line
    while (getline(infile, line)) {

        //process and store each instruction in memory
        flag = addToMemory(line, rf, line_number);
        if (!flag) {
            cout << "Program terminating!\n";
            return 0;
        }
        line_number++;
    }

    PARTITION = PC;
    PC = 0;

    simulator(rf);

    infile.close();
    
    //for (int i = 1; i <= 12; i++) { cout << memory[i - 1] << " ";if (i % 4 == 0) { cout << "\n"; } }
    //cout<<decimalToHexadecimal(-2);
    //rf.print_reg_file();
    //rf.execution_stats();
    //addToMemory("   add  $t1,  $t2 ( 345t )   ", rf);
    return 0;
}
