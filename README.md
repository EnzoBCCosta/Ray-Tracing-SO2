# Ray Tracer Paralelo

Projeto da disciplina de Programação Paralela (P1): renderizador de Ray Tracing 3D em C++, implementado em três versões (sequencial, múltiplos processos com memória compartilhada e múltiplas threads) e comparadas experimentalmente.

## Grupo

| Integrante | Responsabilidade principal |

| Enzo Costa | Programa Sequencial |

| Rafaela Santana | Multiplas Threads |

| João Vitor Ferraz | Multiplos processos com memoria compartilhada |

## Descrição do problema

Renderização de uma cena 3D por ray tracing, com:

- iluminação de Phong (ambiente, difusa e especular);
- sombras;
- reflexões recursivas;
- múltiplas amostras por pixel (antialiasing).

Cada pixel da imagem é calculado de forma independente, o que permite dividir o trabalho entre vários processos ou threads.

## Informações da proposta

| Item | Descrição |
|---|---|
| Linguagem | C++ |
| Origem | Desenvolvimento próprio |
| Entrada | Resolução da imagem (largura e altura) e número de amostras por pixel |
| Saída | Imagem `.ppm` e tempo de execução impresso no terminal |
| Parte paralelizada | Divisão da imagem em blocos de linhas entre processos e threads, com escrita simultânea em um framebuffer compartilhado |
| Requisito de carga | Versão sequencial com pelo menos 2 segundos de execução na entrada de teste |

## Estratégia de paralelização (planejada)

- A imagem é dividida em blocos de linhas.
- Cada worker (processo ou thread) reserva o próximo bloco livre por meio de um contador compartilhado, protegido por operação atômica.
- Versão com threads: framebuffer em memória comum do processo.
- Versão com processos: framebuffer e contador em memória compartilhada (`mmap` com `MAP_SHARED`), criada antes do `fork()`.
- O número de processos/threads é informado na execução, não fixo no código.
- As três versões devem gerar imagens idênticas para a mesma entrada.

## Estrutura prevista do repositório

```
projeto/
├── sequencial/     código-fonte
├── processos/      código-fonte
├── threads/        código-fonte
└── container/      Dockerfile
```


dadadada
# syntax=docker/dockerfile:1
#
# Ray Tracer - UMA imagem com as 3 versoes (sequencial, processos, threads).
#
# O contexto de build e a RAIZ do projeto (pasta que contem sequencial/, processos/,
# threads/ e container/). Rode a partir dela:
#
#   docker build -f container/Dockerfile -t raytracer .
#
# Se suas pastas tiverem outros nomes (ex.: 1-Versao-sequencial), use:
#   docker build -f container/Dockerfile -t raytracer \
#     --build-arg SEQ_DIR=1-Versao-sequencial \
#     --build-arg PROC_DIR=2-Versao-Processos \
#     --build-arg THR_DIR=3-Versao-Threads .

# ---------- Estagio 1: compilacao ----------
FROM ubuntu:24.04 AS build

ARG SEQ_DIR=sequencial
ARG PROC_DIR=processos
ARG THR_DIR=threads

# g++ e libsdl2-dev: o Main.cpp inclui a janela SDL2 na compilacao
RUN apt-get update \
    && apt-get install -y --no-install-recommends g++ libsdl2-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY ${SEQ_DIR}/  sequencial/
COPY ${PROC_DIR}/ processos/
COPY ${THR_DIR}/  threads/

# MESMAS flags dos READMEs (-O2, sem -march=native), para que os tempos de
# dentro e de fora do container sejam comparaveis.
RUN g++ -O2 -std=c++17          -o /out-sequencial sequencial/Main.cpp $(sdl2-config --cflags --libs) \
 && g++ -O2 -std=c++17          -o /out-processos  processos/Main.cpp  $(sdl2-config --cflags --libs) \
 && g++ -O2 -std=c++17 -pthread -o /out-threads    threads/Main.cpp    $(sdl2-config --cflags --libs)

# ---------- Estagio 2: execucao (sem compilador) ----------
FROM ubuntu:24.04

# Apenas a biblioteca de execucao do SDL2 (o binario e ligado a ela)
RUN apt-get update \
    && apt-get install -y --no-install-recommends libsdl2-2.0-0 \
    && rm -rf /var/lib/apt/lists/*

# Usuario comum (nao roda como root) e pasta de saida para o .ppm
RUN useradd --create-home --uid 1000 app \
    && mkdir -p /saida && chown app:app /saida

COPY --from=build /out-sequencial /app/sequencial
COPY --from=build /out-processos  /app/processos
COPY --from=build /out-threads    /app/threads
COPY container/run.sh /app/run.sh
RUN chmod 755 /app/run.sh

USER app
WORKDIR /saida

# Uso: docker run --rm --cpus=N raytracer <sequencial|processos|threads> [argumentos]
# O run.sh sempre acrescenta "--janela 0" (container nao tem tela).
ENTRYPOINT ["/app/run.sh"]
CMD ["sequencial"]
