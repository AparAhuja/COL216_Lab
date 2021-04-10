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
int PC = 0;
// instruction end address
int PARTITION = 0;

// starting address of data section (old value = 699052)
int DATA_START = 699392;

// ROW_ACCESS_DELAY and COL_ACCESS_DELAY
int ROW_ACCESS_DELAY, COL_ACCESS_DELAY;

// execution mode (non-block vs normal), false denotes normal and true denotes non-block
bool MODE = true;

// memory available (2 ^ 20 bytes), used to model 2D DRAM (row major)
int DRAM[1048576];


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
};

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

    // vector used to store instructions
    vector<string> instructions;

    // ROW_BUFFER
    int ROW_BUFFER[1024];

    // start and ending addresses in buffer
    int start, end;

    // boolean flag to tell if the buffer is empty or not
    bool isEmpty;

    // register to memory intermediate (store word intermediate value buffer)
    int data_bus = -1;

    // variables to store number of row_buffer updates

    int writebacks, copies, value_write, value_read;

    // register parameters and clock cycle count
    int ins_cnt[10], reg[32];
    int cycle_cnt = 0;

    // vector to store memory locations which were used
    vector<int> memoryAddress;

    // boolean to tell whether or not do the final writeback
    bool doWriteback;

    // constructor for initialization
    REGI() {

        // initialize registers with 0
        for (int i = 0; i < 32; i++)
            reg[i] = 0;

        // initialize instruction count with 0
        for (int i = 0; i < 10; i++)
            ins_cnt[i] = 0;

        // initialize row buffer
        for (int i = 0; i < 1024; i++) {
            ROW_BUFFER[i] = 0;
        }

        // initially the row buffer is empty
        isEmpty = true;
        doWriteback = false;
        start = -1;
        end = -1;
        writebacks = 0;
        copies = 0;
        value_write = 0;
        value_read = 0;
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
        int n = memoryAddress.size();
        if (n != 0) {
            sort(memoryAddress.begin(), memoryAddress.end());
            cout << "\nMemory content at the end of the execution (Data section):\n";
            for (int i = 0; i < n; i++) {
                cout << memoryAddress[i] << "-" << memoryAddress[i] + 3 << " = " << DRAM[DATA_START + memoryAddress[i]] << " (0x" << decimalToHexadecimal(DRAM[DATA_START + memoryAddress[i]]) << ")\n";
            }
        }
        cout << "\nTotal ROW BUFFER operations (writeback/activation/read/write): " << writebacks + copies + value_write + value_read << "\n";
        cout << "Number of times data was written back on DRAM from ROW BUFFER (WRITEBACK): " << writebacks << "\n";
        cout << "Number of times data was copied from DRAM to ROW BUFFER (ACTIVATION): " << copies << " (ROW BUFFER update)\n";
        cout << "Number of times data was written on ROW BUFFER (WRITE):" << value_write << " (ROW BUFFER update)\n";
        cout << "Number of times data was read from ROW BUFFER (READ):" << value_read << "\n\n";
        cout << "______________________________________________________________________________________________________\n\n";


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
    bool lw(int r1, int offset, int r2, Queue &q) {
        bool flag;

        flag = raiseRequest(r1, offset, r2, q, 7);
        if(!flag) return flag;
        
        return flag;
    }

    // modified function
    bool sw(int r1, int offset, int r2, Queue &q) {
        bool flag;

        flag = raiseRequest(r1, offset, r2, q, 8);
        if(!flag) return flag;

        return flag;
    }

    // new function added
    bool raiseRequest(int r1, int offset, int r2, Queue &q, int ins_num) {
        // check if address is valid
        long long addr = (long long)offset + (long long)reg[r2] + (long long)DATA_START;
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
        // add request to Queue
        if(ins_num == 7) {
            Request req = {(int)addr - DATA_START, ((int)addr - DATA_START) / 1024, 0, r1, "lw", instructions[PC/4],PC};
            q.addRequest(req);
        }    
        else {  
            Request req = {(int)addr - DATA_START, ((int)addr - DATA_START) / 1024, reg[r1], 0, "sw", instructions[PC/4],PC};
            q.addRequest(req);
        }
        // increment PC by 4
        PC += 4;
        return true;     
    }

    // modified function
   bool NonBlockLW(Queue &q) {
        // boolean flag used to detect any errors
        bool flag = true;
        int count = 0;
        int temp = cycle_cnt;

        Request req = q.getRequest();
        int row_start = req.row * 1024 + DATA_START;
        int row_end = row_start + 1023;
        int addr = req.addr;
        int src = req.destination;
        int data = req.data_bus;
        string type = req.type;
        int cycle_start = cycle_cnt, cycle_end;
        vector<int>::iterator iter = find(memoryAddress.begin(), memoryAddress.end(), addr);
        if (iter == memoryAddress.end()) {
            memoryAddress.push_back(addr);
        }    
       
        cout << "DRAM PROCESSING STARTED: Cycle " << cycle_start + 1 << ": Instruction: " << req.instruction << "\n";
        //-> cout<<PC
       cout << "Memory Address of Instruction: " << req.PC << "\n\n";
        // check if the current buffer row is different from current row or not
        if (start == row_start) {
            if(type == "lw") {value_read++;}
            if(type == "sw") {value_write++; doWriteback = true;}
            //row buffer is the same, so no loading required
            while (count < COL_ACCESS_DELAY) {
                if(MODE)
                    flag = executeIndependent(q);

                if (temp == cycle_cnt) {
                    cycle_cnt++;
                    temp++;
                }
                else {
                    temp = cycle_cnt;
                }
                if (!flag) return flag;
                count++;
            }
            cycle_end = cycle_start + COL_ACCESS_DELAY;
            cout<<"Cycle "<<cycle_start + 1 << "-" << cycle_end << ":" <<" DRAM processing for Instruction: " << req.instruction << ": completed.\n";
            //->cout<<PC
            cout << "\t  Memory Address of Instruction: " << req.PC << "\n";
            if(type == "lw") {
                reg[src] = (src==0) ? 0 : ROW_BUFFER[addr + DATA_START - row_start];
                cout << "\t  READ:  Cycle " << cycle_start + 1 << "-" << cycle_end << ":" << " Register value updated: " << num_reg[src] << " = " << ROW_BUFFER[addr + DATA_START - row_start] << " (0x" << decimalToHexadecimal(ROW_BUFFER[addr + DATA_START - row_start]) << ")\n\n";
            }
            if(type == "sw") {
                ROW_BUFFER[addr + DATA_START - row_start] = data;
                cout << "\t  WRITE: Cycle " << cycle_start + 1 << "-" << cycle_end << ":" << " Data updated in ROW BUFFER: Memory Address (Data section): " << addr << "-" << addr + 3 << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
            }
        }
       
        else {
            // row buffer is different, so write it back to memory
            // will take ROW_ACCESS_DELAY cycles
            if(!isEmpty){
                for (int i = 0; i < 1024; i++) {
                    DRAM[start + i] = ROW_BUFFER[i];
                }
                while (count < ROW_ACCESS_DELAY) {
                    if(MODE)
                        flag = executeIndependent(q);

                    if (temp == cycle_cnt) {
                        cycle_cnt++;
                        temp++;
                    }
                    else {
                        temp = cycle_cnt;
                    }
                    if (!flag) return flag;
                    count++;
                }
                writebacks++;
            }
            // now load the new row
            loadBuffer(row_start, row_end);
            copies++;
            if(type == "lw") {value_read++; doWriteback = false;}
            if(type == "sw") {value_write++; doWriteback = true;}
            
            int temp_start = start, temp_end = end;

            // update start and end values
            start = row_start;
            end = row_end;
            
            // execute independent instructions
            count = 0;
            while (count < ROW_ACCESS_DELAY) {
                if(MODE)
                    flag = executeIndependent(q);

                if (temp == cycle_cnt) {
                    cycle_cnt++;
                    temp++;
                }
                else {
                    temp = cycle_cnt;
                }
                if (!flag) return flag;
                count++;
            }
            count = 0;
            // update register value
            while (count < COL_ACCESS_DELAY) {
                if(MODE)
                    flag = executeIndependent(q);

                if (temp == cycle_cnt) {
                    cycle_cnt++;
                    temp++;
                }
                else {
                    temp = cycle_cnt;
                }
                if (!flag) return flag;
                count++;
            }
            cycle_start = cycle_start + (1 - isEmpty) * ROW_ACCESS_DELAY;
            cycle_end = cycle_start + ROW_ACCESS_DELAY + COL_ACCESS_DELAY;
            cout<<"Cycle "<<cycle_start - (1 - isEmpty) * ROW_ACCESS_DELAY + 1 << "-" << cycle_end << ":" <<" DRAM request completed for Instruction: " << req.instruction << "\n";
            //->Cout<<PC
            cout << "\t  Memory Address of Instruction: " << req.PC << "\n";
            if(!isEmpty) {cout << "\t  WRITEBACK:  Cycle " << cycle_start - ROW_ACCESS_DELAY + 1 << "-" << cycle_start << ":" << " Copying from ROW BUFFER to DRAM (Row (Data section): " << temp_start - DATA_START << "-" << temp_end - DATA_START << ")\n";}
            cout << "\t  ACTIVATION: Cycle " << cycle_start + 1 << "-" << cycle_start + ROW_ACCESS_DELAY << ":" << " Copying from DRAM to ROW BUFFER (Row (Data section): " << row_start - DATA_START << "-" << row_end - DATA_START << ")\n";
            if(type == "lw") {
                reg[src] = (src == 0) ? 0 : ROW_BUFFER[addr + DATA_START - row_start];
                cout << "\t  READ:       Cycle " << cycle_start + ROW_ACCESS_DELAY + 1 << "-" << cycle_start + ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " Register value updated: " << num_reg[src] << " = " << ROW_BUFFER[addr + DATA_START - row_start] << " (0x" << decimalToHexadecimal(ROW_BUFFER[addr + DATA_START - row_start]) << ")\n\n";
            }
            if(type == "sw") {
                ROW_BUFFER[addr + DATA_START - row_start] = data;
                cout << "\t  WRITE:      Cycle " << cycle_start + ROW_ACCESS_DELAY + 1 << "-" << cycle_start + ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " Data updated in ROW BUFFER: Memory Address (Data section): " << addr << "-" << addr + 3 << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
            }
            isEmpty = false;
        }
        
        q.deleteRequest();
        // execution completed (both load/store word and other independent instructions)
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
        cycle_cnt++;
        ins_cnt[ins_code]++;
    }

    // new function added
    bool inPwrite(Queue &q, int val1, int val2 = -1, int val3 = -1) {
        string s1 = num_reg[val1], s2 = num_reg[val2], s3 = num_reg[val3];
        return (q.Pwrite.find(s1) != q.Pwrite.end() || q.Pwrite.find(s2) != q.Pwrite.end() || q.Pwrite.find(s3) != q.Pwrite.end());
    }

    void loadBuffer(int row_start, int row_end) {
        // load memory row into buffer
        for (int i = 0; i < 1024; i++) {
            ROW_BUFFER[i] = DRAM[row_start + i];
        }
    }

    // modified function
    bool executeIndependent(Queue &q) {
        int ins_code;
        bool flag;

        // check if PC is less than partition
        if (PC < PARTITION && PC >= 0) {
            ins_code = DRAM[PC];
            int val1 = DRAM[PC + 1], val2 = DRAM[PC + 2], val3 = DRAM[PC + 3];

            switch (ins_code) {
                // add
                case 0:
                    if (inPwrite(q, val1, val2, val3))
                        // don't do anything, break
                        break;
                    flag = add(val1, val2, val3);
                    if (flag) PC += 4;
                    else return flag;
                    break;
                    // sub
                case 1:
                    if (inPwrite(q, val1, val2, val3))
                        // don't do anything, break
                        break;
                    flag = sub(val1, val2, val3);
                    if (flag) PC += 4;
                    else return flag;
                    break;
                    // mul
                case 2:
                    if (inPwrite(q, val1, val2, val3))
                        // don't do anything, break
                        break;
                    flag = mul(val1, val2, val3);
                    if (flag) PC += 4;
                    else return flag;
                    break;
                    // beq
                case 3:
                    if (inPwrite(q, val1, val2))
                        // don't do anything, break
                        break;
                    flag = beq(val1, val2, val3);
                    if (!flag) return flag;
                    break;
                    // bne
                case 4:
                    if (inPwrite(q, val1, val2))
                        // don't do anything, break
                        break;
                    flag = bne(val1, val2, val3);
                    if (!flag) return flag;
                    break;
                    // slt
                case 5:
                    if (inPwrite(q, val1, val2, val3))
                        // don't do anything, break
                        break;
                    slt(val1, val2, val3);
                    PC += 4;
                    break;
                    // j
                case 6:
                    flag = j(val1);
                    if (!flag) return flag;
                    break;
                case 7:
                    if(inPwrite(q, val3))
                        break;
                    flag = raiseRequest(val1, val2, val3, q, 7);
                    if(!flag) return flag;
                    break;
                case 8:
                    if(inPwrite(q, val1, val3))
                        break;
                    flag = raiseRequest(val1, val2, val3, q, 8);
                    if(!flag) return flag;
                    break;
                    // addi                
                case 9:
                    if (inPwrite(q, val1, val2))
                        // don't do anything, break
                        break;
                    flag = addi(val1, val2, val3);
                    if (flag) PC += 4;
                    else return flag;
                    break;
            }
        }

        // instruction executed successfully
        return true;
    }

    // modified function
    void buffer() {
        cout << "PROGRAM EXECUTION ENDED.\n";
        if (!isEmpty && doWriteback) {
            writebacks++;
            for (int i = 0; i < 1024; i++) {
                DRAM[start + i] = ROW_BUFFER[i];
            }
            cout << "Executing pending writeback:\n";
            cout << "Cycle " << cycle_cnt + 1 << ": DRAM request issued\n";
            cout << "Cycle " << cycle_cnt + 2 << "-" << cycle_cnt + ROW_ACCESS_DELAY + 1 << ":" << " WRITEBACK: Copying from ROW BUFFER to DRAM (Row (Data section) : " << start - DATA_START << "-" << end - DATA_START << ")\n";
            cycle_cnt = cycle_cnt + ROW_ACCESS_DELAY + 1;
        }
    }
};

