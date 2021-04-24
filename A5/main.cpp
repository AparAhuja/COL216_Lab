#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <queue>
#include <fstream>
#include <string>

using namespace std;

// program counter (starts at 0)
//->int PC = 0;
// instruction end address
//->int PARTITION = 0;

// starting address of data section (old value = 699052)
//->int DATA_START = 699392;

// ROW_ACCESS_DELAY and COL_ACCESS_DELAY
int ROW_ACCESS_DELAY, COL_ACCESS_DELAY;

// execution mode (non-block vs normal), false denotes normal and true denotes non-block
bool MODE = true;

// memory available (2 ^ 20 bytes), used to model 2D DRAM (row major)
int DRAM[1048576];

int no_of_files, simulation_time;

// vector to store memory locations which were used
vector<int> memoryAddress;


// function that converts a decimal number to its corresponding hexadecimal format
string decimalToHexadecimal(long long a) {

    string ans = "";
    // int to hex map
    map<int, string> hex{ {0,"0"},{1,"1"},{2,"2"},{3,"3"},{4,"4"},{5,"5"},{6,"6"},{7,"7"},{8,"8"},{9,"9"},{10,"a"},{11,"b"},{12,"c"},{13,"d"},{14,"e"},{15,"f"} };

    // 2's complement representation (add 2 ^ 32 to negative numbers)
    if (a < 0) { a += 4294967296; }

    for (int i = 0; i < 8; i++) {
        int dig = a % 16;
        a /= 16;
        ans = hex[dig] + ans;
    }

    // return hexadecimal equivalent
    return ans;
}

struct Request {
    int addr;
    int row;
    int data_bus;
    int destination;
    string type;
    string instruction;
    int PC;
    int procNum;
};


vector<Request> reqBuffer, manBuffer;

struct Queue {
    map<int, vector<int>> MemToAdj;
    map<string, pair<int, int>> Pread, Pwrite;
    map<int, string> reverseMap =
        { {0, "$zero"}, {1, "$at"}, {2, "$v0"}, {3, "$v1"}, {4, "$a0"}, {5, "$a1"}, {6, "$a2"}, {7, "$a3"}, {8, "$t0"}, {9, "$t1"}, {10, "$t2"},
        {11, "$t3"}, {12, "$t4"}, {13, "$t5"}, {14, "$t6"}, {15, "$t7"}, {16, "$s0"}, {17, "$s1"}, {18, "$s2"}, {19, "$s3"}, {20, "$s4"}, {21, "$s5"},
        {22, "$s6"}, {23, "$s7"}, {24, "$t8"}, {25, "$t9"}, {26, "$k0"}, {27, "$k1"}, {28, "$gp"}, {29, "$sp"}, {30, "$fp"}, {31, "$ra"} };
    vector<queue<Request>> Adj;
    int adjIndex = 0;
    
    bool isEmpty() {
        return (adjIndex == Adj.size());
    }

    int BinarySearch(int key, int low, int high, vector<int> &vec) {
        int mid;
        if(low > high)
            return vec.size();
        while(low < high) {
            mid = (low + high) / 2;
            if(vec[mid] < key) {
                low = mid + 1;
            }
            else {
                //assert: vec[mid] >= key
                high = mid;
            }
        }
        //assert: low = high
        if(vec[low] >= key) {
            //check vec[low]
            return low;
        }
        return vec.size();
    }

