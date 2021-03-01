no_of_cases = 10                 #Number of test cases
no_of_operators = 80              #Number of operators
FILE_NAME = "Mixed_XLarge"
opArray = ['-','*','+']


import random
def precedence(op):
    if op == '+' or op == '-':
        return 1
    if op == '*' or op == '/':
        return 2
    return 0

# Function to perform arithmetic
# operations.
def applyOp(a, b, op):
    if op == '+': return a + b
    if op == '-': return a - b
    if op == '*': return a * b
    if op == '/': return a // b

def evaluate(tokens):
    # stack to store integer values.
    values = []
    # stack to store operators.
    ops = []
    i = 0
    while i < len(tokens):
        # Current token is a whitespace,
        # skip it.
        if tokens[i] == ' ':
            i += 1
            continue
        # Current token is an opening
        # brace, push it to 'ops'
        elif tokens[i] == '(':
            ops.append(tokens[i])
        # Current token is a number, push
        # it to stack for numbers.
        elif tokens[i].isdigit():
            val = 0
            # There may be more than one
            # digits in the number.
            while (i < len(tokens) and
                tokens[i].isdigit()):
                val = (val * 10) + int(tokens[i])
                i += 1
            values.append(val)
             
            # right now the i points to 
            # the character next to the digit,
            # since the for loop also increases 
            # the i, we would skip one 
            #  token position; we need to 
            # decrease the value of i by 1 to
            # correct the offset.
            i-=1
         
        # Closing brace encountered, 
        # solve entire brace.
        elif tokens[i] == ')':
         
            while len(ops) != 0 and ops[-1] != '(':
             
                val2 = values.pop()
                val1 = values.pop()
                op = ops.pop()
                 
                values.append(applyOp(val1, val2, op))
             
            # pop opening brace.
            ops.pop()
         
        # Current token is an operator.
        else:
         
            # While top of 'ops' has same or 
            # greater precedence to current 
            # token, which is an operator. 
            # Apply operator on top of 'ops' 
            # to top two elements in values stack.
            while (len(ops) != 0 and
                precedence(ops[-1]) >=
                   precedence(tokens[i])):
                         
                val2 = values.pop()
                val1 = values.pop()
                op = ops.pop()
                 
                values.append(applyOp(val1, val2, op))
             
            # Push current token to 'ops'.
            ops.append(tokens[i])
         
        i += 1
     
    # Entire expression has been parsed 
    # at this point, apply remaining ops 
    # to remaining values.
    while len(ops) != 0:
         
        val2 = values.pop()
        val1 = values.pop()
        op = ops.pop()
                 
        values.append(applyOp(val1, val2, op))
     
    # Top of 'values' contains result,
    # return it.
    return values[-1]
f = open(FILE_NAME+".txt", "w")
g = open(FILE_NAME+"_Expected_output.txt", "w")
h = open(FILE_NAME+"_MIPS_OUTPUT.txt", "w")
def isOperator(input): 
      
    switch = { 
        '+': 1, 
        '-': 1, 
        '*': 1, 
        '/': 1, 
        '%': 1, 
        '(': 1, 
    } 
      
    return switch.get(input, False) 
  
# To check if the input character is an operand 
def isOperand(input): 
      
    if (input in '0123456789'): 
        return 1
    return 0
  
# Function to return precedence value 
# if operator is present in stack 
def inPrec(input): 
      
    switch = { 
        '+': 2, 
        '-': 2, 
        '*': 4, 
        '/': 4, 
        '%': 4, 
        '(': 0, 
    } 
      
    return switch.get(input, 0) 
  
# Function to return precedence value 
# if operator is present outside stack. 
def outPrec(input): 
      
    switch = { 
        '+': 1, 
        '-': 1, 
        '*': 3, 
        '/': 3, 
        '%': 3, 
        '(': 100, 
    } 
      
    return switch.get(input, 0) 
  
# Function to convert infix to postfix 
def inToPost(input): 
      
    i = 0
    s = [] 
    ans = ""
    # While input is not NULL iterate 
    while (len(input) != i): 
  
        # If character an operand 
        if (isOperand(input[i]) == 1): 
            ans+=(input[i]) 
  
        # If input is an operator, push 
        elif (isOperator(input[i]) == 1): 
            if (len(s) == 0 or 
                outPrec(input[i]) > 
                 inPrec(s[-1])): 
                s.append(input[i]) 
              
            else: 
                while(len(s) > 0 and 
                      outPrec(input[i]) <  
                      inPrec(s[-1])): 
                    ans+=(s[-1]) 
                    s.pop() 
                      
                s.append(input[i]) 
  
        # Condition for opening bracket 
        elif(input[i] == ')'): 
            while(s[-1] != '('): 
                ans+=(s[-1]) 
                s.pop() 
  
                # If opening bracket not present 
                if(len(s) == 0): 
                    print('Wrong input') 
                    exit(1) 
  
            # pop the opening bracket. 
            s.pop() 
              
        i += 1
          
    # pop the remaining operators 
    while(len(s) > 0): 
        if(s[-1] == '('): 
            print('Wrong input') 
            exit(1) 
              
        ans+=(s[-1]) 
        s.pop() 
    return ans
  
# Driver code
def output(out):
    g.write(str(out)+"\n")

for _ in range(no_of_cases):
    n = no_of_operators

    digit = random.randint(0,9)
    expression = str(digit)

    for _ in range(n):
        op = opArray[random.randint(0,len(opArray)-1)]
        digit = random.randint(0,9)
        expression += (op+str(digit))
    
    output(evaluate(expression))
    postfix = inToPost(expression)
    f.write(str(postfix)+"\n")
    # h.write(expression+"\n")
f.close()
g.close()
h.close()
