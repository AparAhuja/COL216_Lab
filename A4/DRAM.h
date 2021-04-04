#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <fstream>
#include <string>

using namespace std;

struct Request {
    int addr;
    int row;
    int data_bus;
    int destination;
    string type;    
};

struct DRAM {
    map<int, vector<int>> MemToAdj;
    map<string, pair<int, int>> Pread, Pwrite;
    map<int, string> reverseMap =
        { {0, "$zero"}, {1, "$at"}, {2, "$v0"}, {3, "$v1"}, {4, "$a0"}, {5, "$a1"}, {6, "$a2"}, {7, "$a3"}, {8, "$t0"}, {9, "$t1"}, {10, "$t2"},
        {11, "$t3"}, {12, "$t4"}, {13, "$t5"}, {14, "$t6"}, {15, "$t7"}, {16, "$s0"}, {17, "$s1"}, {18, "$s2"}, {19, "$s3"}, {20, "$s4"}, {21, "$s5"},
        {22, "$s6"}, {23, "$s7"}, {24, "$t8"}, {25, "$t9"}, {26, "$k0"}, {27, "$k1"}, {28, "$gp"}, {29, "$sp"}, {30, "$fp"}, {31, "$ra"} };
    vector<queue<Request>> Adj;
    int adjIndex = 0;
    
    bool isEmpty() {
        return (adjIndex = Adj.size());
    }

    int BinarySearch(int key, int low, int high, vector<int> &vec) {
        int mid;
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
                Pwrite.insert({reg, make_pair(loc, 1)});
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
                            MemToAdj[req.row][0] = MemToAdj[req.row].size();
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
                    Pwrite[memAddr].first = loc;
                    Pwrite[memAddr].second = Pwrite[memAddr].second + 1;
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
                                Pread[memAddr].second = Pread[memAddr].second + 1;
                            }
                            // Add register to Pwrite
                            Pwrite.insert({reg, make_pair(loc, 1)});
                        }
                        else {
                            loc = Adj.size();
                            MemToAdj[req.row].push_back(loc);
                            MemToAdj[req.row][0] = MemToAdj[req.row].size();
                            queue<Request> que;
                            que.push(req);
                            Adj.push_back(que);
                            if(Pread.find(memAddr) == Pread.end()) {
                                // Add to Pread
                                Pread.insert({memAddr, make_pair(loc, 1)});
                            }
                            else {
                                // Update Pread
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