#include "OldFunctions.h"

// this function simulates the execution of MIPS program
// modified function
bool simulator(REGI& rf, Queue &q) {

    int ins_code;
    bool flag;

    // while program counter is less than PARTITION
    while (PC < PARTITION && PC >= 0) {
        
        // get instruction code
        ins_code = DRAM[PC];

        switch (ins_code) {
            // add
            case 0:
                flag = rf.add(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;
                // sub
            case 1:
                flag = rf.sub(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;
                // mul
            case 2:
                flag = rf.mul(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;
                // beq
            case 3:
                flag = rf.beq(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3]);
                if (!flag) return flag;
                break;
                // bne
            case 4:
                flag = rf.bne(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3]);
                if (!flag) return flag;
                break;
                // slt
            case 5:
                rf.slt(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3]);
                PC += 4;
                break;
                // j
            case 6:
                flag = rf.j(DRAM[PC + 1]);
                if (!flag) return flag;
                break;
                // lw
            case 7:
                flag = rf.lw(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3], q);
                if (!flag) return flag;
                break;
                // sw
            case 8:
                flag = rf.sw(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3], q);
                if (!flag) return flag;
                break;
                // addi                
            case 9:
                flag = rf.addi(DRAM[PC + 1], DRAM[PC + 2], DRAM[PC + 3]);
                if (flag) PC += 4;
                else return flag;
                break;
        }
        while(!q.isEmpty()) {
            flag = rf.NonBlockLW(q);
            if(!flag) return flag;    
        }
        // rf.print_reg_file();
    }

    // PC = PARTITION, so execution is complete (successful)
    rf.buffer();
    rf.execution_stats();
    return true;
}

