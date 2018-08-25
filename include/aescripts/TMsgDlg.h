/*
This is a class for displaying cross-platform (Mac and Windows) dialog boxes

It is rather basic in functionality, but usage is simple:
CTMsgDlg cDlg(300,400); // create dialog with size 300x400
int result = cDlg.ShowDialog(
	"titleText", //title of dialog
	"msgText", //message to be displayed
	"OK", //caption of first button
	"Cancel", //optional: caption of second button
	"",  //optional: caption of third button
	true, //true if input textbox should also be displayed
	"" // default text for input text box
	);

Return values:
result: 1 if dialog was closed by pressing one of the buttons, 0 if dialog was closed without pressing a button
cDlg.lBtnSelected contains the button pressed (1,2,3), -1 if the dialog was cancelled
cDlg.sInputText contains the text in the input text box
*/

#ifndef		TMSGDLG_H
#define		TMSGDLG_H


#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define OS_WIN
#else
#define OS_MAC
#endif

#ifdef OS_WIN
#pragma warning (disable:4996)
#include <windows.h>
#endif
#ifdef OS_MAC
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFDictionary.h>
#endif

#include <string>
using namespace std;

class CTMsgDlg
{
private:
#ifdef OS_WIN
	HWND hwndParent, hwndMsgBox, hwndMsgLabel, hwndBtn1, hwndBtn2, hwndBtn3, hwndEditBox;
	HINSTANCE hThisInstance;
	string sClassName;
#endif
#ifdef OS_MAC
#define HINSTANCE long
#define HWND long
#endif
	bool bShowInputBox;
	long lLeftOffset, lTopOffset, lWidth, lHeight, lBtnWidth, lBtnHeight; 
public:
	bool bResult;
	long lInputMaxLength, lBtnSelected;
	string sTitleText, sMsgText, sBtn1Text, sBtn2Text, sBtn3Text, sInputText;

	CTMsgDlg(long width = 300, long height = 140, long btnWidth = 80, long btnHeight = 30,
		long leftOffset = 10, long topOffset = 6, HINSTANCE hInst = 0);
	~CTMsgDlg();
#ifdef OS_WIN
	void create(HWND hwndNew);
	void show();
	void hide();
	void submit();
	void close();
	void destroy();
#endif
	int ShowDialog(
		string titleText, string msgText, 
		string btn1Text = "OK", string btn2Text = "Cancel", string btn3Text = "", 
		bool showInput = false, string inputText = "", 
		HWND hParentWindow = 0);
};

#ifdef OS_WIN

#ifndef GWL_USERDATA
#define GWL_USERDATA        (-21)
#endif

LRESULT CALLBACK TMB_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	CTMsgDlg *_this;
	_this = (CTMsgDlg *)GetWindowLong(hWnd, GWL_USERDATA);
	switch (msg) {
		case WM_CREATE:
			_this = (CTMsgDlg *)((CREATESTRUCT *)lParam)->lpCreateParams;
			SetWindowLong(hWnd, GWL_USERDATA, (long)_this);
			_this->create(hWnd);
			break;
		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDOK: 
				case IDABORT: 
				case IDCANCEL: 
					{
					_this->lBtnSelected = LOWORD(wParam);
					_this->submit();
					_this->close();
					break;
					}
			}
			break;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