    void addRequest(Request req) {
        int loc;
        string memAddr;
        if(MemToAdj.find(req.row) == MemToAdj.end()) {
            loc = Adj.size();
            memAddr = to_string(req.addr);
            // Add to MemToAdj
            vector<int> vec;
            vec.push_back(1);
            vec.push_back(loc);
            MemToAdj.insert({req.row, vec});
            // Add to Adj, create a new queue
            queue<Request> que;
            que.push(req);
            Adj.push_back(que);
            if(req.type == "sw") {
                // Update pending write
                Pwrite.insert({memAddr, make_pair(loc, 1)});
            }
            else {
                // req.type == "lw"
                // Update pending read (memory)
                Pread.insert({memAddr, make_pair(loc, 1)});
                // Update pending write (register)
                int regCode = req.destination;
                string reg = reverseMap[regCode];
                if(Pwrite.find(reg) == Pwrite.end()) {
                    Pwrite.insert({reg, make_pair(loc, 1)});
                }
                else {
                    Pwrite[reg].first = loc;
                    Pwrite[reg].second = Pwrite[reg].second + 1;
                }
            }
        }
        else {
            // memory row is present
            if(req.type == "sw") {
                memAddr = to_string(req.addr);
                if(Pread.find(memAddr) == Pread.end()) {
                    if(Pwrite.find(memAddr) == Pwrite.end()) {
                        // no pending read and no pending write
                        // Add to first Adj location
                        int i = MemToAdj[req.row][0];
                        if(i < MemToAdj[req.row].size()) {
                            loc = MemToAdj[req.row][i];
                            Adj[loc].push(req);
                            // Add to Pwrite
                            Pwrite.insert({memAddr, make_pair(loc, 1)});
                        }
                        else {
                            loc = Adj.size();
                            MemToAdj[req.row].push_back(loc);
                            // Add to Adj
                            queue<Request> que;
                            que.push(req);
                            Adj.push_back(que);
                            // Add to Pwrite
                            Pwrite.insert({memAddr, make_pair(loc, 1)});
                        }
                    }
                    else {
                        // there is a pending write
                        loc = Pwrite[memAddr].first;
                        Adj[loc].push(req);
                        // Update Pwrite
                        Pwrite[memAddr].second = Pwrite[memAddr].second + 1;
                    }
                }
                else {
                    // there is a pending read
                    loc = Pread[memAddr].first;
                    Adj[loc].push(req);
                    // Update Pwrite
                    if(Pwrite.find(memAddr) != Pwrite.end()) {
                        Pwrite[memAddr].first = loc;
                        Pwrite[memAddr].second = Pwrite[memAddr].second + 1;
                    }
                    else {
                        Pwrite.insert({memAddr, make_pair(loc, 1)});
                    }    
                }
            }
            else {
                // req.type == "lw"
                memAddr = to_string(req.addr);
                string reg = reverseMap[req.destination];
                if(Pwrite.find(reg) == Pwrite.end()) {
                    // no pending write on register
                    if(Pwrite.find(memAddr) == Pwrite.end()) {
                        // no pending write on memory
                        int i = MemToAdj[req.row][0];
                        if(i < MemToAdj[req.row].size()) {
                            loc = MemToAdj[req.row][i];
                            Adj[loc].push(req);
                            if(Pread.find(memAddr) == Pread.end()) {
                                // Add to Pread
                                Pread.insert({memAddr, make_pair(loc, 1)});
                            }
                            else {
                                // Update Pread
                                Pread[memAddr].first = loc > Pread[memAddr].first ? loc : Pread[memAddr].first;
                                Pread[memAddr].second = Pread[memAddr].second + 1;
                            }
                            // Add register to Pwrite
                            Pwrite.insert({reg, make_pair(loc, 1)});
                        }
                        else {
                            loc = Adj.size();
                            MemToAdj[req.row].push_back(loc);
                            queue<Request> que;
                            que.push(req);
                            Adj.push_back(que);
                            if(Pread.find(memAddr) == Pread.end()) {
                                // Add to Pread
                                Pread.insert({memAddr, make_pair(loc, 1)});
                            }
                            else {
                                // Update Pread
                                Pread[memAddr].first = loc > Pread[memAddr].first ? loc : Pread[memAddr].first;
                                Pread[memAddr].second = Pread[memAddr].second + 1;
                            }
                            // Add register to Pwrite
                            Pwrite.insert({reg, make_pair(loc, 1)});
                        }
                    }
                    else {
                        // there is a pending write on memory
                        loc = Pwrite[memAddr].first;
                        Adj[loc].push(req);
                        if(Pread.find(memAddr) == Pread.end()) {
                            // Add to Pread
                            Pread.insert({memAddr, make_pair(loc, 1)});
                        }
                        else {
                            // Update Pread
                            if(Pread[memAddr].first < loc) {
                                Pread[memAddr].first = loc;
                            }
                            Pread[memAddr].second = Pread[memAddr].second + 1;
                        }
                        // Add register to Pwrite
                        Pwrite.insert({reg, make_pair(loc, 1)});
                    }
                }
                else {
                    // there is a pending write on register
                    int key = Pwrite[reg].first;
                    int low, high;
                    if(Pwrite.find(memAddr) != Pwrite.end()) {
                        if(key < Pwrite[memAddr].first) {
                            key = Pwrite[memAddr].first;
                        }
                    }
                    low = MemToAdj[req.row][0];
                    high = MemToAdj[req.row].size() - 1;
                    int i = BinarySearch(key, low, high, MemToAdj[req.row]);
                    if(i < low || i > high) {
                        // Add new queue
                        loc = Adj.size();
                        queue<Request> que;
                        que.push(req);
                        Adj.push_back(que);
                        MemToAdj[req.row].push_back(loc);
                        // Update Pwrite
                        Pwrite[reg].first = loc;
                        Pwrite[reg].second = Pwrite[reg].second + 1;
                        if(Pread.find(memAddr) == Pread.end()) {
                            // Add to Pread
                            Pread.insert({memAddr, make_pair(loc, 1)});
                        }
                        else {
                            // Update Pread
                            Pread[memAddr].first = loc;
                            Pread[memAddr].second = Pread[memAddr].second + 1;
                        }
                    }
                    else {
                        // i is the upmost safest queue
                        loc = MemToAdj[req.row][i];
                        Adj[loc].push(req);
                        // Update Pwrite
                        Pwrite[reg].first = loc;
                        Pwrite[reg].second = Pwrite[reg].second + 1;
                        if(Pread.find(memAddr) == Pread.end()) {
                            // Add to Pread
                            Pread.insert({memAddr, make_pair(loc, 1)});
                        }
                        else {
                            // Update Pread
                            if(Pread[memAddr].first < loc) {
                                Pread[memAddr].first = loc;
                            }
                            Pread[memAddr].second = Pread[memAddr].second + 1;
                        }
                    }
                }
            }
        }
        // remove pending write on zero register if any
        if(Pwrite.find("$zero") != Pwrite.end()) {
            Pwrite.erase("$zero");
        }
    }

