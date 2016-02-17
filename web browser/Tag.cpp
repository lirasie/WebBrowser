#include "BrowserParser.h"

//------------------Tag------------------
int Tag::SetTagName(const WCHAR* wstr) {
	tagName = wstring(wstr);
	return 0;
}
wstring Tag::GetTagName() {
	return tagName;
}
int Tag::SetAttribute(const WCHAR* wstr) {
	attribute = wstring(wstr);
	return 0;
}
wstring Tag::GetAttribute() {
	return attribute;
}
int Tag::SetParagraph(const WCHAR* wstr) {
	paragraph = wstring(wstr);
	return 0;
}
wstring Tag::GetParagraph() {
	return paragraph;
}

//----------------Body--------------------
BodyTag::BodyTag() {
	SetTagName(L"body");
}


//-------------------Image------------------
ImgTag::ImgTag() {
	SetTagName(L"img");
	height = L"";
	width = L"";
}

int ImgTag::ParseAttribute(wstring tagInfo) {
	unsigned int pos1 = tagInfo.find(L"img");
	SetAttribute(tagInfo.substr(pos1 + 4).c_str());

	//height
	pos1 = tagInfo.find(L"height");
	if (pos1 != wstring().npos) {
		wstring wstr = tagInfo.substr(pos1);
		int pos2 = wstr.find(L"\"");
		int pos3 = wstr.find(L"\'");
		if (pos2 != wstring().npos) {
			wstr = wstr.substr(pos2 + 1);
			pos3 = wstr.find(L"\"");
			wstr = wstr.substr(0, pos3);
			//height = stof(wstr);
		}
		else if(pos3 != wstring().npos){
			wstr = wstr.substr(pos3 + 1);
			pos2 = wstr.find(L"\'");
			wstr = wstr.substr(0, pos2);
			//height = stof(wstr);
		}
		height = wstr;
	}

	//width
	pos1 = tagInfo.find(L"width");
	if (pos1 != wstring().npos) {
		wstring wstr = tagInfo.substr(pos1);
		int pos2 = wstr.find(L"\"");
		int pos3 = wstr.find(L"\'");
		if (pos2 != wstring().npos) {
			wstr = wstr.substr(pos2 + 1);
			pos3 = wstr.find(L"\"");
			wstr = wstr.substr(0, pos3);
			//width = stof(wstr);
		}
		else if (pos3 != wstring().npos) {
			wstr = wstr.substr(pos3 + 1);
			pos2 = wstr.find(L"\'");
			wstr = wstr.substr(0, pos2);
			//width = stof(wstr);
		}
		width = wstr;
	}

	//src
	//�̰͵� ��ġ�� �ؾ��ϴµ� ���ƾƾƾƾƾƾ�
	pos1 = tagInfo.find(L"src");
	if (pos1 != wstring().npos) {
		wstring wstr = tagInfo.substr(pos1);
		int pos2 = wstr.find(L"\"");
		int pos3 = wstr.find(L"\'");
		if (pos2 != wstring().npos) {
			wstr = wstr.substr(pos2 + 1);
			pos3 = wstr.find(L"\"");
			wstr = wstr.substr(0, pos3);
			src = wstr;
		}
		else if (pos3 != wstring().npos) {
			wstr = wstr.substr(pos3 + 1);
			pos2 = wstr.find(L"\'");
			wstr = wstr.substr(0, pos2);
			src = wstr;
		}

	}

	return 0;
}

wstring ImgTag::GetSrc() {
	return src;
}

float ImgTag::GetHeight() {
	//return height;
	return 0;
}
float ImgTag::GetWidth() {
	//return width;
	return 0;
}