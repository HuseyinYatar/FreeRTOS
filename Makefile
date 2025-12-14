# =========================================================================
# FreeRTOS PC Scheduler - Makefile (Windows/MinGW Uyumlu)
# =========================================================================

# NOT: Bu dosya 'mingw32-make' komutu ile çalýþtýrýlmalýdýr.

# --- 1. Proje Ayarlarý ---
TARGET = freertos_sim.exe # Windows için uzantý eklendi
OUTPUT_DIR = bin
SRC_DIR = src
EXECUTABLE = $(OUTPUT_DIR)/$(TARGET)

# --- 2. FreeRTOS Yollarý ---
FREERTOS_DIR = FreeRTOS
FREERTOS_SRC = $(FREERTOS_DIR)/source
FREERTOS_PORTABLE = $(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix

# --- 3. Kaynak Dosyalarý Listesi ---
C_SOURCES = $(wildcard $(SRC_DIR)/*.c) 
FREERTOS_FILES = $(wildcard $(FREERTOS_SRC)/*.c) \
                 $(wildcard $(FREERTOS_PORTABLE)/*.c)
ALL_SOURCES = $(C_SOURCES) $(FREERTOS_FILES)

# --- 4. Derleyici Ayarlarý ---
CC = gcc

# Include Yollarý (GCC'nin bulmasý gereken tüm baþlýk dizinleri)
INCLUDE_PATHS = -I $(SRC_DIR) \
                -I $(FREERTOS_DIR)/include \
                -I $(FREERTOS_PORTABLE) \
                -I .

# Derleyici Bayraklarý
CFLAGS = -Wall -Wextra -g -std=c99 $(INCLUDE_PATHS)

# Baðlayýcý Bayraklarý 
LDFLAGS = -lm -mconsole -lpthread

.PHONY: all compile run clean directories

all: compile run

# Dizin Oluþturma (MinGW/Windows uyumlu CMD komutu)
directories:
	@if not exist $(OUTPUT_DIR) mkdir $(OUTPUT_DIR)
	
# Derleme hedefi
compile: directories
	@echo "--- FreeRTOS Projesi Derleniyor ---"
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(ALL_SOURCES) $(LDFLAGS)
	@echo "Derleme baþarýlý! Çýktý: $(EXECUTABLE)"
# Çalýþtýrma hedefi (Windows'ta ./bin/freertos_sim.exe giris.txt þeklinde çalýþýr)
run:
	@echo "--- Simülasyon Baþlatýlýyor (giris.txt ile) ---"
	@$(EXECUTABLE) giris.txt
	@echo "--- Simülasyon Tamamlandý ---"

# Temizlik hedefi (rm -rf yerine Windows'a özel rmdir/del komutlarý kullanýlýr)
clean:
	@echo "Temizleniyor..."
	@if exist $(OUTPUT_DIR) rmdir /s /q $(OUTPUT_DIR)
	
# Yardýmcý hedef
info:
	@echo "Kaynak Dosyalar: $(ALL_SOURCES)"
	@echo "Include Yollarý: $(INCLUDE_PATHS)"
	@echo "Derleyici Bayraklarý: $(CFLAGS)"
	@echo "Baðlayýcý Bayraklarý: $(LDFLAGS)"

# =========================================================================
# KULLANIM:
# mingw32-make all     -> Derler ve çalýþtýrýr.
# mingw32-make clean   -> Çýktý dizinini temizler.
# =========================================================================