.data

    HALF: .float 0.5
    msg_input: .asciiz "Enter the number of points (n): "
    error_zero: .asciiz "Error: Number of points is zero. Area under the curve is not defined"
    error_invalid: .asciiz "Error: Number of points cannot be negative."
    error_notSorted: .asciiz "Error: Points not sorted w.r.t X-coordinate."
    newline: .asciiz "\n"
    msg_point: .asciiz "\nPoint "
    msg_Xcod: .asciiz "Enter X coordinate: "
    msg_Ycod: .asciiz "Enter Y coordinate: "
    msg_separator: .asciiz "\n\n________________________________________________________________\n\n"
    msg_total: .asciiz "\nTotal area enclosed by the line plot and X-axis is: "
    prevX: .float 0.0
    prevY: .float 0.0
    currX: .float 0.0
    currY: .float 0.0
    counter: .word 1
    area: .float 0.0
    
    

.text
.globl main
main:
    
    li $v0 4
    la $a0 msg_input 
    syscall           # prints msg_input on console
    
    li $v0 5
    syscall           # reads user input (keyboard; int)  
                      # and stores it in register v0
    
    move $t0 $v0   # stores the input in register t0 (n; int)
    
    beq $v0 $zero error_zeroInput    # if the number of points is 0,
                                     # then the area is not defined
    
    blt $v0 $zero error_invalidInput # if the number of points is -ve,
                                     # then the input is invalid

    # bne $v0 $zero firstInput

    firstInput:
        
        lw $t1 counter($zero)   # load the counter value (1; int) 
								# into register t1
        
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
        
        li $v0 6
        syscall              # reads user input (keyboard; int; but stored as float)
                             # and stores it in register f0

        s.s $f0 prevX($zero)  # stores the input in memory (prevX; float)
        
        li $v0 4
        la $a0 msg_Ycod
        syscall             # prints msg_Ycod on console

        li $v0 6
        syscall             # reads user input (keyboard; int; but stored as float)
                            # and stores it in register f0

        s.s $f0 prevY($zero) # stores the input in memory (prevY; float)

        addi $t1 $t1 1      # increments the counter by 1  

        l.s $f3 HALF($zero) # load the constant 0.5 into register f3
                            # for future reference          
		
		l.s $f2 area($zero) # load the area (0.0) from memory into register	
							# f2 for future reference
        
        j loop      # jump to 'loop' label

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

        li $v0 6
        syscall             # reads user input (keyboard; int; but stored as float)
                            # and stores it in register f0

        s.s $f0 currX($zero) # stores the input in memory (currX; float)
        
        li $v0 4
        la $a0 msg_Ycod
        syscall            # prints msg_Ycod on console
        
        li $v0 6
        syscall            # reads user input (keyboard; int; but stored as float)
                           # and stores it in register f0

        s.s $f0 currY($zero) # stores the input in memory (currY; float)
        
        l.s $f4 prevX($zero) # load prevX (memory; float) into
                            # register f4

        l.s $f5 prevY($zero) # load prevY (memory; float) into
                            # register f5

        l.s $f6 currX($zero) # load currX (memory; float) into
                            # register f6

        # currY is already stored in register f0, so no need to load it                    
        
        # check if input is sorted w.r.t X coordinate
        
        c.lt.s $f6 $f4
        bc1t error_notInOrder

        # else

        mul.s $f7 $f5 $f0  # multiply prevY and currY, to check if they
                         # are on same side of X-axis or opposite and
                         # store the result in register t5

        li.s $f13 0.0
        # if t5 < 0, go to 'caseOpposite' label    
        c.lt.s $f7 $f13 
        bc1t caseOpposite  
        bc1f caseSameSide           

        # else start 'caseSameSide'
        caseSameSide:

            sub.s $f8 $f6 $f4     # f8 = x2 - x1
            add.s $f9 $f5 $f0     # f9 = y1 + y2
            mul.s $f10 $f8 $f9    # f10 = (y1 + y2) * (x2 - x1) 
            mul.s $f10 $f10 $f3  # f10 = 0.5 * f10
            abs.s $f10 $f10     # store the absolute value of expression
                                # as area under graph is taken to be positive  
            add.s $f2 $f2 $f10  # f2 = f2 + f4 (add calculated area)
            
            j updateParameters        # jump to 'updateArea' label
            
                                                 
        caseOpposite:
            
            sub.s $f8 $f6 $f4    # f8 = x2 - x1
            mul.s $f9 $f5 $f5    # f9 = y1 * y1
            mul.s $f10 $f0 $f0   # f10 = y2 * y2
            add.s $f11 $f10 $f9  # f11 = y1^2 + y2^2
            mul.s $f13 $f8 $f11  # f13 = (x2 - x1) * (y1^2 + y2^2)
            sub.s $f14 $f0 $f5   # f14 = y2 - y1
            mul.s $f13 $f13 $f3  # f13 = 0.5 * f13
            div.s $f13 $f13 $f14 # f13 = f13 / f14
            abs.s $f13 $f13      # store the absolute value of expression
                                 # as area under graph is taken to be positive     
            add.s $f2 $f2 $f13  # f2 = f2 + f6 (add calculated area)

            j updateParameters       # jump to 'updateArea' label

                                
        updateParameters:
            
            # exchange prevX, prevY with currX, currY for next iteration
            s.s $f6 prevX($zero) # store current X value in prevX
            s.s $f0 prevY($zero) # store current Y value in prevY

            addi $t1 $t1 1      # increment counter by 1

            j loop              # go to next iteration of the loop                    


    displayArea:
        
        li $v0 4
        la $a0 msg_total
        syscall              # prints msg_total on console

        li $v0 2
        mov.s $f12 $f2
        syscall              # prints the total calculated area

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
        syscall            # exits from program (stops execution) 

.end main    # end procedure
