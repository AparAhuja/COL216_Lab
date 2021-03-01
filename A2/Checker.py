QTSPIM_OUTPUT = "MIPS_output.txt"
no_of_cases = 1

f = open(QTSPIM_OUTPUT, "r")
g = open("Expected_output.txt", "r")
k = open("Difference.txt", "w")

cnt = 0
for i in range(no_of_cases):
    str1 = f.readline()
    str2 = g.readline()
    a1 = int(str1[:-1])
    a2 = int(str2[:-1])
    error = (a1-a2)

    if(error == 0 ):
        k.write("0\n")
        cnt+=1
    else:
        k.write(str(error)+"\n")
k.write("Accuracy is " + str(cnt/no_of_cases*100) + "%\n")

k.close()
f.close()
g.close()
