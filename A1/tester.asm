.data
    inputfile: .asciiz "C:\\Users\\arnav\\Desktop\\sample.txt"   # absolute file path
    buffer: .space 100001    # buffer for reading file (should be sufficient to read all characters)

    n: .word 0
    error_notSorted: .asciiz "Error: Points not sorted w.r.t X-coordinate."
    newline: .asciiz "\n"
    prevX: .float 0.0
    prevY: .float 0.0
    currX: .float 0.0
    currY: .float 0.0
    zero: .float 0.0
    HALF: .float 0.5
    area: .float 0.0

.text
.globl main

main:

    li $t0 3   # initialize with number of test cases (variable)

    li $t1 1   # current test case number 
    li $t2 0   # buffer counter 
    li $s1 10  # the constant 10 
    
    li $v0 13         # open test case file
    la $a0 inputfile    
    li $a1 0          # for reading
    li $a2 0
    syscall

    move $s0 $v0      # store file descriptor in s0

    li $v0 14         # read from file
    move $a0 $s0
    la $a1 buffer     # store it in buffer
    li $a2 100000  # buffer length
    syscall

    loop:
        
        bgt $t1 $t0 exit
        
        li $t8 0
        jal readInt # read n, stored in s0

        move $s7 $s0 # n will be stored in s7

        li $t8 0
        jal readInt # read x1, stored in s0

        mtc1 $s0 $f0
        cvt.s.w $f0 $f0
        s.s $f0 prevX($zero) # store in memory

        li $t8 0
        jal readInt # read y1, stored in s0

        mtc1 $s0 $f0
        cvt.s.w $f0 $f0
        s.s $f0 prevY($zero) # store in memory

        li $t7 2
        l.s $f2 zero($zero)
        l.s $f3 HALF($zero)

        inner_loop:

            bgt $t7 $s7 displayArea

            li $t8 0
            jal readInt # read x2, stored in s0

            mtc1 $s0 $f0
            cvt.s.w $f0 $f0
            s.s $f0 currX($zero) # store in memory

            li $t8 0
            jal readInt # read y2, stored in s0

            mtc1 $s0 $f0
            cvt.s.w $f0 $f0
            s.s $f0 currY($zero) # store in memory

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
            # if f7 < 0, go to 'caseOpposite' label    
            c.lt.s $f7 $f13 
            bc1t caseOpposite  
            bc1f caseSameSide           

            # else start 'caseSameSide'
            caseSameSide:

                sub.s $f8 $f6 $f4     # f8 = x2 - x1
                add.s $f9 $f5 $f0     # f9 = y1 + y2
                mul.s $f10 $f8 $f9    # f10 = (y1 + y2) * (x2 - x1) 
                mul.s $f10 $f10 $f3   # f10 = 0.5 * f10
                abs.s $f10 $f10       # store the absolute value of expression
                                      # as area under graph is taken to be positive  
                add.s $f2 $f2 $f10    # f2 = f2 + f4 (add calculated area)

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
                add.s $f2 $f2 $f13   # f2 = f2 + f6 (add calculated area)

                j updateParameters   # jump to 'updateArea' label

                                
            updateParameters:
            
                # exchange prevX, prevY with currX, currY for next iteration
                s.s $f6 prevX($zero) # store current X value in prevX
                s.s $f0 prevY($zero) # store current Y value in prevY

                addi $t7 $t7 1      # increment counter by 1

                j inner_loop              # go to next iteration of the loop  

            error_notInOrder:

                li $v0 4
                la $a0 error_notSorted
                syscall                # prints error_notSorted on console    

                addi $t1 $t1 1
                j loop

        displayArea:

                li $v0 2
                mov.s $f12 $f2
                syscall

                li $v0 4
                la $a0 newline
                syscall

                addi $t1 $t1 1
                j loop
    exit:

        li $v0 10
        syscall     # exit from program

.end main    

readInt:
    lb $t3 buffer($t2) # load character in t3
    beq $t3 '-' negative
    bne $t3 '-' positive

    negative:
        li $t9 -1
        read_until_neg:

            addi $t2 $t2 1
            lb $t3 buffer($t2)
            blt $t3 '0' update_return_neg
            sub $t3 $t3 '0'
            mul $t8 $t8 $s1
            add $t8 $t8 $t3
            j read_until_neg

        update_return_neg:
            
            mul $t8 $t8 $t9    
            move $s0 $t8
            addi $t2 $t2 1
            jr $ra
    positive:
        li $t9 1
        read_until_pos:

                lb $t3 buffer($t2)
                blt $t3 '0' update_return_pos
                sub $t3 $t3 '0'
                mul $t8 $t8 $s1
                add $t8 $t8 $t3
                addi $t2 $t2 1
                j read_until_pos
        update_return_pos:

                mul $t8 $t8 $t9
                move $s0 $t8
                addi $t2 $t2 1
                jr $ra
.end readInt                


