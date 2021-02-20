QTSPIM_OUTPUT = "Sample.txt"
no_of_cases = 4

f = open(QTSPIM_OUTPUT, "r")
g = open("Output.txt", "r")
h = open("Difference_Exact.txt", "w")
k = open("Difference_5_places.txt", "w")
cnt = 0
for i in range(no_of_cases):
    a1 = int(f.readline())
    a2 = int(g.readline())
    error = abs((a1-a2))
    h.write(str(error)+"\n")
    if(error < 0.00001):
        k.write("0\n")
        cnt+=1
    else:
        k.write(str(error)+"\n")
k.write("Accuracy is " + str(cnt/no_of_cases*100) + "%")

k.close()
f.close()
g.close()
h.close()