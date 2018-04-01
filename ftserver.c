/*****************************************************************************
*Program Description: This program is the server side of a file transfer
*   program implemented in C. It responds to two commands: to send the 
*   client the list of files in the current directory and to send the client
*   a requested file. Upon receiving one of these valid commands, this 
*   program opens a second network connectino over which to send the requested
*   data.
*Name: Danielle Goodman
*Date: 3/12/2018
*****************************************************************************/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <dirent.h>

enum ReceivedCommand {List, GetFile, Invalid};

//header info for functions located below with function definitions
void validateCommandlineArgs(int argCount);
int setupControlSocket(int portNumber);
int acceptControlConnection(int listenSocketFD);
int setupDataSocket(int portNumber);
int validatedPortNumber(char *userEnteredPortNumber);
void validateCommand(enum ReceivedCommand command, int controlFD);
int isValidFilename(char *fileName);
void sendFile(char *fileName, int dataFD);
void sendDirectoryContents(int socketFD);
void executeCommand(enum ReceivedCommand command, int dataPortNum, char *fileName, int controlSocket);
void receiveCommandMessage(int establishedConnectionFD, int *dataPortNum, char *fileName, enum ReceivedCommand *command);


int main(int argc, char *argv[]) {

   int controlConnectionFD = -5; //initialized with value -5 for error checking
   int listenControlFD = -5;
   int dataConnectionFD = -5;
   int dataConnectionPortNum;
   int controlPortNum;
   char fileName[256];
   enum ReceivedCommand command;   

   validateCommandlineArgs(argc);
   controlPortNum = validatedPortNumber(argv[1]);
   listenControlFD = setupControlSocket(controlPortNum);

   //listen for control connections until SIGINT received
   while(1) {
      controlConnectionFD = acceptControlConnection(listenControlFD);
      receiveCommandMessage(controlConnectionFD, &dataConnectionPortNum, fileName, &command);
      validateCommand(command, controlConnectionFD);
      executeCommand(command, dataConnectionPortNum, fileName, controlConnectionFD);
   }   
   return 0;
}

/*****************************************************************************
 * Description: Validates the correct number of arguments were entered
 *    on the command line. If invalid number of arguments, displays error
 *    message and exits program 
 * Inputs: Argment count entered on command line
 * Outputs: None
*****************************************************************************/
void validateCommandlineArgs(int argCount) {
   if(argCount != 2) {
      printf("Error: please start program using format: ftserver.c <PORTNUM>\n");
      exit(1);
   }
}

/*****************************************************************************
 * Description: Sets up listen socket for the control connection
 * Inputs: Port number for control connection. This number is user-entered
 *    from the command line
 * Outputs: File descriptor for the created listen socket
*****************************************************************************/
int setupControlSocket(int portNumber) {

   int listenSocketFD;
   socklen_t sizeOfClientInfo;
   struct sockaddr_in serverAddress;

   memset((char *)&serverAddress, '\0', sizeof(serverAddress));
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_port = htons(portNumber);
   serverAddress.sin_addr.s_addr = INADDR_ANY;

   listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
   bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
   listen(listenSocketFD, 2);

   printf("Socket established.\n");

   return listenSocketFD;
};

/*****************************************************************************
 * Description: Accepts a control connection when received
 * Inputs: File descriptor of the listen socket
 * Outputs: File descriptor of the control connection socket
*****************************************************************************/
int acceptControlConnection(int listenSocketFD) {
   int establishedConnectionFD;
   socklen_t sizeOfClientInfo;
   struct sockaddr_in clientAddress;

   sizeOfClientInfo = sizeof(clientAddress);
   establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);

   printf("Control connection established.\n");

   return establishedConnectionFD;
}

