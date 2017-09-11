#pragma comment(lib, "ws2_32.lib")
#include <WinSock2.h>
#include <iostream>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <ws2tcpip.h>
#include <vector>
#include <fstream>

class Networking {
public:
	// creating a server and connecting the client
	SOCKET connectServer() {
		WSAData wsData;
		WORD ver = MAKEWORD(2, 2);

		int wsOk = WSAStartup(ver, &wsData);
		if (wsOk != 0) {
			std::cerr << "[-] Error [-] | Cannot inialize WinSock!" << std::endl;
			return NULL;
		}

		SOCKET listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
		if (listeningSocket == INVALID_SOCKET) {
			std::cerr << "[-] Error [-] | Cannot create socket! Quitting . . . " << std::endl;
			return NULL;
		}

		SOCKADDR_IN hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(4554);
		hint.sin_addr.S_un.S_addr = INADDR_ANY;

		// binding
		bind(listeningSocket, (SOCKADDR*)&hint, sizeof(hint));
		if (WSAGetLastError() == 0) {
			//std::cout << "bind():  Success" << std::endl;
		}
		else {
			std::cout << "bind():  Error Code >> " << WSAGetLastError() << std::endl;
		}

		// listening
		if (listen(listeningSocket, SOMAXCONN) == SOCKET_ERROR)
			std::cout << "\nError in listening" << std::endl;
		if (WSAGetLastError() == 0) {
			//std::cout << "listen():  Success" << std::endl;
		}
		else {
			std::cout << "listen():  Error Code >> " << WSAGetLastError() << std::endl;
		}

		SOCKADDR_IN client;
		int clientSize = sizeof(client);

		sock = accept(listeningSocket, (SOCKADDR*)&client, &clientSize);
		if (WSAGetLastError() == 0) {
			//std::cout << "accept():  Success" << std::endl;
		}
		else {
			std::cout << "accept():  Error Code >> " << WSAGetLastError() << std::endl;
		}

		char host[NI_MAXHOST];
		char service[NI_MAXHOST];

		ZeroMemory(host, NI_MAXHOST);
		ZeroMemory(service, NI_MAXHOST);

		if (getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0) {
			IpAdress = host;
		}
		else {
			IpAdress = host;
		}

		closesocket(listeningSocket);

		return sock;
	}

	// sending data as 'std::string'
	void sendData(std::string msg) {
		int result = send(sock, msg.c_str(), msg.length(), 0);
		if (result == SOCKET_ERROR) {
			return;
		}
	}
	// sending data as a char array
	void sendData(char msg[]) {
		std::string data = msg;
		int result = send(sock, data.c_str(), data.length(), 0);
		if (result == SOCKET_ERROR) {
			return;
		}
	}

	// recieving data as 'std::string'
	std::string recieveData() {
		std::string output = "";
		char buf[4096];
		int bytes_recv = recv(sock, buf, 4096, 0);
		if (bytes_recv > 0) {
			output = std::string(buf, 0, bytes_recv);
		}
		return output;
	}
	
	// creating client socket and connecting to server
	SOCKET connectClient(char ipAddr[], int port) {
		// initialize winsock
		WSAData data;
		WORD ver = MAKEWORD(2, 2);
		int wsResult = WSAStartup(ver, &data);
		if (wsResult != 0) {
			std::cerr << "Can't start winsock | Error # " << wsResult << std::endl;
			return NULL;
		}

		// create socket
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET) {
			std::cerr << "[-] Error occured when creating a socket | Error # " << WSAGetLastError() << std::endl;
			WSACleanup();
			return NULL;
		}

		// Fill in a hint structure
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(port);
		inet_pton(AF_INET, ipAddr, &hint.sin_addr);

