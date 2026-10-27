# Ray Tracer 3D — versão com threads

Renderiza uma cena 3D (2 esferas + chão xadrez) com Phong, sombras, reflexões
e antialiasing por amostragem, **paralelizado com `std::thread`**. Por padrão abre uma **janela interativa** (SDL2); com `--janela 0` só
imprime o **tempo de execução** no terminal e, opcionalmente, grava um `.ppm`.

## 1. Instalar dependências (Linux)
```bash
sudo apt update && sudo apt install -y g++ libsdl2-dev
```

## 2. Compilar
Compile SOMENTE o `Main.cpp` (os demais `.cpp` são incluídos por ele). O `-pthread` é obrigatório:
```bash
g++ -O2 -std=c++17 -pthread -o raytracer Main.cpp $(sdl2-config --cflags --libs)
```

## 3. Executar
```bash
./raytracer # abre a janela interativa
./raytracer --janela 0  # só mede o tempo (entrada de teste)
./raytracer --janela 0 --saida saida.ppm  # mede o tempo e grava a imagem .ppm
./raytracer --janela 0 --threads 4  # fixa o número de threads
```

Parâmetros: `--largura --altura --amostras --profundidade --sombras 0|1 --reflexoes 0|1 --phong 0|1 --janela 0|1 --saida arq.ppm --threads N`

- **`--threads N`:** número de threads. Se omitido (ou 0), usa todos os núcleos (`std::thread::hardware_concurrency()`).
- **Janela:** abre por padrão. Use `--janela 0` para só medir o tempo (não precisa de ambiente gráfico).
  O tempo de cada renderização aparece no título da janela.
- **`--saida`** é opcional e só vale com `--janela 0`: sem ela nenhum arquivo é gravado.
- Controles da janela: W/A/S/D + Espaço/Shift (câmera), setas + RShift/RCtrl (esfera vermelha),
  botão esquerdo + arrastar (girar), 1/2/3 (liga/desliga Phong/Sombras/Reflexos), Esc (sair).
- O tempo impresso no modo `--janela 0` mede só o cálculo da imagem (não inclui gravar o arquivo).

## Como funciona o paralelismo
- `renderizarParalelo` (em `RayTracing.cpp`) cria N threads. Cada uma pega **blocos de 4 linhas** de uma fila
  baseada em um contador atômico (`std::atomic<int>`) e chama `renderizarLinhas(inicio, fim)`, a mesma função da versão sequencial.
- Não há mutex: a cena é somente leitura durante o render e cada thread escreve só nas suas linhas do framebuffer.
- O gerador aleatório (`RngPixel`) é por pixel, sem estado global. Por isso o resultado com N threads é **idêntico**
  ao sequencial (verificado com `cmp`).
- O balanceamento é dinâmico: linhas mais caras (reflexos/sombras) não deixam threads ociosas.

## Resultados
Configuração: 1920x1080, 25 amostras, profundidade 3, sombras, reflexões e Phong ativos.
Ambiente: VM (VirtualBox) com Ubuntu, 2 núcleos. Três execuções por configuração.

| Threads | Tempos (s) | Média (s) | Speedup |
|---|---|---|---|
| 1 | 21,780 / 22,247 / 21,949 | 21,99 | 1,00 |
| 2 | 11,942 / 11,752 / 12,168 | 11,95 | 1,84 |
| 3 | 11,376 / 11,431 / 11,512 | 11,44 | 1,92 |
| 4 | 11,650 / 12,200 / 12,229 | 12,03 | 1,83 |
| 8 | 11,810 / 12,373 / 11,602 | 11,93 | 1,84 |

Speedup = tempo médio com 1 thread ÷ tempo médio com N threads.

- Com 2 threads o tempo caiu de ~22 s para ~12 s (speedup 1,84), perto do ideal de 2,0.
- Acima de 2 threads o tempo praticamente não muda, pois a máquina tem só 2 núcleos. As pequenas diferenças
  entre 2, 3, 4 e 8 threads estão dentro da variação normal da VM.
- A perda em relação ao ideal vem do custo de criar/sincronizar threads e do acesso concorrente à memória.

## Docker
Para compilar e rodar dentro de um container, veja [`container/README.md`](container/README.md).

## Estrutura
- `Main.cpp` — argumentos, medição de tempo e escrita do .ppm (opcional)
- `Cena.cpp` — vetores, materiais, esferas, chão, interseção raio-objeto
- `RayTracing.cpp` — Phong, sombras, reflexões, `renderizarLinhas(inicio, fim)` e `renderizarParalelo`
- `Movimentos.cpp` — janela interativa (SDL2)
- `container/` — Dockerfile e instruções de uso com Docker