#define WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstring>
#include <ObjIdl.h>
#include <gdiplus.h>
#include <iostream>

#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma comment (lib, "Gdiplus.lib")

#define ID_EDIT 100
#define DEFAULT_BUFLEN 4096
#define DEFAULT_FONT_SIZE 16

#define TYPE_APPLICATION 0			//file save
#define TYPE_HTML 1					//html parsing & print html page
#define TYPE_IMAGE 2				//image file save & print image
#define TYPE_TEXT 3					//print text

using namespace std;
using namespace Gdiplus;

class Tag {
	wstring tagName;
	wstring attribute;
	wstring paragraph;
public:
	int SetTagName(const WCHAR* wstr);
	wstring GetTagName();
	int SetAttribute(const WCHAR* wstr);
	wstring GetAttribute();
	int SetParagraph(const WCHAR* wstr);
	wstring GetParagraph();
};

class BodyTag :public Tag {
public:
	BodyTag();
};

class ImgTag :public Tag {
private:
	wstring alt;
	//float height;
	//float width;
	wstring height;
	wstring width;
	wstring src;
public:
	ImgTag();
	int ParseAttribute(wstring tagInfo);
	wstring GetSrc();
	float GetHeight();
	float GetWidth();
	int SetSrc(wstring wstr);
};

class ATag :public Tag {
private:
	wstring href;
	RECT* aTagRect;
public:
	ATag();
	int ParseAttribute(wstring tagInfo);
	RECT* getATagRect();
};

class WebSocket {
private:
	int charsetCheck(char* recieved);
	int contentTypeCheck(char* recieved);
	WSADATA wsaData;
public:
	string protocol;
	string host;
	string port;
	string uri;
	string request;
	
	SOCKET ConnectSocket;
	
	string recvData;
	wstring recvString;
	char* receiveData;
	int recvLen;
	int encoding;

	bool urlParse(wstring);
	wstring getResponse();
	int MakeSocket(wstring wstr);
	int InitializeSocket(const char* server, const char* host);
	int SendAndRecv(string request);
};

class ResponseParser {
private:
	wstring status;
	wstring* headers;
	wstring messageBody;

public:

	int doParse(wstring response);
	wstring getStatus();
	wstring* getHeaders();
	wstring getMessageBody();

};

class FileParser {
public:
	char* fileData;
	unsigned int fileLength;
	wstring src;
	wstring alt;
	int height;
	int width;
	wstring fileName;
	WebSocket* pageSocket;

	int parseFileData(char* imgData, int fileLen);
	int FileDownload(int check);
	int MakeFile(wstring fileName, void* data, unsigned long fileSize);
	int doImageParse(wstring imageTag, WebSocket* wSocket);
	int setPageSocket(WebSocket* wSocket);
	wstring getFileName();

};

class HtmlParser {
private:
	wstring* html;
	wstring* title;
	wstring* body;
	wstring* images;
	wstring* arrangeHtml(wstring messageBody);
	WebSocket* wSocket;
	Tag* tags;
	int bodyIndex;
	int tagCount;
	int classifyTag();
public:
	int lines;												//line feedÀÇ °¹¼ö
	unsigned int imageCnt;
	FileParser* imageFiles;

	HtmlParser(wstring messageBody, WebSocket* wSocket);
	int doParse(wstring messageBody);
	int splitTag(wstring* html);
	int FindImageTag();
	const WCHAR* getTitle();
	const WCHAR* getHtml();
	Tag* getBodyTags();
	int FindImage();
};

class ContentParser {
public:
	HtmlParser* html;
	FileParser* file;
	wstring text;
	ContentParser() {
		html = NULL;
		file = NULL;
		text = L"";
	}
};