    Request getRequest() {
        Request req = Adj[adjIndex].front();
        return req;
    }

    void deleteRequest() {
        Request req = Adj[adjIndex].front();
        Adj[adjIndex].pop();
        if(Adj[adjIndex].empty()) {
            adjIndex ++;
            MemToAdj[req.row][0] ++;
        }
        string memAddr = to_string(req.addr);
        if(req.type == "sw") {
            Pwrite[memAddr].second --;
            if(Pwrite[memAddr].second == 0) {
                Pwrite.erase(memAddr);
            }
        }
        else {
            // req.type == "lw"
            Pread[memAddr].second --;
            if(Pread[memAddr].second == 0) {
                Pread.erase(memAddr);
            }
            string reg = reverseMap[req.destination];
            Pwrite[reg].second --;
            if(Pwrite[reg].second == 0) {
                Pwrite.erase(reg);
            }
        }
    }
    bool inPwrite(int val1, int val2 = -1, int val3 = -1) {
        string s1 = reverseMap[val1], s2 = reverseMap[val2], s3 = reverseMap[val3];
        return (Pwrite.find(s1) != Pwrite.end() || Pwrite.find(s2) != Pwrite.end() || Pwrite.find(s3) != Pwrite.end());
    }

};

// Register and Instruction structure
struct REGI {

    // instruction code map
    map<string, int> ins_map = { {"add", 0}, {"sub", 1}, {"mul", 2},{"beq", 3},{"bne", 4},{"slt", 5},{"j", 6},{"lw", 7},{"sw", 8},{"addi", 9} };

    // register number map
    map<string, int> reg_map = { {"$zero", 0}, {"$at", 1}, {"$v0", 2}, {"$v1", 3}, {"$a0", 4}, {"$a1", 5}, {"$a2",6}, {"$a3", 7}, {"$t0", 8}, {"$t1", 9}, {"$t2", 10},
                                 {"$t3", 11}, {"$t4", 12}, {"$t5", 13}, {"$t6", 14}, {"$t7", 15}, {"$s0", 16}, {"$s1", 17}, {"$s2", 18}, {"$s3", 19}, {"$s4", 20}, {"$s5", 21},
                                 {"$s6", 22}, {"$s7", 23}, {"$t8", 24}, {"$t9", 25}, {"$k0", 26}, {"$k1", 27}, {"$gp", 28}, {"$sp", 29}, {"$fp", 30}, {"$ra", 31} };

    map<int, string> num_reg = { {-1, "dummy"}, {0, "$zero"}, {1, "$at"}, {2, "$v0"}, {3, "$v1"}, {4, "$a0"}, {5, "$a1"}, {6, "$a2"}, {7, "$a3"}, {8, "$t0"}, {9, "$t1"}, {10, "$t2"},
                                 {11, "$t3"}, {12, "$t4"}, {13, "$t5"}, {14, "$t6"}, {15, "$t7"}, {16, "$s0"}, {17, "$s1"}, {18, "$s2"}, {19, "$s3"}, {20, "$s4"}, {21, "$s5"},
                                 {22, "$s6"}, {23, "$s7"}, {24, "$t8"}, {25, "$t9"}, {26, "$k0"}, {27, "$k1"}, {28, "$gp"}, {29, "$sp"}, {30, "$fp"}, {31, "$ra"} };

    // used to store labels used in program along with their corresponding address
    map<string, int> labels;

    // vector that contains all the labels encountered in instructions during compile time
    // these labels are resolved during linking time
    vector<pair<string, pair<int, int>>> label;
    
    vector<int> insMemory;
    int PC;
    int PARTITION;
    int DATA_START;
    int DATA_END;
    // vector used to store instructions
    vector<string> instructions;
    bool isWorking = true;

