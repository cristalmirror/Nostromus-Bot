#!/bin/bash

#Script de configuracion de Bot en C++

#Detener el script frente a cualquier error
set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'
ORANGE='\033[38;5;208m'

echo -e "${YELLOW}[INFO] Iniciando configuracion del Bot en C++... ${NC}"

echo -e "${RED}[INFO] Paso 1 >> Selecionando el gestor de packets... ${NC}"

#1 decta e instala nala si es que no esta instalado
if command -v nala &> /dev/null; then

    PKG_MAN="nala"
    echo -e "${GREEN} [OK]: Selecionando Nala para la gestion de paquetes"
    
elif command -v apt-get &> /dev/null; then

    PKG_MAN="apt-get"
    echo -e "${GREEN} [OK]: Selecionando apt-get para la gestion de paquetes"

elif command -v apt &> /dev/null; then

    PKG_MAN="apt"
    echo -e "${GREEN} [OK]: Selecionando apt para la gestion de paquetes"

else

    echo -e "${RED} [ERROR] No se encontro ningun gestor de paquetes util para la tarea"
    echo -e "s{YELLOW} [INFO] este script esta pensado para GNU/Linux Debian o Ubuntu base"

fi
#dependencias del bot
DEPENDENCIAS="build-essential cmake git g++ zlib1g-dev libssl-dev libcurl4-openssl-dev libboost-all-dev nlohmann-json3-dev"

echo -e "${RED}[INFO] Paso 2 >> Actualizando los repositorios... ${NC}"
sudo $PKG_MAN update

echo -e "${RED}[INFO] Paso 3 >> Iniciadon la instalaciond e dependencias... ${NC}"
sudo $PKG_MAN install -y $DEPENDENCIAS

# CORRECCIÓN 3: Usar una carpeta temporal evita errores si la carpeta ya existe
# y mantiene limpia tu carpeta de proyecto.
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

echo -e "${RED}[INFO] Paso 4 >> Clonando repositorio tgbot-cpp oficial...${NC}"
git clone https://github.com/reo7sp/tgbot-cpp.git
cd tgbot-cpp

echo -e "${RED}[INFO] Paso 5 >> Compilando tgbot-cpp...${NC}"
cmake .
echo -e "${RED}[INFO] Paso 6 >> Armando tgbot-cpp...${NC}"
make -j$(nproc) # Usa todos los núcleos del CPU para compilar rápido

echo -e "${RED}[INFO] Paso 7 >> Instalando librería en el sistema...${NC}"
sudo make install

# 5. Paso Crítico: Actualizar el cache de librerías (ldconfig)
# Esto soluciona el error "cannot open shared object file" que tuviste ayer
echo -e "${YELLOW}[INFO] Actualizando caché de librerías compartidas (ldconfig)...${NC}"
sudo ldconfig

# Limpieza de la carpeta temporal
cd ~
rm -rf "$TEMP_DIR"

echo -e "${ORANGE}============================================================"
echo -e "${GREEN}[OK]¡INSTALACIÓN COMPLETADA EXITOSAMENTE!${NC}"
echo -e "${YELLOW}[INFO] Ahora puedes ir a la carpeta de tu bot y ejecutar:${NC}"
echo -e "${ORANGE}[OK]  cmake${NC}"
echo -e "${ORANGE}[OK]  make${NC}"
echo -e "${ORANGE}============================================================"
