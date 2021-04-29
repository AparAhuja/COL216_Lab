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

// ROW_ACCESS_DELAY and COL_ACCESS_DELAY
int ROW_ACCESS_DELAY, COL_ACCESS_DELAY;

// flag used to check compiler optimisations (such as removing redundant code)
bool compilerOptimisation = false;

// memory available (2 ^ 20 bytes), used to model 2D DRAM (row major)
int DRAM[1048576];

int N, M;

// vector to store memory locations which were used, to display their contents at the end
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

enum RequestType {
    SW,     // 0
    LW      // 1
};

struct Request {
    int memAddr;         // absolute address
    int memRow;          // absolute row number
    int storeData;
    int writeDest;
    int type;
    int insMemAddr;      // relative address of instruction (lw/sw) in instruction memory                 
    int coreNum;         // core number, 1 <= coreNum <= N
    string instruction;  // the instruction which invoked the request
    int nextPointer;
    int prevPointer;
};


struct Core {
    

    int PC;                       // program counter
    int reg[32];                  // register file
    bool writeActive;             // control signal that tells whether the write port is active or not
    Request buffer[32];           // request buffer, can store up to 32 requests simultaneously
    int headPointer, tailPointer; // head and tail counters (buffer)
    int requestCounter;           // equals the number of request in the buffer
    bool isNotEmpty;              // control signal that tells whether the buffer is empty or not
    bool isFull;                  // control signal that tells whether the buffer is full or not
    bool isWorking;               // control signal that tells whether the core is working or not

    int insMemory[524288];           // instruction memory available to core, 2^19 bytes (2^17 instructions)
    int PARTITION;                // instructions are stored from address 0 to PARTITION - 1
    int DATA_START, DATA_END;     // starting and ending address of the DRAM allocated to the core
    int regPendingWrite[32];      // stores the number of pending writes on the register file
    int freeLocBuffer[32];        // indexes of free locations in buffer
    int start, end;               // start and end of the freeLocBuffer queue (circular)

    // execution parameters
    int insCount[10];             // stores the number of times each category of instruction was executed
    int cycleCount;               // number of cycles that the core worked for
    int coreNum;                  // core number
    vector<string> instructions;  // stores the instructions used in source program
    
    // maps used to do the linking
    map<string, int> labels;
    vector<pair<string, pair<int, int>>> label;

    // maps used to do the printing, output of program
    map<string, int> ins_map = { {"add", 0}, {"sub", 1}, {"mul", 2},{"beq", 3},{"bne", 4},{"slt", 5},{"j", 6},{"lw", 7},{"sw", 8},{"addi", 9} };

    map<string, int> reg_map = { {"$zero", 0}, {"$at", 1}, {"$v0", 2}, {"$v1", 3}, {"$a0", 4}, {"$a1", 5}, {"$a2",6}, {"$a3", 7}, {"$t0", 8}, {"$t1", 9}, {"$t2", 10},
                                 {"$t3", 11}, {"$t4", 12}, {"$t5", 13}, {"$t6", 14}, {"$t7", 15}, {"$s0", 16}, {"$s1", 17}, {"$s2", 18}, {"$s3", 19}, {"$s4", 20}, {"$s5", 21},
                                 {"$s6", 22}, {"$s7", 23}, {"$t8", 24}, {"$t9", 25}, {"$k0", 26}, {"$k1", 27}, {"$gp", 28}, {"$sp", 29}, {"$fp", 30}, {"$ra", 31} };

    map<int, string> num_reg = { {-1, "dummy"}, {0, "$zero"}, {1, "$at"}, {2, "$v0"}, {3, "$v1"}, {4, "$a0"}, {5, "$a1"}, {6, "$a2"}, {7, "$a3"}, {8, "$t0"}, {9, "$t1"}, {10, "$t2"},
                                 {11, "$t3"}, {12, "$t4"}, {13, "$t5"}, {14, "$t6"}, {15, "$t7"}, {16, "$s0"}, {17, "$s1"}, {18, "$s2"}, {19, "$s3"}, {20, "$s4"}, {21, "$s5"},
                                 {22, "$s6"}, {23, "$s7"}, {24, "$t8"}, {25, "$t9"}, {26, "$k0"}, {27, "$k1"}, {28, "$gp"}, {29, "$sp"}, {30, "$fp"}, {31, "$ra"} };  