    // register parameters and clock cycle count
    int ins_cnt[10], reg[32];
    int cycle_cnt = 0;
    int procNum;
    bool pending;


    // constructor for initialization
    REGI() {

        // initialize registers with 0
        for (int i = 0; i < 32; i++)
            reg[i] = 0;

        // initialize instruction count with 0
        for (int i = 0; i < 10; i++)
            ins_cnt[i] = 0;

        insMemory = vector<int>(262144, 0);
        // initially the row buffer is empt
        PC = 0;
        PARTITION = 0;
        DATA_START = 0;
        pending = false;
    }

    void execution_stats() {

        // print execution statistics
        cout << "______________________________________________________________________________________________________\n\n";
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


    bool add(int dest, int src1, int src2) {
        long long sum = (long long)reg[src1] + (long long)reg[src2];
        stat_update(0);

        if (sum > 2147483647 || sum < -2147483648) {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = sum;
            // check if it is the zero register
            if (dest == 0)
                reg[dest] = 0;
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";

            return true;
        }
    }

    bool sub(int dest, int src1, int src2) {
        long long diff = (long long)reg[src1] - (long long)reg[src2];
        stat_update(1);

        if (diff > 2147483647 || diff < -2147483648) {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = diff;
            // check if it is the zero register
            if (dest == 0)
                reg[dest] = 0;
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";

            return true;
        }
    }

    bool mul(int dest, int src1, int src2) {
        long long prod = (long long)reg[src1] * (long long)reg[src2];
        stat_update(2);

        if (prod > 2147483647 || prod < -2147483648) {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = prod;
            // check if it is the zero register
            if (dest == 0)
                reg[dest] = 0;
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";

            return true;
        }
    }

    bool beq(int src1, int src2, int jumpto) {
        int temp = PC;
        if (reg[src1] == reg[src2]) {
            PC = jumpto;
            if (PC > PARTITION || PC < 0) {
                stat_update(3);
                cout << "Cycle " << cycle_cnt << ":\n";
                cout << "Instruction executed: " << instructions[temp / 4] << "\n";
                cout << "Memory Address of Instruction: " << temp << "\n";
                cout << "Warning: Program jumped to a non-instruction memory location. Executing pending DRAM requests (if any).\n\n";
                //execution_stats();
                return true;
            }
        }
        else PC += 4;
        stat_update(3);
        cout << "Cycle " << cycle_cnt << ":\n";
        cout << "Instruction executed: " << instructions[temp / 4] << "\n";
        cout << "Memory Address of Instruction: " << temp << "\n\n";
        return true;
    }

    bool bne(int src1, int src2, int jumpto) {
        int temp = PC;
        if (reg[src1] != reg[src2]) {
            PC = jumpto;
            if (PC > PARTITION || PC < 0) {
                stat_update(4);
                cout << "Cycle " << cycle_cnt << ":\n";
                cout << "Instruction executed: " << instructions[temp / 4] << "\n";
                cout << "Memory Address of Instruction: " << temp << "\n";
                cout << "Warning: Program jumped to a non-instruction memory location. Executing pending DRAM requests (if any).\n\n";
                //execution_stats();
                return true;
            }
        }
        else PC += 4;
        stat_update(4);
        cout << "Cycle " << cycle_cnt << ":\n";
        cout << "Instruction executed: " << instructions[temp / 4] << "\n";
        cout << "Memory Address of Instruction: " << temp << "\n\n";
        return true;
    }

    void slt(int dest, int src1, int src2) {
        reg[dest] = (reg[src1] < reg[src2]) ? 1 : 0;
        // check if it is the zero register
        if (dest == 0)
            reg[dest] = 0;
        stat_update(5);
        cout << "Cycle " << cycle_cnt << ":\n";
        cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        cout << "Memory Address of Instruction: " << PC << "\n";
        cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";
    }

    bool j(int jumpto) {
        int temp = PC;
        PC = jumpto;
        stat_update(6);
        if (PC > PARTITION) {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[temp / 4] << "\n";
            cout << "Memory Address of Instruction: " << temp << "\n";
            cout << "Warning: Program jumped to a non-instruction memory location. Executing pending DRAM requests (if any).\n\n";
            //execution_stats();
            return true;
        }
        else {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[temp / 4] << "\n";
            cout << "Memory Address of Instruction: " << temp << "\n\n";
            return true;
        }
    }

    // modified function
    bool lw(int r1, int offset, int r2) {
        bool flag;

        flag = raiseRequest(r1, offset, r2, 7);
        pending = true;
        if(!flag) return flag;
        
        return flag;
    }

    // modified function
    bool sw(int r1, int offset, int r2) {
        bool flag;

        flag = raiseRequest(r1, offset, r2, 8);
        pending = true;
        if(!flag) return flag;

        return flag;
    }

    // new function added
    bool raiseRequest(int r1, int offset, int r2, int ins_num) {
        // check if address is valid
        long long addr = (long long)offset + (long long)reg[r2] + (long long)DATA_START;
        if (addr >= DATA_END || addr < DATA_START) {
            // invalid address, error
            cout << addr << " " << DATA_START << " " << DATA_END << " " << procNum << "\n";
            cout << "Processor " + to_string(procNum) +" raised Error: Program is trying to access an unavailable memory location. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            if (addr % 4 != 0) {
                // address is not word aligned, error
                cout << "Processor " + to_string(procNum) +" raised Error: Memory address is not word-aligned. Invalid load operation. Program Terminating!\n\n";
                execution_stats();
                return false;
            }
        }
        // else
        
        // update cycle count and instruction stats, request issued to DRAM
        stat_update(ins_num);
        cout << "Cycle " << cycle_cnt << ": ";
        if(r1 == 0 && ins_num == 7) {
            cout << "\nInstruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "No DRAM request issued; $zero register!\n";
            cout << "Register modified: " << num_reg[r1] << " = " << reg[r1] << " (0x" << decimalToHexadecimal(reg[r1]) << ")\n\n";
            PC += 4;
            return true;
        }
        cout << "DRAM request issued for Instruction: " << instructions[PC / 4] << "\n";
        cout << "Memory Address of Instruction: " << PC << "\n\n";
        // add request to reqBuffer
        if(ins_num == 7) {
            Request req = {(int)addr - DATA_START, ((int)addr - DATA_START) / 1024, 0, r1, "lw", instructions[PC/4],PC, procNum};
            reqBuffer.push_back(req);
        }    
        else {  
            Request req = {(int)addr - DATA_START, ((int)addr - DATA_START) / 1024, reg[r1], 0, "sw", instructions[PC/4],PC, procNum};
            reqBuffer.push_back(req);
        }
        // increment PC by 4
        PC += 4;
        return true;     
    }



