	.globl main
	.text

main:
	#print msg1
	li $v0,4
	la $a0, msg1
	syscall

	#input n
	li $v0,5
	syscall
	move $t0,$v0

	#Initialise sum and counter
	li.s $f1, 0.0
	li $t6, 0
	
	#if n == 0
	beq $t0, $t6, error2
	addi $t6, $t6, 1

	#else start input 
	
	#print msg5
	li $v0,4
	la $a0, msg5
	syscall	

	#print cntr
	li $v0,1
	move $a0, $t6
	syscall

	#print msg7
	li $v0,4
	la $a0, msg7
	syscall	

	#input x1
	li $v0,6
	syscall
	mov.s $f2,$f0

	#print msg6
	li $v0,4
	la $a0, msg6
	syscall		

	#print sum
	li $v0,1
	move $a0, $t6
	syscall

	#print msg7
	li $v0,4
	la $a0, msg7
	syscall	

	#input y1
	li $v0,6
	syscall
	mov.s $f3,$f0

loop:
	#If i==n then exit
	beq $t6,$t0,exit

	#i++
	addi $t6,$t6,1

	#print msg5
	li $v0,4
	la $a0, msg5
	syscall	

	#print sum
	li $v0,1
	move $a0, $t6
	syscall

	#print msg7
	li $v0,4
	la $a0, msg7
	syscall	

	#input x2
	li $v0,6
	syscall
	mov.s $f4,$f0

	#print msg6
	li $v0,4
	la $a0, msg6
	syscall	

	#print sum
	li $v0,1
	move $a0, $t6
	syscall
	
	#print msg7
	li $v0,4
	la $a0, msg7
	syscall	

	#input y2
	li $v0,6
	syscall
	mov.s $f5,$f0
	
	#error1
	c.lt.s $f4,$f2
	bc1t error1

	mul.s $f6,$f3,$f5
	c.lt.s $f6,$f12
	bc1f if1	
	bc1t else1

if1:
	sub.s $f7,$f4,$f2
	add.s $f8, $f3, $f5
	mul.s $f7,$f7,$f8
	li.s $f8, 2.0
	div.s $f7, $f7, $f8 
	abs.s $f7,$f7
	j endif1

else1:
	sub.s $f7,$f4,$f2
	mul.s $f8,$f3,$f3
	mul.s $f9,$f5,$f5
	add.s $f8,$f8,$f9
	mul.s $f7,$f7,$f8
	sub.s $f8,$f3,$f5
	div.s $f7, $f7, $f8 
	li.s $f8, 2.0
	div.s $f7, $f7, $f8 
	abs.s $f7,$f7
	j endif1

endif1:
	add.s $f1,$f1,$f7
	mov.s $f2,$f4
	mov.s $f3,$f5
	j loop
   
exit:
	#print msg2
	li $v0,4
	la $a0, msg2
	syscall

	#print sum
	li $v0,2 
	mov.s $f12, $f1
	syscall

	#print msg8
	li $v0,4
	la $a0, msg8
	syscall

	#exit
	li $v0, 10 
	syscall

error1:
	#print msg3
	li $v0,4
	la $a0, msg3
	syscall

	#print msg8
	li $v0,4
	la $a0, msg8
	syscall
	
	#exit
	li $v0, 10 
	syscall
error2:
	#print msg4
	li $v0,4
	la $a0, msg4
	syscall

	#print msg8
	li $v0,4
	la $a0, msg8
	syscall

	#exit
	li $v0, 10 
	syscall

	.data
msg1:	.asciiz "Number of points (n)? "
msg2:	.asciiz "\nArea enclosed is "
msg3:	.asciiz "\nERROR: Input not sorted wrt x "
msg4:	.asciiz "\nERROR: Zero points entered "
msg5:	.asciiz "\nEnter x-coordinate of point "
msg6:	.asciiz "\nEnter y-coordinate of point "
msg7:	.asciiz ":  " 
msg8:	.asciiz "\n\n________________________________\n\n" 
n: .word 0
