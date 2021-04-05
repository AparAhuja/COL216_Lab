#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include "DRAM.h"

using namespace std;

// program counter (starts at 0)
int PC = 0;
// instruction end address
int PARTITION = 0;

// starting address of data section (old value - 699052)
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


// function to check if the string represents an integer or not
bool isInteger(string s) {
    int len = s.length();

    if (len == 0) return false;

    for (int i = 0; i < len; i++) {

        if (i == 0 && s.at(0) == '-')
            // might be negative number
            continue;
        if (s.at(i) < '0' || s.at(i) > '9') {
            // not an integer
            return false;
        }

    }
    // is an integer
    return true;
}


// checks if the label used is a valid identifier or not
bool validId(string label) {

    int len = label.length(), i = 0;

    // empty string is not a valid id
    if (len == 0) return false;
    else {

        if (isalpha(label.at(0)) || label.at(0) == '_') {
            i++;
            while (i < len && (isalpha(label.at(i)) || isdigit(label.at(i)) || label.at(i) == '_')) i++;
            if (i != len)
                // some invalid character is present in the label
                return false;
            else
                // valid id
                return true;
        }
        // not a valid id
        else return false;
    }
}


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
                cout << "Warning: Program jumped to a non-instruction memory location. Program terminated abruptly!\n\n";
                execution_stats();
                return false;
            }
        }
        else PC += 4;
        stat_update(3);
        cout << "Cycle " << cycle_cnt << ":\n";
        cout << "Instruction executed: " << instructions[temp / 4] << "\n\n";
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
                cout << "Warning: Program jumped to a non-instruction memory location. Program terminated abruptly!\n\n";
                execution_stats();
                return false;
            }
        }
        else PC += 4;
        stat_update(4);
        cout << "Cycle " << cycle_cnt << ":\n";
        cout << "Instruction executed: " << instructions[temp / 4] << "\n\n";
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
        cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";
    }

    bool j(int jumpto) {
        int temp = PC;
        PC = jumpto;
        stat_update(6);
        if (PC > PARTITION) {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[temp / 4] << "\n";
            cout << "Warning: Program jumped to a non-instruction memory location. Program terminated abruptly!\n\n";
            execution_stats();
            return false;
        }
        else {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[temp / 4] << "\n\n";
            return true;
        }
    }

    bool lw(int dest, int offset, int src, Queue &q) {
        bool flag;

        flag = raiseRequest(dest, offset, src, q, 7);
        if(!flag) return flag;

        // execute independent instructions
        flag = NonBlockLW(q);

        // check if it is the zero register
        if (dest == 0)
            reg[dest] = 0;

        return flag;
    }

    bool sw(int dest, int offset, int src, Queue &q) {
        bool flag;

        flag = raiseRequest(dest, offset, src, q, 8);
        if(!flag) return flag;

        // execute independent instructions
        flag = NonBlockLW(q);

        return flag;
    }

    bool raiseRequest(int dest, int offset, int src, Queue &q, int ins_num) {
        // check if address is valid
        long long addr = (long long)offset + (long long)reg[src] + (long long)DATA_START;
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
        cout << "DRAM request issued for: " << instructions[PC / 4] << "\n";
        // increment PC by 4
        PC += 4;
        if(ins_num == 7) {
            Request req = {(int)addr - DATA_START, ((int)addr - DATA_START) / 1024, 0, dest, "lw"};
            q.addRequest(req);
        }    
        else {  
            Request req = {(int)addr - DATA_START, ((int)addr - DATA_START) / 1024, reg[dest], 0, "sw"};
            q.addRequest(req);
        }   
        return true;     
    }

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
        vector<int>::iterator iter = find(memoryAddress.begin(), memoryAddress.end(), addr);
        if (iter == memoryAddress.end())
            memoryAddress.push_back(addr);
        // check if row buffer is empty or not
        if (isEmpty) {
            // load the row in buffer (will take a total of ROW_ACCESS_DELAY clock cycles)
            loadBuffer(row_start, row_end);
            // update start and end values
            start = row_start;
            end = row_end;
            isEmpty = false;
            copies++;
            if(type == "lw") {value_read++; doWriteback = false;}
            if(type == "sw") {value_write++; doWriteback = true;}
            if(type == "lw") {
                cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY << ":" << " ACTIVATION: Copying from DRAM to ROW BUFFER (Row (Data section): " << row_start - DATA_START << "-" << row_end - DATA_START << ")\n";
                if(req.destination != 0)
                    cout << "Cycle " << cycle_cnt + ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " READ: Register value updated: " << num_reg[src] << " = " << ROW_BUFFER[addr - row_start] << " (0x" << decimalToHexadecimal(ROW_BUFFER[addr - row_start]) << ")\n\n";
                else
                    cout << "Cycle " << cycle_cnt + ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " READ: Register value updated: " << num_reg[src] << " = 0" << " (" << "0x00000000" << ")\n\n";
            }
            else {
                cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY << ":" << " ACTIVATION: Copying from DRAM to ROW BUFFER (Row (Data section): " << row_start - DATA_START << "-" << row_end - DATA_START << ")\n";
                cout << "Cycle " << cycle_cnt + ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " WRITE: Data updated in ROW BUFFER: Memory Address (Data section): " << addr << "-" << addr + 3 << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
            }
            // simultaneously execute independent instructions if any
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
            // count == COL_ACCESS_DELAY
            if(type == "lw") reg[req.destination] = ROW_BUFFER[addr - row_start];
            if(type == "sw") ROW_BUFFER[addr - row_start] = req.data_bus;
        }
        else {
            // check if the current buffer row is different from current row or not
            if (start == row_start) {
                if(type == "lw") {value_read++;}
                if(type == "sw") {value_write++; doWriteback = true;}
                //row buffer is the same, so no loading required
                if(type == "lw") {
                    if(src != 0)
                        cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + COL_ACCESS_DELAY << ":" << " READ: Register value updated: " << num_reg[src] << " = " << ROW_BUFFER[addr - row_start] << " (0x" << decimalToHexadecimal(ROW_BUFFER[addr - row_start]) << ")\n\n";
                    else
                        cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + COL_ACCESS_DELAY << ":" << " READ: Register value updated: " << num_reg[src] << " = 0" << " (" << "0x00000000" << ")\n\n";
                }
                else {
                    cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + COL_ACCESS_DELAY << ":" << " WRITE: Data updated in ROW BUFFER: Memory Address (Data section): " << addr << "-" << addr + 3 << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
                }
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
                if(type == "lw") reg[req.destination] = ROW_BUFFER[addr - row_start];
                if(type == "sw") ROW_BUFFER[addr - row_start] = req.data_bus;
            }
            else {
                // row buffer is different, so write it back to memory
                // will take ROW_ACCESS_DELAY cycles
                for (int i = 0; i < 1024; i++) {
                    DRAM[start + i] = ROW_BUFFER[i];
                }
                // now load the new row
                loadBuffer(row_start, row_end);
                writebacks++;
                copies++;
                if(type == "lw") {value_read++; doWriteback = false;}
                if(type == "sw") {value_write++; doWriteback = true;}
                if(type == "lw") {
                    cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY << ":" << " WRITEBACK: Copying from ROW BUFFER to DRAM (Row (Data section): " << start - DATA_START << "-" << end - DATA_START << ")\n";
                    cout << "Cycle " << cycle_cnt + ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + 2 * ROW_ACCESS_DELAY << ":" << " ACTIVATION: Copying from DRAM to ROW BUFFER (Row (Data section): " << row_start - DATA_START << "-" << row_end - DATA_START<< ")\n";
                    if(src != 0)
                        cout << "Cycle " << cycle_cnt + 2 * ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + 2 * ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " READ: Register value updated: " << num_reg[src] << " = " << ROW_BUFFER[addr - row_start] << " (0x" << decimalToHexadecimal(ROW_BUFFER[addr - row_start]) << ")\n\n";
                    else
                        cout << "Cycle " << cycle_cnt + 2 * ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + 2 * ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " READ: Register value updated: " << num_reg[src] << " = 0" << " (" << "0x00000000" << ")\n\n";
                }
                else {
                    cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY << ":" << " WRITEBACK: Copying from ROW BUFFER to DRAM (Row (Data section) : " << start - DATA_START << "-" << end - DATA_START << ")\n";
                    cout << "Cycle " << cycle_cnt + ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + 2 * ROW_ACCESS_DELAY << ":" << " ACTIVATION: Copying from DRAM to ROW BUFFER (Row (Data section): " << row_start - DATA_START << "-" << row_end - DATA_START << ")\n";
                    cout << "Cycle " << cycle_cnt + 2 * ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + 2 * ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " WRITE: Data updated in ROW BUFFER: Memory Address (Data section): " << addr << "-" << addr + 3 << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
                }
                // update start and end values
                start = row_start;
                end = row_end;
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
                // update register value
                count = 0;
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
                if(type == "lw") reg[req.destination] = ROW_BUFFER[addr - row_start];
                if(type == "sw") ROW_BUFFER[addr - row_start] = req.data_bus;
            }
        }
        q.deleteRequest();
        // execution completed (both load word and other independent instructions)
        return true;
    }


    bool addi(int dest, int src, int adds) {
        long long sum = (long long)reg[src] + (long long)adds;
        stat_update(9);

        if (sum > 2147483647 || sum < -2147483648) {
            cout << "Cycle " << cycle_cnt << ":\n";
            cout << "Instruction executed: " << instructions[PC / 4] << "\n";
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
            cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";
            return true;
        }
    }

    void stat_update(int ins_code) {
        cycle_cnt++;
        ins_cnt[ins_code]++;
    }

    // bool NonBlockSW(int data, int row_start, int row_end, int addr) {
    //     // boolean flag used to detect any errors
    //     bool flag = true;
    //     int count = 0;
    //     int temp = cycle_cnt;
    //     // check if row buffer is empty or not
    //     if (isEmpty) {
    //         // load the row in buffer (will take a total of ROW_ACCESS_DELAY clock cycles)
    //         loadBuffer(row_start, row_end);
    //         // update start and end values
    //         start = row_start;
    //         end = row_end;
    //         isEmpty = false;
    //         copies++;
    //         value_write++;
    //         cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY << ":" << " ACTIVATION: Copying from DRAM to ROW BUFFER (Row (Data section): " << row_start - DATA_START << "-" << row_end - DATA_START << ")\n";
    //         cout << "Cycle " << cycle_cnt + ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " WRITE: Data updated in ROW BUFFER: Memory Address (Data section): " << addr - DATA_START << "-" << addr + 3 - DATA_START << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
    //         // simultaneously execute independent instructions if any
    //         while (count < ROW_ACCESS_DELAY) {
    //             if(MODE)
    //                 flag = executeIndependent();
    //             if (temp == cycle_cnt) {
    //                 cycle_cnt++;
    //                 temp++;
    //             }
    //             else {
    //                 temp = cycle_cnt;
    //             }
    //             if (!flag) return flag;
    //             count++;
    //         }
    //         count = 0;
    //         // update buffer value
    //         while (count < COL_ACCESS_DELAY) {
    //             if(MODE)
    //                 flag = executeIndependent();
    //             if (temp == cycle_cnt) {
    //                 cycle_cnt++;
    //                 temp++;
    //             }
    //             else {
    //                 temp = cycle_cnt;
    //             }
    //             if (!flag) return flag;
    //             count++;
    //             if (count == COL_ACCESS_DELAY) {
    //                 // update the value in buffer
    //                 ROW_BUFFER[addr - row_start] = data;
    //             }
    //         }
    //         ROW_BUFFER[addr - row_start] = data;
    //     }
    //     else {
    //         // check if the current buffer row is different from current row or not
    //         if (start == row_start) {
    //             // row buffer is the same, so no loading required
    //             value_write++;
    //             cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + COL_ACCESS_DELAY << ":" << " WRITE: Data updated in ROW BUFFER: Memory Address (Data section): " << addr - DATA_START << "-" << addr + 3 - DATA_START << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
    //             while (count < COL_ACCESS_DELAY) {
    //                 if(MODE)
    //                     flag = executeIndependent();
    //                 if (temp == cycle_cnt) {
    //                     cycle_cnt++;
    //                     temp++;
    //                 }
    //                 else {
    //                     temp = cycle_cnt;
    //                 }
    //                 if (!flag) return flag;
    //                 count++;
    //                 if (count == COL_ACCESS_DELAY) {
    //                     // update the value in buffer
    //                     ROW_BUFFER[addr - row_start] = data;
    //                 }
    //             }
    //             ROW_BUFFER[addr - row_start] = data;
    //         }
    //         else {
    //             // row buffer is different, so write it back to memory
    //             // will take ROW_ACCESS_DELAY cycles
    //             for (int i = 0; i < 1024; i++) {
    //                 DRAM[start + i] = ROW_BUFFER[i];
    //             }
    //             writebacks++;
    //             copies++;
    //             value_write++;
    //             cout << "Cycle " << cycle_cnt + 1 << "-" << cycle_cnt + ROW_ACCESS_DELAY << ":" << " WRITEBACK: Copying from ROW BUFFER to DRAM (Row (Data section) : " << start - DATA_START << "-" << end - DATA_START << ")\n";
    //             cout << "Cycle " << cycle_cnt + ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + 2 * ROW_ACCESS_DELAY << ":" << " ACTIVATION: Copying from DRAM to ROW BUFFER (Row (Data section): " << row_start - DATA_START << "-" << row_end - DATA_START << ")\n";
    //             cout << "Cycle " << cycle_cnt + 2 * ROW_ACCESS_DELAY + 1 << "-" << cycle_cnt + 2 * ROW_ACCESS_DELAY + COL_ACCESS_DELAY << ":" << " WRITE: Data updated in ROW BUFFER: Memory Address (Data section): " << addr - DATA_START << "-" << addr + 3 - DATA_START << " = " << data << " (0x" << decimalToHexadecimal(data) << ")\n\n";
    //             while (count < ROW_ACCESS_DELAY) {
    //                 if(MODE)
    //                     flag = executeIndependent();
    //                 if (temp == cycle_cnt) {
    //                     cycle_cnt++;
    //                     temp++;
    //                 }
    //                 else {
    //                     temp = cycle_cnt;
    //                 }
    //                 if (!flag) return flag;
    //                 count++;
    //             }
    //             // now load the new row
    //             loadBuffer(row_start, row_end);
    //             // update start and end values
    //             start = row_start;
    //             end = row_end;
    //             // execute independent instructions
    //             count = 0;
    //             while (count < ROW_ACCESS_DELAY) {
    //                 if(MODE)
    //                     flag = executeIndependent();
    //                 if (temp == cycle_cnt) {
    //                     cycle_cnt++;
    //                     temp++;
    //                 }
    //                 else {
    //                     temp = cycle_cnt;
    //                 }
    //                 if (!flag) return flag;
    //                 count++;
    //             }
    //             // update buffer value
    //             count = 0;
    //             while (count < COL_ACCESS_DELAY) {
    //                 if(MODE)
    //                     flag = executeIndependent();
    //                 if (temp == cycle_cnt) {
    //                     cycle_cnt++;
    //                     temp++;
    //                 }
    //                 else {
    //                     temp = cycle_cnt;
    //                 }
    //                 if (!flag) return flag;
    //                 count++;
    //                 if (count == COL_ACCESS_DELAY) {
    //                     // update the value in buffer
    //                     ROW_BUFFER[addr - row_start] = data;
    //                 }
    //             }
    //             ROW_BUFFER[addr - row_start] = data;
    //         }
    //     }
    //     // execution completed (both store word and other independent instructions)
    //     return true;
    // }

 
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

    bool executeIndependent(Queue &q) {
        int ins_code;
        bool flag;

        // check if PC is less than partition
        if (PC < PARTITION) {
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
                // lw, so do not do anything, as we have only one row buffer
            case 7:
                if(inPwrite(q, val3))
                    break;
                flag = raiseRequest(val1, val2, val3, q, 7);
                if(!flag) return flag;
                break;
                // sw, so do not do anything, as we have only one row buffer
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

    void buffer() {
        if (!isEmpty && doWriteback) {
            writebacks++;
            for (int i = 0; i < 1024; i++) {
                DRAM[start + i] = ROW_BUFFER[i];
            }
            cout << "Final writeback:\n";
            cout << "Cycle " << cycle_cnt + 1 << ": DRAM request issued\n";
            cout << "Cycle " << cycle_cnt + 2 << "-" << cycle_cnt + ROW_ACCESS_DELAY + 1 << ":" << " WRITEBACK: Copying from ROW BUFFER to DRAM (Row (Data section) : " << start - DATA_START << "-" << end - DATA_START << ")\n";
            cycle_cnt = cycle_cnt + ROW_ACCESS_DELAY + 1;
        }
    }
};


// this function simulates the execution of MIPS program
bool simulator(REGI& rf, Queue &q) {

    int ins_code;
    bool flag;

    // while program counter is less than PARTITION
    while (PC < PARTITION) {

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


// checks if a given line of code represents a MIPS label
pair<pair<string, string>, pair<bool, bool>> checkIfLabel(string line, int line_number) {

    int len = line.length(), i = 0;
    string label = "";

    // parse initial white spaces
    while (i < len && (line.at(i) == ' ' || line.at(i) == '\t')) i++;
    if (i == len)
        // no label
        return make_pair(make_pair(label, line), make_pair(false, true));

    // parse label
    while (i < len && line.at(i) != ' ' && line.at(i) != ':' && line.at(i) != '\t') {
        label = label + line[i];
        i++;
    }
    //parse any extra white spaces
    while (i < len && (line.at(i) == ' ' || line.at(i) == '\t')) i++;
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


// function used to parse a line of MIPS code (not containg any label) and generate appropriate arguments
pair<vector<string>, bool> parseInstruction(string instruction) {

    // args stores instruction arguments
    vector<string> args;
    // len stores length of the instruction
    int len = instruction.length();
    // instruction counter
    int i = 0;
    // maximum of 4 arguments can be present in a valid instruction
    string code = "", arg1 = "", arg2 = "", arg3 = "";

    // read all initial white spaces
    while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
    // return if there are only white spaces with true (no error)
    if (i == len)
        return make_pair(args, true);

    // else
    else {
        // read all non-whitespace, non-comma characters, store them in code
        while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',' && instruction.at(i) != '\t') {
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
            while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
            // if there are no more arguments, return error
            if (i == len)
                return make_pair(args, false);

            // else
            else {

                // read all non-whitespace, non-comma characters, store them in arg1                 
                while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',' && instruction.at(i) != '\t') {
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
                    while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
                    // if there are no more arguments, then it can be jump instruction, so return true
                    if (i == len)
                        return make_pair(args, true);

                    // else
                    else {
                        // read comma if any
                        if (instruction.at(i) == ',') i++;
                        // read all intermediary white spaces
                        while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
                        // if there are no more arguments, return error
                        if (i == len)
                            return make_pair(args, false);
                        // else
                        // read all non-white space, non-comma, non-LPAREN characters, store them in arg2
                        while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',' && instruction.at(i) != '(' && instruction.at(i) != '\t') {
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
                            while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
                            // if there are no more arguments, return error 
                            if (i == len)
                                return make_pair(args, false);
                            // else
                            else {
                                // read comma, if any
                                if (instruction.at(i) == ',') {
                                    i++;
                                    // read inter-mediary white spaces
                                    while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
                                    // if there are no more arguments, return error
                                    if (i == len)
                                        return make_pair(args, false);
                                    // read all non-whitespace, non-comma characters, store them in arg3
                                    while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ',' && instruction.at(i) != '\t') {
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
                                        while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
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
                                        while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
                                        // if there are no more arguments, return error
                                        if (i == len)
                                            return make_pair(args, false);
                                        // else
                                        // read all non-whitespace, non-RPAREN characters, store them in arg3
                                        while (i < len && instruction.at(i) != ' ' && instruction.at(i) != ')' && instruction.at(i) != '\t') {
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
                                            while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
                                            // if the next character is not a RPAREN, return error
                                            if (instruction.at(i) != ')')
                                                return make_pair(args, false);
                                            else i++;
                                            // else
                                            // read any trailing white spaces
                                            while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) i++;
                                            // if the instruction is completely parsed, return true, else return false
                                            if (i == len)
                                                return make_pair(args, true);
                                            else return make_pair(args, false);
                                        }
                                    }
                                    // neither LPAREN, nor a comma
                                    else {
                                        // read all non-whitespace characters, store them in arg3
                                        while (i < len && (instruction.at(i) != ' ' || instruction.at(i) == '\t')) {
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
                                            while (i < len && (instruction.at(i) == ' ' || instruction.at(i) == '\t')) {
                                                i++;
                                            }
                                            // if the instruction is completely parsed, return true, else return false
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




// function used to a line of MIPS code to memory, if it is an instruction
bool addToMemory(string line, REGI& rf, int line_number) {

    // args will be used to store the instruction arguments
    vector<string> args;
    bool flag;
    int len;
    // check if there is a label
    pair<pair<string, string>, pair<bool, bool>> label = checkIfLabel(line, line_number);
    if (label.second.second == false)
        // there was some error
        return false;
    else {
        // not a syntax error
        if (label.second.first == true) {
            // valid label
            // check if the label is already there
            if (rf.labels.find(label.first.first) != rf.labels.end()) {
                // syntax error (same label used for two different memory addresses)
                cout << "Error: Same label used to represent different addresses.\nLabel repeated: " << label.first.first << ": at line number: " << line_number << "\n";
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
        cout << "SYNTAX ERROR 1: At line number: " << line_number << ": " << line << "\n";
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
                cout << "SYNTAX ERROR 2: At line number: " << line_number << ": " << line << "\n";
                return false;
            }
            // else store the instruction in memory
            int ins = rf.ins_map[args[0]];
            DRAM[PC] = ins;
            // store the number of arguments in len
            len = args.size();

            //jump
            if (ins == 6) {
                if (len != 2) {
                    // as jump should have only 2 arguments
                    cout << "SYNTAX ERROR 3: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    if (!isInteger(args[1])) {
                        if (!validId(args[1])) {
                            // not a valid label address
                            cout << "SYNTAX ERROR 4: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        // else, it is a valid id, push it
                        rf.label.push_back(make_pair(args[1], make_pair(PC + 1, line_number)));
                    }
                    else {
                        long long addr = stoi(args[1]);
                        if (addr < 0 || addr >= 67108864) {
                            // address starts from 0 and max integer that can be stored is 2^26 - 1
                            cout << "SYNTAX ERROR 5: At line number: " << line_number << ": " << "\n";
                            return false;
                        }
                        // else store the value (absolute address) in memory
                        DRAM[PC + 1] = 4 * addr;
                    }
                }
            }

            //add sub mul slt
            else if (ins == 0 || ins == 1 || ins == 2 || ins == 5) {
                if (len != 4) {
                    // add sub mul slt require 4 arguments each
                    cout << "SYNTAX ERROR 6: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    // check if any of the register used is invalid
                    if (rf.reg_map.find(args[1]) == rf.reg_map.end() || rf.reg_map.find(args[2]) == rf.reg_map.end() || rf.reg_map.find(args[3]) == rf.reg_map.end()) {
                        // invalid register used, syntax error
                        cout << "SYNTAX ERROR 7: At line number: " << line_number << ": " << line << "\n";
                        return false;
                    }
                    else {
                        // store register numbers in memory
                        DRAM[PC + 1] = rf.reg_map[args[1]];
                        DRAM[PC + 2] = rf.reg_map[args[2]];
                        DRAM[PC + 3] = rf.reg_map[args[3]];
                        int temp = DRAM[PC + 1];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = DRAM[PC + 2];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = DRAM[PC + 3];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                    }
                }
            }

            //beq bne addi
            else if (ins == 3 || ins == 4 || ins == 9) {
                if (len != 4) {
                    // beq bne addi require 4 arguments each
                    cout << "SYNTAX ERROR 8: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    if (ins == 9) {
                        // check if any of the register used is invalid
                        // also check if the last argument is an immediate or not
                        if (rf.reg_map.find(args[1]) == rf.reg_map.end() || rf.reg_map.find(args[2]) == rf.reg_map.end() || !isInteger(args[3])) {
                            // invalid arguments, syntax error
                            cout << "SYNTAX ERROR 9: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        else {
                            long long immediate = stoi(args[3]);

                            // raise overflow error if necessary (only 16 bit immediate)
                            if (immediate < -32768 || immediate > 32767) {
                                cout << "Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                                return false;
                            }
                            // else
                            // store register numbers and immediate in memory
                            DRAM[PC + 1] = rf.reg_map[args[1]];
                            DRAM[PC + 2] = rf.reg_map[args[2]];
                            DRAM[PC + 3] = immediate;
                            int temp = DRAM[PC + 1];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                            temp = DRAM[PC + 2];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                        }
                    }
                    else {
                        // bne or beq
                        // check if any of the register used is invalid
                        if (rf.reg_map.find(args[1]) == rf.reg_map.end() || rf.reg_map.find(args[2]) == rf.reg_map.end()) {
                            // invalid arguments, syntax error
                            cout << "SYNTAX ERROR 10: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        else {
                            // store register
                            DRAM[PC + 1] = rf.reg_map[args[1]];
                            DRAM[PC + 2] = rf.reg_map[args[2]];
                            int temp = DRAM[PC + 1];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                            temp = DRAM[PC + 2];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                            if (isInteger(args[3])) {
                                long long immediate = stoi(args[3]);

                                // raise overflow error if necessary
                                if (immediate < -32768 || immediate > 32767) {
                                    cout << "Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                                    return false;
                                }
                                // else
                                // store immediate (absolute jump address) in memory
                                DRAM[PC + 3] = PC + 4 + 4 * immediate;
                            }
                            else {
                                if (validId(args[3])) {
                                    rf.label.push_back(make_pair(args[3], make_pair(PC + 3, line_number)));
                                }
                                else {
                                    // invalid label, raise error
                                    cout << "SYNTAX ERROR 11: At line number: " << line_number << ": " << line << "\n";
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
                    cout << "SYNTAX ERROR 12: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    // check if the registers are valid or not, also check if the second argument is an immediate or not
                    if (rf.reg_map.find(args[1]) == rf.reg_map.end() || !isInteger(args[2]) || rf.reg_map.find(args[3]) == rf.reg_map.end()) {
                        // invalid arguments, syntax error
                        cout << "SYNTAX ERROR 13: At line number: " << line_number << ": " << line << "\n";
                        return false;
                    }
                    else {
                        long long immediate = stoi(args[2]);

                        // raise overflow error if necessary
                        if (immediate < -32768 || immediate > 32767) {
                            cout << "Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                            return false;
                        }
                        // store register numbers and immediate in memory
                        DRAM[PC + 1] = rf.reg_map[args[1]];
                        DRAM[PC + 2] = immediate;
                        DRAM[PC + 3] = rf.reg_map[args[3]];
                        int temp = DRAM[PC + 1];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = DRAM[PC + 3];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                    }
                }
            }
        }
    }
    // increment program counter by 4 (next storage address)
    PC += 4;
    // store the instruction as well
    rf.instructions.push_back(line);
    // instruction stored successfully
    return true;
}


// function used to resolve labels
bool linker(REGI& rf) {

    // label stores all the unresolved labels
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
            DRAM[label.second.first] = label_loc;
        }

    }
    // all labels resolved
    return true;
}

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
            if (ROW_ACCESS_DELAY < 0 || COL_ACCESS_DELAY < 0) {
                // invalid delay (negative)
                cout << "ERROR: DRAM delay is given to be negative. Program terminating!\n";
                return 0;
            }
        }
        if (argc >= 5) {
            if (!isInteger(argv[4])) {
                // mode is not an integer
                cout << "ERROR: Operating mode is not an integer. Either enter 0 or 1. Program terminating!\n";
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
     