    bool addi(int dest, int src, int adds) {
        long long sum = (long long)reg[src] + (long long)adds;
        stat_update(9);

        if (sum > 2147483647 || sum < -2147483648) {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = sum;
            // check if it is the zero register
            if (dest == 0)
                reg[dest] = 0;
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
            cout << "Memory Address of Instruction: " << PC << "\n";
            cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";
            return true;
        }
    }

    void stat_update(int ins_code) {
        ins_cnt[ins_code]++;
    }

};

#include "OldFunctions.h"

bool checkPC(vector<REGI> &rf){
    bool flag = false;
    for(int i = 0; i < no_of_files; i++){
        if((rf[i].PC < rf[i].PARTITION && rf[i].PC >= 0) || rf[i].pending){flag = true;}
        else{rf[i].isWorking = false;}
    }
    return flag;
}

int delay(int size) {
    return 1;
}

// bool NonBlockLW(Queue &q, vector<REGI> &rf) {
    
    
   
//    //  cout << "DRAM PROCESSING STARTED: Cycle " << cycle_start + 1 << ": Instruction: " << req.instruction << "\n";
//    //  //-> cout<<PC
//    // cout << "Memory Address of Instruction: " << req.PC << "\n\n";
   

//     // check if the current buffer row is different from current row or not
//     if (start == row_start) {
//         if(type == "lw") {value_read++;}
//         if(type == "sw") {value_write++; doWriteback = true;}
//         cout<<"Cycle "<<cycle_start + 1 << "-" << cycle_end << ":" <<" DRAM processing for Instruction: " << req.instruction << ": completed.\n";
//         //->cout<<PC
//         cout << "\t  Memory Address of Instruction: " << req.PC << "\n";
//         if(type == "lw") {
//             reg[src] = (src==0) ? 0 : ROW_BUFFER[addr + DATA_START - row_start];
//             cout << "\t  READ:  Cycle " << cycle_start + 1 << "-" << cycle_end << ":" << " Register value updated: " << num_reg[src] << " = " << ROW_BUFFER[addr + DATA_START - row_start] << " (0x" << decimalToHexadecimal(ROW_BUFFER[addr + DATA_START - row_start]) << ")\n\n";
//         }
//         if(type == "sw") {
//             ROW_BUFFER[addr + DATA_START - row_start] = data;
//             cout << "\t  WRITE: Cycle " << cycle_start + 1 << "-" << cycle_end << ":" << " Data updated in ROW BUFFER: Memory Address (Data section): " << addr << "-" << addr + 3 << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
//         }
//     }
   
//     else {
    

