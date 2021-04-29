
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
    if (i == len||instruction.at(i)=='#')
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

// new function added
string removeSpaces(string line){
    int i = 0, j = line.length() - 1;
    while(i<line.length() && (line.at(i) == ' ' || line.at(i) == '\t')){
        i++;
    }
    while(j >= 0 && (line.at(j) == ' ' || line.at(j) == '\t')){
        j--;
    }
    return line.substr(i, j + 1 - i);
}

// function used to a line of MIPS code to memory, if it is an instruction
// modified function
// removed redundant white spaces / tabs
bool addToMemory(string line, Core& core, int line_number, int coreNum) {
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
            if (core.labels.find(label.first.first) != core.labels.end()) {
                // syntax error (same label used for two different memory addresses)
                cout << "Core " + to_string(coreNum) + " raised Error: Same label used to represent different addresses.\nLabel repeated: " << label.first.first << ": at line number: " << line_number << "\n";
                return false;
            }
            else {
                // add label to map
                core.labels.insert({ label.first.first, core.PC });
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
        cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 1: At line number: " << line_number << ": " << line << "\n";
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
            if (core.ins_map.find(args[0]) == core.ins_map.end()) {
                // invalid instruction
                cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 2: At line number: " << line_number << ": " << line << "\n";
                return false;
            }
            // else store the instruction in memory
            int ins = core.ins_map[args[0]];
            core.insMemory[core.PC] = ins;
            // store the number of arguments in len
            len = args.size();

            //jump
            if (ins == 6) {
                if (len != 2) {
                    // as jump should have only 2 arguments
                    cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 3: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    if (!isInteger(args[1])) {
                        if (!validId(args[1])) {
                            // not a valid label address
                            cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 4: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        // else, it is a valid id, push it
                        core.label.push_back(make_pair(args[1], make_pair(core.PC + 1, line_number)));
                    }
                    else {
                        long long addr = stoi(args[1]);
                        if (addr < 0 || addr >= 67108864) {
                            // address starts from 0 and max integer that can be stored is 2^26 - 1
                            cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 5: At line number: " << line_number << ": " << "\n";
                            return false;
                        }
                        // else store the value (absolute address) in memory
                        core.insMemory[core.PC + 1] = 4 * addr;
                    }
                }
            }

            //add sub mul slt
            else if (ins == 0 || ins == 1 || ins == 2 || ins == 5) {
                if (len != 4) {
                    // add sub mul slt require 4 arguments each
                    cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 6: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    // check if any of the register used is invalid
                    if (core.reg_map.find(args[1]) == core.reg_map.end() || core.reg_map.find(args[2]) == core.reg_map.end() || core.reg_map.find(args[3]) == core.reg_map.end()) {
                        // invalid register used, syntax error
                        cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 7: At line number: " << line_number << ": " << line << "\n";
                        return false;
                    }
                    else {
                        // store register numbers in memory
                        core.insMemory[core.PC + 1] = core.reg_map[args[1]];
                        core.insMemory[core.PC + 2] = core.reg_map[args[2]];
                        core.insMemory[core.PC + 3] = core.reg_map[args[3]];
                        int temp = core.insMemory[core.PC + 1];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = core.insMemory[core.PC + 2];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = core.insMemory[core.PC + 3];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                    }
                }
            }

            //beq bne addi
            else if (ins == 3 || ins == 4 || ins == 9) {
                if (len != 4) {
                    // beq bne addi require 4 arguments each
                    cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 8: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    if (ins == 9) {
                        // check if any of the register used is invalid
                        // also check if the last argument is an immediate or not
                        if (core.reg_map.find(args[1]) == core.reg_map.end() || core.reg_map.find(args[2]) == core.reg_map.end() || !isInteger(args[3])) {
                            // invalid arguments, syntax error
                            cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 9: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        else {
                            long long immediate = stoi(args[3]);

                            // raise overflow error if necessary (only 16 bit immediate)
                            if (immediate < -32768 || immediate > 32767) {
                                cout << "Core " + to_string(coreNum) + " raised Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                                return false;
                            }
                            // else
                            // store register numbers and immediate in memory
                            core.insMemory[core.PC + 1] = core.reg_map[args[1]];
                            core.insMemory[core.PC + 2] = core.reg_map[args[2]];
                            core.insMemory[core.PC + 3] = immediate;
                            int temp = core.insMemory[core.PC + 1];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                            temp = core.insMemory[core.PC + 2];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                        }
                    }
                    else {
                        // bne or beq
                        // check if any of the register used is invalid
                        if (core.reg_map.find(args[1]) == core.reg_map.end() || core.reg_map.find(args[2]) == core.reg_map.end()) {
                            // invalid arguments, syntax error
                            cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 10: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        else {
                            // store register
                            core.insMemory[core.PC + 1] = core.reg_map[args[1]];
                            core.insMemory[core.PC + 2] = core.reg_map[args[2]];
                            int temp = core.insMemory[core.PC + 1];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                            temp = core.insMemory[core.PC + 2];
                            if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                                cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                                return false;
                            }
                            if (isInteger(args[3])) {
                                long long immediate = stoi(args[3]);

                                // raise overflow error if necessary
                                if (immediate < -32768 || immediate > 32767) {
                                    cout << "Core " + to_string(coreNum) + " raised Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                                    return false;
                                }
                                // else
                                // store immediate (absolute jump address) in memory
                                core.insMemory[core.PC + 3] = core.PC + 4 + 4 * immediate;
                            }
                            else {
                                if (validId(args[3])) {
                                    core.label.push_back(make_pair(args[3], make_pair(core.PC + 3, line_number)));
                                }
                                else {
                                    // invalid label, raise error
                                    cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 11: At line number: " << line_number << ": " << line << "\n";
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
                    cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 12: At line number: " << line_number << ": " << line << "\n";
                    return false;
                }
                else {
                    // check if the registers are valid or not, also check if the second argument is an immediate or not
                    if (core.reg_map.find(args[1]) == core.reg_map.end() || !isInteger(args[2]) || core.reg_map.find(args[3]) == core.reg_map.end()) {
                        // invalid arguments, syntax error
                        cout << "Core " + to_string(coreNum) + " raised SYNTAX ERROR 13: At line number: " << line_number << ": " << line << "\n";
                        return false;
                    }
                    else {
                        long long immediate = stoi(args[2]);

                        // raise overflow error if necessary
                        if (immediate < -32768 || immediate > 32767) {
                            cout << "Core " + to_string(coreNum) + " raised Immediate Overflow ERROR: At line number: " << line_number << ": " << "\n";
                            return false;
                        }
                        // store register numbers and immediate in memory
                        core.insMemory[core.PC + 1] = core.reg_map[args[1]];
                        core.insMemory[core.PC + 2] = immediate;
                        core.insMemory[core.PC + 3] = core.reg_map[args[3]];
                        int temp = core.insMemory[core.PC + 1];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                        temp = core.insMemory[core.PC + 3];
                        if (temp == 1 || temp == 26 || temp == 27 || temp == 28) {
                            cout << "Core " + to_string(coreNum) + " raised Access Error: Trying to access reserved register: At line number: " << line_number << ": " << line << "\n";
                            return false;
                        }
                    }
                }
            }
        }
    }
    // increment program counter by 4 (next storage address)
    core.PC += 4;
    
    // store the instruction as well
    line = removeSpaces(label.first.second);
    core.instructions.push_back(line);
    // instruction stored successfully
    return true;
}


// function used to resolve labels
bool linker(Core& core, int coreNum) {

    // label stores all the unresolved labels
    int len = core.label.size();

    for (int i = 0; i < len; i++) {

        pair<string, pair<int, int>> label = core.label[i];
        // check if label is actually defined
        if (core.labels.find(label.first) == core.labels.end()) {
            // illegal label, raise error
            cout << "Core " + to_string(coreNum) + " raised Linking Error: Unable to resolve label used on line: " << label.second.second << "\n";
            return false;
        }
        else {
            // resolve label
            int label_loc = core.labels[label.first];
            core.insMemory[label.second.first] = label_loc;
        }
    }
    // all labels resolved
    return true;
}