CTMsgDlg::CTMsgDlg(long width, long height, long btnWidth, long btnHeight, long leftOffset, long topOffset, HINSTANCE hInst)
{
	WNDCLASSEX wndMsgBox;
	RECT rect;
	memset(this, 0, sizeof(wndMsgBox));
	this->hThisInstance = hInst;
	sTitleText = "Title";
	sMsgText = "Message";
	sBtn1Text = "OK";
	sBtn2Text = "Cancel";
	sBtn3Text = "";
	bShowInputBox = true;
	lLeftOffset = leftOffset;
	lTopOffset = topOffset;
	lWidth = width;
	lHeight = height;
	lBtnWidth = btnWidth;
	lBtnHeight = btnHeight;
	lBtnSelected = -1;
	lInputMaxLength = 100;

	wndMsgBox.cbSize = sizeof(wndMsgBox);
	sClassName = string("TMsgDlg");
	for (int i = 0; i < 10; i++) sClassName += (char)(65 + rand() % 26);
	wndMsgBox.lpszClassName = sClassName.c_str();
	wndMsgBox.style = CS_HREDRAW | CS_VREDRAW;
	wndMsgBox.lpfnWndProc = TMB_WndProc;
	wndMsgBox.lpszMenuName = NULL;
	wndMsgBox.hIconSm = NULL;
	wndMsgBox.cbClsExtra = 0;
	wndMsgBox.cbWndExtra = 0;
	wndMsgBox.hInstance = hInst;
	wndMsgBox.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wndMsgBox.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndMsgBox.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	RegisterClassEx(&wndMsgBox);
	if (this->hwndParent) 
		GetWindowRect(this->hwndParent, &rect);
	else 
		GetWindowRect(GetDesktopWindow(), &rect);

	this->hwndMsgBox = CreateWindow(sClassName.c_str(), sTitleText.c_str(),
		(WS_BORDER | WS_CAPTION), rect.left+(rect.right-rect.left-lWidth)/2,
		rect.top+(rect.bottom-rect.top-lHeight)/2,
		lWidth, lHeight, this->hwndParent, NULL,
		this->hThisInstance, this);
}

void CTMsgDlg::destroy()
{
	EnableWindow(this->hwndParent, true);
	DestroyWindow(this->hwndMsgBox);
}

CTMsgDlg::~CTMsgDlg()
{
	destroy();
	UnregisterClass(sClassName.c_str(), this->hThisInstance);
}

void CTMsgDlg::submit()
{
	this->bResult = true;
	if (!bShowInputBox) return;
	char result[1024];
	strcpy(result, "");

	WORD wInputLength = (int)SendMessage(this->hwndEditBox, EM_LINELENGTH, 0, 0);
	if (wInputLength) {
		*((LPWORD)result) = (WORD)lInputMaxLength;
		wInputLength = (WORD)SendMessage(this->hwndEditBox, EM_GETLINE, 0, (LPARAM)result);
	}
	result[wInputLength] = '\0';
	this->sInputText = string(result);
	PostMessage(this->hwndMsgBox, WM_CLOSE, 0, 0);
}

