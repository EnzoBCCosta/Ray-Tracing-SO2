# Ray Tracer 3D (versao com processos) - container autocontido
#
# Rodar a partir da raiz do projeto (a pasta que contem Main.cpp e a pasta container):
#
#   docker build -f container/Dockerfile -t raytracer .
#
# Entrada padrao do programa: 1920x1080, 25 amostras, 1 processo (use --processos N).

FROM ubuntu:24.04

# g++ para compilar e libsdl2-dev (o Main.cpp inclui a janela SDL2 na compilacao)
RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ libsdl2-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Compila somente o Main.cpp (os demais .cpp sao incluidos por ele)
RUN g++ -O2 -std=c++17 -o raytracer Main.cpp $(sdl2-config --cflags --libs)

# Pasta onde a imagem .ppm e gravada (monte um volume com -v para pegar o arquivo)
RUN mkdir -p /saida && chmod 777 /saida

# No container nao ha tela: sempre --janela 0 (so mede o tempo e grava /saida/saida.ppm).
# Argumentos depois do nome da imagem (--processos, --largura, --amostras, ...) sao somados a estes.
ENTRYPOINT ["./raytracer", "--janela", "0", "--saida", "/saida/saida.ppm"]