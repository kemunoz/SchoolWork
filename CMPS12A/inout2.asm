#inout2.asm
#Author: Kevin Munoz
#This program promots the user for an integer and prints it out
.text
main:
	#prompt user to enter a string
	li $v0, 4
	la $a0, prompt
	syscall
	#Read string
	li $v0, 8
	la $a0, input
	la $a1, inputSize
	syscall
	#output the text
	li $v0, 4
	la $a0, output
	syscall
	#Print the string
	li $v0, 4
	la $a0, input
	syscall
	#end the program
	li $v0, 10
	syscall
.data 
prompt: .asciiz "Please enter a string"
input: .word 16
inputSize: .word 15
output: .asciiz "You entered: "