/*****************************************************************************
 * Description: Establishes client-side connection and creates the data 
 *    connection
 * Inputs: Port number for the data connection. This number is sent by the
 *    client connection over the command connection
 * Outputs: File descriptor for the data socket
*****************************************************************************/
int setupDataSocket(int portNumber) {
   int socketFD;
   struct sockaddr_in serverAddress;
   struct hostent* serverHostInfo;

   memset((char*)&serverAddress, '\0', sizeof(serverAddress));

   serverAddress.sin_family = AF_INET;
   serverAddress.sin_port = htons(portNumber);
   serverHostInfo = gethostbyname("flip2");

   memcpy((char*)&serverAddress.sin_addr.s_addr, (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

   socketFD = socket(AF_INET, SOCK_STREAM, 0);

   connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

   printf("Data connection established.\n");
   return socketFD;
}

/*****************************************************************************
 * Description: Validates that the port number entered as argument on the 
 *    command line is valid. If the port number is not valide, an error 
 *    message is displayed and the program exits
 * Inputs: The user-entered command line argument
 * Outputs: The validated port number. 
*****************************************************************************/
int validatedPortNumber(char *userEnteredPortNumber) {
   int portNum = atoi(userEnteredPortNumber);

   if(portNum <= 0 || portNum > 65535) {
      printf("Invalid port number.\n");
      exit(1);
   }
   else if(portNum >= 1 && portNum <= 1023) {
      printf("Please use a port number that's not reserved.\n");
      exit(1);
   }
   else return portNum;
}

/*****************************************************************************
 * Description: Validates command received by the client via the control
 *    connection
 * Inputs: The command received by the client and the control connection
 *   file descriptor
 * Outputs: None
*****************************************************************************/
void validateCommand(enum ReceivedCommand command, int controlFD) {
   char ackMessage[5];
   memset(ackMessage, '\0', 5);
   strcpy(ackMessage, "ACK");

   if(command == Invalid) {
	send(controlFD, "ERROR: Received invalid command. Please enter -g or -l as command.", 67, 0);
   }
   else {
	send(controlFD, ackMessage, sizeof(ackMessage), 0);
   }
}

/*****************************************************************************
 * Description: Receives command message from the client via the control 
 *    connection and sets the "command" variable based on the message
 *    received
 * Inputs: File descriptor of the control connection, data connection port
 *    number, the file name received by the client, and the command received
 *    by the client
 * Outputs: None
*****************************************************************************/
void receiveCommandMessage(int establishedConnectionFD, int *dataPortNum, char *fileName, enum ReceivedCommand *command) {
   char buffer[256];
   char fileTemp[256];
   int portNum;
   memset(buffer, '\0', 256);

   recv(establishedConnectionFD, buffer, 255, 0);

   //if command is list
   if(buffer[0] == '-' && buffer[1] == 'l') {
	sscanf(buffer, "%*s %d" , &portNum);
	*dataPortNum = portNum;
	*command = List;
   }
   //if command is get file
   else if(buffer[0] == '-' && buffer[1] == 'g') {
	sscanf(buffer, "%*s %s %d", fileTemp, &portNum);
	*dataPortNum = portNum;
	strcpy(fileName, fileTemp);
	*command = GetFile;
   }
   else *command = Invalid;  
}

/*****************************************************************************
 * Description: Gets list of files in current directory and sends to the 
 *    client.
 * Inputs: File descriptor of data connection
 * Outputs: None
*****************************************************************************/
void sendDirectoryContents(int socketFD) {
   char filesString[500];
   DIR *d;
   struct dirent *dir;

   memset(filesString, '\0', 500);

   d = opendir(".");
   if(d) {
      while ((dir = readdir(d)) != NULL) {
	if(strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
	   strcat(filesString, dir->d_name);
	   strcat(filesString, "\n");
	}
      }
   }
   filesString[strlen(filesString) - 1] = '\0'; //replace last newline char with null terminator
   send(socketFD, filesString, sizeof(filesString), 0);
}

/*****************************************************************************
 * Description: Confirms file requested is located in current directory
 * Inputs: The file name received by client
 * Outputs: int that acts as bool to indicate if file name is valid
*****************************************************************************/
int isValidFilename(char *fileName) {
   DIR *d;
   struct dirent *dir;
   d = opendir(".");
   if (d) {
      while ((dir = readdir(d)) != NULL) {
	if (strcmp(fileName, dir->d_name) == 0) {
	   closedir(d);   
	   return 1;
	}
      }
   }
   closedir(d);
   return 0;
}

/*****************************************************************************
 * Description: Sends requested file to client
 * Inputs: File and data connection file descriptor
 * Outputs: None
*****************************************************************************/
void sendFile(char *fileName, int dataFD) {
   FILE *f;
   char fileContents[256];
   size_t numBytes = 0;

   memset(fileContents, '\0', 256);

   //open file for reading
   f = fopen(fileName, "r");
   if(f == NULL) {
      printf("Error opening file.\n");
      exit(1);
   }

   printf("Sending requested file.\n");

   //read all data in the file
   while(numBytes = fread(fileContents, 1, 256, f) > 0) { 
      send(dataFD, fileContents, 256, 0);
      memset(fileContents, '\0', 256);
   }   
   fclose(f);
}

/*****************************************************************************
 * Description: Executes the commmand received by the client. Opens a data
 *    connection when valid command is received.
 * Inputs: The command received, port number of the data connection, the
 *    file name, and the control connection file descriptor
 * Outputs: Void
*****************************************************************************/
void executeCommand(enum ReceivedCommand command, int dataPortNum, char *filename, int controlSocket) {
   int dataSocketFD;
   char ACKmessage[256];
   char ERRORmessage[256];

   memset(ACKmessage, '\0', 256);
   memset(ERRORmessage, '\0', 256);

   strcpy(ACKmessage, "ACK: data connection successful");
   strcpy(ERRORmessage, "ERROR: file not found");


   if (command == Invalid) {
	return;
   }
   else if (command == List) {
	//open connection
	dataSocketFD = setupDataSocket(dataPortNum);
	//send ACK for successful data connection
	send(controlSocket, ACKmessage, sizeof(ACKmessage), 0);
	printf("Sending directory contents.\n");
	//send directory contents
	sendDirectoryContents(dataSocketFD);
	//close connection
	printf("Closing data connection.\n");
	close(dataSocketFD);
   }
   else if (command == GetFile) {
	//open connection
	dataSocketFD = setupDataSocket(dataPortNum);
	//validate file name
	if (isValidFilename(filename)) {
	   send(controlSocket, ACKmessage, sizeof(ACKmessage), 0);
	   //send file contents
	   sendFile(filename, dataSocketFD);
	}
	else {
	   send(controlSocket, ERRORmessage, sizeof(ERRORmessage), 0);
	}
	printf("Closing data connection.\n");
	close(dataSocketFD);	
   }
}
