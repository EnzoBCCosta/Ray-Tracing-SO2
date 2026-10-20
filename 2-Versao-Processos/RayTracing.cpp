// Ray tracing: configuracao, iluminacao (Phong, sombras, reflexoes) e renderizacao por linhas.
#pragma once
#include "Cena.cpp"
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <atomic>
#include <cstring>
#include <new>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

struct Config {
    int largura = 1920, altura = 1080;
    int amostras = 25;  // amostras por pixel (antialiasing)
    int profundidade = 3;   // profundidade maxima de reflexao
    bool sombras = true, reflexoes = true, phong = true;
    string saida;   // se preenchido, grava a imagem .ppm neste caminho (usado no Docker)
    int processos = 1;  // numero de processos (--processos N)
    bool janela = true; // abre a janela (use --janela 0 para so medir o tempo)
};

//Iluminacao
static Vec3 fundo(const Raio& r) {
    double t = 0.5 * (r.direcao.y + 1.0);
    return Vec3(1, 1, 1) * (1.0 - t) + Vec3(0.4, 0.6, 1.0) * t;
}

static Vec3 tracar(const Cena& cena, const Config& cfg, const Raio& r, int nivel) {
    Hit h;
    if (!intersecta(cena, r, 1e30, h)) return fundo(r);

    Vec3 cor = h.mat.cor;   // sem Phong: cor chapada
    if (cfg.phong) {
        Vec3 paraLuz = cena.luz - h.ponto;
        double distLuz = comprimento(paraLuz);
        paraLuz = paraLuz * (1.0 / distLuz);

        cor = h.mat.cor * h.mat.ka;   // ambiente

        Hit tmp;
        bool emSombra = cfg.sombras &&
                        intersecta(cena, Raio{h.ponto + h.normal * EPS, paraLuz}, distLuz, tmp);
        if (!emSombra) {
            double nl = max(0.0, dot(h.normal, paraLuz));
            cor = cor + h.mat.cor * (h.mat.kd * nl);   // difusa
            if (nl > 0) {                              // especular
                double rv = max(0.0, dot(refletir(paraLuz * -1.0, h.normal), r.direcao * -1.0));
                cor = cor + Vec3(1, 1, 1) * (h.mat.ks * pow(rv, h.mat.brilho));
            }
        }
    }

    if (cfg.reflexoes && nivel < cfg.profundidade && h.mat.reflexo > 0) {
        Raio rr{h.ponto + h.normal * EPS, normalizar(refletir(r.direcao, h.normal))};
        cor = cor * (1.0 - h.mat.reflexo) + tracar(cena, cfg, rr, nivel + 1) * h.mat.reflexo;
    }
    return cor;
}

//Renderizacao
struct RngPixel {
    uint64_t s;

    RngPixel(int x, int y) {
        s = ((uint64_t)x * 73856093ULL) ^ ((uint64_t)y * 19349663ULL) ^ 0x9E3779B97F4A7C15ULL;
        if (s == 0) s = 1;
        for (int i = 0; i < 4; i++) proximo();
    }
    uint64_t proximo() { s ^= s << 13; s ^= s >> 7; s ^= s << 17; return s; }
    double uniforme() { return (proximo() >> 11) * (1.0 / 9007199254740992.0); }   // [0,1)
};

static unsigned char paraByte(double v) {
    return (unsigned char)(sqrt(limitar(v, 0.0, 1.0)) * 255.0 + 0.5);   // gamma 2.0
}

// Unidade de trabalho: renderiza as linhas [inicio, fim) no framebuffer RGB (3 bytes/pixel).
// As versoes com processos e threads vao chamar ESTA funcao.
static void renderizarLinhas(const Cena& cena, const Config& cfg, unsigned char* fb, int inicio, int fim) {
    const double aspecto = (double)cfg.largura / cfg.altura;
    const double escala = tan(cena.fov * 0.5 * M_PI / 180.0);
    const Vec3 frente(sin(cena.yaw) * cos(cena.pitch), sin(cena.pitch), -cos(cena.yaw) * cos(cena.pitch));
    const Vec3 direita(cos(cena.yaw), 0.0, sin(cena.yaw));
    const Vec3 cima = cross(direita, frente);

    // amostras quadradas (4, 9, 16...): divide o pixel em grade e sorteia em cada celula (menos ruido)
    const int k = (int)lround(sqrt((double)cfg.amostras));
    const bool grade = cfg.amostras > 1 && k * k == cfg.amostras;

    for (int y = inicio; y < fim; y++) {
        for (int x = 0; x < cfg.largura; x++) {
            RngPixel rng(x, y);
            Vec3 acc;

            for (int s = 0; s < cfg.amostras; s++) {
                double jx = 0.5, jy = 0.5;
                if (grade) {
                    jx = ((s % k) + rng.uniforme()) / k;
                    jy = ((s / k) + rng.uniforme()) / k;
                } else if (cfg.amostras > 1) {
                    jx = rng.uniforme();
                    jy = rng.uniforme();
                }
                double u = (2.0 * (x + jx) / cfg.largura - 1.0) * aspecto * escala;
                double v = (1.0 - 2.0 * (y + jy) / cfg.altura) * escala;
                Raio raio{cena.camera, normalizar(direita * u + cima * v + frente)};
                acc = acc + tracar(cena, cfg, raio, 0);
            }

            acc = acc * (1.0 / cfg.amostras);
            unsigned char* p = fb + ((size_t)y * cfg.largura + x) * 3;
            p[0] = paraByte(acc.x);
            p[1] = paraByte(acc.y);
            p[2] = paraByte(acc.z);
        }
    }
}

// ===== Versao com multiplos processos + memoria compartilhada =====
// Regiao compartilhada: [ atomic<int> proxima | framebuffer RGB ]
struct Compartilhado { std::atomic<int> proxima; };

static bool renderizarProcessos(const Cena& cena, const Config& cfg, unsigned char* fbSaida, int nProc) {
    const size_t bytesFb = (size_t)cfg.largura * cfg.altura * 3;
    const size_t total = sizeof(Compartilhado) + bytesFb;

    void* mem = mmap(nullptr, total, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED) { perror("mmap"); return false; }

    Compartilhado* comp = new (mem) Compartilhado;
    comp->proxima.store(0);
    unsigned char* fb = (unsigned char*)mem + sizeof(Compartilhado);

    const int BLOCO = 8;   // linhas por tarefa
    int criados = 0;
    for (int i = 0; i < nProc; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); break; }
        if (pid == 0) {    // FILHO
            while (true) {
                int inicio = comp->proxima.fetch_add(BLOCO);   // regiao critica (atomica)
                if (inicio >= cfg.altura) break;
                int fim = min(inicio + BLOCO, cfg.altura);
                renderizarLinhas(cena, cfg, fb, inicio, fim);
            }
            _exit(0);
        }
        criados++;
    }

    bool ok = (criados == nProc);
    for (int i = 0; i < criados; i++) {
        int status = 0;
        wait(&status);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) ok = false;
    }

    if (ok) memcpy(fbSaida, fb, bytesFb);
    munmap(mem, total);
    return ok;
}