#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;

// default file path used
string file_path = "/Users/aparahuja/Desktop/Test Cases/Overlapping/overlap_LARGE.txt";

// program counter (starts at 0)
int PC = 0;
// instruction end address
int PARTITION = 0;
// starting address of data section

int DATA_START = 524288;
// memory available (2 ^ 20 bytes)

//int memory[1048576];
int memory[1024][1024];
int row_buffer[1025];
int buffer_update_cnt = 0;
int row_delay = 10, col_delay = 2;
// - > vector<int> each_ins_cnt;

bool part1 = false;

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
    map<int, string> rev_map = { {0,"$zero"}, {1,"$at"}, {2,"$v0"}, {3,"$v1"}, {4,"$a0"}, {5,"$a1"}, {6,"$a2"}, {7,"$a3"}, {8,"$t0"}, {9,"$t1"}, {10,"$t2"},
                                 {11,"$t3"}, {12,"$t4"}, {13,"$t5"}, {14,"$t6"}, {15,"$t7"}, {16,"$s0"}, {17,"$s1"}, {18,"$s2"}, {19,"$s3"}, {20,"$s4"}, {21,"$s5"},
                                 {22,"$s6"}, {23,"$s7"}, {24,"$t8"}, {25,"$t9"}, {26,"$k0"}, {27,"$k1"}, {28,"$gp"}, {29,"$sp"}, {30,"$fp"}, {31,"$ra"} };
    // used to store labels used in program along with their corresponding address
    map<string, int> labels;

    // vector that contains all the labels encountered in instructions during compile time
    // these labels are resolved during linking time
    vector<pair<string, pair<int, int>>> label;

    // register parameters and clock cycle count
    int ins_cnt[10], reg[32];
    int cycle_cnt = 0;

    // constructor for initialization
    REGI() {

        // initialize registers with 0
        for (int i = 0; i < 32; i++)
            reg[i] = 0;

        // initialize instruction count with 0
        for (int i = 0; i < 10; i++)
            ins_cnt[i] = 0;
    }

    void print_reg_file() {

        // print register contents
        cout << "Register file contents after " << cycle_cnt << " clock cycles:\n";

        cout << "zero: " << decimalToHexadecimal(reg[0]) << " " << "at: " << decimalToHexadecimal(reg[1]) << " " << "v0: " << decimalToHexadecimal(reg[2]) << " " << "v1: " << decimalToHexadecimal(reg[3]) << " " << "a0: " << decimalToHexadecimal(reg[4]) << "\n";
        cout << "a1: " << decimalToHexadecimal(reg[5]) << " " << "a2: " << decimalToHexadecimal(reg[6]) << " " << "a3: " << decimalToHexadecimal(reg[7]) << " " << "t0: " << decimalToHexadecimal(reg[8]) << " " << "t1: " << decimalToHexadecimal(reg[9]) << "\n";
        cout << "t2: " << decimalToHexadecimal(reg[10]) << " " << "t3: " << decimalToHexadecimal(reg[11]) << " " << "t4: " << decimalToHexadecimal(reg[12]) << " " << "t5: " << decimalToHexadecimal(reg[13]) << " " << "t6: " << decimalToHexadecimal(reg[14]) << "\n";
        cout << "t7: " << decimalToHexadecimal(reg[15]) << " " << "s0: " << decimalToHexadecimal(reg[16]) << " " << "s1: " << decimalToHexadecimal(reg[17]) << " " << "s2: " << decimalToHexadecimal(reg[18]) << " " << "s3: " << decimalToHexadecimal(reg[19]) << "\n";
        cout << "s4: " << decimalToHexadecimal(reg[20]) << " " << "s5: " << decimalToHexadecimal(reg[21]) << " " << "s6: " << decimalToHexadecimal(reg[22]) << " " << "s7: " << decimalToHexadecimal(reg[23]) << " " << "t8: " << decimalToHexadecimal(reg[24]) << "\n";
        cout << "t9: " << decimalToHexadecimal(reg[25]) << " " << "k0: " << decimalToHexadecimal(reg[26]) << " " << "k1: " << decimalToHexadecimal(reg[27]) << " " << "gp: " << decimalToHexadecimal(reg[28]) << " " << "sp: " << decimalToHexadecimal(reg[29]) << "\n";
        cout << "fp: " << decimalToHexadecimal(reg[30]) << " " << "ra: " << decimalToHexadecimal(reg[31]) << "\n\n\n";
    }

    
    //counts number of cycles required
    int cycle(int offset, int arg2){
        long long addr = (long long)offset + (long long)reg[arg2] + (long long)DATA_START;
        if(row_buffer[0]==addr/1024){return col_delay;}
        if(row_buffer[0]==-1){return row_delay+col_delay;}
        return 2*row_delay+col_delay;
    }

    void execution_stats() {

        // print execution statistics
        cout << "Total clock cycles: " << cycle_cnt << "\n";
        cout << "Total row buffer updates: " << buffer_update_cnt << "\n\n";
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
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = sum;
            // check if it is the zero register
            if (dest == 0)
                reg[dest] = 0;
            cout<<"cycle " << 1+cycle_cnt << ": Instruction - add " << rev_map[dest]<< " " << rev_map[src1] << " " << rev_map[src2] << ": " <<rev_map[dest] << " = " << sum <<"\n";
            return true;
        }
    }

    bool sub(int dest, int src1, int src2) {
        long long diff = (long long)reg[src1] - (long long)reg[src2];
        stat_update(1);

        if (diff > 2147483647 || diff < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = diff;
            // check if it is the zero register
            if (dest == 0)
                reg[dest] = 0;
            cout<<"cycle " << 1+cycle_cnt << ": Instruction - sub " << rev_map[dest]<< " " << rev_map[src1] << " " << rev_map[src2] << ": " <<rev_map[dest] << " = " << diff <<"\n";
            return true;
        }
    }

    bool mul(int dest, int src1, int src2) {
        long long prod = (long long)reg[src1] * (long long)reg[src2];
        stat_update(2);

        if (prod > 2147483647 || prod < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = prod;
            // check if it is the zero register
            if (dest == 0)
                reg[dest] = 0;
            cout<<"cycle " << 1+cycle_cnt << ": Instruction - mul " << rev_map[dest]<< " " << rev_map[src1] << " " << rev_map[src2] << ": " <<rev_map[dest] << " = " << prod <<"\n";
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
        cout<<"cycle " << 1+cycle_cnt << ": Instruction - BEQ\n";
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
        cout<<"cycle " << 1+cycle_cnt << ": Instruction - BNE\n";
        return true;
    }

    void slt(int dest, int src1, int src2) {
        reg[dest] = (reg[src1] < reg[src2]) ? 1 : 0;
        // check if it is the zero register
        cout<<"cycle " << 1+cycle_cnt << ": Instruction - SLT " << rev_map[dest] << " " <<rev_map[src1] << " " <<rev_map[src2] << ": " << rev_map[dest] << " = " << reg[dest]<<"\n";

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
            {cout<<"cycle " << 1+cycle_cnt << ": Instruction - Jump\n";
            return true;}
    }

    bool lw(int dest, int offset, int src) {
        // check if address is valid
        long long addr = (long long)offset + (long long)reg[src] + (long long)DATA_START;
        int start = cycle(offset, src);
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
        int row = addr/1024;
        int xyz = 0, mno = row_buffer[0];
        if(row == row_buffer[0]){
            reg[dest] = row_buffer[addr%1024+1];
            xyz=1;
        }
        else{
            if(row_buffer[0]!=-1){
                //copy buffer to memory
                for(int m = 0; m < 1024; m++){
                    memory[row_buffer[0]][m] = row_buffer[m+1];
                }
            }
            buffer_update_cnt+=1;
            //copy new row to buffer
            for(int m = 0; m < 1024; m++){
                row_buffer[m+1] = memory[row][m];
            }
            //update buffer row number
            row_buffer[0] = row;
            reg[dest] = row_buffer[addr%1024+1];
        }
        // - > reg[dest] = memory[addr%1024][addr/1024];
        // check if it is the zero register
        if (dest == 0)
            reg[dest] = 0;
        stat_update(7);
        cout<<"cycle " << 2+cycle_cnt-start<<"-"<<cycle_cnt+1 << ": Instruction - lw " << rev_map[dest]<< " " << offset << "(" << rev_map[src]<<"): "<< rev_map[dest] <<" = "<<row_buffer[addr%1024+1] << "\n";
        if(mno==-1){cout<<"         DRAM Activity: Copy row "<<row<<" from memory to buffer.\n"<<"                        Copy Data at column "<<addr/1024<<" to "<<rev_map[dest]<<".\n"<<"                        Memory Address: "<<addr-DATA_START<<" - "<<addr-DATA_START+3<<" loaded\n";}
        else if(xyz == 0){cout<<"         DRAM Activity: Copy row "<<mno<<" from buffer back to memory.\n"<<"                        Copy row "<<row<<" from memory to buffer.\n"<<"                        Copy Data at column "<<addr/1024<<" to "<<rev_map[dest]<<".\n"<<"                        Memory Address: "<<addr-DATA_START<<" - "<<addr-DATA_START+3<<" loaded\n";}
        else{cout<<"         DRAM Activity: Copy Data at column "<<addr/1024<<" to "<<rev_map[dest]<<".\n"<<"                        Memory Address: "<<addr-DATA_START<<" - "<<addr-DATA_START+3<<" loaded\n";}
        
        return true;
    }

    bool sw(int src, int offset, int dest) {
        // check if address is valid
        int start = cycle(offset, dest);
        long long addr = (long long)offset + (long long)reg[dest] + (long long)DATA_START;
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
        
        int row = addr/1024, mno = row_buffer[0], xyz = 0;
        if(row == row_buffer[0]){
            row_buffer[addr%1024+1] = reg[src];xyz=1;
            buffer_update_cnt+=1;
        }
        else{
            if(row_buffer[0]!=-1){
                //copy buffer to memory
                for(int m = 0; m < 1024; m++){
                    memory[row_buffer[0]][m] = row_buffer[m+1];
                }
            }
            buffer_update_cnt+=2;
            //copy new row to buffer
            for(int m = 0; m < 1024; m++){
                row_buffer[m+1] = memory[row][m];
            }
            //update buffer row number
            row_buffer[0] = row;
            row_buffer[addr%1024+1] = reg[src];
        }
        
        // - > memory[addr%1024][addr/1024] = reg[src];
        stat_update(8);
        cout<<"cycle " << 2+cycle_cnt-start <<"-"<<cycle_cnt+1 << ": Instruction - sw " << rev_map[src]<< " " << offset << "(" << rev_map[dest]<<"): "<<"\n";
        if(mno==-1){cout<<"         DRAM Activity: Copy row "<<row<<" from memory to buffer.\n"<<"                        Copy Data to column "<<addr%1024<<" from "<<rev_map[dest]<<".\n"<<"                        Memory Address: "<<addr-DATA_START<<" - "<<addr-DATA_START+3<<" = "<<reg[src]<<"\n";}
        else if(xyz == 0){cout<<"         DRAM Activity: Copy row "<<mno<<" from buffer back to memory.\n"<<"                        Copy row "<<row<<" from memory to buffer.\n"<<"                        Copy Data to column "<<addr%1024<<" from "<<rev_map[dest]<<".\n"<<"                        Memory Address: "<<addr-DATA_START<<" - "<<addr-DATA_START+3<<" = "<<reg[src]<<"\n";}
        else{cout<<"         DRAM Activity: Copy Data to column "<<addr%1024<<" from "<<rev_map[dest]<<".\n"<<"                        Memory Address: "<<addr-DATA_START<<" - "<<addr-DATA_START+3<<" = "<<reg[src]<<"\n";}
        
        return true;
    }

    bool addi(int dest, int src, int adds) {
        long long sum = (long long)reg[src] + (long long)adds;
        stat_update(9);
        if (sum > 2147483647 || sum < -2147483648) {
            cout << "Error: Arithmetic Overflow. Program terminating!\n\n";
            execution_stats();
            return false;
        }
        else {
            reg[dest] = sum;
            // check if it is the zero register
            if (dest == 0)
                reg[dest] = 0;
            cout<<"cycle " << 1+cycle_cnt << ": Instruction - addi " << rev_map[dest]<<" "<<rev_map[src]<<" "<<adds<<": "<<rev_map[dest]<<" = "<<sum<<"\n";
            return true;
        }
    }

    void stat_update(int ins_code) {
        
        ins_cnt[ins_code]++;
    }
};

