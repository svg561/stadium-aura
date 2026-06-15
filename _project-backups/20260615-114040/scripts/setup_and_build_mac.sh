#!/usr/bin/env bash

set -Eeuo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
JUCE_DIR="${PROJECT_ROOT}/JUCE"
BUILD_DIR="${PROJECT_ROOT}/build"
TOOLS_DIR="${PROJECT_ROOT}/.tools"
LAST_COMMAND=""

LOCAL_CMAKE_BIN="${TOOLS_DIR}/cmake-3.31.8-macos-universal/CMake.app/Contents/bin"
if [[ -x "${LOCAL_CMAKE_BIN}/cmake" ]]; then
    export PATH="${LOCAL_CMAKE_BIN}:${PATH}"
fi

bold='\033[1m'
green='\033[0;32m'
yellow='\033[0;33m'
red='\033[0;31m'
reset='\033[0m'

info() {
    printf "\n${bold}%s${reset}\n" "$1"
}

success() {
    printf "${green}Success:${reset} %s\n" "$1"
}

warning() {
    printf "${yellow}Note:${reset} %s\n" "$1"
}

fail() {
    printf "\n${red}${bold}Setup stopped.${reset}\n" >&2
    printf "The command that failed was:\n  %s\n\n" "${LAST_COMMAND:-Unknown command}" >&2
    printf "Please paste this command and all error text above it back into Codex.\n" >&2
    exit 1
}

trap fail ERR

run() {
    LAST_COMMAND="$(printf '%q ' "$@")"
    printf "Running: %s\n" "${LAST_COMMAND}"
    "$@"
}

confirm() {
    local prompt="$1"
    local answer

    if [[ ! -t 0 ]]; then
        printf "This step needs confirmation, but the script is not running interactively.\n" >&2
        printf "Run the script directly in Terminal and try again.\n" >&2
        exit 1
    fi

    read -r -p "${prompt} [y/N] " answer
    [[ "${answer}" =~ ^[Yy]([Ee][Ss])?$ ]]
}

info "Stadium Aura macOS setup and build"
printf "Project: %s\n" "${PROJECT_ROOT}"
printf "This script only installs Xcode Command Line Tools, Homebrew, CMake, Git, and JUCE when needed.\n"

info "1. Checking Xcode Command Line Tools"
if xcode-select -p >/dev/null 2>&1; then
    success "Xcode Command Line Tools are installed."
else
    warning "Xcode Command Line Tools are required by Homebrew and Xcode builds."
    if confirm "Open Apple's installer now?"; then
        LAST_COMMAND="xcode-select --install"
        xcode-select --install || true
        printf "\nFinish the Apple installer, then run this script again.\n"
        exit 0
    fi

    printf "Installation was not started. Run this script again when you are ready.\n"
    exit 1
fi

info "2. Checking Homebrew"
if ! command -v brew >/dev/null 2>&1; then
    # Apple Silicon Homebrew may exist before the current shell has loaded its environment.
    if [[ -x /opt/homebrew/bin/brew ]]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    elif [[ -x /usr/local/bin/brew ]]; then
        eval "$(/usr/local/bin/brew shellenv)"
    fi
fi

if command -v brew >/dev/null 2>&1; then
    success "Homebrew is installed."
else
    warning "Homebrew is needed to install only the missing build tools."
    if ! confirm "Install Homebrew now?"; then
        printf "Homebrew installation was declined. No changes were made.\n"
        exit 1
    fi

    LAST_COMMAND='/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"'
    printf "Running the official Homebrew installer...\n"
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

    if [[ -x /opt/homebrew/bin/brew ]]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    elif [[ -x /usr/local/bin/brew ]]; then
        eval "$(/usr/local/bin/brew shellenv)"
    fi

    command -v brew >/dev/null 2>&1
    success "Homebrew was installed."
fi

info "3. Checking CMake"
if command -v cmake >/dev/null 2>&1; then
    success "CMake is installed: $(cmake --version | head -n 1)"
else
    if confirm "CMake is missing. Install it with Homebrew?"; then
        if run brew install cmake; then
            success "CMake was installed."
        else
            warning "Homebrew could not install CMake on this version of macOS."
            if ! confirm "Download the official CMake tools into this project instead?"; then
                printf "CMake installation was not completed. The build cannot continue.\n"
                exit 1
            fi

            CMAKE_VERSION="3.31.8"
            CMAKE_ARCHIVE="${TOOLS_DIR}/cmake-${CMAKE_VERSION}-macos-universal.tar.gz"
            CMAKE_FOLDER="${TOOLS_DIR}/cmake-${CMAKE_VERSION}-macos-universal"
            run mkdir -p "${TOOLS_DIR}"
            run curl -fL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-macos-universal.tar.gz" -o "${CMAKE_ARCHIVE}"
            run tar -xzf "${CMAKE_ARCHIVE}" -C "${TOOLS_DIR}"
            export PATH="${CMAKE_FOLDER}/CMake.app/Contents/bin:${PATH}"
            command -v cmake >/dev/null 2>&1
            success "Official CMake tools are ready inside the project."
        fi
    else
        printf "CMake installation was declined. The build cannot continue.\n"
        exit 1
    fi
fi

info "4. Checking Git"
if command -v git >/dev/null 2>&1; then
    success "Git is installed: $(git --version)"
else
    if confirm "Git is missing. Install it with Homebrew?"; then
        run brew install git
        success "Git was installed."
    else
        printf "Git installation was declined. JUCE cannot be downloaded.\n"
        exit 1
    fi
fi

info "5. Checking JUCE"
if [[ -f "${JUCE_DIR}/CMakeLists.txt" ]]; then
    success "JUCE already exists at ${JUCE_DIR}."
elif [[ -e "${JUCE_DIR}" ]]; then
    printf "${red}The JUCE path exists but is not a valid JUCE checkout:${reset}\n  %s\n" "${JUCE_DIR}" >&2
    printf "Nothing was overwritten. Move or repair that folder, then run this script again.\n" >&2
    exit 1
else
    warning "JUCE will be downloaded from the official juce-framework repository."
    if confirm "Clone JUCE into ${JUCE_DIR}?"; then
        run git clone --depth 1 https://github.com/juce-framework/JUCE.git "${JUCE_DIR}"
        success "JUCE was downloaded."
    else
        printf "JUCE download was declined. The build cannot continue.\n"
        exit 1
    fi
fi

info "6. Configuring the project"
cd "${PROJECT_ROOT}"
if xcodebuild -version >/dev/null 2>&1; then
    run cmake -S . -B build -G Xcode
else
    warning "The full Xcode app is not installed, so the Xcode generator is unavailable."
    if ! confirm "Use Apple's Command Line Tools with Unix Makefiles instead?"; then
        printf "Install Xcode from the App Store, open it once, then run this script again.\n"
        exit 1
    fi
    run cmake -S . -B build -G "Unix Makefiles"
fi
success "Build files were configured in ${BUILD_DIR}."

info "7. Building Stadium Aura in Release mode"
run cmake --build build --config Release
success "The Release build completed."

info "8. Running tests"
run ctest --test-dir build -C Release --output-on-failure
success "All Stadium Aura tests passed."

printf "\n${green}${bold}Stadium Aura is configured, built, and tested successfully.${reset}\n"
printf "Build output: %s\n" "${BUILD_DIR}"
