.PHONY: help
.PHONY: deps build run clean rebuild
.PHONY: env-check

.DEFAULT_GOAL := help

# ============================================
# Variables
# ============================================

BUILD_DIR    := build
SRC_DIR      := src
BINARY       := $(BUILD_DIR)/minio_poc

CONAN        := conan
MESON        := meson
NINJA        := ninja

BUILD_TYPE   := Release

# Lê credenciais do .env na raiz do projeto (um nível acima)
ENV_FILE     := ../.env
-include $(ENV_FILE)
export

# Colors for output
RED    := \033[0;31m
GREEN  := \033[0;32m
YELLOW := \033[1;33m
BLUE   := \033[0;34m
NC     := \033[0m


# ============================================
# Help
# ============================================

help: ## Show this help message
	@echo ""
	@echo "$(BLUE)MinIO C++ PoC$(NC)"
	@echo "$(BLUE)==============$(NC)"
	@echo ""
	@echo "$(YELLOW)Build:$(NC)"
	@grep -E '^(deps|build|rebuild|run|clean|env-check):.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  $(GREEN)%-18s$(NC) %s\n", $$1, $$2}'
	@echo ""
	@echo "$(YELLOW)Workflow:$(NC)"
	@echo "  make deps    # 1. Instala dependências via Conan"
	@echo "  make build   # 2. Configura Meson + compila"
	@echo "  make run     # 3. Executa o binário"
	@echo ""


# ============================================
# Targets
# ============================================

env-check: ## Verify required environment variables are set
	@echo "$(BLUE)Verificando variáveis de ambiente...$(NC)"
	@[ -f $(ENV_FILE) ] || { \
		echo "$(YELLOW)⚠  $(ENV_FILE) não encontrado — usando variáveis do ambiente do shell.$(NC)"; }
	@[ -n "$(MINIO_SVC_ACCESS_KEY)" ] || { \
		echo "$(RED)✗ MINIO_SVC_ACCESS_KEY não definida$(NC)"; exit 1; }
	@[ -n "$(MINIO_SVC_SECRET_KEY)" ] || { \
		echo "$(RED)✗ MINIO_SVC_SECRET_KEY não definida$(NC)"; exit 1; }
	@[ -n "$(MINIO_SVC_USERNAME)" ] || { \
		echo "$(RED)✗ MINIO_SVC_USERNAME não definida$(NC)"; exit 1; }
	@echo "$(GREEN)✓ Variáveis OK$(NC)"
	@echo "  Endpoint : $(MINIO_ENDPOINT)"
	@echo "  Username : $(MINIO_SVC_USERNAME)"


deps: ## Install C++ dependencies via Conan (builds aws-sdk-cpp S3 on first run ~10 min)
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Instalando dependências via Conan...$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@command -v $(CONAN) >/dev/null 2>&1 || { \
		echo "$(RED)✗ Conan não encontrado. Instale com: pip install conan$(NC)"; exit 1; }
	@command -v $(MESON) >/dev/null 2>&1 || { \
		echo "$(RED)✗ Meson não encontrado. Instale com: pip install meson$(NC)"; exit 1; }
	@command -v $(NINJA) >/dev/null 2>&1 || { \
		echo "$(RED)✗ Ninja não encontrado. Instale com: pip install ninja$(NC)"; exit 1; }
	@$(CONAN) profile detect --force >/dev/null 2>&1 || true
	$(CONAN) install . \
		--output-folder=$(BUILD_DIR) \
		--build=missing \
		-s build_type=$(BUILD_TYPE)
	@echo "$(GREEN)✓ Dependências instaladas$(NC)"


build: ## Configure Meson and compile the binary
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Compilando...$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@[ -f $(BUILD_DIR)/conan_meson_native.ini ] || { \
		echo "$(YELLOW)⚠  Conan native file não encontrado. Rodando 'make deps' primeiro...$(NC)"; \
		$(MAKE) deps; }
	@if [ ! -f $(BUILD_DIR)/build.ninja ]; then \
		echo "$(BLUE)Configurando Meson...$(NC)"; \
		$(MESON) setup $(BUILD_DIR) \
			--native-file $(BUILD_DIR)/conan_meson_native.ini \
			--cmake-prefix-path $(BUILD_DIR) \
			--buildtype=$(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]') \
			--wipe 2>/dev/null || \
		$(MESON) setup $(BUILD_DIR) \
			--native-file $(BUILD_DIR)/conan_meson_native.ini \
			--cmake-prefix-path $(BUILD_DIR) \
			--buildtype=$(shell echo $(BUILD_TYPE) | tr '[:upper:]' '[:lower:]'); \
	fi
	$(MESON) compile -C $(BUILD_DIR)
	@echo ""
	@echo "$(GREEN)✓ Build concluído: $(BINARY)$(NC)"


run: env-check ## Run the PoC binary (reads credentials from ../.env)
	@echo "$(BLUE)========================================$(NC)"
	@echo "$(BLUE)Executando MinIO C++ PoC...$(NC)"
	@echo "$(BLUE)========================================$(NC)"
	@[ -f $(BINARY) ] || { \
		echo "$(YELLOW)⚠  Binário não encontrado. Rodando 'make build' primeiro...$(NC)"; \
		$(MAKE) build; }
	@MINIO_ENDPOINT=$(MINIO_ENDPOINT) \
	 MINIO_SVC_ACCESS_KEY=$(MINIO_SVC_ACCESS_KEY) \
	 MINIO_SVC_SECRET_KEY=$(MINIO_SVC_SECRET_KEY) \
	 MINIO_SVC_USERNAME=$(MINIO_SVC_USERNAME) \
	 MINIO_SECURE=$(MINIO_SECURE) \
	 $(BINARY)


rebuild: ## Wipe build dir, reinstall deps, and recompile from scratch
	@echo "$(RED)========================================$(NC)"
	@echo "$(RED)Rebuild completo (wipe + deps + build)$(NC)"
	@echo "$(RED)========================================$(NC)"
	@$(MAKE) clean
	@$(MAKE) deps
	@$(MAKE) build


clean: ## Remove build directory and compiled artifacts
	@echo "$(YELLOW)Removendo $(BUILD_DIR)/...$(NC)"
	@rm -rf $(BUILD_DIR)
	@echo "$(GREEN)✓ Build directory removido$(NC)"