void CTMsgDlg::create(HWND hwndNew)
{
	HFONT myFont;
	this->hwndMsgBox = hwndNew;
	myFont = (HFONT )GetStockObject(DEFAULT_GUI_FONT);

	SetWindowPos(hwndMsgBox, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	long nwid = lWidth-2*lLeftOffset;
	long swid = nwid/3;
	long pwid = (swid - lBtnWidth)/2;

	this->hwndMsgLabel = CreateWindow("Static", "", WS_CHILD | WS_VISIBLE,
		lLeftOffset,
		lTopOffset, 
		nwid, 
		lBtnHeight*4,
		this->hwndMsgBox, NULL, this->hThisInstance, NULL);
	this->hwndEditBox = CreateWindow("Edit", "", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_LEFT, 
		lLeftOffset,
		lHeight - lTopOffset*2 - lBtnHeight*4, //y
		nwid, 
		lBtnHeight,
		this->hwndMsgBox, NULL, this->hThisInstance, NULL);

	this->hwndBtn1 = CreateWindow("Button", sBtn1Text.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		pwid,
		lHeight - lTopOffset*4 - lBtnHeight*2,
		lBtnWidth, lBtnHeight, this->hwndMsgBox, (HMENU)IDOK,
		this->hThisInstance, NULL);
	this->hwndBtn2 = CreateWindow("Button", sBtn2Text.c_str(),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		swid + pwid,
		lHeight - lTopOffset*4 - lBtnHeight*2,
		lBtnWidth, lBtnHeight,
		this->hwndMsgBox, (HMENU)IDCANCEL, this->hThisInstance, NULL);
	this->hwndBtn3 = CreateWindow("Button", sBtn3Text.c_str(),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		swid*2 + pwid,
		lHeight - lTopOffset*4 - lBtnHeight*2,
		lBtnWidth, lBtnHeight,
		this->hwndMsgBox, (HMENU)IDABORT, this->hThisInstance, NULL);
	SendMessage(this->hwndMsgLabel, WM_SETFONT, (WPARAM)myFont,FALSE);
	SendMessage(this->hwndEditBox, WM_SETFONT, (WPARAM)myFont,FALSE);
	SendMessage(this->hwndBtn1, WM_SETFONT, (WPARAM)myFont,FALSE);
	SendMessage(this->hwndBtn2, WM_SETFONT, (WPARAM)myFont,FALSE);
	SendMessage(this->hwndBtn3, WM_SETFONT, (WPARAM)myFont,FALSE);
}

void CTMsgDlg::close()
{
	PostMessage(this->hwndMsgBox, WM_CLOSE, 0, 0);
}

void CTMsgDlg::hide()
{
	ShowWindow(this->hwndMsgBox, SW_HIDE);
}

void CTMsgDlg::show()
{
	SetWindowText(this->hwndMsgBox, this->sTitleText.c_str());
	SetWindowText(this->hwndEditBox, this->sInputText.c_str());
	SetWindowText(this->hwndMsgLabel, this->sMsgText.c_str());
	SetWindowText(this->hwndBtn1, this->sBtn1Text.c_str());
	SetWindowText(this->hwndBtn2, this->sBtn2Text.c_str());
	SetWindowText(this->hwndBtn3, this->sBtn3Text.c_str());
	SendMessage(this->hwndEditBox, EM_LIMITTEXT, lInputMaxLength, 0);
	SendMessage(this->hwndEditBox, EM_SETSEL, 0, -1);
	SetFocus(this->hwndEditBox);
	ShowWindow(this->hwndMsgBox, SW_NORMAL);
	if (!this->bShowInputBox) ShowWindow(this->hwndEditBox, SW_HIDE);
	if (this->sBtn1Text == "") ShowWindow(this->hwndBtn1, SW_HIDE);
	if (this->sBtn2Text == "") ShowWindow(this->hwndBtn2, SW_HIDE);
	if (this->sBtn3Text == "") ShowWindow(this->hwndBtn3, SW_HIDE);
}

int CTMsgDlg::ShowDialog(string titleText, string msgText, string btn1Text, string btn2Text, string btn3Text, bool showInput, string inputText, HWND hParentWindow)
{
	this->sBtn1Text = btn1Text;
	this->sBtn2Text = btn2Text;
	this->sBtn3Text = btn3Text;
	this->sInputText = inputText;
	this->sTitleText = titleText;
	this->sMsgText = msgText;
	this->bShowInputBox = showInput;
	EnableWindow(hwndParent, false);

	MSG	msg;
	BOOL bRet;
	this->hwndParent = hParentWindow;
	this->bResult = false;
	this->show();
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {

		if (bRet == -1) {
			int error = 1;
		} else {
			if (msg.message == WM_KEYDOWN) {
				switch (msg.wParam) {
				case VK_RETURN: this->submit(); break; 
				case VK_ESCAPE: { this->lBtnSelected = -1; this->close(); } break;
				default: TranslateMessage(&msg); break;
				}
			} else {
				TranslateMessage(&msg);
			}
			DispatchMessage(&msg);	
		}
		if (msg.message == WM_CLOSE) {
			break;
		}
	}
    return this->bResult?1:0;
}
#endif

#ifdef OS_MAC
char* sc(CFStringRef aString);


CTMsgDlg::CTMsgDlg(long width, long height, long btnWidth, long btnHeight, long leftOffset, long topOffset, HINSTANCE hInst)
{
	bShowInputBox = true;
	lLeftOffset = leftOffset;
	lTopOffset = topOffset;
	lWidth = width;
	lHeight = height;
	lBtnWidth = btnWidth;
	lBtnHeight = btnHeight;
	lBtnSelected = -1;
	lInputMaxLength = 100;
}

char* sc(CFStringRef aString)
{
    if (aString == NULL) return NULL;
    
    CFIndex length = CFStringGetLength(aString);
    CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8);
    char *buffer = (char *)malloc(maxSize);
    if (CFStringGetCString(aString, buffer, maxSize, kCFStringEncodingUTF8)) {
        return buffer;
    }
    return NULL;
}

