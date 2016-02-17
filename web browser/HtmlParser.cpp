#include "BrowserParser.h"
#include <regex>

HtmlParser::HtmlParser(wstring messageBody, WebSocket* ws) {
	wSocket = ws;
	title = new wstring(L"");
	if (doParse(messageBody) < 0) {
		printf("no message Body");
	}
}

int HtmlParser::doParse(wstring messageBody) {
	html = arrangeHtml(messageBody);
	
	tagCount = splitTag(html);
	bodyIndex = classifyTag();
	FindImage();
	
	return 0;
}

wstring* HtmlParser::arrangeHtml(wstring messageBody) {
	wstring* html = new wstring(messageBody);
	wstring tmp = *(html);
	unsigned int pos1 = 0, pos2;

	while ((pos1 = tmp.find(L"<!--")) != -1) {		//주석은 제거해준다.
		pos2 = tmp.find(L"-->");
		tmp.replace(pos1, pos2 - pos1 + 3, L"");
	}

	wregex tabToSpace(L"[\t\r\n' ']+");
	wregex delSpace(L" <");
	wstring lower = L"";

	tmp = regex_replace(tmp, tabToSpace, L" ");				// \t는 띄어쓰기로 -> ""안의 file name 등은 제외. -> ""안 빼내기 실패
	tmp = regex_replace(tmp, delSpace, L"<");				// < 앞의 \t나 띄어쓰기는 제거
	delSpace = wregex(L"< ");
	tmp = regex_replace(tmp, delSpace, L"<");				// < 바로 안의 \t나 띄어쓰기는 제거
	delSpace = wregex(L" >");
	tmp = regex_replace(tmp, delSpace, L">");				// < 바로 안의 \t나 띄어쓰기는 제거

	while ((pos1 = tmp.find(L"<")) != -1) {					// <> 안의 대소문자는 통일 -> "" 안 (파일 path)는 어떻게 제외해줘야 하지..?? 
		lower += tmp.substr(0, pos1);
		pos2 = tmp.find(L">");
		wstring target = tmp.substr(pos1, pos2 - pos1 + 1);
		WCHAR* tmpChar = new WCHAR[target.length() + 1];
		wcscpy(tmpChar, target.c_str());
		_wcslwr(tmpChar);
		lower.append(tmpChar);
		tmp = tmp.substr(pos2 + 1);
	}

	delete html;
	html = new wstring(lower);

	pos1 = (*html).find(L"<body");
	if (pos1 != -1) {
		wstring wstr = (*html).substr(pos1);
		pos1 = wstr.find(L">");
		pos2 = wstr.find(L"</body>");
		body = new wstring(wstr.substr(pos1 + 1, pos2 - pos1 - 1));
	}
	else
		body = new wstring(L"");
	return html;
}


int HtmlParser::splitTag(wstring* body) {
	
	int count = 0;
	unsigned int pos1 =0, pos2 =0;
	unsigned int pos1End = 0, pos2End = 0;
	wstring tmp = (*body).data();

	while ((pos1 = tmp.find(L"<")) != wstring().npos) {
		pos1End = tmp.find(L">");
		tmp = tmp.substr(pos1End + 1);
		count++;
	}

	tags = new Tag[count];
	tmp = (*body).data();
	int i = 0;
	while ((pos1 = tmp.find(L"<")) != wstring().npos) {
		pos1End = tmp.find(L">");
		wstring tag1 = tmp.substr(pos1, (pos1End - pos1 + 1));
		pos2 = tag1.find(L" ");
		if (pos2 == wstring().npos && tag1.length() > 0) {
			tags[i].SetTagName(tag1.substr(1, tag1.length() - 2).c_str());
			tags[i].SetAttribute(L"");
		}
		else if(tag1.length() > 0){
			tags[i].SetTagName(tag1.substr(1, pos2 - 1).c_str());
			tags[i].SetAttribute(tag1.substr(pos2 + 1, tag1.length() - pos2 - 2).c_str());
		}
		tmp = tmp.substr(pos1End + 1);
		tags[i].SetParagraph(tmp.substr(0, tmp.find(L"<")).c_str());
		i++;
	}

	return count;
}

int HtmlParser::classifyTag() {
	unsigned int iCount = 0;
	int bodyIndex = 0;
	for (int i = 0; i < tagCount; i++) {
		if (wcscmp(tags[i].GetTagName().c_str(), L"title") == 0 ){
			delete title;
			title = new wstring(tags[i].GetParagraph());
		}
		else if (wcscmp(tags[i].GetTagName().c_str(), L"body") == 0) {
			bodyIndex = i;
		}
		else if (wcscmp(tags[i].GetTagName().c_str(), L"img") == 0) {
			ImgTag iTag;
			iTag.ParseAttribute(tags[i].GetAttribute());
			iTag.SetAttribute(tags[i].GetAttribute().c_str());
			iTag.SetParagraph(tags[i].GetParagraph().c_str());
			tags[i] = iTag;
			iCount++;
		}
		else if (wcscmp(tags[i].GetTagName().c_str(), L"br") == 0 || wcscmp(tags[i].GetTagName().c_str(), L"br/") == 0) {
			wstring wstr(L"\n");
			wstr.append(tags[i].GetParagraph().c_str());
			tags[i].SetParagraph(wstr.c_str());
		}
		printf("pause");
	}
	imageCnt = iCount;
	return bodyIndex;
}

const WCHAR* HtmlParser::getTitle() {

	if (wcscmp((*title).c_str(), L"") == 0) {
		return L"no title tag";
	}
	return (*title).c_str();
}

Tag* HtmlParser::getBodyTags() {
	return &(tags[bodyIndex]);
}

const WCHAR* HtmlParser::getHtml() {
	return (*html).c_str();
}

int HtmlParser::FindImage() {
	unsigned int pos1 = 0, pos2 = 0;
	wstring tmp = *body;
	int cnt = 0;

	while ((pos1 = (tmp).find(L"<img ")) != -1) {
		cnt++;
		tmp = tmp.substr(pos1 + 5);
	}

	images = new wstring[cnt];
	tmp = *body;

	for (int i = 0; i < cnt; i++) {
		pos1 = tmp.find(L"<img ");
		tmp = tmp.substr(pos1 + 5);
		pos2 = tmp.find(L">");
		images[i] = tmp.substr(0, pos2);
	}

	imageCnt = cnt;
	
	imageFiles = new FileParser[imageCnt];
	for (int i = 0; i < imageCnt; i++) {

		int check = imageFiles[i].doImageParse(images[i], wSocket);
		if (imageFiles[i].FileDownload(check) < 0) {
			imageCnt--;
			i--;
		}
	}


	return 0;
}

