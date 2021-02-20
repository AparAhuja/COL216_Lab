no_of_cases = 100               #Number of test cases
max_val_of_n = 20               #Number of points in a test case
min_val_of_coordinate = -10000  #Minimum value of x and y coordinate
max_val_of_coordinate = 10000   #Maximum value of x and y coordinate


import random
f = open("TestCases.txt", "w")
g = open("Output.txt", "w")

def output(a):
    area = 0
    for i in range(1,len(a)):
        x1, y1 = a[i-1]
        x2, y2 = a[i]
        if(y2*y1<0):
            area += abs((x2-x1)*(y1*y1+y2*y2)/(2*(y2-y1)))
        else:
            area += abs((x2-x1)*(y1+y2)/2)
    g.write(str(area)+"\n")

for _ in range(no_of_cases):
    n = random.randint(2, max_val_of_n)
    f.write(str(n)+"\n")
    array=[]
    for _ in range(n):
        x = random.randint(min_val_of_coordinate, max_val_of_coordinate)
        y = random.randint(min_val_of_coordinate, max_val_of_coordinate)
        array+=[(x,y)]
    array.sort()
    output(array)
    for element in array:
        f.write(str(element[0])+"\n")
        f.write(str(element[1])+"\n")
f.close()
g.close()