#include <iostream>
#include <fstream>
#include <string>
using namespace std;
class register_file{
    
};

//add line to memory
void addToMemory(string line){
    cout<<line<<"\n"; // for debugging
}
int main(int argc, const char * argv[]) {
    ifstream infile("/Users/aparahuja/Desktop/IITD/COL216 - Arch/COL216_Lab/A3/A3/input.txt");
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
