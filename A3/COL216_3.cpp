#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

string file_path = "sample.txt";

// program counter
int PC = 0;
// partition between instructions and data
int PARTITION = 0;
int DATA_START = 699052;
// memory available
int memory[1048576];


// function that converts a decimal number to its corresponding hexadecimal format
string decimalToHexadecimal(long long a) {

    string ans = "";
    // int to hex map
    map<int, string> hex{ {0,"0"},{1,"1"},{2,"2"},{3,"3"},{4,"4"},{5,"5"},{6,"6"},{7,"7"},{8,"8"},{9,"9"},{10,"a"},{11,"b"},{12,"c"},{13,"d"},{14,"e"},{15,"f"} };

    // 2's complement representation
    if (a < 0) { a += 4294967296; }

    for (int i = 0; i < 8; i++) {
        int dig = a % 16;
        a /= 16;
        ans = hex[dig] + ans;
    }

    // return hexadecimal equivalent
    return ans;
}


// function to check if the string represents an integer or not
bool isInteger(string s) {
    int len = s.length();

    if (len == 0) return false;

    for (int i = 0; i < len; i++) {
        if (i == 0 && s.at(0) == '-') continue;
        if (s.at(i) < '0' || s.at(i) > '9') { return false; }
    }
    return true;
}


struct REGI {
    
    //INSTRUCTION COUNT
    //int ins_cnt_add, ins_cnt_sub...
    // check lower case and upper case

    // instruction code map
    map<string, int> ins_map = { {"add", 0}, {"sub", 1}, {"mul", 2},{"beq", 3},{"bne", 4},{"slt", 5},{"j", 6},{"lw", 7},{"sw", 8},{"addi", 9} };

    // register number map
    map<string, int> reg_map = { {"$zero", 0}, {"$at", 1}, {"$v0", 2}, {"$v1", 3}, {"$a0", 4}, {"$a1", 5}, {"$a2",6}, {"$a3", 7}, {"$t0", 8}, {"$t1", 9}, {"$t2", 10},
                                 {"$t3", 11}, {"$t4", 12}, {"$t5", 13}, {"$t6", 14}, {"$t7", 15}, {"$s0", 16}, {"$s1", 17}, {"$s2", 18}, {"$s3", 19}, {"$s4", 20}, {"$s5", 21},
                                 {"$s6", 22}, {"$s7", 23}, {"$t8", 24}, {"$t9", 25}, {"$k0", 26}, {"$k1", 27}, {"$gp", 28}, {"$sp", 29}, {"$fp", 30}, {"$ra", 31} };
    
    map<string, int> labels;

    vector<pair<string, pair<int, int>>> label;

    // register parameters and clock cycle count
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

        // print register contents
        cout << "Register file contents after " << cycle_cnt << " clock cycles:\n\n";
        
