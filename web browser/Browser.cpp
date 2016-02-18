#include "BrowserParser.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK StaticProc(HWND, UINT, WPARAM, LPARAM);
int DrawHtml(HDC hdc, RECT* baseRect);
int DrawImage(HDC hDC, RECT* baseRect);
int DrawParagraph(HDC hDC, RECT* baseRect, HFONT* hFont, Tag* tags, int tagType);

HINSTANCE g_hInst;
LPCTSTR lpszClass = TEXT("KM's browser");

HWND hWnd_parent;

HWND hEdit;
WNDPROC editProc;
RECT* editRect;

int receivedType;
ResponseParser* response = NULL;
ContentParser* content;

ATag* hyperlinkTag;
int hyperlinks;
int maximumLen;
int xPos, yPos;
int xMax, yMax;


RECT windowRect;
PAINTSTRUCT ps;
HDC hDC;
HFONT* baseFont;
HFONT* h1Font;


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
			xPos = 0;
			yPos = 0;
			xMax = 0;
			yMax = 0;


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
	SCROLLINFO si_x, si_y;
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

			CopyRect(&windowRect, baseRect);
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
			if (xPos == 0 && yPos == 0)
			{
				si_x.cbSize = sizeof(SCROLLINFO);
				si_x.fMask = SIF_RANGE | SIF_PAGE;
				si_x.nMin = 0;
				si_x.nMax = xMax;
				si_x.nPage = (*baseRect).right - (*baseRect).left;
				SetScrollInfo(hWnd_parent, SB_HORZ, &si_x, TRUE);

				si_y.cbSize = sizeof(SCROLLINFO);
				si_y.fMask = SIF_RANGE | SIF_PAGE;
				si_y.nMin = 0;
				si_y.nMax = yMax;
				si_y.nPage = (*baseRect).bottom - (*baseRect).top;
				
				SetScrollInfo(hWnd_parent, SB_VERT, &si_y, TRUE);
				SetScrollPos(hWnd_parent, SB_VERT, 0, TRUE);
				SetScrollPos(hWnd_parent, SB_HORZ, 0, TRUE);

			}
		}
		

		EndPaint(hWnd, &ps);
		return 0;
	case WM_HSCROLL:
		si_x.cbSize = sizeof(si_x);
		si_x.fMask = SIF_ALL;

		GetScrollInfo(hWnd, SB_VERT, &si_x);
		xPos = si_x.nPos;

		switch (LOWORD(wParam)) {
		case SB_LINEUP:
			si_x.nPos -= 1;
			break;
		case SB_LINEDOWN:
			si_x.nPos += 1;
			break;
		case SB_PAGEUP:
			si_x.nPos -= si_x.nPage;
			break;
		case SB_PAGEDOWN:
			si_x.nPos += si_x.nPage;
			break;
		case SB_THUMBTRACK:
			si_x.nPos = si_x.nTrackPos;
			break;
		default:
			break;
		}
		si_x.fMask = SIF_POS;
		SetScrollInfo(hWnd, SB_VERT, &si_x, TRUE);
		GetScrollInfo(hWnd, SB_VERT, &si_x);

		if (si_x.nPos != xPos) {
			ScrollWindow(hWnd, 0, xPos - si_x.nPos, &windowRect, &windowRect);
			xPos = si_x.nPos;
			UpdateWindow(hWnd);
		}

		return 0;

	case WM_VSCROLL:
		si_y.cbSize = sizeof(si_y);
		si_y.fMask = SIF_ALL;

		GetScrollInfo(hWnd, SB_VERT, &si_y);
		yPos = si_y.nPos;
	
		switch (LOWORD(wParam)) {
		case SB_LINEUP:
			si_y.nPos -= 1;
			break;
		case SB_LINEDOWN:
			si_y.nPos += 1;
			break;
		case SB_PAGEUP:
			si_y.nPos -= si_y.nPage;
			break;
		case SB_PAGEDOWN:
			si_y.nPos += si_y.nPage;
			break;
		case SB_THUMBTRACK:
			si_y.nPos = si_y.nTrackPos;
			break;
		default:
			break;
		}
		si_y.fMask = SIF_POS;
		SetScrollInfo(hWnd, SB_VERT, &si_y, TRUE);
		GetScrollInfo(hWnd, SB_VERT, &si_y);

		if (si_y.nPos != yPos) {
			ScrollWindow(hWnd, 0, yPos - si_y.nPos, &windowRect, &windowRect);
			yPos = si_y.nPos;
			UpdateWindow(hWnd);
		}

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
	RECT* htmlRect = new RECT({0,0,0,0});
	Graphics graphics(hDC);
	HtmlParser* hParse = (*content).html;
	wstring title((*hParse).getTitle());
	Tag* tags = (*hParse).getBodyTags();
	int i = 0;
	wstring printString(L"");
	int windowWidth = (*baseRect).right - (*baseRect).left;
	
	maximumLen = 0;
	hyperlinks = 0;
	baseFont = new HFONT(CreateFont(DEFAULT_FONT_SIZE, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, NULL));
	h1Font = new HFONT(CreateFont(DEFAULT_FONT_SIZE * 2, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, NULL));

	if (wcscmp(title.c_str(), L"no title tag") != 0) {
		SetWindowText(hWnd_parent, (*hParse).getTitle());
	}
	(*htmlRect).top = (*baseRect).top - yPos;
	(*htmlRect).left = (*baseRect).left - xPos;
	(*htmlRect).bottom = (*htmlRect).top;
	(*htmlRect).right = (*htmlRect).left;
	

	SelectObject(hDC, baseFont);
	i = DrawParagraph(hDC, htmlRect, baseFont, tags, -1);
	printf("%d", i);

	//if(yMax < (*htmlRect).bottom)
	yMax = (*htmlRect).bottom;
	//if(xMax < (*htmlRect).left + maximumLen)
	xMax = (*htmlRect).left + maximumLen;
		
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

