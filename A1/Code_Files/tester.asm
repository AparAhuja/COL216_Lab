# ------------------------- START OF CODE --------------------------

.data
	
	# absolute file path
    inputfile: .asciiz "C:\\Users\\arnav\\Desktop\\Test_Input.txt"
	
	# buffer for reading file (should be sufficient to read all characters)
    buffer: .space 100001    

    error_notSorted: .asciiz "Error: Points not sorted w.r.t X-coordinate."
    newline: .asciiz "\n"

	# memory used in loop iterations
	# since area is created by successive points
	# it is sufficient to keep track of just two points
	# at any point in time (iteration)
	
    prevX: .double 0.0   # x1
    prevY: .double 0.0   # y1
    currX: .double 0.0   # x2
    currY: .double 0.0   # y2
	

.text
.globl main

main:

    li $t0 1   # initialize with number of test cases (variable)

    li $t1 1   # current test case number 
    li $t2 0   # buffer counter 
	
    li $s1 10  # the constant 10 
    
    li $v0 13         # open test case file
    la $a0 inputfile    
    li $a1 0          # for reading
    li $a2 0
    syscall           # file descriptor is stored in v0

    move $s0 $v0      # store file descriptor in s0

    li $v0 14         # read from file
    move $a0 $s0
    la $a1 buffer     # store it in buffer
    li $a2 100000     # buffer length
    syscall           # file data is stored in memory (buffer)


    loop: #    (looping over test cases)
        
        bgt $t1 $t0 exit    # if current test number is greater than total test
		                    # cases, then exit
        
        li $t8 0
        jal readInt         # read n, stored in s0 
							# 'n' is the number of points in the test case

        move $s7 $s0        # n will be stored in s7

        li $t8 0
        jal readInt         # read x1, stored in s0

        mtc1 $s0 $f0         # move value to f0
        cvt.d.w $f0 $f0      # convert word to double
        s.d $f0 prevX($zero) # store in memory

        li $t8 0
        jal readInt         # read y1, stored in s0

        mtc1 $s0 $f0         # move value to f0
        cvt.d.w $f0 $f0      # convert word to double
        s.d $f0 prevY($zero) # store in memory

        li $t7 2            # point counter
        li.d $f4 0.0        # initialize area with 0.0
        li.d $f6 0.5        # the constant 0.5

        inner_loop:  # (looping over points in a test case)

            bgt $t7 $s7 displayArea     # if counter > n, display Area

            li $t8 0
            jal readInt          # read x2, stored in s0

            mtc1 $s0 $f0         # move value to f0
            cvt.d.w $f0 $f0      # convert word to double
            s.d $f0 currX($zero) # store in memory

            li $t8 0
            jal readInt          # read y2, stored in s0

            mtc1 $s0 $f0         # move value to f0
			cvt.d.w $f0 $f0      # convert word to double
			s.d $f0 currY($zero) # store in memory

            l.d $f8 prevX($zero) # load prevX (memory; double) into
                                 # register f8

            l.d $f10 prevY($zero) # load prevY (memory; double) into
                                  # register f10

            l.d $f12 currX($zero) # load currX (memory; double) into
                                  # register f12

            # currY is already stored in register f0, so no need to load it                    
        
            # check if input is sorted w.r.t X coordinate

            c.lt.d $f12 $f8        # x2 < x1 ?
            bc1t error_notInOrder

            # else

            mul.d $f14 $f10 $f0  # multiply prevY and currY, to check if they
                                 # are on same side of X-axis or opposite and
                                 # store the result in register f14

            # if y1*y2 < 0.0, go to 'caseOpposite' label
		
			li.d $f26 0.0        # load 0.0 in f26 for comparison with 0
		
			c.lt.d $f14 $f26     # f14 < f26 ? 
			bc1t caseOpposite    # if true
			bc1f caseSameSide    # else
			
            caseSameSide:

				sub.d $f16 $f12 $f8     # f16 = x2 - x1
				add.d $f18 $f10 $f0     # f18 = y1 + y2
				mul.d $f20 $f16 $f18    # f20 = (x2 - x1) * (y2 + y1) 
				mul.d $f20 $f20 $f6     # f20 = 0.5 * (x2 - x1) * (y2 + y1)
				abs.d $f20 $f20     	# store the absolute value of expression
										# as area under graph is taken to be positive  
				add.d $f4 $f4 $f20  	# f4 = f4 + f20 (add calculated area to current area)
            
				j updateParameters      # jump to 'updateParameters' label
            
                                                 
            caseOpposite:
            
				sub.d $f16 $f12 $f8     # f16 = x2 - x1
				mul.d $f18 $f10 $f10    # f18 = y1 * y1
				mul.d $f20 $f0 $f0   	# f20 = y2 * y2
				add.d $f22 $f20 $f18  	# f22 = y1^2 + y2^2
				mul.d $f26 $f16 $f22  	# f26 = (x2 - x1) * (y1^2 + y2^2)
				sub.d $f28 $f0 $f10   	# f28 = y2 - y1
				mul.d $f26 $f26 $f6  	# f26 = 0.5 * (x2 - x1) * (y1^2 + y2^2)
				div.d $f26 $f26 $f28 	# f26 = 0.5 * (x2 - x1) * (y1^2 + y2^2) / (y2 - y1)
										# this is the formula for calculating area enclosed when
										# successive points are on opposite sides of the axis
				abs.d $f26 $f26      	# store the absolute value of expression
										# as area under graph is taken to be positive     
				add.d $f4 $f4 $f26  	# f4 = f4 + f26 (add calculated area to the current area)

				j updateParameters       # jump to 'updateParameters' label

                                
            updateParameters:
            
				# make currX as prevX and currY as prevY for next iteration
			
				s.d $f12 prevX($zero) 	# store current X (stored in f12) value in prevX
				s.d $f0 prevY($zero) 	# store current Y (stored in f0) value in prevY

				addi $t7 $t7 1      	# increment point counter by 1

				j inner_loop            # go to next iteration of the 'inner_loop'  

        error_notInOrder:

            li $v0 4
            la $a0 error_notSorted
            syscall                # prints error_notSorted on console    

            addi $t1 $t1 1         # increment test case counter by 1
				
            j loop                 # jump back to 'loop' label

        displayArea:

                li $v0 3
                mov.d $f12 $f4
                syscall               # prints area (double) on console

                li $v0 4
                la $a0 newline
                syscall               # prints newline ('\n')

                addi $t1 $t1 1        # increment test case counter by 1
				
                j loop                # jump back to 'loop' label
				
    exit:
	
        li $v0 10
        syscall          # exit from program


