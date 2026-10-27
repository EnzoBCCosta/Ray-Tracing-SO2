// Ray Tracer 3D sequencial: Phong + sombras + reflexoes.
// Saida: janela interativa (padrao) ou, com --janela 0, so o tempo no terminal (+ .ppm se usar --saida).
// Uso: raytracer [--largura N] [--altura N] [--amostras N] [--profundidade N]
//                [--sombras 0|1] [--reflexoes 0|1] [--phong 0|1] [--janela 0|1] [--saida arq.ppm]
//
// Compile SOMENTE este arquivo (os outros sao incluidos aqui):
//   g++ -O2 -std=c++17 -o raytracer Main.cpp $(sdl2-config --cflags --libs)

#include "Cena.cpp"
#include "RayTracing.cpp"
#include "Movimentos.cpp"

static bool lerInt(const char* texto, int& saida) {
    char* fim = nullptr;
    long v = strtol(texto, &fim, 10);
    if (fim == texto || *fim != '\0') return false;
    saida = (int)v;
    return true;
}

static bool lerArgumentos(int argc, char** argv, Config& cfg) {
    for (int i = 1; i < argc; i += 2) {
        const string a = argv[i];
        if (i + 1 >= argc) {
            fprintf(stderr, "Argumento sem valor: %s\n", argv[i]);
            return false;
        }

        if (a == "--saida") { cfg.saida = argv[i + 1]; continue; }

        int v;
        if (!lerInt(argv[i + 1], v)) {
            fprintf(stderr, "Valor invalido para %s: %s\n", argv[i], argv[i + 1]);
            return false;
        }

        if (a == "--largura") cfg.largura = v;
        else if (a == "--altura") cfg.altura = v;
        else if (a == "--amostras") cfg.amostras = v;
        else if (a == "--profundidade") cfg.profundidade = v;
        else if (a == "--sombras") cfg.sombras = v != 0;
        else if (a == "--reflexoes") cfg.reflexoes = v != 0;
        else if (a == "--phong") cfg.phong = v != 0;
        else if (a == "--janela") cfg.janela = v != 0;
        else if (a == "--threads") cfg.threads = v;
        else {
            fprintf(stderr, "Argumento desconhecido: %s\n", argv[i]);
            return false;
        }
    }

    if (cfg.largura < 1 || cfg.altura < 1 || cfg.amostras < 1 || cfg.profundidade < 0 || cfg.threads < 0) {
        fprintf(stderr, "Valores invalidos.\n");
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    Config cfg;
    if (!lerArgumentos(argc, argv, cfg)) return 1;

    Cena cena = criarCena();

    if (cfg.janela) return executarJanela(cena, cfg);

    vector<unsigned char> fb((size_t)cfg.largura * cfg.altura * 3);

    auto t0 = chrono::steady_clock::now();
    renderizarParalelo(cena, cfg, fb.data());
    double seg = chrono::duration<double>(chrono::steady_clock::now() - t0).count();

    if (!cfg.saida.empty()) {
        FILE* f = fopen(cfg.saida.c_str(), "wb");
        if (!f) {
            fprintf(stderr, "Erro ao salvar %s\n", cfg.saida.c_str());
            return 1;
        }
        fprintf(f, "P6\n%d %d\n255\n", cfg.largura, cfg.altura);
        fwrite(fb.data(), 1, fb.size(), f);
        fclose(f);
    }

    printf("Resolucao: %dx%d | amostras: %d | profundidade: %d | sombras: %d | reflexoes: %d | phong: %d\n",
           cfg.largura, cfg.altura, cfg.amostras, cfg.profundidade, cfg.sombras, cfg.reflexoes, cfg.phong);
           printf("Threads: %d\n", cfg.threads > 0 ? cfg.threads : (int)max(1u, thread::hardware_concurrency()));
           printf("Tempo de execucao: %.3f s\n", seg);
    if (!cfg.saida.empty()) printf("Imagem salva em: %s\n", cfg.saida.c_str());
    return 0;
}
