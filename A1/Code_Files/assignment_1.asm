# ------------------------- START OF CODE --------------------------

.data

    msg_input: .asciiz "Enter the number of points (n): "
    error_zero: .asciiz "Error: Number of points is zero. Area under the curve is not defined."
    error_invalid: .asciiz "Error: Number of points cannot be negative."
    error_notSorted: .asciiz "Error: Points not sorted w.r.t X-coordinate."
    newline: .asciiz "\n"
    msg_point: .asciiz "\nPoint "
    msg_Xcod: .asciiz "Enter X coordinate: "
    msg_Ycod: .asciiz "Enter Y coordinate: "
    msg_separator: .asciiz "\n\n________________________________________________________________\n\n"
    msg_total: .asciiz "\nTotal area enclosed by the line plot and X-axis is: "

	# memory used in loop iterations
	# since area is created by successive points
	# it is sufficient to keep track of just two points
	# at any point in time (iteration)
	
    prevX: .double 0.0    # x1
    prevY: .double 0.0    # y1
    currX: .double 0.0    # x2
    currY: .double 0.0    # y2
    
    

.text
.globl main

main:
    
    li $v0 4
    la $a0 msg_input 
    syscall           # prints msg_input on console
    
    li $v0 5
    syscall           # reads user input (keyboard; int)  
                      # and stores it in register v0
    
    move $t0 $v0      # stores the input in register t0 (n; int)
	
	# t0 stores the "number of points"
    
    beq $v0 $zero error_zeroInput    # if the number of points is 0,
                                     # then the area is not defined
    
    blt $v0 $zero error_invalidInput # if the number of points is -ve,
                                     # then the input is invalid

    # bne $v0 $zero firstInput

    firstInput:
        
        li $t1 1        # t1 is used to store the counter value
						# Initialize it with the value 1 
        
        li $v0 4
        la $a0 msg_point
        syscall          # prints msg_point on console
        
        li $v0 1
        move $a0 $t1
        syscall          # prints point number (1; int) on console
        
        li $v0 4
        la $a0 newline
        syscall          # prints a newline on console

        la $a0 msg_Xcod
        syscall          # prints msg_Xcod on console
        
        li $v0 7
        syscall              # reads user input (keyboard; int; but stored as double)
                             # and stores it in register f0

        s.d $f0 prevX($zero) # stores the input in memory (prevX; double)
        
        li $v0 4
        la $a0 msg_Ycod
        syscall              # prints msg_Ycod on console

        li $v0 7
        syscall              # reads user input (keyboard; int; but stored as double)
                             # and stores it in register f0

        s.d $f0 prevY($zero) # stores the input in memory (prevY; float)

        addi $t1 $t1 1       # increments the counter by 1  

        li.d $f6 0.5         # load the constant 0.5 into register f6
                             # for future reference (f6's value is fixed)          
		
		li.d $f4 0.0         # f4 is used to store the area (fixed purpose)
							 # initialize the area with the value 0.0 before loop
        
        j loop               # jump to 'loop' label


    loop:
        
        # t0 stores number of points and t1 stores the counter value
        # check if counter > n

        bgt $t1 $t0 displayArea # loop complete, so go to displayArea
                                # and display total area enclosed
        
        # else

        li $v0 4
        la $a0 msg_point
        syscall             # prints msg_point on console

        li $v0 1
        move $a0 $t1
        syscall             # prints counter value (int) on console

        li $v0 4
        la $a0 newline
        syscall             # prints a newline on console

        la $a0 msg_Xcod
        syscall             # prints msg_Xcod on console

        li $v0 7
        syscall             # reads user input (keyboard; int; but stored as double)
                            # and stores it in register f0

        s.d $f0 currX($zero) # stores the input in memory (currX; double)
        
        li $v0 4
        la $a0 msg_Ycod
        syscall            # prints msg_Ycod on console
        
        li $v0 7
        syscall            # reads user input (keyboard; int; but stored as double)
                           # and stores it in register f0

        s.d $f0 currY($zero)  # stores the input in memory (currY; double)
        
        l.d $f8 prevX($zero)  # load prevX (memory; double) into
                              # register f8

        l.d $f10 prevY($zero) # load prevY (memory; double) into
                              # register f10

        l.d $f12 currX($zero) # load currX (memory; float) into
                              # register f12

        # currY is already stored in register f0, so no need to load it  
		
		# f8 stores x1
		# f10 stores y1
		# f12 stores x2
		# f0 stores y2
		
		# f4 stores area enclosed till now
		# f6 stores the constant 0.5
        
        # check if input is sorted w.r.t X coordinate
        
        c.lt.d $f12 $f8          # x2 < x1 ?
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

        # else start 'caseSameSide'
        
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

            addi $t1 $t1 1      # increment counter by 1

            j loop              # go to next iteration of the loop                    


    displayArea:
        
        li $v0 4
        la $a0 msg_total
        syscall              # prints msg_total on console

        li $v0 3
        mov.d $f12 $f4
        syscall              # prints the total calculated area (double)

        j exit               # jump to 'exit' label    

    error_zeroInput:   
        
        li $v0 4
        la $a0 error_zero
        syscall               # prints error_zero on console  
        
        j exit                # jump to 'exit' label

    error_invalidInput:

        li $v0 4
        la $a0 error_invalid
        syscall              # prints error_invalid on console

        j exit               # jump to 'exit' label
    
    error_notInOrder:

        li $v0 4
        la $a0 error_notSorted
        syscall                # prints error_notSorted on console

        j exit                # jump to 'exit' label

    exit:
        
        li $v0 4
		la $a0 msg_separator
        syscall               # prints msg_separator on console
        
        li $v0 10
        syscall               # exits from program (stops execution) 

.end main    # end procedure


# ---------------------- END OF CODE -------------------------
