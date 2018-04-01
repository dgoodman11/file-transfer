# file-transfer

This project has 2 files: ftserver.c and ftclient.py. Below are the steps I took to successfully run the program.

1) Open two PuTTY windows, one using flip1.engr.oregonstate.edu, and the other using flip2.engr.oregonstate.edu

2) In the flip1 window, compile the server program with the following command:
	gcc -o ftserver ftserver.c
   Run the program using the command:
	ftserver <PORTNUM>
   I used port 55655 for testing.

3) In the flip2 window, start the client program using the command:
	ftclient.py <SERVERHOST> <SERVERPORT> <COMMAND> <FILENAME> <DATAPORT> 
   <FILENAME> is optional depending on command given For testing, I used the values:
	ftclient.py flip1 55655 -l 55651

4) The two programs will have established two connections, and the server will send the information requested. 

5) After the information has finished transmiting or an error has occurred, the client program will complete and both connections will be closed.

6) To send another command, repeat from step (3) 



Sources used:
1) Python client and server code from Lecture 15
2) docs.python.org and TutorialsPoint for Python syntax
3) Beej's guide
4) Stack Overflow for implementation of functionality to read contents of directory in C 