int CTMsgDlg::ShowDialog(string titleText, string msgText, string btn1Text, string btn2Text, string btn3Text, bool showInput, string inputText, HWND hParentWindow)
{
	this->sBtn1Text = btn1Text;
	this->sBtn2Text = btn2Text;
	this->sBtn3Text = btn3Text;
	this->sInputText = inputText;
	this->sTitleText = titleText;
	this->sMsgText = msgText;
	this->bShowInputBox = showInput;
	{
		CFUserNotificationRef dialog;
		SInt32 error;
		CFDictionaryRef dialogTemplate;
		CFOptionFlags responseFlags;
		int button;
		CFStringRef inputRef;
		const void* keysInp[] = {
			kCFUserNotificationAlertHeaderKey,
            kCFUserNotificationTextFieldTitlesKey,
			kCFUserNotificationDefaultButtonTitleKey,
			kCFUserNotificationAlternateButtonTitleKey,
			kCFUserNotificationOtherButtonTitleKey
		};
 		const void* keysNoInp[] = {
			kCFUserNotificationAlertHeaderKey,
			kCFUserNotificationDefaultButtonTitleKey,
			kCFUserNotificationAlternateButtonTitleKey,
			kCFUserNotificationOtherButtonTitleKey
		};       
//        CFStringRef c1 = CFStringCreateWithCString(kCFAllocatorDefault, titleText.c_str(), kCFStringEncodingMacRoman);
        CFStringRef c2 = CFStringCreateWithCString(kCFAllocatorDefault, (titleText + string("\r") + msgText).c_str(), kCFStringEncodingMacRoman);
        CFStringRef c3 = CFStringCreateWithCString(kCFAllocatorDefault, btn3Text.c_str(), kCFStringEncodingMacRoman);
        CFStringRef c4 = CFStringCreateWithCString(kCFAllocatorDefault, btn1Text.c_str(), kCFStringEncodingMacRoman);
        CFStringRef c5 = CFStringCreateWithCString(kCFAllocatorDefault, btn2Text.c_str(), kCFStringEncodingMacRoman);
		const void* valuesInp[] = {
			c2,
			CFSTR(""),
            c3,
            c4,
            c5
		};
        const void* valuesNoInp[] = {
			c2,
            c3,
            c4,
            c5
		};
        if (showInput)
            dialogTemplate = CFDictionaryCreate(kCFAllocatorDefault, keysInp, valuesInp,
                sizeof(keysInp)/sizeof(*keysInp),
                &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        else
            dialogTemplate = CFDictionaryCreate(kCFAllocatorDefault, keysNoInp, valuesNoInp,
                                                sizeof(keysNoInp)/sizeof(*keysNoInp),
                                                &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		this->lBtnSelected = -1;
		dialog = CFUserNotificationCreate(
			kCFAllocatorDefault, 0,
			kCFUserNotificationNoteAlertLevel,
			&error, dialogTemplate);
//        CFRelease(c1);
        CFRelease(c2);
        CFRelease(c3);
        CFRelease(c4);
        CFRelease(c5);
		if (error) return 0;
		error = CFUserNotificationReceiveResponse(dialog, 0, &responseFlags);
		if (error) return 0;
        if (showInput) {
            inputRef = CFUserNotificationGetResponseValue(dialog, 
                kCFUserNotificationTextFieldValuesKey, 
                0);
            char* output=sc(inputRef);
            if (output) {
                this->sInputText = output;
                delete output;
            }
        }
		button = responseFlags & 0x3;
		if (button == kCFUserNotificationAlternateResponse) this->lBtnSelected = 1;
		else if (button == kCFUserNotificationOtherResponse) this->lBtnSelected = 2;
		else if (button == kCFUserNotificationDefaultResponse) this->lBtnSelected = 3;
		else return 0;
	}
    return 1;
}

CTMsgDlg::~CTMsgDlg()
{
}

#endif


#endif


