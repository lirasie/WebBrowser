#include "BrowserParser.h"


int FileParser::doImageParse(wstring imageTag, WebSocket* wSocket) {
	setPageSocket(wSocket);

	unsigned int pos1;
	wstring tmp = imageTag;
	//find src
	pos1 = tmp.find(L"src");
	tmp = tmp.substr(pos1);
	pos1 = tmp.find(L"\"");
	if (pos1 != wstring().npos) {
		tmp = tmp.substr(pos1 + 1);
		pos1 = tmp.find(L"\"");
		src = tmp.substr(0, pos1);
	}
	else {
		pos1 = tmp.find(L"\'");
		tmp = tmp.substr(pos1 + 1);
		pos1 = tmp.find(L"\'");
		src = tmp.substr(0, pos1);
	}

	//src가 http or https 로 시작하는지 아닌지 확인
	if (src.find(L"http://") != -1) {
		return 1;
	}
	else if (src.find(L"https://") != -1) {
		return 1;
	}
	else return 0;
}

int FileParser::FileDownload(int check) {
	string str;
	WebSocket ws;
	switch (check) {
	case 2:
		//direct image download
		ws = (*pageSocket);
		str = ws.uri;
		src = wstring(str.length(), ' ');
		copy(str.begin(), str.end(), src.begin());
		fileName = src.substr(src.find_last_of('/') + 1);
		break;
	case 1:
		
		ws.MakeSocket(src);
		str = ws.uri;
		src = wstring(str.length(), ' ');
		copy(str.begin(), str.end(), src.begin());
		fileName = src.substr(src.find_last_of('/') + 1);

		break;
	case 0:
		
		//host와 port 정보를 html을 받았던 소켓에서 불러온다
		str = "";
		str.append((*pageSocket).host);
		str.append(":" + (*pageSocket).port);
		wstring wstr(str.length(), ' ');
		copy(str.begin(), str.end(), wstr.begin());

		//uri의 정보를 소켓에서 불러온다 (원래 경로 정보)
		unsigned pos = (*pageSocket).uri.find_last_of("/");
		string uri = (*pageSocket).uri.substr(0, pos + 1);

		wstring targetURI(uri.length(),' ');
		copy(uri.begin(), uri.end(), targetURI.begin());

		//파일의 src 정보를 추가해준 후 소켓을 만든다
		
		wstr.append(targetURI + src);
		
		ws.MakeSocket(wstr);
		fileName = src;
		break;
	}

	ResponseParser rp;
	rp.doParse(ws.getResponse());
	if (rp.getStatus().find(L"200") == wstring().npos) {
		return -1;
	}

	parseFileData(ws.receiveData, ws.recvLen);

	MakeFile(fileName, fileData, fileLength);

	return 0;
}

int FileParser::parseFileData(char* data, int fileLen) {
	char* headerCheck;
	char* headerCheck1 = strstr(data, "\r\n\r\n");
	char* headerCheck2 = strstr(data, "\n\n");
	
	if (headerCheck1 == NULL) {
		headerCheck = headerCheck2 + 2;
		*(headerCheck - 1) = 0;
		fileLength = fileLen - strlen(data);
		fileLength -= 1;
	}
	else {
		headerCheck = headerCheck1 + 4;
		*(headerCheck - 1) = 0;
		fileLength = fileLen - strlen(data);
		fileLength -= 2;
	}

	fileData = (char*)malloc(fileLength);
	for (int i = 0; i < fileLength; i++) {
		fileData[i] = headerCheck[i];
	}

	return fileLength;
}

int FileParser::setPageSocket(WebSocket* WebSocket) {
	pageSocket = WebSocket;
	return 0;
}


wstring FileParser::getFileName() {
	return fileName;
}

int FileParser::MakeFile(wstring fileName, void* data, unsigned long fileSize) {

	DWORD d = 0;
	HANDLE hFile = CreateFile(fileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	WriteFile(hFile, data, fileSize, &d, NULL);

	CloseHandle(hFile);
	free(data);

	return 0;
}