        cout << "zero: " << decimalToHexadecimal(reg[0]) << " " << "at: " << decimalToHexadecimal(reg[1]) << " " << "v0: " << decimalToHexadecimal(reg[2]) << " " << "v1: " << decimalToHexadecimal(reg[3]) << " " << "a0: " << decimalToHexadecimal(reg[4]) << "\n";
        cout << "a1: " << decimalToHexadecimal(reg[5]) << " " << "a2: " << decimalToHexadecimal(reg[6]) << " " << "a3: " << decimalToHexadecimal(reg[7]) << " " << "t0: " << decimalToHexadecimal(reg[8]) << " " << "t1: " << decimalToHexadecimal(reg[9]) << "\n";
        cout << "t2: " << decimalToHexadecimal(reg[10]) << " " << "t3: " << decimalToHexadecimal(reg[11]) << " " << "t4: " << decimalToHexadecimal(reg[12]) << " " << "t5: " << decimalToHexadecimal(reg[13]) << " " << "t6: " << decimalToHexadecimal(reg[14]) << "\n";
        cout << "t7: " << decimalToHexadecimal(reg[15]) << " " << "s0: " << decimalToHexadecimal(reg[16]) << " " << "s1: " << decimalToHexadecimal(reg[17]) << " " << "s2: " << decimalToHexadecimal(reg[18]) << " " << "s3: " << decimalToHexadecimal(reg[19]) << "\n";
        cout << "s4: " << decimalToHexadecimal(reg[20]) << " " << "s5: " << decimalToHexadecimal(reg[21]) << " " << "s6: " << decimalToHexadecimal(reg[22]) << " " << "s7: " << decimalToHexadecimal(reg[23]) << " " << "t8: " << decimalToHexadecimal(reg[24]) << "\n";
        cout << "t9: " << decimalToHexadecimal(reg[25]) << " " << "k0: " << decimalToHexadecimal(reg[26]) << " " << "k1: " << decimalToHexadecimal(reg[27]) << " " << "gp: " << decimalToHexadecimal(reg[28]) << " " << "sp: " << decimalToHexadecimal(reg[29]) << "\n";
        cout << "fp: " << decimalToHexadecimal(reg[30]) << " " << "ra: " << decimalToHexadecimal(reg[31]) << "\n\n\n";
    }

    void execution_stats() {

        // print execution statistics
        cout << "Total clock cycles: " << cycle_cnt << "\n\n";
        cout << "Number of instructions executed for each type are given below:-\n";
        int j = 1;
        for (auto i = ins_map.begin(); i != ins_map.end(); i++) {
            cout << (i->first) << ": " << ins_cnt[(i->second)];
            if (j % 5 == 0) cout << "\n";
            else cout << ", ";
            j++;
        }
    }

    
    //int labelToAddr(string label) {

    //    // if label is integer return else hash
    //    if (isInteger(label))
    //        return stoi(label);
    //    else
    //        return reg_map[label];
    //}

    bool add(int dest, int src1, int src2) {
        long long sum = (long long)reg[src1] + (long long)reg[src2];

        if (sum > 2147483647 || sum < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = sum;
            if (dest == 0)
                reg[dest] = 0;
            stat_update(0);
            return true;
        }
    }

    bool sub(int dest, int src1, int src2) {
        long long diff = (long long)reg[src1] - (long long)reg[src2];

        if (diff > 2147483647 || diff < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = diff;
            if (dest == 0)
                reg[dest] = 0;
            stat_update(1);
            return true;
        }
    }

    bool mul(int dest, int src1, int src2) {
        long long prod = (long long)reg[src1] * (long long)reg[src2];

        if (prod > 2147483647 || prod < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = prod;
            if (dest == 0)
                reg[dest] = 0;
            stat_update(2);
            return true;
        }
    }

    bool beq(int src1, int src2, int jumpto) {
        if (reg[src1] == reg[src2]) {
            PC = jumpto;
            if (PC > PARTITION || PC < 0) {
                cout << "Warning: Program jumped to a non-instruction memory location. Program terminated abruptly!\n\n";
                stat_update(3);
                execution_stats();
                return false;
            }
        }
        else PC += 4;
        stat_update(3);
        return true;
    }

    bool bne(int src1, int src2, int jumpto) {
        if (reg[src1] != reg[src2]) {
            PC = jumpto;
            if (PC > PARTITION || PC < 0) {
                cout << "Warning: Program jumped to a non-instruction memory location. Program terminated abruptly!\n\n";
                stat_update(4);
                execution_stats();
                return false;
            }
        }
        else PC += 4;
        stat_update(4);
        return true;
    }

    void slt(int dest, int src1, int src2) {
        reg[dest] = (reg[src1] < reg[src2]) ? 1 : 0;
        if (dest == 0)
            reg[dest] = 0;
        stat_update(5);
    }

    bool j(int jumpto) {
        PC = jumpto;
        stat_update(6);
        if (PC > PARTITION) {
            cout << "Warning: Program jumped to a non-instruction memory location. Program terminated abruptly!\n\n";
            execution_stats();
            return false;
        }
        else
            return true;
    }

    bool lw(int dest, int offset, int src) {
        // check if address is valid
        int addr = offset + reg[src] + DATA_START;
        if (addr >= 1048576 || addr < DATA_START) {
            // invalid address, error
            cout << "Error: Program is trying to access an unavailable memory location. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            if (addr % 4 != 0) {
                // address is not word aligned, error
                cout << "Error: Memory address is not word-aligned. Invalid load operation. Program Terminating!\n\n";
                execution_stats();
                return false;
            }
        }
        // else
        reg[dest] = memory[addr];
        if (dest == 0)
            reg[dest] = 0;
        stat_update(7);
        return true;
    }

    bool sw(int src, int offset, int dest) {
        // check if address is valid
        int addr = offset + reg[dest] + DATA_START;
        if (addr < DATA_START || addr >= 1048576) {
            // invalid address, error
            cout << "Error: Program is trying to access an unavailable memory location. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            if (addr % 4 != 0) {
                // address is not word aligned, error
                cout << "Error: Memory address is not word-aligned. Invalid store operation. Program Terminating!\n\n";
                execution_stats();
                return false;
            }
        }
        // else
        memory[addr] = reg[src];
        stat_update(8);
        return true;
    }

    bool addi(int dest, int src, int adds) {
        long long sum = (long long)reg[src] + (long long)adds;

        if (sum > 2147483647 || sum < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = sum;
            if (dest == 0)
                reg[dest] = 0;
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
                flag = rf.beq(memory[PC + 1], memory[PC + 2], memory[PC + 3]);
                if (!flag) return flag;
                break;

            case 4:
                flag = rf.bne(memory[PC + 1], memory[PC + 2], memory[PC + 3]);
                if (!flag) return flag;
                break;

            case 5:
                rf.slt(memory[PC + 1], memory[PC + 2], memory[PC + 3]);
                PC += 4;
                break;

            case 6:
                flag = rf.j(memory[PC + 1]);
                if (!flag) return flag;
                break;

            case 7:
                flag = rf.lw(memory[PC + 1], memory[PC + 2], memory[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;

            case 8:
                flag = rf.sw(memory[PC + 1], memory[PC + 2], memory[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;
                
            case 9:
                flag = rf.addi(memory[PC + 1], memory[PC + 2], memory[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;
        }

        rf.print_reg_file();
    }

    // PC = PARTITION, so execution is complete
    rf.execution_stats();
    return true;
}

pair<vector<string>, bool> parseInstruction(string instruction) {

    // args stores instruction arguments
    vector<string> args;
    // len stores length of the instruction
    int len = instruction.length();
    int arg_count = 0;
    // instruction counter
    int i = 0;
    bool isInstruction = false;
    // maximum of 4 arguments can be present in an instruction
    string code = "", arg1 = "", arg2 = "", arg3 = "";
  
    // read all initial white spaces
    while (i < len && instruction.at(i) == ' ') i++;
    // return if there are only white spaces with true (no error)
    if (i == len)
        return make_pair(args, true);
    // else
    else {
        // read all non-whitespace, non-comma characters, store them in code
        while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',') {
            code = code + instruction[i];
            i++;
        }
        // return error if code is still empty
        if (code.length() == 0) {
            return make_pair(args, false);
        }
        // else
        else {
            // push back the first argument
            args.push_back(code);
            // read all immediate white spaces
            while (i < len && instruction.at(i) == ' ') i++;
            // if there are no more arguments, return error
            if (i == len)
                return make_pair(args, false);
            // else
            else {
                // read all non-whitespace, non-comma characters, store them in arg1                 
                while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',') {
                    arg1 = arg1 + instruction[i];
                    i++;
                }
                // return error if arg1 is still empty
                if (arg1.length() == 0)
                    return make_pair(args, false);
                // else
                else {
                    // push back the second argument
                    args.push_back(arg1);
                    // read all immediate white spaces
                    while (i < len && instruction.at(i) == ' ') i++;
                    // if there are no more arguments, then it can be jump instruction, so return true
                    if (i == len)
                        return make_pair(args, true);
                    // else
                    else {
                        // read comma if any
                        if (instruction.at(i) == ',') i++;
                        // read all inter-mediary white spaces
                        while (i < len && instruction.at(i) == ' ') i++;
                        // if there are no more arguments, return error
                        if (i == len)
                            return make_pair(args, false);
                        // else
                        // read all non-white space, non-comma, non-LPAREN characters, store them in arg2
                        while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',' && instruction.at(i) != '(') {
                            arg2 = arg2 + instruction[i];
                            i++;
                        }
                        // return error if arg2 is still empty
                        if (arg2.length() == 0)
                            return make_pair(args, false);
                        // else
                        else {
                            // push back the third argument
                            args.push_back(arg2);
                            // read immediate white spaces
                            while (i < len && instruction.at(i) == ' ') i++;
                            // if there are no more arguments, return error 
                            if (i == len)
                                return make_pair(args, false);
                            // else
                            else {
                                // read comma, if any
                                if (instruction.at(i) == ',') {
                                    i++;
                                    // read inter-mediary white spaces
                                    while (i < len && instruction.at(i) == ' ') i++;
                                    // if there are no more arguments, return error
                                    if (i == len)
                                        return make_pair(args, false);
                                    // read all non-whitespace, non-comma characters, store them in arg3
                                    while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',') {
                                        arg3 = arg3 + instruction[i];
                                        i++;
                                    }
                                    // return error if arg3 is still empty
                                    if (arg3.length() == 0)
                                        return make_pair(args, false);
                                    // else
                                    else {
                                        // push back the fourth argument
                                        args.push_back(arg3);
                                        // read trailing white spaces
                                        while (i < len && instruction.at(i) == ' ') i++;
                                        // if instruction has ended, then return true
                                        if (i == len)
                                            return make_pair(args, true);
                                        // else, it is a non-instruction
                                        else return make_pair(args, false);
                                    }
                                }
                                else {
                                    // if there was a LPAREN (basically lw/sw commands)
                                    if (instruction.at(i) == '(') {
                                        i++;
                                        // read any inter-mediary white spaces
                                        while (i < len && instruction.at(i) == ' ') i++;
                                        // if there are no more arguments, return error
                                        if (i == len)
                                            return make_pair(args, false);
                                        // else
                                        // read all non-whitespace, non-RPAREN characters, store them in arg3
                                        while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ')') {
                                            arg3 = arg3 + instruction[i];
                                            i++;
                                        }
                                        // return error if arg3 is still empty
                                        if (arg3.length() == 0)
                                            return make_pair(args, false);
                                        // else
                                        else {
                                            // push back the fourth argument
                                            args.push_back(arg3);
                                            // read any trailing white spaces
                                            while (i < len && instruction.at(i) == ' ') i++;
                                            // if the next character is not a RPAREN, return error
                                            if (instruction.at(i) != ')')
                                                return make_pair(args, false);
                                            // else
                                            // read any trailing white spaces
                                            while (i < len && instruction.at(i) == ' ') i++;
                                            // if the instruction is completely parsed, return true, else return false
                                            if (i == len)
                                                return make_pair(args, true);
                                            else return make_pair(args, false);
                                        }
                                    }
                                    // neither LPAREN, nor a comma
                                    else {
                                        // read all non-whitespace characters, store them in arg3
                                        while (i < len && instruction.at(i) != ' ') {
                                            arg3 = arg3 + instruction[i];
                                            i++;
                                        }
                                        // return error if arg3 is still empty
                                        if (arg3.length() == 0)
                                            return make_pair(args, false);
                                        // else
                                        else {
                                            // push back the fourth argument
                                            args.push_back(arg3);
                                            // read trailing white spaces, if any
                                            while (i < len && instruction.at(i) == ' ') {
                                                i++;
                                            }
                                            // if the instruction is completely passed, return true, else return false
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


bool validId(string label) {
    int len = label.length(), i = 0;
    if (len == 0) return false;
    else {
        if (isalpha(label.at(0)) || label.at(0) == '_') {
            i++;
            while (i < len && (isalpha(label.at(i)) || isdigit(label.at(i)) || label.at(i) == ' ')) i++;
            if (i != len)
                return false;
            else return true;
        }
        else return false;
    }
}

pair<pair<string, string>, pair<bool, bool>> checkIfLabel(string line, int line_number) {
    int len = line.length(), i = 0;
    string label = "";
    // parse initial white spaces
    while (i < len && line.at(i) == ' ') i++;
    if (i == len)
        // no label
        return make_pair(make_pair(label, line), make_pair(false, true));
    // parse label
    while (i < len && line.at(i) != ' ') {
        label = label + line[i];
        i++;
    }
    //parse any extra white spaces
    while (i < len && line.at(i) == ' ') i++;
    if (i == len)
        // no label
        return make_pair(make_pair(label, line), make_pair(false, true));
    // else
    // check if there is a colon
    if (line.at(i) == ':') {
        i++;
        // can be a label
        // check if it is a valid identifier
        bool flag = validId(label);
        if (!flag) {
            // syntax error
            cout << "SYNTAX ERROR (Illegal Label): At line number: " << line_number << ": " << line << "\n";
            return make_pair(make_pair(label, line), make_pair(false, false));
        }
        else {
            // valid label
            // make remaining string
            string line_r = "";
            while (i < len) {
                line_r = line_r + line[i];
                i++;
            }
            // return
            return make_pair(make_pair(label, line_r), make_pair(true, true));
        }
    }
    // else it is not a label
    return make_pair(make_pair(label, line), make_pair(false, true));

}

//add line to memory
bool addToMemory(string line, REGI rf, int line_number) {

    // args will be used to store the instruction arguments
    vector<string> args;
    bool flag;
    int len;
    pair<pair<string, string>, pair<bool, bool>> label = checkIfLabel(line, line_number);
    if (label.second.second == false)
        return false;
    else {
        if (label.second.first == true) {
            // check if the label is already there
            if (rf.labels.find(label.first.first) != rf.labels.end()) {
                // syntax error
                cout << "Error: Same label used to represent different addresses.\n Label repeated: " << label.first.first << ": at line number: " << line_number << "\n";
                return false;
            }
            else {
                // add label to map
                rf.labels.insert({ label.first.first, PC });
            }
        }
    }
    pair<vector<string>, bool> Pair = parseInstruction(label.first.second);
    // store arguments
    args = Pair.first;
    // store whether or not there was a syntax error
    flag = Pair.second;

    if (!flag) {
        // there was a syntax error
        cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
        return false;
    }
    else {
        // check if there are no arguments
        if (args.size() == 0)
            return true;
        else {
            
            // ignore the case of instruction, convert it into lower case
            transform(args[0].begin(), args[0].end(), args[0].begin(), ::tolower);
            
            // check if the instruction is valid or not
            if (rf.ins_map.find(args[0]) == rf.ins_map.end()) { 
                // invalid instruction
                cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n"; 
                return false;
            }
            // else store the instruction in memory
            int ins = rf.ins_map[args[0]];
            memory[PC] = ins;
            // store the number of arguments in len
            len = args.size();

            //jump
            if (ins == 6) {
                if (len != 2) {
                    // as jump should have only 2 arguments
                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    if (!isInteger(args[1])) { 
                        if (!validId(args[1])) {
                            // not a valid label address
                            cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        // else, it is a valid id, push it
                        rf.label.push_back(make_pair(args[1], make_pair(PC + 1, line_number)));
                    }
                    else {
                        int addr = stoi(args[1]);
                        if (addr < 0 || addr >= 67108864) { 
                            // address starts from 0 and max integer that can be stored is 2^26 - 1
                            cout << "Invalid Address ERROR: At line number: " << line_number << ": " << "\n"; 
                            return false;
                        }
                        // else store the value in memory
                        memory[PC + 1] = 4 * addr;
                    }
                }
            }

            //add sub mul slt
            else if (ins == 0 || ins == 1 || ins == 2 || ins == 5) {
                if (len != 4) {
                    // add sub mul slt require 4 arguments each
                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    // check if any of the register used is invalid
                    if (rf.reg_map.find(args[1]) == rf.reg_map.end() || rf.reg_map.find(args[2]) == rf.reg_map.end() || rf.reg_map.find(args[3]) == rf.reg_map.end()) {
                        // invalid register used, syntax error
                        cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                        return false;
                    }
                    else {
                        // store register numbers in memory
                        memory[PC + 1] = rf.reg_map[args[1]];
                        memory[PC + 2] = rf.reg_map[args[2]];
                        memory[PC + 3] = rf.reg_map[args[3]];
                    }
                }
            }

            //beq bne addi
            else if (ins == 3 || ins == 4 || ins == 9) {
                if (len != 4) {
                    // beq bne addi require 4 arguments each
                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    if (ins == 9) {
                        // check if any of the register used is invalid
                        // also check if the last argument is an immediate or not
                        if (rf.reg_map.find(args[1]) == rf.reg_map.end() || rf.reg_map.find(args[2]) == rf.reg_map.end() || !isInteger(args[3])) {
                            // invalid arguments, syntax error
                            cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        else {
                            int immediate = stoi(args[3]);

                            // raise overflow error if necessary
                            if (immediate < -32768 || immediate > 32767) {
                                cout << "Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                                return false;
                            }
                            // else
                            // store register numbers and immediate in memory
                            memory[PC + 1] = rf.reg_map[args[1]];
                            memory[PC + 2] = rf.reg_map[args[2]];
                            memory[PC + 3] = immediate;
                        }
                    }
                    else {
                        // bne or beq
                        // check if any of the register used is invalid
                        if (rf.reg_map.find(args[1]) == rf.reg_map.end() || rf.reg_map.find(args[2]) == rf.reg_map.end()) {
                            // invalid arguments, syntax error
                            cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        else {
                            // store register
                            memory[PC + 1] = rf.reg_map[args[1]];
                            memory[PC + 2] = rf.reg_map[args[2]];
                            if (isInteger(args[3])) {
                                int immediate = stoi(args[3]);

                                // raise overflow error if necessary
                                if (immediate < -32768 || immediate > 32767) {
                                    cout << "Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                                    return false;
                                }
                                // else
                                // store immediate in memory
                                memory[PC + 3] = PC + 4 + 4 * immediate;
                            }
                            else {
                                if (validId(args[3])) {
                                    rf.label.push_back(make_pair(args[3], make_pair(PC + 3, line_number)));
                                }
                                else {
                                    // invalid label, raise error
                                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                                    return false;
                                }
                            }
                        }
                    }
                }
            }

            //sw lw
            else {
                if (len != 4) {
                    // sw lw require 4 arguments each
                    cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    // check if the registers are valid or not, also check if the second argument is an immediate or not
                    if (rf.reg_map.find(args[1]) == rf.reg_map.end() || !isInteger(args[2]) || rf.reg_map.find(args[3]) == rf.reg_map.end()) {
                        // invalid arguments, syntax error
                        cout << "SYNTAX ERROR: At line number: " << line_number << ": " << line << "\n";
                        return false;
                    }
                    else {
                        int immediate = stoi(args[2]);

                        // raise overflow error if necessary
                        if (immediate < -32768 || immediate > 32767) {
                            cout << "Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                            return false;
                        }
                        // store register numbers and immediate in memory
                        memory[PC + 1] = rf.reg_map[args[1]];
                        memory[PC + 2] = immediate;
                        memory[PC + 3] = rf.reg_map[args[3]];
                    }
                }
            }
        }
    }
    // increment program counter by 4 (next storage address)
    PC += 4;
    // instruction stored successfully
    return true;
}

bool linker(REGI rf) {
    
    int len = rf.label.size();
    
    for (int i = 0; i < len; i++) {
    
        pair<string, pair<int, int>> label = rf.label[i];
        // check if label is actually defined
        if (rf.labels.find(label.first) == rf.labels.end()) {
            // illegal label, raise error
            cout << "Linking Error: Unable to resolve label used on line: " << label.second.second << "\n";
            return false;
        }
        else {
            // resolve label
            int label_loc = rf.labels[label.first];
            memory[label.second.first] = label_loc;
        }

    }
    return true;
}

int main(int argc, const char* argv[]) {

    ifstream infile(file_path);

    //File error
    if (!infile.is_open()) {
        cout << "ERROR: Unable to open file. Program terminating!\n";
        return 0;
    }

    // create register file
    REGI rf;

    // variable to store a line
    string line;

    // initializations
    int line_number = 1;
    bool flag;

    //read line by line
    while (getline(infile, line)) {

        //process and store each instruction in memory
        if (PC >= DATA_START) {
            cout << "Error: Instruction memory overflow. Program terminating!\n";
            return 0;
        }
        flag = addToMemory(line, rf, line_number);

        // check if there was some error
        if (!flag) {
            // terminate program
            cout << "Program terminating!\n";
            return 0;
        }
        // read next line
        line_number++;
    }
    
    // initialize PARTITION to PC 
    // PARTITION stores the starting address of data section
    PARTITION = PC;

    // link labels used in program
    flag = linker(rf);

    if (!flag)
        return 0;

    // re-initialize PC to 0 for execution
    PC = 0;

    // execute the program
    flag = simulator(rf);

    if (flag)
        cout << "Program executed successfully!\n\n";

    // close the input file stream
    infile.close();

    return 0;
}
