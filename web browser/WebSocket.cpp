#include "BrowserParser.h"


int WebSocket::MakeSocket(wstring wstr) {
	if (urlParse(wstr)) {

		int res = InitializeSocket(host.c_str(), port.c_str());
		if (res < 0) {
			printf("socket error");
			return -1;
		}
		request = "GET " + uri + " HTTP/1.1\r\n\r\n";
		return SendAndRecv(request);
	}
	return -1;
}

bool WebSocket::urlParse(wstring wstr) {

	string str(wstr.length(), ' ');
	copy(wstr.begin(), wstr.end(), str.begin());
	string tmp;

	unsigned int pos;

	pos = str.find("://");		//없을경우 -1 반환
	if (pos == -1) {
		protocol = "http";
		tmp = str;
	}
	else {
		protocol = str.substr(0, pos);
		if (protocol.compare("http") == 0) {
			tmp = str.substr(pos + 3);
		}
		else
			return false;
	}

	pos = tmp.find("/");
	if (pos == -1) {
		tmp += '/';
		pos = tmp.length() - 1;
	}

	host = tmp.substr(0, pos);
	tmp = tmp.substr(pos);

	pos = host.find(":");
	if (pos != -1) {
		port = host.substr(pos + 1);
		host = host.substr(0, pos);
	}
	else {
		port = "80";
	}

	uri = tmp;

	return true;
}

wstring WebSocket::getResponse() {
	if (encoding == 1) {
		wstring wstr(recvData.length(), L' ');
		copy(recvData.begin(), recvData.end(), wstr.begin());
		return wstr.data();
	}
	else
		return recvString;
}

int WebSocket::InitializeSocket(const char* server, const char* port) {
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	//create a socket for client
	//addrinfo
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	iResult = getaddrinfo(server, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	//create SOCKET

	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("Error at socket(): %ld\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}

		//connect socket to server
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}
	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	return 0;
}

int WebSocket::SendAndRecv(string request) {
	int contentType;
	int iResult;
	int recvbuflen = 4096;
	char recvbuf[4096];

	recvLen = 0;
	receiveData = NULL;

	iResult = send(ConnectSocket, request.c_str(), request.length(), 0);
	if (iResult == SOCKET_ERROR) {
		printf("send failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return -1;
	}

	//printf("Bytes Sent: %ld\n", iResult);

	// shutdown the connection for sending since no more data will be sent
	// the client can still use the ConnectSocket for receiving data
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return -1;
	}

	// Receive data until the server closes the connection
	do {
		memset(recvbuf, 0, recvbuflen);
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			int tmp = recvLen;
			recvLen += iResult;
			receiveData = (char*)realloc(receiveData, recvLen);

			for (int i = tmp; i < recvLen; i++) {
				receiveData[i] = recvbuf[i - tmp];
			}

			//printf("received byte: %d\n", iResult);
		}
		else if (iResult == 0)
			printf("Connection closed");
		else
			printf("recv failed: %d\n", WSAGetLastError());
	} while (iResult > 0);

	if (recvLen > 0) {
		contentType = contentTypeCheck(receiveData);

		encoding = charsetCheck(receiveData);
		if (encoding == 1) {	//ascii
			recvData.assign(receiveData);
		}
		else {
			int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, receiveData, recvLen, NULL, 0);
			WCHAR* wstr = new WCHAR[sizeNeeded];
			MultiByteToWideChar(CP_UTF8, 0, receiveData, recvLen, wstr, sizeNeeded);
			recvString = wstring(wstr);
			
		}
	}
	// shutdown the send half of the connection since no more data will be sent
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return -1;
	}

	// cleanup
	closesocket(ConnectSocket);
	WSACleanup();

	return contentType;

}

int WebSocket::charsetCheck(char* recieved) {
	char* charset = strstr(recieved, "charset=");
	if (charset == NULL) {
		return 1;		//charset이 적혀있지 않을 경우 ascii로 기본
	}
	else {
		char* charsetEnd = NULL;
		char* charsetEnd1 = strstr(charset, "\r\n");
		char* charsetEnd2 = strstr(charset, "\"");
		char* charsetEnd3 = strstr(charset, "\'");

		if (charsetEnd1!= NULL) {
			//charsetEnd1 is NULL & charsetEnd2 is NULL
			charsetEnd = charsetEnd1;
		}
		else if(charsetEnd3 == NULL){
			charsetEnd = charsetEnd2;
		}
		else if(charsetEnd2 == NULL){
			charsetEnd = charsetEnd3;
		}
		else if(charsetEnd2 < charsetEnd3){
			charsetEnd = charsetEnd2;
		}
		else {
			charsetEnd = charsetEnd3;
		}

		char* check = (char*)malloc(charsetEnd - charset + 1);
		for (int i = 0; i < charsetEnd - charset; i++) {
			check[i] = charset[i];
		}
		check[charsetEnd - charset] = '\0';
		charset = check + strlen("charset=");
		if (strcmp(charset, "us-ascii") == 0 || strcmp(charset, "US-ASCII") == 0) {
			//ascii면
			free(check);
			return 1;
		}
		else if (strcmp(charset, "utf-8") == 0 || strcmp(charset, "UTF-8") == 0) {
			//utf-8이면
			free(check);
			return 0;
		}
		else {
			// 기본은 ascii로 취급
			free(check);
			return 1;
		}
	}

}

int WebSocket::contentTypeCheck(char* recieved) {
	char* contentType = strstr(recieved, "Content-Type:");
	if (contentType == NULL) {
		return TYPE_HTML;
	}
	else {
		char* contentTypeEnd;
		char* contentTypeEnd1 = strstr(contentType, ";");
		char* contentTypeEnd2 = strstr(contentType, "\r\n");
		char* contentTypeEnd3 = strstr(contentType, "\n");

		if (contentTypeEnd1 == NULL && contentTypeEnd2 == NULL) {
			//contentTypeEnd1 is NULL & contentTypeEnd2 is NULL
			contentTypeEnd = contentTypeEnd3;
		}
		else if (contentTypeEnd1 == NULL || contentTypeEnd1 > contentTypeEnd2) {
			contentTypeEnd = contentTypeEnd2;
		}
		else {
			contentTypeEnd = contentTypeEnd1;
		}

		char* check = (char*)malloc(contentTypeEnd - contentType + 1);
		for (int i = 0; i < contentTypeEnd - contentType; i++) {
			check[i] = contentType[i];
		}
		check[contentTypeEnd - contentType] = '\0';
		contentType = check + strlen("Content-Type: ");

		//strcmp 대신 find로 고치는 방법을 고민중

		if (strcmp(contentType, "text/html") == 0) {
			free(check);
			return TYPE_HTML;
		}
		else if (strncmp(contentType, "image/", 6) == 0) {
			free(check);
			return TYPE_IMAGE;
		}
		else if (strncmp(contentType, "text/", 5) == 0) {
			free(check);
			return TYPE_TEXT;
		}
		else if (strncmp(contentType, "application/", 12) == 0) {
			free(check);
			return TYPE_APPLICATION;

		}
		else {
			free(check);
			return TYPE_HTML;
		}
	}
}