//         cycle_start = cycle_start + (1 - isEmpty) * ROW_ACCESS_DELAY;
//         cycle_end = cycle_start + ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
//         cout<<"Cycle "<<cycle_start - (1 - isEmpty) * ROW_ACCESS_DELAY + 1 << "-" << cycle_end << ":" <<" DRAM request completed for Instruction: " << req.instruction << "\n";
//         //->Cout<<PC
//         cout << "\t  Memory Address of Instruction: " << req.PC << "\n";
//         if(!isEmpty) {cout << "\t  WRITEBACK:  Cycle " << cycle_start - ROW_ACCESS_DELAY + 1 << "-" << cycle_start << ":" << " Copying from ROW BUFFER to DRAM (Row (Data section): " << temp_start - DATA_START << "-" << temp_end - DATA_START << ")\n";}
//         cout << "\t  ACTIVATION: Cycle " << cycle_start + 1 << "-" << cycle_start + ROW_ACCESS_DELAY << ":" << " Copying from DRAM to ROW BUFFER (Row (Data section): " << row_start - DATA_START << "-" << row_end - DATA_START << ")\n";
//         if(type == "lw") {
//             reg[src] = (src == 0) ? 0 : ROW_BUFFER[addr + DATA_START - row_start];
//             cout << "\t  READ:       Cycle " << cycle_start + ROW_ACCESS_DELAY + 1 << "-" << cycle_start + ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " Register value updated: " << num_reg[src] << " = " << ROW_BUFFER[addr + DATA_START - row_start] << " (0x" << decimalToHexadecimal(ROW_BUFFER[addr + DATA_START - row_start]) << ")\n\n";
//         }
//         if(type == "sw") {
//             ROW_BUFFER[addr + DATA_START - row_start] = data;
//             cout << "\t  WRITE:      Cycle " << cycle_start + ROW_ACCESS_DELAY + 1 << "-" << cycle_start + ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " Data updated in ROW BUFFER: Memory Address (Data section): " << addr << "-" << addr + 3 << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
//         }
//         isEmpty = false;
//     }
    
//     q.deleteRequest();
//     // execution completed (both load/store word and other independent instructions)
//     return true;
// }



