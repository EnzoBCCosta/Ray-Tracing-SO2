// Janela interativa (SDL2).
// Controles: W/A/S/D + Espaco/Shift = camera | setas + RShift/RCtrl = esfera vermelha
//            arrastar com o botao esquerdo = girar camera | 1/2/3 = Phong/Sombras/Reflexos | Esc = sair
#pragma once

#include <SDL2/SDL.h>
#include "RayTracing.cpp"

static bool andar(Cena& cena, Vec3& p, double f, double d, double v, double passo, double yMin) {
    if (f == 0 && d == 0 && v == 0) return false;
    double dx = sin(cena.yaw) * f + cos(cena.yaw) * d;
    double dz = -cos(cena.yaw) * f + sin(cena.yaw) * d;
    double len = sqrt(dx * dx + dz * dz);
    if (len > 1e-9) { p.x += dx / len * passo; p.z += dz / len * passo; }
    p.y = max(yMin, p.y + v * passo);
    return true;
}

static bool mover(Cena& cena, double dt) {
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    auto eixo = [&](SDL_Scancode mais, SDL_Scancode menos) { return (k[mais] ? 1.0 : 0.0) - (k[menos] ? 1.0 : 0.0); };
    Esfera& e = cena.esferas[0];
    bool a = andar(cena, cena.camera, eixo(SDL_SCANCODE_W, SDL_SCANCODE_S), eixo(SDL_SCANCODE_D, SDL_SCANCODE_A),
                      eixo(SDL_SCANCODE_SPACE, SDL_SCANCODE_LSHIFT), 3.0 * dt, cena.yChao + 0.1);
    bool b = andar(cena, e.centro, eixo(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN), eixo(SDL_SCANCODE_RIGHT, SDL_SCANCODE_LEFT),
                      eixo(SDL_SCANCODE_RSHIFT, SDL_SCANCODE_RCTRL), 3.0 * dt, cena.yChao + e.raio);
    return a || b;
}

static int executarJanela(Cena& cena, Config& cfg) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "Erro ao iniciar SDL: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* win = SDL_CreateWindow("Ray Tracer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       cfg.largura, cfg.altura, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, 0) : nullptr;
    SDL_Texture* tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
                                               cfg.largura, cfg.altura) : nullptr;
    if (!tex) {
        fprintf(stderr, "Erro ao criar janela: %s\n", SDL_GetError());
        return 1;
    }

    vector<unsigned char> rgb((size_t)cfg.largura * cfg.altura * 3, 0);
    bool sujo = true, arrastando = false, movendoAntes = false, rodando = true;
    int ultX = 0, ultY = 0;
    auto anterior = chrono::steady_clock::now();

    while (rodando) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) rodando = false;
            else if (ev.type == SDL_KEYDOWN && !ev.key.repeat) {
                if (ev.key.keysym.sym == SDLK_1) { cfg.phong = !cfg.phong; sujo = true; }
                if (ev.key.keysym.sym == SDLK_2) { cfg.sombras = !cfg.sombras; sujo = true; }
                if (ev.key.keysym.sym == SDLK_3) { cfg.reflexoes = !cfg.reflexoes; sujo = true; }
                if (ev.key.keysym.sym == SDLK_ESCAPE) rodando = false;
            } else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                arrastando = true; ultX = ev.button.x; ultY = ev.button.y;
            } else if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
                arrastando = false;
            } else if (ev.type == SDL_MOUSEMOTION && arrastando) {
                cena.yaw += (ev.motion.x - ultX) * 0.005;
                cena.pitch = limitar(cena.pitch - (ev.motion.y - ultY) * 0.005, -1.5, 1.5);
                ultX = ev.motion.x; ultY = ev.motion.y;
                sujo = true;
            } else if (ev.type == SDL_WINDOWEVENT && ev.window.event == SDL_WINDOWEVENT_EXPOSED) {
                sujo = true;
            }
        }

        auto agora = chrono::steady_clock::now();
        double dt = min(0.1, chrono::duration<double>(agora - anterior).count());
        anterior = agora;

        bool mexeu = (SDL_GetWindowFlags(win) & SDL_WINDOW_INPUT_FOCUS) ? mover(cena, dt) : false;
        bool movendo = mexeu || arrastando;
        if (mexeu || (movendoAntes && !movendo)) sujo = true;
        movendoAntes = movendo;

        if (sujo) {
            sujo = false;
            Config c = cfg;
            if (movendo) c.amostras = 1;

            auto t0 = chrono::steady_clock::now();
            renderizarParalelo(cena, c, rgb.data());
            double ms = chrono::duration<double, milli>(chrono::steady_clock::now() - t0).count();

            SDL_UpdateTexture(tex, nullptr, rgb.data(), c.largura * 3);
            SDL_RenderClear(ren);
            SDL_RenderCopy(ren, tex, nullptr, nullptr);
            SDL_RenderPresent(ren);

            char titulo[200];
            snprintf(titulo, sizeof titulo, "Ray Tracer - %dx%d | %d amostras | %.0f ms | Phong:%d Sombras:%d Reflexos:%d",
                     c.largura, c.altura, c.amostras, ms, (int)c.phong, (int)c.sombras, (int)c.reflexoes);
            SDL_SetWindowTitle(win, titulo);
        } else {
            SDL_Delay(1);
        }
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}