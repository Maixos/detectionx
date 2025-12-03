#!/bin/bash

set -euo pipefail

SCRIPT_NAME=$(basename "$0")
JOBS=$(nproc)
SOC=""
ARCH=""
NPU_VERSION=""
TOOLCHAIN_FILE=""

# 支持的 SoC 分类
ARMHF_SOC_LIST=("rv1126" "rv1109" "rk1808" "rk3288")
AARCH64_SOC_LIST=("rk3399" "rk3566" "rk3588" "rk3688")

# 用法说明
usage() {
  echo "Usage: $SCRIPT_NAME --soc <soc_name> [--jobs N] [clean]"
  echo ""
  echo "Options:"
  echo "  --soc     Target SoC name (e.g., rv1126, rk3588)"
  echo "  --jobs    Parallel build jobs (default: $(nproc))"
  echo "  clean     Clean build, install, lib, and release directories"
  echo ""
  echo "Example:"
  echo "  $SCRIPT_NAME --soc rv1126 --jobs 8"
  echo "  $SCRIPT_NAME clean"
  exit 1
}

# 解析命令行参数
parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --soc)
        SOC="$2"
        shift 2
        ;;
      --jobs)
        JOBS="$2"
        shift 2
        ;;
      clean)
        CLEAN="yes"
        shift
        ;;
      *)
        echo "❌ Unknown argument: $1"
        usage
        ;;
    esac
  done
}

# 判断 SOC 类型，设置 ARCH/NPU_VERSION
detect_soc_config() {
  if [[ -z "$SOC" ]]; then
    echo "No --soc specified, assuming native build on target device."
    TOOLCHAIN_FILE=""  # 不使用工具链文件

    # 默认使用 RKNPU2
    NPU_VERSION="${NPU_VERSION:-RKNPU2}"

    export NPU_VERSION
    echo "Native build, NPU_VERSION=$NPU_VERSION"
    return
  fi

  if [[ " ${ARMHF_SOC_LIST[*]} " == *" ${SOC} "* ]]; then
    ARCH="armhf"
    NPU_VERSION="RKNPU1"
  elif [[ " ${AARCH64_SOC_LIST[*]} " == *" ${SOC} "* ]]; then
    ARCH="aarch64"
    NPU_VERSION="RKNPU2"
  else
    echo "❌ Unsupported SOC: $SOC"
    usage
  fi

  TOOLCHAIN_FILE="../cmake/toolchain-${ARCH}.cmake"

  export NPU_VERSION
  echo "SOC: $SOC"
  echo "ARCH: $ARCH"
  echo "NPU_VERSION: $NPU_VERSION"
  echo "TOOLCHAIN: $TOOLCHAIN_FILE"
}

# 执行 clean
clean() {
  echo "Cleaning build outputs..."
  rm -rf build cmake-build-debug/* lib/* install/* release/* runs/* bins/*
  echo "✅ Clean complete."
}

# 构建主流程
build() {
  echo "Starting build..."
  mkdir -p build
  cd build

  cmake \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DNPU_VERSION="$NPU_VERSION" \
    ..

  echo "Building with $JOBS threads..."
  make -j"$JOBS"

  echo "Installing..."
  make install

  cd ..
  echo "✅ Build and install complete."
}

# 主入口
main() {
  parse_args "$@"

  if [[ "${CLEAN:-}" == "yes" ]]; then
    clean
    exit 0
  fi

  detect_soc_config
  build
}

main "$@"
