no_of_instructions = 20
no_of_files = 2

import random

def makeFile(filex):
    if(no_of_instructions > 174763):
        print("ERROR: Too many instructions.\n")
        exit()
    f = open("TestCase" + str(filex) + ".txt", "w")
    labels = set()
    usedMemAddr = set()

    registers = { 0: "$zero", 1: "$zero", 2: "$v0", 3: "$v1", 4: "$a0", 5: "$a1", 6: "$a2", 7: "$a3", 8: "$t0", 9: "$t1", 10: "$t2",11: "$t3", 12: "$t4", 13: "$t5", 14: "$t6", 15: "$t7", 16: "$s0", 17: "$s1", 18: "$s2", 19: "$s3", 20: "$s4", 21: "$s5",22: "$s6", 23: "$s7", 24: "$t8", 25: "$t9", 26: "$zero", 27: "$zero", 28: "$zero", 29: "$sp", 30: "$fp", 31: "$ra"}

    for i in range(no_of_instructions):
        ins_code = random.randint(0, 10)
        r1 = registers[random.randint(0,31)]
        r2 = registers[random.randint(0,31)]
        r3 = registers[random.randint(0,31)]
        if ins_code == 0:
            f.write("add " + r1 + " " + r2 + " " + r3 + "\n")
        if ins_code == 1:
            f.write("sub " + r1 + " " + r2 + " " + r3 + "\n")
        if ins_code == 2:
            f.write("mul " + r1 + " " + r2 + " " + r3 + "\n")
        if ins_code == 3:
            relativeOffset = random.randint(- i - 1, no_of_instructions - i - 1)
            if len(labels) == 0:
                continue
            else:
                label = random.sample(labels, 1)
                f.write("beq " + r1 + " " + r2 + " " + label[0] + "\n")
        if ins_code == 4:
            if len(labels) == 0:
                continue
            else:
                label = random.sample(labels, 1)
                f.write("bne " + r1 + " " + r2 + " " + label[0] + "\n")
        if ins_code == 5:
            f.write("slt " + r1 + " " + r2 + " " + r3 + "\n")
        if ins_code == 6:
            if len(labels) == 0:
                continue
            else:
                label = random.sample(labels, 1)
                f.write("j " + label[0] + "\n")
        if ins_code == 7:
            addr = str(4*random.randint(0, 87295))
            if random.randint(0,1) == 1 and len(usedMemAddr) != 0:
                addr = random.sample(usedMemAddr, 1)[0]
            usedMemAddr.add(addr)
            f.write("sw " + r1 + " " + addr + "($zero)\n")
        if ins_code == 8:
            addr = str(4*random.randint(0, 87295))
            if random.randint(0,1) == 1 and len(usedMemAddr) != 0:
                addr = random.sample(usedMemAddr, 1)[0]
            usedMemAddr.add(addr)
            f.write("lw " + r1 + " " + addr + "($zero)\n")
        if ins_code == 9:
            f.write("addi " + r1 + " " + r2 + " " + str(random.randint(-2**15, 2**15 - 1)) + "\n")
        if ins_code == 10:
            label = "Label" + str(i) + ":\n"
            f.write(label)
            labels.add(label)
    f.close()

for i in range(no_of_files):
    makeFile(i+1)
