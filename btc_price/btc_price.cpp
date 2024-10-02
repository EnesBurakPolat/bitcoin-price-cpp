#include <windows.h>
#include <wininet.h>
#include <iostream>
#include <string>
#include <sstream>      // For stringstream
#include <iomanip>      // For setprecision and fixed
#include <locale>      // For locale
#include <algorithm>    // For string manipulation

#pragma comment(lib, "wininet.lib")

#include "resource.h"
//windres resource.rc -O coff -o resource.res
//g++ btc_price.cpp resource.res -o btc_price.exe -lwininet -lgdi32 -mwindows -static -static-libgcc -static-libstdc++

std::string json_data;
double oldPrice = 0;
double lastPercentChange = 0;

// Global değişken olarak renk tutucusu ekleyin
COLORREF backgroundColor = RGB(255, 255, 255);  // Başlangıçta beyaz

// Function to fetch Bitcoin price
double fetchBitcoinPrice() {
    HINTERNET hInternet = InternetOpen("BitcoinPriceApp", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet == NULL) {
        return 0.0;
    }

    HINTERNET hConnect = InternetOpenUrl(hInternet, 
        "https://api.coindesk.com/v1/bpi/currentprice/USD.json", 
        NULL, 0, INTERNET_FLAG_RELOAD, 0);

    if (hConnect == NULL) {
        InternetCloseHandle(hInternet);
        return 0.0;
    }

    char buffer[1024];
    DWORD bytesRead;
    json_data.clear();

    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        json_data.append(buffer, bytesRead);
    }

    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    size_t pos = json_data.find("\"rate_float\":");
    if (pos != std::string::npos) {
        pos += 13;
        std::string price_str = json_data.substr(pos, json_data.find(",", pos) - pos);
        return std::stod(price_str);
    }

    return 0.0;
}

// Binlik ayırıcı ekleyen yardımcı fonksiyon
std::string formatWithThousandsSeparator(double value) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%.2f", value);
    std::string str = buffer;
    
    size_t dot = str.find('.');
    if (dot == std::string::npos) {
        dot = str.length();
    }
    
    for (int i = dot - 3; i > 0; i -= 3) {
        str.insert(i, ".");
    }
    
    return str;
}

// Function to update price and calculate percentage change
void updatePrice(HWND hwnd) {
    double newPrice = fetchBitcoinPrice();
    if (newPrice != oldPrice) {
        lastPercentChange = ((newPrice - oldPrice) / oldPrice) * 100;
        oldPrice = newPrice;

        std::string formattedPrice = formatWithThousandsSeparator(newPrice);

        // Arka plan rengini ayarlayın
        if (lastPercentChange > 0)
            backgroundColor = RGB(200, 255, 200);  // Açık yeşil
        else if (lastPercentChange < 0)
            backgroundColor = RGB(255, 200, 200);  // Açık kırmızı
        else
            backgroundColor = RGB(255, 255, 255);  // Beyaz (değişim yoksa)

        // Pencereyi yeniden çizilmesi için işaretleyin
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

// WindowProc fonksiyonuna WM_PAINT işleyicisi ekleyin
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        SetTimer(hwnd, 1, 5000, NULL);  // 5-second update interval
        return 0;

    case WM_TIMER:
        updatePrice(hwnd);
        return 0;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Arka planı boyayın
        RECT rect;
        GetClientRect(hwnd, &rect);
        HBRUSH hBrush = CreateSolidBrush(backgroundColor);
        FillRect(hdc, &rect, hBrush);
        DeleteObject(hBrush);

        // Metni çizin
        char buffer[256];
        std::string formattedPrice = formatWithThousandsSeparator(oldPrice);
        snprintf(buffer, sizeof(buffer), "Bitcoin Price: %s$\n (Change: %.2f%%)", formattedPrice.c_str(), lastPercentChange);
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 0, 0));  // Siyah metin
        DrawText(hdc, buffer, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "Bitcoin Price Tracker";
    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "Bitcoin Price",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,  // Bu satır değiştirildi
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 150,
        NULL, NULL, hInstance, NULL
    );
    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
