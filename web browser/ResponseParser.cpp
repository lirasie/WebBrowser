#include "BrowserParser.h"


int ResponseParser::doParse(wstring response) {
	wstring tmp;					//������ : \r\n
	wstring headerTmp = L"";							//header�� messagebody�� ������ : \r\n�� �ִ� �κ�
	int headerCnt = 0;
	unsigned int pos;
	unsigned int pos1, pos2;
	int lineDelimeter;

	pos1 = response.find(L"\r\n");
	pos2 = response.find(L"\n");

	if ((pos1 == -1) && (pos2 == -1) ){
		return -1;
	}
	else if (pos1 == -1) {
		lineDelimeter = 2;
		status = response.substr(0, pos2);				//status line�� status�� ����
		tmp = response.substr(pos2 + 1);
	}
	else {
		lineDelimeter = 1;
		status = response.substr(0, pos1);				//status line�� status�� ����
		tmp = response.substr(pos1 + 2);
	}

	switch (lineDelimeter) {
	case 1:
		//header���� �����ڰ� \r\n�� ���
		while ((pos = tmp.find(L"\r\n")) != -1) {
			//header lines�� header temp�� ���Ƽ� ����

			wstring str = tmp.substr(0, pos + 2);
			if (str.compare(L"\r\n") == 0) {
				messageBody = tmp.substr(pos + 2);
				break;
			}
			else {
				headerTmp.append(str);
				headerCnt++;
				tmp = tmp.substr(pos + 2);
			}
		}

		//header line���� �迭�� ��ȯ, headers�� �Ҵ�
		if (headerCnt > 0) {
			headers = new wstring[headerCnt];
			tmp = headerTmp;
			for (int i = 0; i < headerCnt; i++) {
				pos = tmp.find(L"\r\n");
				headers[i] = tmp.substr(0, pos);
				tmp = tmp.substr(pos + 2);
			}
		}
		else {
			headers = new wstring[1];
			headers[0] = L"";
		}
		break;

	case 2:
		//header���� �����ڰ� \n�� ���
		while ((pos = tmp.find(L"\n")) != -1) {
			//header lines�� header temp�� ���Ƽ� ����

			wstring str = tmp.substr(0, pos + 1);
			if (str.compare(L"\n") == 0) {
				messageBody = tmp.substr(pos + 1);
				break;
			}
			else {
				headerTmp.append(str);
				headerCnt++;
				tmp = tmp.substr(pos + 1);
			}
		}

		//header line���� �迭�� ��ȯ, headers�� �Ҵ�
		if (headerCnt > 0) {
			headers = new wstring[headerCnt];
			tmp = headerTmp;
			for (int i = 0; i < headerCnt; i++) {
				pos = tmp.find(L"\n");
				headers[i] = tmp.substr(0, pos);
				tmp = tmp.substr(pos + 1);
			}
		}
		else {
			headers = new wstring[1];
			headers[0] = L"";
		}
		break;
	}

	return 0;
}

wstring ResponseParser::getStatus() {
	return status.data();
}

wstring* ResponseParser::getHeaders() {
	return headers;
}

wstring ResponseParser::getMessageBody() {
	return messageBody.data();
}
