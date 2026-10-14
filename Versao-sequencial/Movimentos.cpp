#pragma once
#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif
#include <windows.h>

#include "raytracing.cpp"

static Cena* gCena;
static Config* gCfg;
static HWND gHwnd;
static vector<unsigned char> gRgb;
static vector<uint32_t> gPix;
static bool gSujo = true, gArrastando = false;
static int gUltX = 0, gUltY = 0;

static bool tecla(int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; }

static bool andar(Vec3& p, double f, double d, double v, double passo, double yMin) {
    if (f == 0 && d == 0 && v == 0) return false;
    double dx = sin(gCena->yaw) * f + cos(gCena->yaw) * d;
    double dz = -cos(gCena->yaw) * f + sin(gCena->yaw) * d;
    double len = sqrt(dx * dx + dz * dz);
    if (len > 1e-9) { p.x += dx / len * passo; p.z += dz / len * passo; }
    p.y = max(yMin, p.y + v * passo);
    return true;
}

static bool mover(double dt) {
    if (GetForegroundWindow() != gHwnd) return false;
    auto eixo = [](int mais, int menos) { return (tecla(mais) ? 1.0 : 0.0) - (tecla(menos) ? 1.0 : 0.0); };
    Esfera& e = gCena->esferas[0];
    bool a = andar(gCena->camera, eixo('W', 'S'), eixo('D', 'A'), eixo(VK_SPACE, VK_LSHIFT), 3.0 * dt, gCena->yChao + 0.1);
    bool b = andar(e.centro, eixo(VK_UP, VK_DOWN), eixo(VK_RIGHT, VK_LEFT), eixo(VK_RSHIFT, VK_RCONTROL), 3.0 * dt, gCena->yChao + e.raio);
    return a || b;
}

static void renderizarQuadro(bool movendo) {
    Config c = *gCfg;
    if (movendo) c.amostras = 1;

    auto t0 = chrono::steady_clock::now();
    renderizarLinhas(*gCena, c, gRgb.data(), 0, c.altura);
    double ms = chrono::duration<double, milli>(chrono::steady_clock::now() - t0).count();

    for (size_t i = 0; i < gPix.size(); i++)
        gPix[i] = ((uint32_t)gRgb[3 * i] << 16) | ((uint32_t)gRgb[3 * i + 1] << 8) | gRgb[3 * i + 2];

    char titulo[200];
    snprintf(titulo, sizeof titulo, "Ray Tracer - %dx%d | %d amostras | %.0f ms | Phong:%d Sombras:%d Reflexos:%d",
             c.largura, c.altura, c.amostras, ms, (int)c.phong, (int)c.sombras, (int)c.reflexoes);
    SetWindowTextA(gHwnd, titulo);
}

static void desenhar(HDC hdc) {
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = gCfg->largura;
    bmi.bmiHeader.biHeight = -gCfg->altura;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(hdc, 0, 0, gCfg->largura, gCfg->altura, 0, 0, gCfg->largura, gCfg->altura,
                  gPix.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

static LRESULT CALLBACK procJanela(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_KEYDOWN:
            if (lp & (1 << 30)) return 0;   // ignora repeticao automatica
            if (wp == '1') { gCfg->phong = !gCfg->phong; gSujo = true; }
            if (wp == '2') { gCfg->sombras = !gCfg->sombras; gSujo = true; }
            if (wp == '3') { gCfg->reflexoes = !gCfg->reflexoes; gSujo = true; }
            if (wp == VK_ESCAPE) DestroyWindow(hwnd);
            return 0;

        case WM_LBUTTONDOWN:
            gArrastando = true;
            gUltX = (short)LOWORD(lp);
            gUltY = (short)HIWORD(lp);
            SetCapture(hwnd);
            return 0;

        case WM_LBUTTONUP:
            gArrastando = false;
            ReleaseCapture();
            return 0;

        case WM_MOUSEMOVE:
            if (gArrastando) {
                int mx = (short)LOWORD(lp), my = (short)HIWORD(lp);
                gCena->yaw += (mx - gUltX) * 0.005;
                gCena->pitch = limitar(gCena->pitch - (my - gUltY) * 0.005, -1.5, 1.5);
                gUltX = mx;
                gUltY = my;
                gSujo = true;
            }
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            desenhar(BeginPaint(hwnd, &ps));
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static int executarJanela(Cena& cena, Config& cfg) {
    SetProcessDPIAware();
    gCena = &cena;
    gCfg = &cfg;
    gRgb.assign((size_t)cfg.largura * cfg.altura * 3, 0);
    gPix.assign((size_t)cfg.largura * cfg.altura, 0);

    WNDCLASSA wc = {};
    wc.lpfnWndProc = procJanela;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.hCursor = LoadCursorA(nullptr, MAKEINTRESOURCEA(32512));
    wc.lpszClassName = "RayTracerJanela";
    RegisterClassA(&wc);

    DWORD estilo = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
    RECT r = {0, 0, cfg.largura, cfg.altura};
    AdjustWindowRect(&r, estilo, FALSE);
    gHwnd = CreateWindowA("RayTracerJanela", "Ray Tracer", estilo, CW_USEDEFAULT, CW_USEDEFAULT,
                          r.right - r.left, r.bottom - r.top, nullptr, nullptr, wc.hInstance, nullptr);
    if (!gHwnd) return 1;
    ShowWindow(gHwnd, SW_SHOW);

    auto anterior = chrono::steady_clock::now();
    bool movendoAntes = false, rodando = true;
    while (rodando) {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) rodando = false;
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        auto agora = chrono::steady_clock::now();
        double dt = min(0.1, chrono::duration<double>(agora - anterior).count());
        anterior = agora;

        bool mexeu = mover(dt), movendo = mexeu || gArrastando;
        if (mexeu || (movendoAntes && !movendo)) gSujo = true;
        movendoAntes = movendo;

        if (gSujo) {
            gSujo = false;
            renderizarQuadro(movendo);
            HDC hdc = GetDC(gHwnd);
            desenhar(hdc);
            ReleaseDC(gHwnd, hdc);
        } else {
            Sleep(1);
        }
    }
    return 0;
}

#endif