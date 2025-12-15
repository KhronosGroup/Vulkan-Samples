#!/bin/bash

# Install script for Simple Game Engine dependencies on Linux
# This script installs all required dependencies for building the Simple Game Engine

set -e  # Exit on any error

echo "Installing Simple Game Engine dependencies for Linux..."

# Detect the Linux distribution
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
elif type lsb_release >/dev/null 2>&1; then
    OS=$(lsb_release -si)
    VER=$(lsb_release -sr)
elif [ -f /etc/lsb-release ]; then
    . /etc/lsb-release
    OS=$DISTRIB_ID
    VER=$DISTRIB_RELEASE
elif [ -f /etc/debian_version ]; then
    OS=Debian
    VER=$(cat /etc/debian_version)
else
    OS=$(uname -s)
    VER=$(uname -r)
fi

echo "Detected OS: $OS $VER"

# Function to install dependencies on Ubuntu/Debian
install_ubuntu_debian() {
    echo "Installing dependencies for Ubuntu/Debian..."

    # Update package list
    sudo apt update

    # Install build essentials
    sudo apt install -y build-essential cmake git

    # Install Vulkan SDK
    echo "Installing Vulkan SDK..."
    wget -qO - https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo apt-key add -
    sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-focal.list https://packages.lunarg.com/vulkan/lunarg-vulkan-focal.list
    sudo apt update
    sudo apt install -y vulkan-sdk

    # Install other dependencies
    sudo apt install -y \
        libglfw3-dev \
        libglm-dev \
        libopenal-dev \
        libktx-dev

    # Install Slang compiler (for shader compilation)
    echo "Installing Slang compiler..."
    if [ ! -f /usr/local/bin/slangc ]; then
        SLANG_VERSION="2024.1.21"
        wget "https://github.com/shader-slang/slang/releases/download/v${SLANG_VERSION}/slang-${SLANG_VERSION}-linux-x86_64.tar.gz"
        tar -xzf "slang-${SLANG_VERSION}-linux-x86_64.tar.gz"
        sudo cp slang/bin/slangc /usr/local/bin/
        sudo chmod +x /usr/local/bin/slangc
        rm -rf slang "slang-${SLANG_VERSION}-linux-x86_64.tar.gz"
    fi
}

# Function to install dependencies on Fedora/RHEL/CentOS
install_fedora_rhel() {
    echo "Installing dependencies for Fedora/RHEL/CentOS..."

    # Install build essentials
    sudo dnf install -y gcc gcc-c++ cmake git

    # Install Vulkan SDK
    echo "Installing Vulkan SDK..."
    sudo dnf install -y vulkan-devel vulkan-tools

    # Install other dependencies
    sudo dnf install -y \
        glfw-devel \
        glm-devel \
        openal-soft-devel

    # Note: Some packages might need to be built from source on RHEL/CentOS
    echo "Note: Some dependencies (libktx, tinygltf) may need to be built from source"
    echo "Please refer to the project documentation for manual installation instructions"
}

# Function to install dependencies on Arch Linux
install_arch() {
    echo "Installing dependencies for Arch Linux..."

    # Update package database
    sudo pacman -Sy

    # Install build essentials
    sudo pacman -S --noconfirm base-devel cmake git

    # Install dependencies
    sudo pacman -S --noconfirm \
        vulkan-devel \
        glfw-wayland \
        glm \
        openal

    # Install AUR packages (requires yay or another AUR helper)
    if command -v yay &> /dev/null; then
        yay -S --noconfirm libktx
    else
        echo "Note: Please install yay or another AUR helper to install libktx packages"
        echo "Alternatively, build these dependencies from source"
    fi
}

# Install dependencies based on detected OS
case "$OS" in
    "Ubuntu"* | "Debian"* | "Linux Mint"*)
        install_ubuntu_debian
        ;;
    "Fedora"* | "Red Hat"* | "CentOS"* | "Rocky"*)
        install_fedora_rhel
        ;;
    "Arch"* | "Manjaro"*)
        install_arch
        ;;
    *)
        echo "Unsupported Linux distribution: $OS"
        echo "Please install the following dependencies manually:"
        echo "- CMake (3.29 or later)"
        echo "- Vulkan SDK"
        echo "- GLFW3 development libraries"
        echo "- GLM (OpenGL Mathematics) library"
        echo "- OpenAL development libraries"
        echo "- KTX library"
        echo "- STB library"
        echo "- tinygltf library"
        echo "- Slang compiler"
        exit 1
        ;;
esac

echo ""
echo "Dependencies installation completed!"
echo ""
echo "To build the Simple Game Engine:"
echo "1. cd to the simple_engine directory"
echo "2. mkdir build && cd build"
echo "3. cmake .."
echo "4. make -j$(nproc)"
echo ""
echo "Or use the provided CMake build command:"
echo "cmake --build cmake-build-debug --target SimpleEngine -j 10"