    // functions
    Core();
    void execution_stats();
    void stat_update(int insNum);
    //void optimise();
    bool addRequest(int r1, int offset, int r2, int insNum);
    void deleteRequest();
    bool add(int r1, int r2, int r3);
    bool sub(int r1, int r2, int r3);
    bool mul(int r1, int r2, int r3);
    void slt(int r1, int r2, int r3);
    bool addi(int r1, int r2, int immediate);
    bool beq(int r1, int r2,  int offset);
    bool bne(int r1, int r2, int offset);
    bool j(int jumpAddr);
    bool lw(int r1, int offset, int r2);
    bool sw(int r1, int offset, int r2);


};

Core::Core() {
    
    PC = 0; PARTITION = 0;
    DATA_START = 0; DATA_END = 0;
    headPointer = 0; tailPointer = 0;
    requestCounter = 0;
    cycleCount = 0; coreNum = 0;
    isNotEmpty = false;
    isWorking = true;
    writeActive = false;
    start = 0; end = 31;

    for(int i = 0; i < 32; i ++) {
        reg[i] = 0;
    }    
    for(int i = 0; i < 524288; i ++) {
        insMemory[i] = 0;
    }
    for(int i = 0; i < 32; i ++) {
        regPendingWrite[i] = 0;
    }
    for(int i = 0; i < 10; i ++) {
        insCount[i] = 0;
    }
    for(int i = 0; i < 32; i ++) {
        freeLocBuffer[i] = i;
    }
}

void Core::execution_stats() {
    // print execution statistics
    std::cout << "______________________________________________________________________________________________________\n\n";
    std::cout << "Total clock cycles: " << cycleCount << "\n\n";
    std::cout << "Number of instructions executed for each type are given below:-\n";
    int j = 1;
    for (auto i = ins_map.begin(); i != ins_map.end(); i++) {
        std::cout << (i->first) << ": " << insCount[(i->second)];
        if (j % 5 == 0) std::cout << "\n";
        else std::cout << ", ";
        j++;
    }
}

void Core::stat_update(int insNum) {
    insCount[insNum] ++;
}

bool Core::add(int dest, int src1, int src2) {
    long long sum = (long long)reg[src1] + (long long)reg[src2];
    stat_update(0);

    if (sum > 2147483647 || sum < -2147483648) {
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
        return false;
    }
    else {
        reg[dest] = sum;
        // check if it is the zero register
        if (dest == 0)
            reg[dest] = 0;
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";

        return true;
    }
}

bool Core::sub(int dest, int src1, int src2) {
    long long diff = (long long)reg[src1] - (long long)reg[src2];
    stat_update(1);

    if (diff > 2147483647 || diff < -2147483648) {
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
        return false;
    }
    else {
        reg[dest] = diff;
        // check if it is the zero register
        if (dest == 0)
            reg[dest] = 0;
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";

        return true;
    }
}

bool Core::mul(int dest, int src1, int src2) {
    long long prod = (long long)reg[src1] * (long long)reg[src2];
    stat_update(2);

    if (prod > 2147483647 || prod < -2147483648) {
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
        return false;
    }
    else {
        reg[dest] = prod;
        // check if it is the zero register
        if (dest == 0)
            reg[dest] = 0;
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";

        return true;
    }
}

bool Core::beq(int src1, int src2, int jumpto) {
    int temp = PC;
    stat_update(3);    
    if (reg[src1] == reg[src2]) {
        PC = jumpto;
        if (PC > PARTITION || PC < 0) {
            std::cout << "Cycle " << cycleCount << ":\n";
            std::cout << "Instruction executed: " << instructions[temp / 4] << "\n";
            std::cout << "Memory Address of Instruction: " << temp << "\n";
            std::cout << "Warning: Program jumped to a non-instruction memory location. Executing pending DRAM requests (if any).\n\n";
            return false;
        }
    }
    else PC += 4;
    std::cout << "Cycle " << cycleCount << ":\n";
    std::cout << "Instruction executed: " << instructions[temp / 4] << "\n";
    std::cout << "Memory Address of Instruction: " << temp << "\n\n";
    return true;
}