//counts number of cycles required
int cycles(int offset, int arg2, REGI &rf){
    long long addr = (long long)offset + (long long)rf.reg[arg2] + (long long)DATA_START;
    
    if (addr >= 1048576 || addr < DATA_START) {
        // invalid address, error
        cout << "Error: Program is trying to access an unavailable memory location. Program terminating!\n\n";
        rf.execution_stats();
        return -11;
    }
    else {
        if (addr % 4 != 0) {
            // address is not word aligned, error
            cout << "Error: Memory address is not word-aligned. Invalid load operation. Program Terminating!\n\n";
            rf.execution_stats();
            return -11;
        }
    }
    
    if(row_buffer[0]==addr/1024){return col_delay;}
    if(row_buffer[0]==-1){return row_delay+col_delay;}
    return 2*row_delay+col_delay;
}

// this function simulates the execution of MIPS program
bool simulator(REGI& rf) {

    int ins_code, cycle_to_go = -1, reg_used1 = -10, reg_used2 = -10, temp_pc = -10;
    bool flag;
    string ins_last;
    // while program counter is less than PARTITION
    while (PC < PARTITION || cycle_to_go != -1) {
        // - > each_ins_cnt[(int)PC/4]++;
        ins_code = memory[PC/1024][PC%1024];
        int r1 = memory[(PC + 1)/1024][(PC + 1)%1024], r2 = memory[(PC + 2)/1024][(PC + 2)%1024], r3 = memory[(PC + 3)/1024][(PC + 3)%1024];
        if(part1 && cycle_to_go!=-1){goto skipped_ins;}
        switch (ins_code) {
            // add
        case 0:
            if(cycle_to_go!=-1 && (r1==reg_used1||r2==reg_used1||r3==reg_used1||r1==reg_used2||r2==reg_used2||r3==reg_used2)){break;}
            if(PC>=PARTITION){break;}
            flag = rf.add(memory[(PC + 1)/1024][(PC + 1)%1024], memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024]);
            if (flag) PC += 4;
            else return flag;
            break;
            // sub
        case 1:
                if(cycle_to_go!=-1 && (r1==reg_used1||r2==reg_used1||r3==reg_used1||r1==reg_used2||r2==reg_used2||r3==reg_used2)){break;}
                if(PC>=PARTITION){break;}

                flag = rf.sub(memory[(PC + 1)/1024][(PC + 1)%1024], memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024]);
            if (flag) PC += 4;
            else return flag;
            break;
            // mul
        case 2:
                if(cycle_to_go!=-1 && (r1==reg_used1||r2==reg_used1||r3==reg_used1||r1==reg_used2||r2==reg_used2||r3==reg_used2)){break;}
                if(PC>=PARTITION){break;}

                flag = rf.mul(memory[(PC + 1)/1024][(PC + 1)%1024], memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024]);
            if (flag) PC += 4;
            else return flag;
            break;
            // beq
        case 3:
                if(cycle_to_go!=-1 && (r1==reg_used1||r2==reg_used1||r3==reg_used1||r1==reg_used2||r2==reg_used2||r3==reg_used2)){break;}
                if(PC>=PARTITION){break;}
                flag = rf.beq(memory[(PC + 1)/1024][(PC + 1)%1024], memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024]);
            if (!flag) return flag;
            break;
            // bne
        case 4:
                if(cycle_to_go!=-1 && (r1==reg_used1||r2==reg_used1||r3==reg_used1||r1==reg_used2||r2==reg_used2||r3==reg_used2)){break;}
                if(PC>=PARTITION){break;}
                flag = rf.bne(memory[(PC + 1)/1024][(PC + 1)%1024], memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024]);
            if (!flag) return flag;
            break;
            // slt
        case 5:
                if(cycle_to_go!=-1 && (r1==reg_used1||r2==reg_used1||r3==reg_used1||r1==reg_used2||r2==reg_used2||r3==reg_used2)){break;}
                if(PC>=PARTITION){break;}
                rf.slt(memory[(PC + 1)/1024][(PC + 1)%1024], memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024]);
            PC += 4;
            break;
            // j
        case 6:
                if(PC>=PARTITION){break;}
                flag = rf.j(memory[(PC + 1)/1024][(PC + 1)%1024]);
            if (!flag) return flag;
            break;
            // lw
        case 7:
                if(cycle_to_go == -1){
                    cout<<"cycle " << 1+rf.cycle_cnt << ": DRAM request issued(Load)\n";
                    cycle_to_go = cycles(memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024],rf) + 1;
                    if(cycle_to_go==-10){return false;}
                    ins_last = "lw";
                    reg_used1 = memory[(PC + 1)/1024][(PC + 1)%1024];
                    reg_used2 = memory[(PC + 3)/1024][(PC + 3)%1024];
                    temp_pc = PC;
                    PC += 4;
                }
            break;
            // sw
        case 8:
                if(cycle_to_go == -1){
                    cout<<"cycle " << 1+rf.cycle_cnt << ": DRAM request issued(Store)\n";
                    cycle_to_go = cycles(memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024],rf) + 1;
                    if(cycle_to_go==-10){return false;}
                    ins_last = "sw";
                    reg_used1 = memory[(PC + 1)/1024][(PC + 1)%1024];
                    reg_used2 = memory[(PC + 3)/1024][(PC + 3)%1024];
                    temp_pc = PC;
                    PC += 4;
                }
            break;
            // addi
        case 9:
                if(cycle_to_go!=-1 && (r1==reg_used1||r2==reg_used1||r1==reg_used2||r2==reg_used2)){break;}
                if(PC>=PARTITION){break;}
                flag = rf.addi(memory[(PC + 1)/1024][(PC + 1)%1024], memory[(PC + 2)/1024][(PC + 2)%1024], memory[(PC + 3)/1024][(PC + 3)%1024]);
            if (flag) PC += 4;
            else return flag;
            break;
        }
        skipped_ins:
        if(cycle_to_go>-1){cycle_to_go--;}
        if(cycle_to_go == 0){
            if(ins_last == "lw"){
                flag = rf.lw(memory[(temp_pc + 1)/1024][(temp_pc + 1)%1024], memory[(temp_pc + 2)/1024][(temp_pc + 2)%1024], memory[(temp_pc + 3)/1024][(temp_pc + 3)%1024]);
            }
            else{
                flag = rf.sw(memory[(temp_pc + 1)/1024][(temp_pc + 1)%1024], memory[(temp_pc + 2)/1024][(temp_pc + 2)%1024], memory[(temp_pc + 3)/1024][(temp_pc + 3)%1024]);
            }
            if (!flag){return flag;}
            cycle_to_go = -1;
        }
        
        rf.cycle_cnt++;
        //rf.print_reg_file();
    }
    
    if(row_buffer[0]!=-1){
        cout<<"\nExecuting Write-Back. Write-Back clock cycles are excluded below.\n\n";
        //copy buffer to memory
        for(int m = 0; m < 1024; m++){
            memory[row_buffer[0]][m] = row_buffer[m+1];
        }
    }
    // PC = PARTITION, so execution is complete (successful)
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
    // - > each_ins_cnt.push_back(0);
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
            memory[PC/1024][PC%1024] = ins;
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
                        memory[(PC + 1)/1024][(PC+1)%1024] = 4 * addr;
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
                        memory[(PC + 1)/1024][(PC + 1)%1024] = rf.reg_map[args[1]];
                        memory[(PC + 2)/1024][(PC + 2)%1024] = rf.reg_map[args[2]];
                        memory[(PC + 3)/1024][(PC + 3)%1024] = rf.reg_map[args[3]];
                        int temp = memory[(PC + 1)/1024][(PC + 1)%1024];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = memory[(PC + 2)/1024][(PC + 2)%1024];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = memory[(PC + 3)/1024][(PC + 3)%1024];
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
                            memory[(PC + 1)/1024][(PC + 1)%1024] = rf.reg_map[args[1]];
                            memory[(PC + 2)/1024][(PC + 2)%1024] = rf.reg_map[args[2]];
                            memory[(PC + 3)/1024][(PC + 3)%1024] = immediate;
                            int temp = memory[(PC + 1)/1024][(PC + 1)%1024];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                            temp = memory[(PC + 2)/1024][(PC + 2)%1024];
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
                            memory[(PC + 1)/1024][(PC + 1)%1024] = rf.reg_map[args[1]];
                            memory[(PC + 2)/1024][(PC + 2)%1024] = rf.reg_map[args[2]];
                            int temp = memory[(PC + 1)/1024][(PC + 1)%1024];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                            temp = memory[(PC + 2)/1024][(PC + 2)%1024];
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
                                memory[(PC + 3)/1024][(PC + 3)%1024] = PC + 4 + 4 * immediate;
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
                        memory[(PC + 1)/1024][(PC + 1)%1024] = rf.reg_map[args[1]];
                        memory[(PC + 2)/1024][(PC + 2)%1024] = immediate;
                        memory[(PC + 3)/1024][(PC + 3)%1024] = rf.reg_map[args[3]];
                        int temp = memory[(PC + 1)/1024][(PC + 1)%1024];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = memory[(PC + 3)/1024][(PC + 3)%1024];
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
            memory[label.second.first/1024][label.second.first%1024] = label_loc;
        }

    }
    // all labels resolved
    return true;
}

int main(int argc, char** argv) {

    if (argc > 1) {
        // optional file name provided
        file_path = argv[1];
    }
    if(argc > 3){
        if(!isInteger(argv[2])){cout<<"ERROR: non-integer row/col delay entered. Program terminating!\n"; return 0;}
        if(!isInteger(argv[3])){cout<<"ERROR: non-integer row/col delay entered. Program terminating!\n"; return 0;}
        row_delay = stoi(argv[2]);
        col_delay = stoi(argv[3]);
        if(row_delay<0 || col_delay<0){cout<<"ERROR: negative row/col delay entered. Program terminating!\n"; return 0;}
    }
    if(argc > 4){
        if(!isInteger(argv[4])){cout<<"ERROR: non-integer optional part-1 argument. Program terminating!\n"; return 0;}
        if(stoi(argv[4]) == 1){part1 = true;}
    }
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
    
    //initialize with -1
    row_buffer[0] = -1;

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
    flag = simulator(rf);

    if (flag)
        cout << "Program executed successfully!\n\n";

    // close the input file stream
    infile.close();

    return 0;
}
