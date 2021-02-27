.data
    array: .space 320
	expression: .space 100000
    inputfile: .asciiz "C:\\Users\\arnav\\Desktop\\Test_Input.txt"
    error_invalidChar: .asciiz "ERROR: There was an invalid character present in the expression.\nMake sure that the operands are in the range 0-9, and only +, - and * operators are used.\nProgram terminating!"
    error_illegalExp: .asciiz "ERROR: The number of operands and number of operators do not match.\nMake sure that the operands are in the range 0-9, and that for each operator exactly 2 operands are provided.\nProgram terminating!"
    msg_input: .asciiz "Enter the postfix expression that needs to be evaluated: "
    msg_output: .asciiz "The value of the postfix expression is: "
    msg_separator: .asciiz "\n\n_______________________________________________________________\n\n"
    msg_lf: .asciiz "\n"


.text
.globl main
main:

    li $s7 10   # initialize with number of test cases (variable)

    li $v1 1   # current test case number 
	
    li $s1 10  # the constant 10 
    
    li $v0 13         # open test case file
    la $a0 inputfile    
    li $a1 0          # for reading
    li $a2 0
    syscall           # file descriptor is stored in v0

    move $a0 $v0      # store file descriptor in s0

    li $v0 14         # read from file
    la $a1 expression # store it in expression buffer
    li $a2 100000     # buffer length
    syscall           # file data is stored in memory (buffer)

    # t0 is expression buffer counter
    li $t0 0


    li $s0 48    # ascii code of 0
    li $s1 57    # ascii code of 9
    li $s2 42    # ascii code of *
    li $s3 43    # ascii code of +
    li $s4 45    # ascii code of -
    li $s5 10    # ascii code of '\n'
    li $s6 4     # offset factor for array
    loop:

        # t1 is array counter (length of array)
        li $t1 0
        bgt $v1 $s7 exit
        j inner_loop


    inner_loop: 
        # loop while t0 < 100000 || expression(t0) = '\n'

        bge $t0 $a2 displayResult

        # t0 < 100000

        lb $t3 expression($t0)

        # increment expression counter by 1
        addi $t0 $t0 1

        beq $t3 $s5 displayResult

        # t3 stores a character of expression

        bge $t3 $s0 ifNumber

        # t3 is not a number

        beq $t3 $s2 mulNumbers
        beq $t3 $s3 addNumbers
        beq $t3 $s4 subNumbers

        # not a valid character

        error_invalid:

            li $v0 4
            la $a0 error_invalidChar
            syscall

            j exit
        error_illegal:

            li $v0 4
            la $a0 error_illegalExp
            syscall

            j exit    
        ifNumber:

            ble $t3 $s1 storeNumber

            # not a number

            j error_invalid
        storeNumber:

            # store value of number in t2
            sub $t2 $t3 $s0

            # calculate offset
            mul $t4 $t1 $s6

            # store word in array
            sw $t2 array($t4)

            # increment array length by 1
            addi $t1 $t1 1

            j inner_loop
        mulNumbers:

            # check if there are fewer than two numbers
            li $t7 2
            blt $t1 $t7 error_illegal

            # t1 >= 2
            
            # decrement counter by 1 and read right operand
            addi $t1 $t1 -1
			# calculate offset
            mul $t4 $t1 $s6
            
			lw $t5 array($t4)
            
            # decrement counter by 1 and read left operand
            addi $t1 $t1 -1
            # calculate offset
            mul $t4 $t1 $s6
			
			lw $t6 array($t4)

            # multiply the numbers
            mul $t7 $t6 $t5

            # store the result in the array
            sw $t7 array($t4)

            # increment size of array by 1
            addi $t1 $t1 1

            j inner_loop
        addNumbers:

            # check if there are fewer than two numbers
            li $t7 2
            blt $t1 $t7 error_illegal

            # t1 >= 2
            
            # decrement counter by 1 and read right operand
            addi $t1 $t1 -1
			# calculate offset
            mul $t4 $t1 $s6
			
            lw $t5 array($t4)
            
            # decrement counter by 1 and read left operand
            addi $t1 $t1 -1
			# calculate offset
            mul $t4 $t1 $s6
			
            lw $t6 array($t4)

            # add the numbers
            add $t7 $t6 $t5

            # store the result in the array
            sw $t7 array($t4)

            # increment size of array by 1
            addi $t1 $t1 1

            j inner_loop
        subNumbers:

            # check if there are fewer than two numbers
            li $t7 2
            blt $t1 $t7 error_illegal

            # t1 >= 2
            
            # decrement counter by 1 and read right operand
            addi $t1 $t1 -1
			# calculate offset
            mul $t4 $t1 $s6
			
            lw $t5 array($t4)
            
            # decrement counter by 1 and read left operand
            addi $t1 $t1 -1
			# calculate offset
            mul $t4 $t1 $s6
			
            lw $t6 array($t4)

            # subtract the numbers
            sub $t7 $t6 $t5

            # store the result in the array
            sw $t7 array($t4)

            # increment size of array by 1
            addi $t1 $t1 1

            j inner_loop     
        displayResult:

            # check if array is containing more than one or less than element
            li $t7 1
            bne $t1 $t7 error_illegal

            # there is exactly one element in the array, and that is the value of the expression
            
            # display result of expression
            li $v0 1
            lw $a0 array($zero)
            syscall

            li $v0 4
            la $a0 msg_lf
            syscall

            addi $v1 $v1 1

            j loop
        exit:

            # exit from program
            li $v0 10
            syscall

.end main
            









                                