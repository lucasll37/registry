# meson.build — MinIO C++ PoC
#
# Build workflow:
#   1. conan install . --output-folder=build --build=missing -s build_type=Release
#   2. meson setup build --native-file build/conan_meson_native.ini   (Linux/macOS)
#      OR
#      meson setup build --native-file build/conan_meson_native.ini \
#                        --cmake-prefix-path build  (Windows, adjust as needed)
#   3. meson compile -C build
#   4. ./build/minio_poc
#
# Meson integrates with Conan-generated CMake find modules via the
# cmake dependency method. The CMakeDeps generator creates
# aws-sdk-cpp-config.cmake (and friends) inside build/, which Meson
# picks up through CMAKE_PREFIX_PATH.

project(
  'minio-cpp-poc',
  'cpp',
  version:         '0.1.0',
  license:         'MIT',
  default_options: [
    'cpp_std=c++17',          # aws-sdk-cpp requires C++11; we use 17 for convenience
    'warning_level=2',
    'buildtype=release',
  ],
)

# ── Locate aws-sdk-cpp via Conan-generated CMake modules ─────────────────────
#
# Conan's CMakeDeps creates <build>/aws-sdk-cpp-config.cmake.
# We point Meson at it with CMAKE_PREFIX_PATH (set automatically when using
# --native-file build/conan_meson_native.ini, or pass -Dcmake_prefix_path=build).

cmake   = import('cmake')

aws_sdk = cmake.find_package(
  'AWSSDK',
  modules:  ['aws-cpp-sdk-core', 'aws-cpp-sdk-s3'],
  required: true,
)

aws_core_dep = aws_sdk.dependency('aws-cpp-sdk-core')
aws_s3_dep   = aws_sdk.dependency('aws-cpp-sdk-s3')

# ── Executable ────────────────────────────────────────────────────────────────

exe = executable(
  'minio_poc',
  sources: [
    'src/main.cpp',
  ],
  dependencies: [
    aws_core_dep,
    aws_s3_dep,
  ],
  install: false,
)