// this function simulates the execution of MIPS program
// modified function
bool simulator(vector<REGI> &rf, Queue &q) {

    int ins_code;
    bool flag;
    int val1, val2, val3;

    int cycle = 0;
    bool manIsIdle = true;
    bool dramIsIdle = true;
    int completionCycleMan = 0;
    int completionCycleDRAM = 0;
    Request req;
    int row_start, row_end, addr, src, data, i;
    int start = -1, end = -1;
    bool isEmpty = true, doWriteback = false;
    int data_bus = -1;
    int writebacks, copies, value_write, value_read;
    writebacks = copies = value_write = value_read = 0;
    int ROW_BUFFER[1024];
    string type;
    // while program counter is less than PARTITION
    while (checkPC(rf)) {
        cycle ++;
        if(dramIsIdle) {
            if(!q.isEmpty()) {
                req = q.getRequest();
                i = req.procNum - 1;
                row_start = req.row * 1024 + rf[i].DATA_START;
                row_end = row_start + 1023;
                addr = req.addr;
                src = req.destination;  // lw
                data = req.data_bus;    // sw
                type = req.type;
                if(type == "sw")
                    value_write++;
                else if(type == "lw")
                    value_read++;

                if(start == row_start) {
                    if(type == "sw")
                        doWriteback = true;
                    completionCycleDRAM = cycle + COL_ACCESS_DELAY - 1;
                }
                else {
                    if(type == "sw") {
                        doWriteback = true;
                    }
                    else {
                        doWriteback = false;
                    }
                    if(isEmpty) 
                        completionCycleDRAM = cycle + ROW_ACCESS_DELAY + COL_ACCESS_DELAY - 1;
                    else {
                        completionCycleDRAM = cycle + 2 * ROW_ACCESS_DELAY + COL_ACCESS_DELAY - 1;
                        for(int i = 0; i < 1024; i ++) {
                            DRAM[start + i] = ROW_BUFFER[i];
                        }
                        writebacks++;
                    }
                    for (int i = 0; i < 1024; i++) {
                        ROW_BUFFER[i] = DRAM[row_start + i];
                    }
                    copies++;
                }  
                start = row_start;
                end = row_end;
                isEmpty = false; 
                vector<int>::iterator iter = find(memoryAddress.begin(), memoryAddress.end(), addr);
                if (iter == memoryAddress.end()) {
                    memoryAddress.push_back(addr + rf[i].DATA_START);
                }                             
                dramIsIdle = false;

            }
            else {
                completionCycleDRAM = 0;
            }
        }
        else {
            if(cycle == completionCycleDRAM) {
                if(type == "sw") {
                     ROW_BUFFER[addr + rf[i].DATA_START - row_start] = data;
                }
                else {
                    rf[i].reg[src] = (src == 0) ? 0 : ROW_BUFFER[addr + rf[i].DATA_START - row_start];
                }
                q.deleteRequest();
                rf[i].pending = false;
                dramIsIdle = true;
                completionCycleDRAM = 0;
            }
        }

        if(manIsIdle) {
            if(!manBuffer.empty()) {
                manIsIdle = false;
                completionCycleMan = cycle + delay(manBuffer.size());
            }
            else {
                for(int i = 0; i < reqBuffer.size(); i ++) {
                    manBuffer.push_back(reqBuffer[i]);
                }
                reqBuffer.clear();
            }
        }
        else {
            if(cycle == completionCycleMan) {
                for(int i = 0; i < manBuffer.size(); i ++) {
                    q.addRequest(manBuffer[i]);
                }
                manIsIdle = true;
                manBuffer.clear();
                completionCycleMan = 0;
            }
        }

        for(int i = 0; i < no_of_files; i++){
            if(!rf[i].isWorking){continue;}
            // get instruction code
            if(rf[i].PC >= rf[i].PARTITION) {rf[i].cycle_cnt ++; continue;}
            ins_code = rf[i].insMemory[rf[i].PC];
            val1 = rf[i].insMemory[rf[i].PC + 1]; val2 = rf[i].insMemory[rf[i].PC + 2]; val3 = rf[i].insMemory[rf[i].PC + 3];
            switch (ins_code) {
                // add
                case 0:
                    if (q.inPwrite(val1, val2, val3)) {
                        // don't do anything, break
                        break;
                    }    
                    flag = rf[i].add(val1, val2, val3);
                    if (flag) rf[i].PC += 4;
                    else rf[i].isWorking = flag;
                    break;
                    // sub
                case 1:
                    if (q.inPwrite(val1, val2, val3)) {
                        // don't do anything, break
                        break;
                    } 
                    flag = rf[i].sub(val1, val2, val3);
                    if (flag) rf[i].PC += 4;
                    else rf[i].isWorking = flag;
                    break;
                    // mul
                case 2:
                    if (q.inPwrite(val1, val2, val3)) {
                        // don't do anything, break
                        break;
                    } 
                    flag = rf[i].mul(val1, val2, val3);
                    if (flag) rf[i].PC += 4;
                    else rf[i].isWorking = flag;
                    break;
                    // beq
                case 3:
                    if (q.inPwrite(val1, val2)) {
                        // don't do anything, break
                        break;
                    } 
                    flag = rf[i].beq(val1, val2, val3);
                    if (!flag) rf[i].isWorking = flag;
                    break;
                    // bne
                case 4:
                    if (q.inPwrite(val1, val2)) {
                        // don't do anything, break
                        break;
                    } 
                    flag = rf[i].bne(val1, val2, val3);
                    if (!flag) rf[i].isWorking = flag;
                    break;
                    // slt
                case 5:
                    if (q.inPwrite(val1, val2, val3)) {
                        // don't do anything, break
                        break;
                    } 
                    rf[i].slt(val1, val2, val3);
                    rf[i].PC += 4;
                    break;
                    // j
                case 6:
                    flag = rf[i].j(val1);
                    if (!flag) rf[i].isWorking = flag;
                    break;
                case 7:
                    if (q.inPwrite(val3)) {
                        // don't do anything, break
                        break;
                    } 
                    flag = rf[i].lw(val1, val2, val3);
                    if(!flag) rf[i].isWorking = flag;
                    break;
                case 8:
                    if (q.inPwrite(val1, val3)) {
                        // don't do anything, break
                        break;
                    } 
                    flag = rf[i].sw(val1, val2, val3);
                    if(!flag) rf[i].isWorking = flag;
                    break;
                    // addi                
                case 9:
                    if (q.inPwrite(val1, val2)) {
                        // don't do anything, break
                        break;
                    } 
                    flag = rf[i].addi(val1, val2, val3);
                    if (flag) rf[i].PC += 4;
                    else rf[i].isWorking = flag;
                    break;
            }
            rf[i].cycle_cnt ++;
        }
    }
    cout << "PROGRAM EXECUTION ENDED.\n";
    if (!isEmpty && doWriteback) {
        writebacks++;
        for (int i = 0; i < 1024; i++) {
            DRAM[start + i] = ROW_BUFFER[i];
        }
        cout << "Executing pending writeback:\n";
        cout << "Cycle " << cycle + 1 << ": DRAM request issued\n";
        cout << "Cycle " << cycle + 2 << "-" << cycle + ROW_ACCESS_DELAY + 1 << ":" << " WRITEBACK: Copying from ROW BUFFER to DRAM (Row (Data section) : " << start << "-" << end << ")\n";
        cycle = cycle + ROW_ACCESS_DELAY + 1;
    }
    for(int i = 0; i < no_of_files; i ++) {
        cout << "Processor " << i + 1 << "\n";
        rf[i].execution_stats();
    }
    int n = memoryAddress.size();
    if (n != 0) {
        sort(memoryAddress.begin(), memoryAddress.end());
        cout << "\nMemory content at the end of the execution (Data section):\n";
        for (int i = 0; i < n; i++) {
            cout << memoryAddress[i] << "-" << memoryAddress[i] + 3 << " = " << DRAM[memoryAddress[i]] << " (0x" << decimalToHexadecimal(DRAM[memoryAddress[i]]) << ")\n";
        }
    }
    cout << "\nTotal ROW BUFFER operations (writeback/activation/read/write): " << writebacks + copies + value_write + value_read << "\n";
    cout << "Number of times data was written back on DRAM from ROW BUFFER (WRITEBACK): " << writebacks << "\n";
    cout << "Number of times data was copied from DRAM to ROW BUFFER (ACTIVATION): " << copies << " (ROW BUFFER update)\n";
    cout << "Number of times data was written on ROW BUFFER (WRITE):" << value_write << " (ROW BUFFER update)\n";
    cout << "Number of times data was read from ROW BUFFER (READ):" << value_read << "\n\n";
    cout << "______________________________________________________________________________________________________\n\n";
    return true;
}

