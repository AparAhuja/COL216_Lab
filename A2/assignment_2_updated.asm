.data
    # array used for implementing stack (max. 160 integers)
    array: .space 640
	
	# character buffer used to store the input string
	expression: .space 160
	
	# invalid character error
    error_invalidChar: .asciiz "\nERROR: There is an invalid character present in the expression.\nMake sure that the operands are in the range 0-9, and only +, - and * operators are used.\nProgram terminating!"

	# illegal expression error
    error_illegalExp: .asciiz "\nERROR: The number of operands and number of operators do not match.\nMake sure that the operands are in the range 0-9, and that for each operator exactly 2 operands are provided.\nProgram terminating!"
    
	# overflow error (integer overflow, over 32 bits)
	error_overflow: .asciiz "\nERROR: Arithmetic Overflow"

	# input message
    msg_input: .asciiz "Enter the postfix expression that needs to be evaluated: "

	# output message
    msg_output: .asciiz "\nThe value of the postfix expression is: "

	# separator and newline string
    msg_separator: .asciiz "\n\n_______________________________________________________________\n\n"
    msg_lf: .asciiz "\n"


.text
.globl main
main:

	# print input message on console
    li $v0 4
    la $a0 msg_input
    syscall

	# read input string (expression) and store it in "expression" buffer
	# (max. 160 characters)
    li $v0 8
    la $a0 expression
    li $a1 160
    syscall

	# number of valid characters as last character is a null character
    li $a1 159

    # t0 is expression buffer counter
    li $t0 0

    # t1 is array counter (length of array)
    li $t1 0

    li $s0 48    # ascii code of 0
    li $s1 57    # ascii code of 9
    li $s2 42    # ascii code of *
    li $s3 43    # ascii code of +
    li $s4 45    # ascii code of -
    li $s5 10    # ascii code of '\n'
    li $s6 4     # offset factor for array (integer: 4 byte)
	
    loop: 
        
		# loop while t0 < 159 and expression(t0) != '\n'
        bge $t0 $a1 displayResult

        # t0 < 159
		# so load character (byte)
        lb $t3 expression($t0)

        # increment expression counter by 1
        addi $t0 $t0 1
		
		# check if it is a newline
        beq $t3 $s5 displayResult

        # check if it can be a number
        bge $t3 $s0 ifNumber

        # t3 is not a number
		# check if it an operator
        beq $t3 $s2 mulNumbers     # multiply two numbers
        beq $t3 $s3 addNumbers     # add two numbers
        beq $t3 $s4 subNumbers     # subtract two numbers

        # not a valid character
		# so print error
        error_invalid:

			# print invalid character error
            li $v0 4
            la $a0 error_invalidChar
            syscall

			# go to exit label
            j exit
			
        error_illegal:
			
			# print illegal expression error
            li $v0 4
            la $a0 error_illegalExp
            syscall
			
			# go to exit label
            j exit 
			
        ifNumber:

            # check if character is a number (<= 9)
			# store number if it is a number
			ble $t3 $s1 storeNumber

            # not a number, so raise invalid character error
            j error_invalid
			
        storeNumber:

            # store value of number in t2
            sub $t2 $t3 $s0

            # calculate offset (= array_counter * 4)
            mul $t4 $t1 $s6
            
            # store word in array
            sw $t2 array($t4)

            # increment array length (counter) by 1
            addi $t1 $t1 1
			
			# go to loop
            j loop

        mulNumbers:

            # check if there are fewer than two numbers
            li $t7 2
            blt $t1 $t7 error_illegal

            # t1 >= 2
            
            # decrement counter by 1 and read right operand
            addi $t1 $t1 -1
			
			# calculate offset
            mul $t4 $t1 $s6
           
			# load right operand in t5
            lw $t5 array($t4)
            
            # decrement counter by 1 and read left operand
            addi $t1 $t1 -1
			
            # calculate offset
            mul $t4 $t1 $s6
			
			# load left operand in t6
	        lw $t6 array($t4)

            # multiply the numbers
            mul $t7 $t6 $t5
			
			# check overflow
	        mfhi $t8 
            bgt $t8 $zero overflow
            blt $t7 $zero overflow
			
			# no overflow detected
			
            # store the result in the array (at current address)
            sw $t7 array($t4)

            # increment size of array by 1
            addi $t1 $t1 1

			# go back to loop
            j loop
			
        addNumbers:

            # check if there are fewer than two numbers
            li $t7 2
            blt $t1 $t7 error_illegal

            # t1 >= 2
            
            # decrement counter by 1 and read right operand
            addi $t1 $t1 -1
			
			# calculate offset
            mul $t4 $t1 $s6
            
			# load right operand in t5
            lw $t5 array($t4)
            
            # decrement counter by 1 and read left operand
            addi $t1 $t1 -1
			
			# calculate offset
            mul $t4 $t1 $s6
            
            # load left operand in t6
			lw $t6 array($t4)

            # add the numbers
            add $t7 $t6 $t5

			# Arithmetic overflow is automatically detected by QtSPIM
			
            # store the result in the array (at current address)
            sw $t7 array($t4)

            # increment size of array by 1
            addi $t1 $t1 1

			# go back to loop
            j loop
			
        subNumbers:

            # check if there are fewer than two numbers
            li $t7 2
            blt $t1 $t7 error_illegal

            # t1 >= 2
            
            # decrement counter by 1 and read right operand
            addi $t1 $t1 -1
			
			# calculate offset
            mul $t4 $t1 $s6
            
			# load right operand in t5
            lw $t5 array($t4)
            
            # decrement counter by 1 and read left operand
            addi $t1 $t1 -1
			
			# calculate offset
            mul $t4 $t1 $s6
	        
			# load left operand in t6
            lw $t6 array($t4)

            # subtract the numbers
            sub $t7 $t6 $t5

			# Arithmetic overflow is automatically detected by QtSPIM
			
            # store the result in the array (at current address)
            sw $t7 array($t4)

            # increment size of array by 1
            addi $t1 $t1 1
			
			# go back to loop
            j loop     
			
        displayResult:

            # check if array is containing more than one or less than element
            li $t7 1
            bne $t1 $t7 error_illegal

            # there is exactly one element in the array, and that is the value of the expression
            
            # print output message on console
            li $v0 4
            la $a0 msg_output
            syscall

            # load result in a0, and print the number on consoles(integer)
            lw $a0 array($zero)
            li $v0 1
            syscall
			
			# go to exit label
            j exit
	   
	   overflow:
		   
		   # print overflow error on console
		   li $v0 4
	       la $a0 error_overflow
		   syscall

		   # go to exit label
		   j exit
        
	   exit:

            # print separator
            li $v0 4
            la $a0 msg_separator
            syscall

            # exit from program
            li $v0 10
            syscall

.end main
            
