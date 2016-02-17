#include "BrowserParser.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StaticProc(HWND, UINT, WPARAM, LPARAM);
int DrawHtml(HDC hdc, RECT* baseRect);
int DrawImage(HDC hDC, RECT* baseRect);
int DrawParagraph(HDC hDC, RECT* baseRect, HFONT hFont, Tag* tags, int tagType);

HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("KM's browser");

HWND hWnd_parent;

HWND hEdit;
WNDPROC editProc;
RECT* editRect;

int receivedType;
ResponseParser* response = NULL;
ContentParser* content;

RECT windowRect;
PAINTSTRUCT ps;
HDC hDC;

int APIENTRY WinMain(HINSTANCE hinstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hinstance;

	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	printf("hello world\n");
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hinstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = NULL;
	WndClass.style = CS_HREDRAW | CS_VREDRAW;

	RegisterClass(&WndClass);

	hWnd_parent = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, (HMENU)NULL, hinstance, NULL);

	ShowWindow(hWnd_parent, nCmdShow);

	while (GetMessage(&Message, NULL, 0, 0))
	{
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	GdiplusShutdown(gdiplusToken);
	return (int)Message.wParam;

}


LRESULT CALLBACK EditProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{

	switch (iMessage)
	{
	case WM_KEYDOWN:
		if (wParam == VK_RETURN) {
			WCHAR str_edit[128];

			GetWindowText(hEdit, str_edit, 128);
			wstring wstr = wstring(str_edit);

			if (response != NULL) {		//response가 비어있지 않다면 비워준다
				delete response;
				response = NULL;
			}

			if (content != NULL) {
				if ((*content).html != NULL)
					delete (*content).html;
				if ((*content).file != NULL)
					delete (*content).file;

				delete content;
				content = NULL;
			}

			WebSocket* wSocket = new WebSocket;
			if ((receivedType = (*wSocket).MakeSocket(wstr)) < 0) {
				printf("error");
				break;
			}

			content = new ContentParser();
			response = new ResponseParser();

			switch (receivedType)
			{
			case TYPE_HTML:				
				(*response).doParse((*wSocket).getResponse());
				(*content).html = new HtmlParser((*response).getMessageBody(), wSocket);
				break;
			case TYPE_TEXT:
				(*response).doParse((*wSocket).getResponse());
				(*content).text = (*response).getMessageBody();
				break;
			case TYPE_IMAGE:
				(*content).file = new FileParser();
				(*(*content).file).setPageSocket(wSocket);
				(*(*content).file).FileDownload(2);				//들어온 data를 직접 download
				break;
			case TYPE_APPLICATION:
				(*content).file = new FileParser();
				(*(*content).file).setPageSocket(wSocket);
				(*(*content).file).FileDownload(2);				//들어온 data를 직접 download
				break;
			default:
				printf("\n---------out----------\n");
				break;
			}

			InvalidateRect(hWnd_parent, NULL, TRUE);

		}
		break;
	}

	return CallWindowProc(editProc, hWnd, iMessage, wParam, lParam);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{

	switch (iMessage)
	{
	case WM_CREATE:
		editRect = new RECT;
		CreateWindow(TEXT("static"), TEXT("url"), WS_CHILD | WS_VISIBLE | SS_CENTER, 10, 5, 30, 25, hWnd, (HMENU)-1, g_hInst, NULL);
		GetClientRect(hWnd, editRect);
		hEdit = CreateWindow(TEXT("edit"), NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, 50, 5, (*editRect).right - (*editRect).left - 60, 25, hWnd, (HMENU)ID_EDIT, g_hInst, NULL);
		editProc = (WNDPROC)SetWindowLongPtr(hEdit, GWL_WNDPROC, (LONG_PTR)EditProc);

		return 0;

	case WM_PAINT:
		GetClientRect(hWnd_parent, editRect);
		SetWindowPos(hEdit, HWND_BOTTOM, 50, 5, (*editRect).right - (*editRect).left - 60, 25, SWP_NOZORDER);

		hDC = BeginPaint(hWnd, &ps);
		
		if (response != NULL)
		{
			RECT* baseRect = new RECT;
			GetClientRect(hWnd_parent, baseRect);
			(*baseRect).left += 10;
			(*baseRect).top += 35;
			(*baseRect).right -= 10;
			(*baseRect).bottom -= 5;
			WCHAR str_edit[128];
			GetWindowText(hEdit, str_edit, 128);
			SetWindowText(hWnd_parent, str_edit);

			switch (receivedType) {
			case TYPE_HTML:
				DrawHtml(hDC, baseRect);
				break;
			case TYPE_IMAGE:
				DrawImage(hDC, baseRect);
				break;
			case TYPE_TEXT:
				DrawText(hDC, (*content).text.c_str(), -1, baseRect, DT_LEFT | DT_WORDBREAK);
				break;
			case TYPE_APPLICATION:
				DrawText(hDC, L"File is downloaded", -1, baseRect, DT_LEFT | DT_WORDBREAK);
				break;
			default:
				SetWindowText(hWnd_parent, L"Socket Error");
				DrawText(hDC, L"Connect Error - check your input please", -1, baseRect, DT_LEFT | DT_WORDBREAK);
				break;
			}

			delete(baseRect);
		}

		EndPaint(hWnd, &ps);
		return 0;

	case WM_DESTROY:
		SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)editProc);
		UpdateWindow(hWnd);
		PostQuitMessage(0);
		return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}