// modified function
// added Queue data structure
int main(int argc, char** argv) {
    
    string file_path;

    if (argc >= 5) {
        // raise warning if extra arguments are provided
        if (argc > 5) {
            cout << "WARNING: Extra command line arguments were provided! Four arguments are expected from the user.\nRefer to README.md for more details..\n";
        }
        // store the command line arguments
        // file name
        
        if (!isInteger(argv[1]) || !isInteger(argv[2]) || !isInteger(argv[3]) || !isInteger(argv[4])) {
            cout << "ERROR: Some arguments are non-integers. Integer arguments are expected. Program terminating!\n";
            return 0;
        }
        else {
            no_of_files = stoi(argv[1]);
            simulation_time = stoi(argv[2]);
            ROW_ACCESS_DELAY = stoi(argv[3]);
            COL_ACCESS_DELAY = stoi(argv[4]);
            if (simulation_time <= 0 || no_of_files <= 0 || ROW_ACCESS_DELAY <= 0 || COL_ACCESS_DELAY <= 0) {
                cout << "ERROR: Some argument are negative or zero. Positive arguments are expected. Program terminating!\n";
                return 0;
            }
        }
    }
    else {
        // insufficient number of arguments
        cout << "ERROR: Insufficient number of arguments were provided! Four arguments are expected from the user.\nRefer to README.md for more details..\n";
        cout << "Program terminating!\n";
        return 0;
    }
    vector<ifstream> infile(no_of_files);
    // create register file
    vector<REGI> rf(no_of_files);
    
    for(int i = 0; i < no_of_files; i++){
        infile[i].open("t"+to_string(i+1)+".txt");
        if (!infile[i].is_open()) {
            rf[i].isWorking = false;
            cout << "PROCESSOR "+to_string(i+1)+" raised ERROR: Unable to open file t" + to_string(i+1)+".txt.!\n";
        }
        rf[i].procNum = i + 1;
    }
    
    // create DRAM queue
    Queue q;

    // variable to store a line
    string line;

    // initializations
    bool flag;
    int line_number;
    for(int i = 0; i < no_of_files; i++){
        if(!rf[i].isWorking){continue;}
        line_number = 1;
        rf[i].DATA_START = i*(1024/no_of_files) * 1024;
        rf[i].DATA_END = (i+1)*(1024/no_of_files) * 1024;
        //read file, line by line
        while (getline(infile[i], line)) {
            if(rf[i].PC > 262143){
                cout<<"Processor " + to_string(i+1) << " raised ERROR: Instruction memory overflow.\n";
                rf[i].isWorking = false;
                break;
            }
            //process and store each instruction in memory
            flag = addToMemory(line, rf[i], line_number, i+1);
            // check if there was some error
            rf[i].isWorking = flag;
            if(!flag){break;}
            // read next line
            line_number++;
        }
        if(!rf[i].isWorking){continue;}
        // initialize PARTITION to PC
        // PARTITION denotes end of stored instructions
        rf[i].PARTITION = rf[i].PC;

        // link labels used in program
        flag = linker(rf[i], i+1);
        rf[i].isWorking = flag;
        // re-initialize PC to 0 for execution of MIPS program
        rf[i].PC = 0;
    }

    // execute the program
    flag = simulator(rf, q);

    if (flag)
        cout << "\nProgram executed successfully!\n\n";

    // close the input file stream
    for(int i = 0; i < no_of_files; i++){
        infile[i].close();
    }
    return 0;
}
     
