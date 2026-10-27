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
CMD ["sequencial"]
