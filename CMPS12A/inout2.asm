#inout2.asm
#Author: Kevin Munoz
#This program promots the user for an integer and prints it out
.text
main:
	#prompt user for integer
	li $v0, 4
	la $a0, prompt
	syscall
	#read the integer
	#end the program
	li $v0, 10
	syscall
.data
prompt: .asciiz "Please enter an integer: "
