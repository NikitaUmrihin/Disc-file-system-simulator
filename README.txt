Disc File System Simulator

==Description==

Program stimulates a disc file managment system.
DISK_SIM_FILE.txt  -> simulates our disc

Program runs in an infinite while loop and asks user's input.
0 : End program
1 : Print open file descriptors and disc content
2 : Format disc to specific block size provided by the user
3 : Create new file
4 : Open file
5 : Close file
6 : Write to file
7 : Read from file
8 : Delete file

==How to compile?==
compile: g++ main.cpp -o x6

run: ./x6

== Input ==
The input is what was defined by the exercise.
Mostly : int (fd), string (FileName, buf)
Input is asked to know what function to call and to use them later 

== Output ==
ListAll function shows disc content, open file descriptors
ReadFromFile shows file content
DISK_SIM_FILE.txt shows content of disc