		// connect to the server
		int connResult;
		connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
		while (connResult == SOCKET_ERROR) {
			connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
		}
		return sock;
	}

	// get connected client's IP address
	std::string getClientIp() {
		return IpAdress;
	}

	// file transfer
	// *****************************************************
	// |  Mode 0: Send File     Requires: Source Path      |
	// |  Mode 1: Recieve File  Requires: Destination Path |
	// *****************************************************
	void transferFile(int mode, std::string src, std::string dest) {
		if (mode != 0 && mode != 1) {
			return;
		}

		if (mode == 0)
		{
			const int partitionSize = 4000;
			std::cout << "Reading file . . .\n\n";
			FILE *file;
			file = fopen(src.c_str(), "rb");
			if (file == NULL) { fputs("File error", stderr); exit(-1); }

			// obtaining file size
			fseek(file, 0, SEEK_END);
			long fileSize = ftell(file);
			rewind(file);
			int partitionsCount = (int)(fileSize / partitionSize);
			if ((partitionSize * partitionsCount) < fileSize) {
				partitionsCount++;
			}
			std::cout << "File Size:  " << fileSize;
			std::cout << "\n\nPartitions:  " << partitionsCount << "\n\n";
			std::string partitions = std::to_string((partitionsCount - 1)); // message to the server to notify the number of partitions
			int length = partitions.length();
			send(sock, partitions.c_str(), length, 0);

			int offset = 0;
			for (int i = 0; i < partitionsCount - 1; i++) {
				//fseek(file, offset, SEEK_END);
				char* buf = (char*)malloc(partitionSize);

				size_t result = fread(buf, 1, partitionSize, file);

				Sleep(26);
				int sendResult = send(sock, buf, partitionSize, 0);
				offset += partitionSize;
			}

			// sending info about the last partition
			int lastPartitionSize = fileSize - ((partitionsCount - 1) * partitionSize);
			std::string lastSize = std::to_string(lastPartitionSize); // message to the server to notify the number of partitions
			int len = lastSize.length();
			send(sock, lastSize.c_str(), len, 0);
			std::cout << "Last Partition Size:  " << lastSize << "\n";

			// sending last partition data
			char* buf = (char*)malloc(lastPartitionSize);

			size_t result = fread(buf, 1, lastPartitionSize, file);

			Sleep(26);
			int sendResult = send(sock, buf, lastPartitionSize, 0);

			char lastBuf[4096];
			int last_recv = recv(sock, lastBuf, 4096, 0);
			if (last_recv > 0) {
				std::string output = std::string(lastBuf, 0, last_recv);
				std::cout << output << "\n" << std::endl;
			}

		}
		else
		{
			std::vector<byte> file_buffer;

			// getting number of partitions
			int partitionsCount = 0;
			char buffer[4096];
			int bytes_recv = recv(sock, buffer, 4096, 0);
			if (bytes_recv > 0) {
				std::string output = std::string(buffer, 0, bytes_recv);
				partitionsCount = std::stoi(output);
			}

			std::cout << "Partitions:  " << partitionsCount << std::endl;
			int partition = 1;
			for (int i = 0; i < partitionsCount; i++) {
				const int partitionSize = 4000;
				// receiving data
				char buf[4000];
				int bytes_recv = recv(sock, buf, partitionSize, 0);
				if (bytes_recv > 0) {
					std::cout << "Downloading Partition:  " << (i + 1) << std::endl;
					partition++;
					for (int ii = 0; ii < partitionSize; ii++) {
						file_buffer.push_back(buf[ii]);
					}
				}
			}

			int lastPartitionSize = 0;
			ZeroMemory(buffer, 4096);
			int by_recv = recv(sock, buffer, 4096, 0);
			if (by_recv > 0) {
				std::string output = std::string(buffer, 0, by_recv);
				lastPartitionSize = std::stoi(output);
				std::cout << "Downloading partition:  " << partition << std::endl;
			}

			// receiving last data
			char* buf = new char[lastPartitionSize];
			int b_recv = recv(sock, buf, lastPartitionSize, 0);
			if (b_recv > 0) {
				//std::cout << buf << std::endl;
				for (int ii = 0; ii < lastPartitionSize; ii++) {
					file_buffer.push_back(buf[ii]);
				}
			}
			std::cout << "\n";

			// final steps
			int fileSize = file_buffer.size();
			if (fileSize < 1024)
			{
				std::cout << "File Transfered Successfuly! |  Size:  " << fileSize << "  bytes" << std::endl;
			}
			else if (fileSize > 1024)
			{
				std::cout << "File Transfered Successfuly! |  Size:  " << (fileSize / 1024) << "  kB" << std::endl;
			}
			else if (fileSize > (1024 * 1024))
			{
				std::cout << "File Transfered Successfuly! |  Size:  " << (fileSize / 1024 / 1024) << "  MB" << std::endl;
			}
			else if (fileSize > (1024 * 1024 * 1024))
			{
				std::cout << "File Transfered Successfuly! |  Size:  " << (fileSize / 1024 / 1024 / 1024) << "  GB" << std::endl;
			}

			writeFile(file_buffer, dest);
		}
	}

private:
	SOCKET sock;
	std::string IpAdress = "";

	void writeFile(std::vector<byte> bytes, std::string dest_path) {
		std::string path(dest_path);
		std::ofstream outfile(path, std::ios::binary);
		int size = bytes.size();
		char* buffer = new char[size];
		for (int i = 0; i < size; i++) {
			buffer[i] = bytes.at(i); // filling up the array with needed bytes
		}
		outfile.write(buffer, size);
		// release dynamically-allocated memory
		delete[] buffer;
		outfile.close();
	}
};