// modified function
// added Queue data structure
int main(int argc, char** argv) {
    
    string file_path;

    if (argc >= 4) {
        // raise warning if extra arguments are provided
        if (argc > 5) {
            cout << "WARNING: Extra command line arguments were provided! Maximum four arguments are expected from the user.\nRefer to README.md for more details..\n";
        }
        // store the command line arguments
        // file name
        
        file_path = argv[1];
        
        
        if (!isInteger(argv[2]) || !isInteger(argv[3])) {
            // ROW_ACCESS_DELAY or COL_ACCESS_DELAY is not an integer
            cout << "ERROR: DRAM delay is not an integer. Program terminating!\n";
            return 0;
        }
        else {
            ROW_ACCESS_DELAY = stoi(argv[2]);
            COL_ACCESS_DELAY = stoi(argv[3]);
            if (ROW_ACCESS_DELAY <= 0 || COL_ACCESS_DELAY <= 0) {
                // invalid delay (negative)
                cout << "ERROR: DRAM delay is given to be negative or zero. Program terminating!\n";
                return 0;
            }
        }
        if (argc >= 5) {
            if (!isInteger(argv[4])) {
                // mode is not an integer
                cout << "ERROR: Operating mode is not an integer. Enter 0 or 1. Program terminating!\n";
                return 0;
            }
            else {
                int n = stoi(argv[4]);
                if (n == 0)
                    MODE = false;
                else
                    MODE = true;
            }
        }
    }
    else {
        // insufficient number of arguments
        cout << "ERROR: Insufficient number of arguments were provided! Three arguments are expected from the user.\nRefer to README.md for more details..\n";
        cout << "Program terminating!\n";
        return 0;
    }

    ifstream infile(file_path);

    //File error
    if (!infile.is_open()) {
        cout << "ERROR: Unable to open file. Program terminating!\n";
        return 0;
    }

    // create register file
    REGI rf;
    // create DRAM queue
    Queue q;

    // variable to store a line
    string line;

    // initializations
    int line_number = 1;
    bool flag;

    //read file, line by line
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
    // PARTITION denotes end of stored instructions
    PARTITION = PC;

    // link labels used in program
    flag = linker(rf);

    if (!flag)
        // some linking error
        return 0;

    // re-initialize PC to 0 for execution of MIPS program
    PC = 0;

    // execute the program
    flag = simulator(rf, q);

    if (flag)
        cout << "\nProgram executed successfully!\n\n";

    // close the input file stream
    infile.close();

    return 0;
}
     
