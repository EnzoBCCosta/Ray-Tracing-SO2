# Ray Tracer 3D — versão sequencial

Renderiza uma cena 3D (2 esferas + chão xadrez) com Phong, sombras, reflexões
e antialiasing por amostragem. Por padrão abre uma **janela interativa** (SDL2); com `--janela 0` só
imprime o **tempo de execução** no terminal e, opcionalmente, grava um `.ppm`.

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
./raytracer # abre a janela interativa
./raytracer --janela 0  # só mede o tempo (entrada de teste)
./raytracer --janela 0 --saida saida.ppm  # mede o tempo e grava a imagem .ppm
```

Parâmetros: `--largura --altura --amostras --profundidade --sombras 0|1 --reflexoes 0|1 --phong 0|1 --janela 0|1 --saida arq.ppm`

- **Janela:** abre por padrão. Use `--janela 0` para só medir o tempo (não precisa de ambiente gráfico).
  O tempo de cada renderização aparece no título da janela.
- **`--saida`** é opcional e só vale com `--janela 0`: sem ela nenhum arquivo é gravado.
- Controles da janela: W/A/S/D + Espaço/Shift (câmera), setas + RShift/RCtrl (esfera vermelha),
  botão esquerdo + arrastar (girar), 1/2/3 (liga/desliga Phong/Sombras/Reflexos), Esc (sair).
- O tempo impresso no modo `--janela 0` mede só o cálculo da imagem (não inclui gravar o arquivo).

## Docker
Para compilar e rodar dentro de um container (com limite de CPU), veja [`container/README.md`](container/README.md).

## Estrutura
- `Main.cpp` — argumentos, medição de tempo e escrita do .ppm (opcional)
- `Cena.cpp` — vetores, materiais, esferas, chão, interseção raio-objeto
- `RayTracing.cpp` — Phong, sombras, reflexões e `renderizarLinhas(inicio, fim)`
- `Movimentos.cpp` — janela interativa (SDL2)
- `container/` — Dockerfile e instruções de uso com Docker