.end main    


# procedure to read integer from a file

readInt:
    
	lb $t3 buffer($t2)     # load character from buffer into t3
    beq $t3 '-' negative   # if t3 represents '-' character
    bne $t3 '-' positive   # if t3 does not represent '-' character

    negative:
	
        li $t9 -1          # store -1 in t9 (negative number)
		
        read_until_neg:

            addi $t2 $t2 1      # increment buffer counter by 1
			
            lb $t3 buffer($t2)  # load character from buffer into t3
            
			blt $t3 '0' update_return_neg  # t3 < '0'  (basically indicates a non-digit ('\n')
            
			sub $t3 $t3 '0'     # store the number (t3 - '0') by subtracting ASCII codes
            mul $t8 $t8 $s1     # t8 = t8 * 10
            add $t8 $t8 $t3     # t8 = t8 * 10 + t3 (number) (forming number from characters)
			
            j read_until_neg    # repeat till all digits are not read

        update_return_neg:
            
            mul $t8 $t8 $t9     # t8 = -1 * t8 (negative)
			
            move $s0 $t8        # save the number is s0

            addi $t2 $t2 1      # increment the buffer counter by 1
            
			jr $ra              # return to main
			
    positive:
	
        li $t9 1         # store 1 in t9 (positive)
		
        read_until_pos:

                lb $t3 buffer($t2)    # load character from buffer into t3
				
                blt $t3 '0' update_return_pos  # t3 < '0'  (basically indicates a non-digit ('\n')
				
                sub $t3 $t3 '0'     # store the number (t3 - '0') by subtracting ASCII codes
                mul $t8 $t8 $s1     # t8 = t8 * 10
                add $t8 $t8 $t3     # t8 = t8 * 10 + t3 (number) (forming number from characters)
                
				addi $t2 $t2 1      # increment buffer counter by 1 
                
				j read_until_pos    # repeat till all digits are not read
				
        update_return_pos:

                mul $t8 $t8 $t9     # t8 = 1 * t8 (positive)
				
                move $s0 $t8        # save the number is s0
				
                addi $t2 $t2 1      # increment buffer counter by 1
				
                jr $ra              # return to main
				
				
.end readInt                

# ---------------------- END OF CODE -------------------------
