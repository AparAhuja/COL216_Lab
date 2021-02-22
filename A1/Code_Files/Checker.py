from decimal import Decimal

QTSPIM_OUTPUT = "MIPS_output.txt"
no_of_cases = 1

f = open(QTSPIM_OUTPUT, "r")
g = open("Expected_output.txt", "r")
# h = open("Difference_Exact.txt", "w")
k = open("Difference_12_decimal_places.txt", "w")
cnt = 0
for i in range(no_of_cases):
    str1 = f.readline()
    str2 = g.readline()
    a1 = float(str1[:-1])
    a2 = float(str2[:-1])
    error = abs((a1-a2))
    # h.write(str(error)+"\n")
    if(error < 0.000000000001):
        k.write("0\n")
        cnt+=1
    else:
        k.write(str(error)+"\n")
k.write("Accuracy is " + str(cnt/no_of_cases*100) + "%\n")
k.write("Precision: 12 places after decimal point.")

k.close()
f.close()
g.close()
# h.close()