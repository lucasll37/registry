# MinIO C++ PoC

Prova de conceito que replica o comportamento de `python/bucket_demo.py`
usando o **AWS SDK for C++** (componente S3) contra o MinIO / AIStor.

## Pré-requisitos

| Ferramenta | Versão mínima | Instalação |
|------------|--------------|------------|
| C++ compiler | GCC 11 / Clang 14 / MSVC 2022 | distro / brew / winget |
| CMake | 3.22 | necessário internamente pelo Conan |
| Conan | 2.x | `pip install conan` |
| Meson | 1.3 | `pip install meson` |
| Ninja | 1.11 | `pip install ninja` |

> O MinIO / AIStor deve estar em execução (`make minio-up` na raiz do projeto).

## Estrutura

```
cpp/
├── conanfile.py          # Dependência: aws-sdk-cpp (apenas componente S3)
├── meson.build           # Build system — integra Conan via cmake module
├── meson_options.txt     # Opções configuráveis (endpoint, secure)
├── src/
│   └── main.cpp          # Demo: shared, readonly, pessoal + assert de acesso negado
└── .gitignore
```

## Build

```bash
cd cpp

# 1. Instalar dependências via Conan (compila aws-sdk-cpp na 1ª vez — ~10 min)
conan install . --output-folder=build --build=missing -s build_type=Release

# 2. Configurar Meson apontando para os módulos CMake gerados pelo Conan
meson setup build \
  --native-file build/conan_meson_native.ini \
  --cmake-prefix-path build

# 3. Compilar
meson compile -C build
```

## Executar

As credenciais são lidas das variáveis de ambiente — use o mesmo `.env` do projeto:

```bash
# Carrega o .env da raiz
export $(grep -v '^#' ../.env | xargs)

./build/minio_poc
```

Saída esperada:

```
=== MinIO C++ PoC ===
  Endpoint : http://localhost:9000
  Username : lucas

── Bucket: shared (leitura e escrita) ──
  ✓ upload  → s3://shared/experimento-01/config.json
  ✓ download ← s3://shared/experimento-01/config.json
  conteúdo: { "modelo": "mlp", ... }
  objetos: experimento-01/config.json
  ✓ delete   → s3://shared/experimento-01/config.json

── Bucket: readonly (somente leitura) ──
  (vazio — aguardando conteúdo do admin)
  ✓ escrita bloqueada como esperado: ...

── Bucket: users/lucas/ (pasta pessoal) ──
  ✓ upload  → s3://users/lucas/resultado-42.json
  ✓ download ← s3://users/lucas/resultado-42.json
  conteúdo: { "acuracia": 0.97, ... }
  objetos: lucas/resultado-42.json
  ✓ delete   → s3://users/lucas/resultado-42.json
  ✓ acesso a pasta alheia bloqueado: ...

✓ PoC concluído com sucesso.
```

## Opções de build

```bash
# Endpoint diferente
meson setup build --native-file build/conan_meson_native.ini \
  -Dendpoint=192.168.0.10:9000

# HTTPS (requer certificado válido ou desabilite verifySSL no código)
meson setup build --native-file build/conan_meson_native.ini \
  -Dsecure=true
```

## Perfil Conan (opcional — recomendado para builds repetidos)

```bash
# Criar perfil padrão
conan profile detect

# Build com perfil explícito
conan install . --output-folder=build --build=missing \
  --profile=default -s build_type=Release
```

## Como funciona a integração Conan ↔ Meson

```
conanfile.py
  └─ generators: CMakeDeps, CMakeToolchain
        └─ gera: build/aws-sdk-cpp-config.cmake
                 build/conan_meson_native.ini   ← --native-file
meson.build
  └─ cmake.find_package('AWSSDK', ...)
        └─ lê build/aws-sdk-cpp-config.cmake
              └─ resolve includes + libs → linka o executável
```