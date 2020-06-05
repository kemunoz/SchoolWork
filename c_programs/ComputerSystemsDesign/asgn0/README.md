Kevin Munoz
CruzID:kemunoz


This program mimicks the cat command in linux.
To run this program use make in order to produce the executable.


How does it work?
You do not have to give any parameters when running this program. If no parameters are given then it will simply
read from stdin and print it back out to stdout. The same happens when the user enters - as one of the arguments.
If a valid file is given (meaning the file exists and is not a directory) then the program reads the file and prints it out to stdout.
If an invalid file is given then the program will print out a message to stderr and move on to the next argument.