bool Core::bne(int src1, int src2, int jumpto) {
    int temp = PC;
    stat_update(4);    
    if (reg[src1] != reg[src2]) {
        PC = jumpto;
        if (PC > PARTITION || PC < 0) {
            std::cout << "Cycle " << cycleCount << ":\n";
            std::cout << "Instruction executed: " << instructions[temp / 4] << "\n";
            std::cout << "Memory Address of Instruction: " << temp << "\n";
            std::cout << "Warning: Program jumped to a non-instruction memory location. Executing pending DRAM requests (if any).\n\n";
            return false;
        }
    }
    else PC += 4;
    std::cout << "Cycle " << cycleCount << ":\n";
    std::cout << "Instruction executed: " << instructions[temp / 4] << "\n";
    std::cout << "Memory Address of Instruction: " << temp << "\n\n";
    return true;
}

void Core::slt(int dest, int src1, int src2) {
    reg[dest] = (reg[src1] < reg[src2]) ? 1 : 0;
    // check if it is the zero register
    if (dest == 0)
        reg[dest] = 0;
    stat_update(5);
    std::cout << "Cycle " << cycleCount << ":\n";
    std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
    std::cout << "Memory Address of Instruction: " << PC << "\n";
    std::cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";
}

bool Core::j(int jumpAddr) {
    int temp = PC;
    PC = jumpAddr;
    stat_update(6);
    if (PC > PARTITION) {
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[temp / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << temp << "\n";
        std::cout << "Warning: Program jumped to a non-instruction memory location. Executing pending DRAM requests (if any).\n\n";
        return false;
    }
    else {
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[temp / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << temp << "\n\n";
        return true;
    }
}

bool Core::lw(int r1, int offset, int r2) {
    bool flag;

    flag = addRequest(r1, offset, r2, 7);
    regPendingWrite[r1] ++;

    return flag;
}

bool Core::sw(int r1, int offset, int r2) {
    bool flag;

    flag = addRequest(r1, offset, r2, 8);

    return flag;
}

bool Core::addi(int dest, int src, int adds) {
    long long sum = (long long)reg[src] + (long long)adds;
    stat_update(9);

    if (sum > 2147483647 || sum < -2147483648) {
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
        return false;
    }
    else {
        reg[dest] = sum;
        // check if it is the zero register
        if (dest == 0)
            reg[dest] = 0;
        std::cout << "Cycle " << cycleCount << ":\n";
        std::cout << "Instruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "Register modified: " << num_reg[dest] << " = " << reg[dest] << " (0x" << decimalToHexadecimal(reg[dest]) << ")\n\n";
        return true;
    }
}

bool Core::addRequest(int r1, int offset, int r2, int ins_num) {
    // check if address is valid
    long long addr = (long long)offset + (long long)reg[r2] + (long long)DATA_START;
    if (addr >= DATA_END || addr < DATA_START) {
        // invalid address, error
        std::cout << "Core " + to_string(coreNum) +" raised Error: Program is trying to access an unavailable memory location. Program terminating!\n\n";
        return false;
    }
    else {
        if (addr % 4 != 0) {
            // address is not word aligned, error
            std::cout << "Core " + to_string(coreNum) +" raised Error: Memory address is not word-aligned. Invalid load operation. Program Terminating!\n\n";
            return false;
        }
    }
    // else
    
    // update cycle count and instruction stats, request issued to DRAM
    stat_update(ins_num);
    std::cout << "Cycle " << cycleCount << ": ";
    if(r1 == 0 && ins_num == 7) {
        std::cout << "\nInstruction executed: " << instructions[PC / 4] << "\n";
        std::cout << "Memory Address of Instruction: " << PC << "\n";
        std::cout << "No DRAM request issued; $zero register!\n";
        std::cout << "Register modified: " << num_reg[r1] << " = " << reg[r1] << " (0x" << decimalToHexadecimal(reg[r1]) << ")\n\n";
        PC += 4;
        return true;
    }
    std::cout << "DRAM request issued for Instruction: " << instructions[PC / 4] << "\n";
    std::cout << "Memory Address of Instruction: " << PC << "\n\n";
    // add request to core buffer
    if(ins_num == 7) {
        Request req = {(int)addr, (int)addr / 1024, 0, r1, LW, PC, coreNum, instructions[PC / 4], 0, 0};
        if(isNotEmpty) {
            // increment the number of requests
            requestCounter ++;
            // use up a location
            if(start == 31)
                start = 0;
            else start ++;
            // previous pointer to the last element
            req.prevPointer = tailPointer;
            // assign next pointer
            if(requestCounter == 32) {
                isFull = true;
                req.nextPointer = -1;
            }
            else {
                // next free location
                req.nextPointer = freeLocBuffer[start];
            }
            // fill up an empty location with req
            buffer[buffer[tailPointer].nextPointer] = req;
            // update tail pointer
            tailPointer = buffer[tailPointer].nextPointer;
        }
        else {
            // buffer is empty
            // pick a free location
            headPointer = freeLocBuffer[start];
            tailPointer = headPointer;
            // increment the number of requests
            requestCounter ++;
            // use up a location
            if(start == 31)
                start = 0;
            else start ++;
            // update previous and next pointers
            req.prevPointer = -1;
            req.nextPointer = freeLocBuffer[start];
            // store request
            buffer[headPointer] = req;
            isNotEmpty = true;
            cout << "Gi\n";
        }
    }    
    else {  
        Request req = {(int)addr, (int)addr / 1024, reg[r1], 0, SW, PC, coreNum, instructions[PC / 4], 0, 0};
        if(isNotEmpty) {
            // increment the number of requests
            requestCounter ++;
            // use up a location
            if(start == 31)
                start = 0;
            else start ++;
            // previous pointer to the last element
            req.prevPointer = tailPointer;
            // assign next pointer
            if(requestCounter == 32) {
                isFull = true;
                req.nextPointer = -1;
            }
            else {
                // next free location
                req.nextPointer = freeLocBuffer[start];
            }
            // fill up an empty location with req
            buffer[buffer[tailPointer].nextPointer] = req;
            // update tail pointer
            tailPointer = buffer[tailPointer].nextPointer;
        }
        else {
            // buffer is empty
            // pick a free location
            headPointer = freeLocBuffer[start];
            tailPointer = headPointer;
            // increment the number of requests
            requestCounter ++;
            // use up a location
            if(start == 31)
                start = 0;
            else start ++;
            // update previous and next pointers
            req.prevPointer = -1;
            req.nextPointer = freeLocBuffer[start];
            // store request
            buffer[headPointer] = req;
            isNotEmpty = true;
            cout << "Gir\n";
        }
    }
    // increment PC by 4
    PC += 4;
    return true;     
}

void Core::deleteRequest() {
    // free up the head pointer
    if(end == 31) end = 0;
    else end ++;
    Request req = buffer[headPointer];
    // remove dependency
    if(req.type == LW) {
        regPendingWrite[req.writeDest] --;
    } 
    freeLocBuffer[end] = headPointer;
    // decrement the number of requests
    requestCounter --;
    std::cout << "Hello\n";
    if(requestCounter == 0) {
        // update head and tail pointers
        headPointer = freeLocBuffer[start];
        tailPointer = freeLocBuffer[start];
        isNotEmpty = false;
    }
    else {
        // update only head pointer
        headPointer = buffer[headPointer].nextPointer;
        buffer[headPointer].prevPointer = -1;
    }
    isFull = false;
}


struct Manager {
    
    int rowNum;          // current row present in DRAM row buffer. -1 if empty
    int coreNum;         // core, request of which is being processed by DRAM
    bool isIdle;         // flag that tells if the MRM is idle or not
    bool foundNextReq;   // control signal that tells that the manager found the next request
    vector<Core*> cores;  // all the cores
    Request* currReq;    // request being currently processed by DRAM
    Request* nextReq;    // next request which should be processed by DRAM
    int searchPointer;   // used to search for next request
    int currentPointer;  // pointer to current index
    bool isSearching;    // flag that tells whether manager is searching or not

    int dependency[32];

    Manager(vector<Core> &cores);
    void refresh();
    void firstRequest();
    void searchRequest();
};


Manager::Manager(vector<Core> &cores) {
    rowNum = -1;
    coreNum = 0;
    isIdle = true;
    foundNextReq = false;
    searchPointer = -1;
    currReq = NULL;
    nextReq = NULL;
    isSearching = false;
    refresh();
    this->cores.resize(cores.size());
    for(int i = 0; i < cores.size(); i ++) {
        this->cores[i] = &cores[i];
    }
}

void Manager::refresh() {
    for(int i = 0; i < 32; i ++) {
        dependency[i] = 0;
    }
}

// single clock cycle (executed when both currReq and nextReq are NULL)
void Manager::firstRequest() {
    if(coreNum != 0 && (*cores[coreNum - 1]).isNotEmpty) {
        if((*cores[coreNum - 1]).buffer[(*cores[coreNum - 1]).headPointer].memRow == rowNum) {
            foundNextReq = true;
            currentPointer = (*cores[coreNum - 1]).headPointer;
            nextReq = &(*cores[coreNum - 1]).buffer[(*cores[coreNum - 1]).headPointer];
            return;
        }
    }
    for(int i = 0; i < cores.size(); i ++) {
        std::cout << (*cores[i]).isNotEmpty << "\n";
        if(!(*cores[i]).isNotEmpty) continue;
        else {
            coreNum = i + 1;
            foundNextReq = true;
            currentPointer = (*cores[i]).headPointer;
            nextReq = &(*cores[i]).buffer[(*cores[i]).headPointer];
            std::cout << "got in\n";
            break;
        }
    }
}

// searches for next request to send to the DRAM, can take multiple clock cycles
void Manager::searchRequest() {
    int i = 0;
    searchPointer = (*cores[coreNum - 1]).buffer[currentPointer].nextPointer;
    std::cout << searchPointer;
    while(searchPointer != -1 && i < 16) {
        Request req = (*cores[coreNum - 1]).buffer[searchPointer];
        if(req.memRow == rowNum) {
            // possible next request
            if(req.type == SW) {
                // next request confirmed
                nextReq = &req;
                foundNextReq = true;
                currentPointer = searchPointer;
                return;
            }
            else {
                // req.type == LW
                // check for dependency
                if(dependency[req.writeDest] != 0) {
                    // re ordering not possible
                    dependency[req.writeDest] ++;
                    currentPointer = searchPointer;
                    searchPointer = (*cores[coreNum - 1]).buffer[searchPointer].nextPointer;
                }
                else {
                    // next request confirmed
                    nextReq = &req;
                    foundNextReq = true;
                    currentPointer = searchPointer;
                    return;
                }
            }
        }
        else {
            if(req.type == LW) {
                dependency[req.writeDest] ++;
            }
            currentPointer = searchPointer;
            searchPointer = (*cores[coreNum - 1]).buffer[searchPointer].nextPointer;
        }
    }
}

#include "parsing.hpp"

bool checkPC(vector<Core> &cores){
    
    bool flag = false;
    for(int i = 0; i < N; i++){
        if(cores[i].isWorking) flag = true;
    }

    return flag;
}


// this function simulates the execution of MIPS program
bool simulator(vector<Core> &cores) {

    // code of instruction which needs to be executed
    int ins_code;
    // error reporting flag
    bool flag;
    // three possible values
    int val1, val2, val3;

    // cycle count
    int cycle = 0;
    // memory request manager
    Manager reqManager(cores);
    int completionCycleMan = 0;

    // dram parameters
    bool dramIsIdle = true;
    bool dramWriteback = false;
    int completionCycleDRAM = 0;
    int start = -1;
    bool isEmpty = true, doWriteback = false;
    // execution statistics (DRAM)
    int writebacks = 0, copies = 0, value_write = 0, value_read = 0;
    // DRAM row buffer
    int ROW_BUFFER[1024];
    
    // parameters of request to be processed by the DRAM  
    Request req;
    int row_start, addr, src, data, i, type;
    int data_bus = -1;
    bool deleteReq = false;
    // while there is a functioning core
    while (checkPC(cores) && cycle < M) {
        // increment cycle count by 1
        std::cout << reqManager.searchPointer << "jnsd\n";
        cycle ++;
        deleteReq = false;
        // check if dram is idle, pick up a request from memory request manager
        if(dramIsIdle) {
            if(reqManager.nextReq != NULL) {
                std::cout << "kon";
                // make the next request as current request
                reqManager.currReq = reqManager.nextReq;
                reqManager.nextReq = NULL;
                // store the request in req
                req = *(reqManager.currReq);
                i = req.coreNum - 1;
                row_start = req.memRow * 1024; // absolute row number
                addr = req.memAddr;            // absolute memory address
                src = req.writeDest;           // lw
                data = req.storeData;          // sw
                type = req.type;
                // update statistics
                if(type == SW)
                    value_write++;
                else if(type == LW)
                    value_read++;

                if(start == row_start) {
                    if(type == SW)
                        doWriteback = true;
                    completionCycleDRAM = cycle + COL_ACCESS_DELAY - 1;
                }
                else {
                    if(type == SW) {
                        doWriteback = true;
                    }
                    else {
                        doWriteback = false;
                    } 
                    completionCycleDRAM = cycle + ROW_ACCESS_DELAY + COL_ACCESS_DELAY - 1;
                    // copy new row into the buffer
                    for (int i = 0; i < 1024; i++) {
                        ROW_BUFFER[i] = DRAM[row_start + i];
                    }
                    copies++;
                }  
                std::cout << "ho\n";
                start = row_start;
                isEmpty = false; 
                vector<int>::iterator iter = find(memoryAddress.begin(), memoryAddress.end(), addr);
                if (iter == memoryAddress.end()) {
                    memoryAddress.push_back(addr);
                }                             
                dramIsIdle = false;

            }
            else {
                // no request sent by manager
                completionCycleDRAM = 0;
                // MRM checks for any requests
                if(reqManager.searchPointer != -1) {
                    reqManager.searchRequest();
                }
                else {
                    reqManager.firstRequest();
                    if(reqManager.nextReq != NULL) {
                        if(reqManager.rowNum != -1) {
                            dramWriteback = true;
                            dramIsIdle = false;
                            completionCycleDRAM = cycle + ROW_ACCESS_DELAY - 1;
                            for(int i = 0; i < 1024; i ++) {
                                DRAM[start + i] = ROW_BUFFER[i];
                            }
                        }
                        else {
                            dramIsIdle = true;
                        }    
                    }
                }    
            }
        }
        else {
            if(dramWriteback) {
                if(cycle == completionCycleDRAM) {
                    dramWriteback = false;
                    dramIsIdle = true;
                    completionCycleDRAM = 0;
                }
            }
            else {
                std::cout << "kaho\n";
                if(cycle == completionCycleDRAM) {
                    if(type == SW) {
                        // write into buffer
                        ROW_BUFFER[addr + row_start] = data;
                    }
                    else {
                        cores[i].reg[src] = (src == 0) ? 0 : ROW_BUFFER[addr - row_start];
                        cores[i].writeActive = true;
                    }
                    reqManager.currReq = NULL;
                    deleteReq = true;
                    dramIsIdle = true;
                    completionCycleDRAM = 0;
                }
                else {
                    if(reqManager.nextReq == NULL || !reqManager.foundNextReq) {
                        // MRM searches for next request
                        std::cout << "hls" << reqManager.searchPointer << "\n";
                        reqManager.searchRequest();
                    }
                }
            }
        }
        

        for(int i = 0; i < N; i++){
            if(!cores[i].isWorking) continue; 
            // increment core cycle count
            cores[i].cycleCount ++;
            // get instruction code
            if(cores[i].PC >= cores[i].PARTITION) {
                if(!cores[i].isNotEmpty)
                    cores[i].isWorking = false;
               continue;
            }    
            ins_code = cores[i].insMemory[cores[i].PC];
            val1 = cores[i].insMemory[cores[i].PC + 1]; val2 = cores[i].insMemory[cores[i].PC + 2]; val3 = cores[i].insMemory[cores[i].PC + 3];
            switch (ins_code) {
                // add
                case 0:
                    if (cores[i].regPendingWrite[val1] != 0 || cores[i].regPendingWrite[val2] != 0 || cores[i].regPendingWrite[val3] != 0 || cores[i].writeActive) {
                        // don't do anything, break
                        break;
                    }    
                    flag = cores[i].add(val1, val2, val3);
                    if (flag) cores[i].PC += 4;
                    else cores[i].isWorking = flag;
                    break;
                    // sub
                case 1:
                    if (cores[i].regPendingWrite[val1] != 0 || cores[i].regPendingWrite[val2] != 0 || cores[i].regPendingWrite[val3] != 0 || cores[i].writeActive) {
                        // don't do anything, break
                        break;
                    } 
                    flag = cores[i].sub(val1, val2, val3);
                    if (flag) cores[i].PC += 4;
                    else cores[i].isWorking = flag;
                    break;
                    // mul
                case 2:
                    if (cores[i].regPendingWrite[val1] != 0 || cores[i].regPendingWrite[val2] != 0 || cores[i].regPendingWrite[val3] != 0 || cores[i].writeActive) {
                        // don't do anything, break
                        break;
                    } 
                    flag = cores[i].mul(val1, val2, val3);
                    if (flag) cores[i].PC += 4;
                    else cores[i].isWorking = flag;
                    break;
                    // beq
                case 3:
                    if (cores[i].regPendingWrite[val1] != 0 || cores[i].regPendingWrite[val2] != 0) {
                        // don't do anything, break
                        break;
                    } 
                    flag = cores[i].beq(val1, val2, val3);
                    if (!flag) cores[i].isWorking = flag;
                    break;
                    // bne
                case 4:
                    if (cores[i].regPendingWrite[val1] != 0 || cores[i].regPendingWrite[val2] != 0) {
                        // don't do anything, break
                        break;
                    } 
                    flag = cores[i].bne(val1, val2, val3);
                    if (!flag) cores[i].isWorking = flag;
                    break;
                    // slt
                case 5:
                    if (cores[i].regPendingWrite[val1] != 0 || cores[i].regPendingWrite[val2] != 0 || cores[i].regPendingWrite[val3] != 0 || cores[i].writeActive) {
                        // don't do anything, break
                        break;
                    } 
                    cores[i].slt(val1, val2, val3);
                    cores[i].PC += 4;
                    break;
                    // j
                case 6:
                    flag = cores[i].j(val1);
                    if (!flag) cores[i].isWorking = flag;
                    break;
                case 7:
                    if (cores[i].regPendingWrite[val3] != 0) {
                        // don't do anything, break
                        break;
                    } 
                    flag = cores[i].lw(val1, val2, val3);
                    if(!flag) cores[i].isWorking = flag;
                    break;
                case 8:
                    if (cores[i].regPendingWrite[val1] != 0 || cores[i].regPendingWrite[val3] != 0) {
                        // don't do anything, break
                        break;
                    } 
                    flag = cores[i].sw(val1, val2, val3);
                    if(!flag) cores[i].isWorking = flag;
                    break;
                    // addi                
                case 9:
                    if (cores[i].regPendingWrite[val1] != 0 || cores[i].regPendingWrite[val2] != 0 || cores[i].writeActive) {
                        // don't do anything, break
                        break;
                    } 
                    flag = cores[i].addi(val1, val2, val3);
                    if (flag) cores[i].PC += 4;
                    else cores[i].isWorking = flag;
                    break;
            }
        }
        if(deleteReq)
            cores[i].deleteRequest();

    }
    std::cout << "PROGRAM SIMULATION ENDED.\n";
    if (!isEmpty && doWriteback) {
        writebacks++;
        for (int i = 0; i < 1024; i++) {
            DRAM[start + i] = ROW_BUFFER[i];
        }
        std::cout << "Executing pending writeback:\n";
        std::cout << "Cycle " << cycle + 1 << ": DRAM request issued\n";
        std::cout << "Cycle " << cycle + 2 << "-" << cycle + ROW_ACCESS_DELAY + 1 << ":" << " WRITEBACK: Copying from ROW BUFFER to DRAM (Row (Data section) : " << start << "-" << start + 1023 << ")\n";
        cycle = cycle + ROW_ACCESS_DELAY + 1;
    }
    for(int i = 0; i < N; i ++) {
        std::cout << "Core " << i + 1 << "\n";
        cores[i].execution_stats();
    }
    int n = memoryAddress.size();
    if (n != 0) {
        sort(memoryAddress.begin(), memoryAddress.end());
        std::cout << "\nMemory content at the end of the execution (Data section):\n";
        for (int i = 0; i < n; i++) {
            std::cout << memoryAddress[i] << "-" << memoryAddress[i] + 3 << " = " << DRAM[memoryAddress[i]] << " (0x" << decimalToHexadecimal(DRAM[memoryAddress[i]]) << ")\n";
        }
    }
    std::cout << "\nTotal ROW BUFFER operations (writeback/activation/read/write): " << writebacks + copies + value_write + value_read << "\n";
    std::cout << "Number of times data was written back on DRAM from ROW BUFFER (WRITEBACK): " << writebacks << "\n";
    std::cout << "Number of times data was copied from DRAM to ROW BUFFER (ACTIVATION): " << copies << " (ROW BUFFER update)\n";
    std::cout << "Number of times data was written on ROW BUFFER (WRITE):" << value_write << " (ROW BUFFER update)\n";
    std::cout << "Number of times data was read from ROW BUFFER (READ):" << value_read << "\n\n";
    std::cout << "______________________________________________________________________________________________________\n\n";
    return true;
}

int main(int argc, char** argv) {
    
    string file_path;

    if (argc >= 5) {
        // raise warning if extra arguments are provided
        if ((argc > 5 && !isInteger(argv[5])) || argc > 6) {
            std::cout << "WARNING: Extra command line arguments were provided! Four/Five integer arguments are expected from the user.\nRefer to README.md for more details..\n";
        }
        else if(argc > 5 && isInteger(argv[5])) {
            int n = stoi(argv[5]);
            if(n == 0) {
                compilerOptimisation = false;
            }
            else {
                compilerOptimisation = true;
            }
        }
        // store the command line arguments
        // file name
        
        if (!isInteger(argv[1]) || !isInteger(argv[2]) || !isInteger(argv[3]) || !isInteger(argv[4])) {
            std::cout << "ERROR: Some arguments are non-integers. Integer arguments are expected. Program terminating!\n";
            return 0;
        }
        else {
            N = stoi(argv[1]);
            M = stoi(argv[2]);
            ROW_ACCESS_DELAY = stoi(argv[3]);
            COL_ACCESS_DELAY = stoi(argv[4]);
            if (M <= 0 || N <= 0 || ROW_ACCESS_DELAY <= 0 || COL_ACCESS_DELAY <= 0) {
                std::cout << "ERROR: Some argument are negative or zero. Positive arguments are expected. Program terminating!\n";
                return 0;
            }
        }
    }
    else {
        // insufficient number of arguments
        std::cout << "ERROR: Insufficient number of arguments were provided! Four arguments are expected from the user.\nRefer to README.md for more details..\n";
        std::cout << "Program terminating!\n";
        return 0;
    }
    // read from N files
    vector<ifstream> infile(N);
    // create N cores
    vector<Core> cores(N);
    
    for(int i = 0; i < N; i++){
        infile[i].open("t"+to_string(i+1)+".txt");
        if (!infile[i].is_open()) {
            cores[i].isWorking = false;
            std::cout << "Core "+to_string(i+1)+" raised ERROR: Unable to open file t" + to_string(i+1)+".txt.!\n";
        }
        cores[i].coreNum = i + 1;
        cores[i].DATA_START = i * (1024 / N) * 1024;
        if(i != N - 1)
            cores[i].DATA_END = (i + 1) * (1024 / N) * 1024;
        else
            cores[i].DATA_END = 1024 * 1024;
    }

    string line;      // string used to store an instruction
    bool flag;        // error reporting flag
    int line_number;  // line number in file


    // parse all files
    for(int i = 0; i < N; i++){
        if(!cores[i].isWorking)
            continue;
        line_number = 1;       // initialize line number to 1
       
        //read file, line by line
        while (getline(infile[i], line)) {
            if(cores[i].PC >= 524288){
                std::cout<<"Core " + to_string(i+1) << " raised ERROR: Instruction memory overflow.\n";
                cores[i].isWorking = false;
                break;
            }
            //process and store each instruction in memory
            flag = addToMemory(line, cores[i], line_number, i + 1);
            cores[i].isWorking = flag;

            // check if there was some error
            if(!flag)
                break;
            // read next line
            line_number++;
        }
        if(!cores[i].isWorking)
            continue;

        // initialize PARTITION to PC
        // PARTITION denotes end of stored instructions
        cores[i].PARTITION = cores[i].PC;

        // link labels used in program
        flag = linker(cores[i], i + 1);
        cores[i].isWorking = flag;
        // re-initialize PC to 0 for execution of MIPS program
        cores[i].PC = 0;
    }

    // optimise code
    // if(compilerOptimisation) {
    //     for(int i = 0; i < N; i ++) {
    //         cores[i].optimise();
    //     }
    // }
    // execute the program

    flag = simulator(cores);

    if (flag)
        std::cout << "\nProgram executed successfully!\n\n";

    // close the input file stream
    for(int i = 0; i < N; i++){
        infile[i].close();
    }

    return 0;
}
     