int DrawHtml(HDC hDC, RECT* baseRect) {

	HFONT baseFont;
	Graphics graphics(hDC);
	HtmlParser* hParse = (*content).html;
	wstring title((*hParse).getTitle());
	if (wcscmp(title.c_str(), L"no title tag") != 0) {
		SetWindowText(hWnd_parent, (*hParse).getTitle());
	}

	int windowWidth = (*baseRect).right - (*baseRect).left;
	baseFont = CreateFont(DEFAULT_FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, NULL);
	SelectObject(hDC, baseFont);

	Tag* tags = (*hParse).getBodyTags();
	int i = 0;
	wstring printString(L"");

	DrawParagraph(hDC, baseRect, baseFont, tags, -1);

	return 0;
}

int DrawImage(HDC hDC, RECT* baseRect) {
	Graphics graphics(hDC);
	FileParser* iParse = (*content).file;

	Image img((*iParse).getFileName().c_str(), FALSE);
	Rect imageRect((*baseRect).left, (*baseRect).top, img.GetWidth(), img.GetHeight());
	graphics.DrawImage(&img, imageRect);

	return 0;
}

int DrawParagraph(HDC hDC, RECT* rect, HFONT hFont, Tag* tags, int tagType) {
	wstring printString(L"");
	int index = 0;
	Graphics graphics(hDC);
	ImgTag imgTag;
	Rect imageRect;
	Image* img;
	HFONT h1Font = CreateFont(DEFAULT_FONT_SIZE*2, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, NULL);
	int textHeight = 0;
	SelectObject(hDC, hFont);
	switch (tagType) {
	case 0:
		// <p>
		while (true) {
			if (wcscmp(tags[index].GetTagName().c_str(), L"h1") == 0) {
				SelectObject(hDC, hFont);
				textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
				(*rect).top += textHeight + 5;
				printString = L"";
				//h1에 대한 DrawParagraph
				index += DrawParagraph(hDC, rect, hFont, &tags[index], 1);
			}
			else if (wcscmp(tags[index].GetTagName().c_str(), L"img") == 0) {
				textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
				(*rect).top += textHeight + 1;
				printString = L"";
				//img tag
				index += DrawParagraph(hDC, rect, hFont, &tags[index], 10);
			}
			else if (wcscmp(tags[index].GetTagName().c_str(), L"/p") == 0) {
				break;
			}

			printString.append(tags[index].GetParagraph());

			index++;
		}
		SelectObject(hDC, hFont);
		textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
		(*rect).top += textHeight + 5;
		break;
	case 1:
		// <h1>
		SelectObject(hDC, h1Font);
		while (true) {
			if (wcscmp(tags[index].GetTagName().c_str(), L"p") == 0) {
				SelectObject(hDC, h1Font);
				textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
				(*rect).top += textHeight + 5;
				printString = L"";
				//p에 대한 DrawParagraph
				index += DrawParagraph(hDC, rect, hFont, &(tags[index]), 0);
			}
			else if (wcscmp(tags[index].GetTagName().c_str(), L"img") == 0) {
				textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
				(*rect).top += textHeight + 1;
				printString = L"";
				//img tag
				index += DrawParagraph(hDC, rect, hFont, &tags[index], 10);
			}
			else if (wcscmp(tags[index].GetTagName().c_str(), L"/h1") == 0) {
				break;
			}
			printString.append(tags[index].GetParagraph());
			index++;
		}
		SelectObject(hDC, h1Font);
		textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
		(*rect).top += textHeight + 5;
		break;
	case 10:
		//<img>
		imgTag = ImgTag();
		imgTag.ParseAttribute(tags[index].GetAttribute());
		img = new Image(imgTag.GetSrc().c_str(), FALSE);
		
		imageRect = Rect((*rect).left, (*rect).top, (*img).GetWidth(), (*img).GetHeight());
		graphics.DrawImage(img, imageRect);
		(*rect).top += (*img).GetHeight() + 1;
		delete img;
		
		break;

	default:
		// <body>
		while (true) {

			if (wcscmp(tags[index].GetTagName().c_str(), L"p") == 0) {
				SelectObject(hDC, hFont);
				textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
				(*rect).top += textHeight + 5;
				printString = L"";
				//p에 대한 DrawParagraph
				index += DrawParagraph(hDC, rect, hFont, &(tags[index]), 0);
			}
			else if (wcscmp(tags[index].GetTagName().c_str(), L"h1") == 0) {
				textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
				(*rect).top += textHeight + 5;
				printString = L"";
				//h1에 대한 DrawParagraph
				index += DrawParagraph(hDC, rect, hFont, &tags[index], 1);
			}
			else if (wcscmp(tags[index].GetTagName().c_str(), L"img") == 0) {
				textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
				(*rect).top += textHeight + 1;
				printString = L"";
				//img tag
				index += DrawParagraph(hDC, rect, hFont, &tags[index], 10);
			}
			else if (wcscmp(tags[index].GetTagName().c_str(), L"/body") == 0) {
				break;
			}
			else if (wcscmp(tags[index].GetTagName().c_str(), L"/html") == 0) {
				break;
			}
			printString.append(tags[index].GetParagraph());
			index++;
		}
		SelectObject(hDC, hFont);
		textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
		(*rect).top += textHeight + 5;
	}
	
	return index;
}