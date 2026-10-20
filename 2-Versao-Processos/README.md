# Ray Tracer 3D — versão com processos

Renderiza uma cena 3D (2 esferas + chão xadrez) com Phong, sombras, reflexões
e antialiasing por amostragem.

Nesta versão a imagem é dividida em blocos de 8 linhas e distribuída entre **N processos**
(`--processos N`), criados com `fork()`. Os processos filhos escrevem no mesmo framebuffer, que fica
em memória compartilhada (`mmap`), e pegam o próximo bloco por um contador atômico.

Por padrão o programa abre uma **janela interativa** (SDL2). Com `--janela 0` ele só imprime o
**tempo de execução** no terminal e, opcionalmente, grava um `.ppm`.

## 1. Instalar dependências (Linux)
```bash
sudo apt update && sudo apt install -y g++ libsdl2-dev
```

## 2. Compilar
Compile SOMENTE o `Main.cpp` (os demais `.cpp` são incluídos por ele):
```bash
g++ -O2 -std=c++17 -o raytracer Main.cpp $(sdl2-config --cflags --libs)
```

## 3. Executar
```bash
./raytracer                                          # abre a janela interativa
./raytracer --janela 0                               # mede o tempo com 1 processo
./raytracer --janela 0 --processos 4                 # mede o tempo com 4 processos
./raytracer --janela 0 --processos 4 --saida saida.ppm   # mede o tempo e grava a imagem .ppm
```

Parâmetros: `--largura --altura --amostras --profundidade --sombras 0|1 --reflexoes 0|1 --phong 0|1 --janela 0|1 --processos N --saida arq.ppm`

| Parâmetro | Padrão | Descrição |
|---|---|---|
| `--largura` / `--altura` | 1920 / 1080 | Resolução da imagem |
| `--amostras` | 25 | Amostras por pixel (antialiasing) |
| `--profundidade` | 3 | Profundidade máxima de reflexão |
| `--sombras`, `--reflexoes`, `--phong` | 1 | Liga (1) ou desliga (0) cada efeito |
| `--janela` | 1 | `0` desliga a janela e só mede o tempo |
| `--processos` | 1 | Número de processos que dividem o trabalho |
| `--saida` | (vazio) | Caminho do `.ppm` a gravar |

Observações:
- **Janela:** abre por padrão. Use `--janela 0` para só medir o tempo (não precisa de ambiente gráfico).
  O tempo de cada renderização aparece no título da janela.
- **`--processos`** só vale com `--janela 0`. A janela interativa renderiza em um único processo.
- **`--saida`** é opcional e só vale com `--janela 0`: sem ela nenhum arquivo é gravado.
- O tempo impresso no modo `--janela 0` mede só o cálculo da imagem (não inclui gravar o arquivo).
- A imagem gerada é a mesma para qualquer número de processos; só o tempo muda.
- Controles da janela: W/A/S/D + Espaço/Shift (câmera), setas + RShift/RCtrl (esfera vermelha),
  botão esquerdo + arrastar (girar), 1/2/3 (liga/desliga Phong/Sombras/Reflexos), Esc (sair).

## Docker
Para compilar e rodar dentro de um container (com limite de CPU), veja [`container/README.md`](container/README.md).

## Estrutura
- `Main.cpp` — argumentos, medição de tempo e escrita do .ppm (opcional)
- `Cena.cpp` — vetores, materiais, esferas, chão, interseção raio-objeto
- `RayTracing.cpp` — Phong, sombras, reflexões, `renderizarLinhas(inicio, fim)` e `renderizarProcessos(...)`
- `Movimentos.cpp` — janela interativa (SDL2)
- `container/` — Dockerfile e instruções de uso com Docker
- `saida/` — pasta onde o container grava o `.ppm`