int DrawParagraph(HDC hDC, RECT* rect, HFONT* hFont, Tag* tags, int tagType) {
	wstring printString(L"");
	int index = 0;
	Graphics graphics(hDC);
	ImgTag imgTag;
	Rect imageRect;
	Image* img;
	int textHeight = 0;
	SIZE* textStringSize = new SIZE;
	int x, y;

	SelectObject(hDC, (*hFont));

	// <body>
	while (true) {
		printString.append(tags[index].GetParagraph());
		index++;
		if (wcscmp(tags[index].GetTagName().c_str(), L"p") == 0) {
			//p tag를 만났을 때
			
			//font 지정
			SelectObject(hDC, (*hFont));
			
			//p를 만나기 전까지 받았던 string 출력
			GetTextExtentPoint32(hDC, printString.c_str(), printString.size(), textStringSize);
			x = (*textStringSize).cx;
			y = (*textStringSize).cy;
			(*rect).right = (*rect).left + x;
			(*rect).bottom = (*rect).top + y;
			textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);

			//문단 간격을 위해 여백을 만들어 준다
			(*rect).top += textHeight + 5;
			(*rect).bottom += textHeight + 5;
			if (x > maximumLen)
				maximumLen = x;


			printString = L"";
			//p에 대한 DrawParagraph
			index += DrawParagraph(hDC, rect, hFont, &(tags[index]), 0);
		}
		else if (wcscmp(tags[index].GetTagName().c_str(), L"h1") == 0) {
			//h1 태그를 만났을 때

			//font 지정
			SelectObject(hDC, (*hFont));

			//h1을 만나기 전까지 받았던 string 출력
			GetTextExtentPoint32(hDC, printString.c_str(), printString.size(), textStringSize);
			x = (*textStringSize).cx;
			y = (*textStringSize).cy;
			(*rect).right = (*rect).left + x;
			(*rect).bottom = (*rect).top + y;
			textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);

			//문단 간격을 위해 여백을 만들어 준다
			(*rect).top += textHeight + 5;
			(*rect).bottom += textHeight + 5;
			if (x > maximumLen)
				maximumLen = x;

			printString = L"";
			//h1에 대한 DrawParagraph
			index += DrawParagraph(hDC, rect, h1Font, &tags[index], 0);
		}
		
		else if (wcscmp(tags[index].GetTagName().c_str(), L"a") == 0) {
			//a 태그를 만났을 때

			//font 지정
			SelectObject(hDC, (*hFont));

			//a를 만나기 전까지 받았던 string 출력
			GetTextExtentPoint32(hDC, printString.c_str(), printString.size(), textStringSize);
			x = (*textStringSize).cx;
			y = (*textStringSize).cy;
			(*rect).right = (*rect).left + x;
			(*rect).bottom = (*rect).top + y;
			textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
			
			if (x > maximumLen)
				maximumLen = x;

			printString = L"";
			hyperlinks++;
		}
		else if ((wcscmp(tags[index].GetTagName().c_str(), L"br") == 0) || (wcscmp(tags[index].GetTagName().c_str(), L"/br") == 0)){
			//br 태그를 만났을 때
			//출력하고, 높이 기록
			//루프 안으로 돌아오기는 한다
			SelectObject(hDC, (*hFont));
			GetTextExtentPoint32(hDC, printString.c_str(), printString.size(), textStringSize);
			x = (*textStringSize).cx;
			y = (*textStringSize).cy;
			(*rect).right = (*rect).left + x;
			(*rect).bottom = (*rect).top + y;
			textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
			(*rect).top += textHeight + 5;
			(*rect).bottom += textHeight + 5;
			if (x > maximumLen)
				maximumLen = x;

			printString = L"";
		}
		
		else if (wcscmp(tags[index].GetTagName().c_str(), L"img") == 0) {
			//img 태그를 만났을 때
			SelectObject(hDC, (*hFont));

			GetTextExtentPoint32(hDC, printString.c_str(), printString.size(), textStringSize);
			x = (*textStringSize).cx;
			y = (*textStringSize).cy;
			(*rect).right = (*rect).left + x;
			(*rect).bottom = (*rect).top + y;
			textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
			(*rect).top += textHeight + 1;
			(*rect).bottom += textHeight + 1;
			if (x > maximumLen)
				maximumLen = x;

			printString = L"";
			//img tag
			imgTag = ImgTag();
			imgTag.ParseAttribute(tags[index].GetAttribute());
			img = new Image(imgTag.GetSrc().c_str(), FALSE);

			imageRect = Rect((*rect).left, (*rect).top, (*img).GetWidth(), (*img).GetHeight());
			graphics.DrawImage(img, imageRect);
			(*rect).top += (*img).GetHeight() + 1;
			delete img;
			index++;
			//break;
		}
		else if (wcscmp(tags[index].GetTagName().c_str(), L"/p") == 0) {
			break;
		}
		else if (wcscmp(tags[index].GetTagName().c_str(), L"/h1") == 0) {
			break;
		}
		//else if (wcscmp(tags[index].GetTagName().c_str(), L"/a") == 0) {
			// a 태그 닫힘
		//}
		else if (wcscmp(tags[index].GetTagName().c_str(), L"/body") == 0) {
			break;
		}
		else if (wcscmp(tags[index].GetTagName().c_str(), L"/html") == 0) {
			break;
		}
		
	}
	SelectObject(hDC, (*hFont));

	GetTextExtentPoint32(hDC, printString.c_str(), printString.size(), textStringSize);
	x = (*textStringSize).cx;
	y = (*textStringSize).cy;
	(*rect).right = (*rect).left + x;
	(*rect).bottom = (*rect).top + y;
	textHeight = DrawText(hDC, printString.c_str(), -1, rect, DT_LEFT | DT_WORDBREAK);
	(*rect).top += textHeight + 5;
	(*rect).bottom += textHeight + 5;
	if (x > maximumLen)
		maximumLen = x;
	
	delete textStringSize;

	return index;
}