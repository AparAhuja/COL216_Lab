no_of_cases = 1     #Number of test cases
max_val_of_n = 20               #Number of points in a test case
min_val_of_x_coordinate = -10  #Minimum value of x coordinate
max_val_of_x_coordinate = 10  #Maximum value of x coordinate
min_val_of_y_coordinate = -10
max_val_of_y_coordinate = 10

import random
f = open("Test_Input.txt", "w")
g = open("Expected_output.txt", "w")
h = open("MIPS_output.txt", "w")

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
        x = random.randint(min_val_of_x_coordinate, max_val_of_x_coordinate)
        y = random.randint(min_val_of_y_coordinate, max_val_of_y_coordinate)
        array+=[(x,y)]
    array.sort()
    output(array)
    for element in array:
        f.write(str(element[0])+"\n")
        f.write(str(element[1])+"\n")
h.close()
f.close()
g.close()
