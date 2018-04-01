#!/usr/bin/python

###############################################################################
# Program Description: This program is the client side of a file transfer
#   program implemented in Python. It sends commands to the server to 
#   request to view the contents of the server's directory or to request
#   a specific file to be tranfered
# Name: Danielle Goodman
# Date: 3/12/2018
###############################################################################

from socket import *
import sys
import os.path

###############################################################################
# Description: Validates the command line parameters and exits the program
#   if any of the parameters are invalid
# Inputs: Command line arguments given for the program command
# Outputs: None
###############################################################################
def validateCommandLineParameters():
	if((len(sys.argv) < 5) or (len(sys.argv) > 6)):
		print "Please enter the correct number of arguments"
		sys.exit()
	if(int(sys.argv[2]) < 1025 or int(sys.argv[2]) > 65535):
		print "Please enter a valid port number for the first argument"
		sys.exit()
	if(len(sys.argv) == 6 or sys.argv[3] == "-g"):
		if(".txt" not in sys.argv[4]):
			print "Please enter a .txt file as the 4th parameter if using -g command"
			sys.exit() 

###############################################################################
# Description: Sets up client side of the control connection
# Inputs: Command line arguments given for host name and port number
# Outputs: Control connection file descriptor
###############################################################################
def setupControlConnection():
	serverName = sys.argv[1] 
	serverPort = int(sys.argv[2])
	clientSocket = socket(AF_INET, SOCK_STREAM)
	clientSocket.connect((serverName, serverPort))
	return clientSocket

###############################################################################
# Description: Sets up server side of data connection 
# Inputs: Command line argument given for data connection port number
# Outputs: Listen data socket file descriptor
###############################################################################
def setupDataConnection():
	if(len(sys.argv) == 6):
		serverPort = int(sys.argv[5]);
	elif(len(sys.argv) == 5):
		serverPort = int(sys.argv[4]);

	serverSocket = socket(AF_INET, SOCK_STREAM)
	serverSocket.bind(('', serverPort))
	serverSocket.listen(1)
	return serverSocket	


###############################################################################
# Description: Sends command to server via control socket
# Inputs: Control socket file descriptor, command line argument for command
#   and command line arguments for data port number and file name
# Outputs: None
###############################################################################
def sendCommand(clientSocket):
	command = sys.argv[3]

	if(len(sys.argv) == 5):
		dataPortNum = sys.argv[4]
		sendMessage = command + ' ' + dataPortNum
		
	elif(len(sys.argv) == 6):
		fileName = sys.argv[4]
		dataPortNum = sys.argv[5]
		sendMessage = command + ' ' + fileName + ' ' + dataPortNum

	clientSocket.send(sendMessage)

###############################################################################
# Description: Accepts data connection 
# Inputs: Listen connection socket file descriptor
# Outputs: Control connection socket file descriptor
###############################################################################
def acceptDataConnection(serverSocket):
	connectionSocket, addr = serverSocket.accept()
	return connectionSocket

###############################################################################
# Description: Saves data as file. If the file name given as the command line
#   argument already exists in the current directory, the file is saved
#   under a different name 
# Inputs: The data to be saved as a file
# Outputs: None
###############################################################################
def saveDataAsFile(fileData):
	fileNameSaveAs = sys.argv[4]

	if os.path.isfile(sys.argv[4]): #if file already exists
		print "Requested file already exists. Renaming file to " + "DUP" + sys.argv[4] 	
		fileNameSaveAs = "DUP" + fileNameSaveAs	
	
	f = open(fileNameSaveAs, 'w')
	f.write(fileData)
	f.close()

###############################################################################
# Description: Receives message via control connection. Exits program if
#   receives an error message
# Inputs: Control connection socket file descriptor
# Outputs: None
###############################################################################
def receiveControlMessage(serverSocket):
	messageRecv = serverSocket.recv(1024)
#if message is error, exit program
	if "ERROR" in messageRecv:
		print sys.argv[1] + " says " + messageRecv
		serverSocket.close()
		sys.exit()
#if message is acknowledgement, do nothing
	elif "ACK" in messageRecv:
		pass
	else:
		print messageRecv

###############################################################################
# Description: Receives message via data connection
# Inputs: Data connection socket file descriptor
# Outputs: None
###############################################################################
def receiveDataMessage(dataSocket):
	if sys.argv[3] == "-g":
		tFile = ''
		while True:
			recvData = dataSocket.recv(256)
			tFile += recvData
			if len(recvData) < 256:
				break
		saveDataAsFile(tFile)
		print "File transfer complete."
	else:
		recvData = dataSocket.recv(1024)
		print recvData


#run the program
validateCommandLineParameters()

#set up control and data sockets
controlSocket = setupControlConnection()
dataListenSocket = setupDataConnection()

sendCommand(controlSocket)

#receive ACK of sent command
receiveControlMessage(controlSocket)

#open data connection and receive messages
dataSocket = acceptDataConnection(dataListenSocket)
receiveControlMessage(controlSocket)
receiveDataMessage(dataSocket)

print "Closing control connection."
controlSocket.close()
