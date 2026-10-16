# Container

Estrutura (o build roda dentro da pasta `sequencial/`, onde ficam o `Main.cpp` e a pasta `container/`):
```
sequencial/
  Main.cpp  Cena.cpp  RayTracing.cpp  Movimentos.cpp
  container/Dockerfile
  .dockerignore
```
Tudo que o programa precisa (Ubuntu + g++) é instalado e compilado **dentro da imagem**:
basta ter o Docker instalado.

## Instalar o Docker (Ubuntu)
```bash
sudo apt update && sudo apt install -y docker.io
sudo systemctl enable --now docker
sudo usermod -aG docker $USER   # depois faça logout/login (ou: newgrp docker)
docker run --rm hello-world     # teste
```

## Construir a imagem (o g++ compila dentro da imagem)
```bash
cd sequencial
docker build -f container/Dockerfile -t raytracer .
```

## Executar com limite de CPU
```bash
mkdir -p saida
docker run --rm --cpus="1" --user "$(id -u):$(id -g)" -v "$PWD/saida":/saida raytracer
docker run --rm --cpus="2" --user "$(id -u):$(id -g)" -v "$PWD/saida":/saida raytracer
docker run --rm --cpus="4" --user "$(id -u):$(id -g)" -v "$PWD/saida":/saida raytracer
```
Sem argumentos usa a entrada de teste (1920x1080, 25 amostras). Para outra entrada,
passe os argumentos depois do nome da imagem:
```bash
docker run --rm --cpus="2" --user "$(id -u):$(id -g)" -v "$PWD/saida":/saida raytracer --largura 1280 --altura 720 --amostras 4
```
O tempo de execução aparece no terminal e a imagem é gravada em `saida/saida.ppm`
(o `-v` é o que traz o arquivo de dentro do container para a sua pasta `saida/`).
Para só medir o tempo, pode omitir o `-v` e o `--user`.

## Observações
- No container não há janela: o ENTRYPOINT já passa --janela 0 e sempre grava o .ppm.
- A janela interativa só abre fora do Docker (veja o README da pasta sequencial).
- Sem argumentos, a entrada é a de teste: 1920x1080, 25 amostras.
- A versão sequencial usa 1 núcleo: --cpus maior que 1 não a deixa mais rápida
  (só fará diferença nas versões com processos/threads).
- As medições "fora do container" devem ser feitas com o binário compilado direto na VM
  (com --janela 0).
