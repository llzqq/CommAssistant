#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
APP_ROOT="${REPO_ROOT}/ScriptCommunicator"

BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build-linux-arm64}"
DELETE_BUILD_DIR="${DELETE_BUILD_DIR:-${REPO_ROOT}/build-deletefolder-linux-arm64}"
PACKAGE_ROOT="${PACKAGE_ROOT:-${BUILD_DIR}/package}"
STAGE_DIR="${STAGE_DIR:-${PACKAGE_ROOT}/CommAssistant-linux-arm64}"
ARCHIVE_PATH="${ARCHIVE_PATH:-${PACKAGE_ROOT}/CommAssistant-linux-arm64.tar.gz}"

QMAKE_BIN="${QMAKE_BIN:-qmake6}"
MAKE_BIN="${MAKE_BIN:-make}"
JOBS="${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)}"
SKIP_BUILD="${SKIP_BUILD:-0}"

SCRIPT_PRO="${APP_ROOT}/ScriptCommunicator.pro"
DELETE_PRO="${APP_ROOT}/DeleteFolder/DeleteFolder/DeleteFolder.pro"

SCRIPT_RELEASE_DIR="${BUILD_DIR}/release"
DELETE_RELEASE_DIR="${DELETE_BUILD_DIR}/release"

function resolve_binary_path() {
    local build_dir="$1"
    local release_dir="$2"
    local binary_name="$3"

    local candidates=(
        "${release_dir}/${binary_name}"
        "${build_dir}/${binary_name}"
    )

    local candidate
    for candidate in "${candidates[@]}"; do
        if [[ -f "${candidate}" ]]; then
            printf '%s\n' "${candidate}"
            return 0
        fi
    done

    local discovered
    discovered="$(find "${build_dir}" -maxdepth 3 -type f -name "${binary_name}" | head -n 1 || true)"
    if [[ -n "${discovered}" ]]; then
        printf '%s\n' "${discovered}"
        return 0
    fi

    return 1
}

function print_build_dir_snapshot() {
    local build_dir="$1"

    echo "build directory snapshot: ${build_dir}" >&2
    if [[ -d "${build_dir}" ]]; then
        find "${build_dir}" -maxdepth 3 \( -type f -o -type l \) | sort >&2 || true
    else
        echo "directory does not exist" >&2
    fi
}

function require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "required command not found: $1" >&2
        exit 1
    fi
}

function build_qmake_project() {
    local project_file="$1"
    local output_dir="$2"

    mkdir -p "${output_dir}"
    pushd "${output_dir}" >/dev/null
    "${QMAKE_BIN}" "${project_file}" "CONFIG+=release"
    "${MAKE_BIN}" -j"${JOBS}"
    popd >/dev/null
}

function copy_tree_if_exists() {
    local source_dir="$1"
    local target_dir="$2"

    if [[ -d "${source_dir}" ]]; then
        mkdir -p "$(dirname "${target_dir}")"
        rm -rf "${target_dir}"
        cp -a "${source_dir}" "${target_dir}"
    fi
}

function copy_file_if_exists() {
    local source_file="$1"
    local target_file="$2"

    if [[ -f "${source_file}" ]]; then
        mkdir -p "$(dirname "${target_file}")"
        cp -a "${source_file}" "${target_file}"
    fi
}

function copy_qt_plugin_dir() {
    local plugin_root="$1"
    local plugin_name="$2"

    if [[ -d "${plugin_root}/${plugin_name}" ]]; then
        mkdir -p "${STAGE_DIR}/plugins/${plugin_name}"
        cp -a "${plugin_root}/${plugin_name}/." "${STAGE_DIR}/plugins/${plugin_name}/"
    fi
}

function set_rpath_if_possible() {
    local file_path="$1"
    local rpath="$2"

    if command -v patchelf >/dev/null 2>&1 && [[ -f "${file_path}" ]]; then
        patchelf --set-rpath "${rpath}" "${file_path}"
    fi
}

require_command "${QMAKE_BIN}"
require_command "${MAKE_BIN}"
require_command "tar"

QT_INSTALL_LIBS="$("${QMAKE_BIN}" -query QT_INSTALL_LIBS)"
QT_INSTALL_PLUGINS="$("${QMAKE_BIN}" -query QT_INSTALL_PLUGINS)"

if [[ "${SKIP_BUILD}" != "1" ]]; then
    build_qmake_project "${SCRIPT_PRO}" "${BUILD_DIR}"
    build_qmake_project "${DELETE_PRO}" "${DELETE_BUILD_DIR}"
fi

SCRIPT_BIN="$(resolve_binary_path "${BUILD_DIR}" "${SCRIPT_RELEASE_DIR}" "CommAssistant" || true)"
DELETE_BIN="$(resolve_binary_path "${DELETE_BUILD_DIR}" "${DELETE_RELEASE_DIR}" "DeleteFolder" || true)"

if [[ ! -f "${SCRIPT_BIN}" ]]; then
    echo "release executable not found in: ${SCRIPT_RELEASE_DIR} or ${BUILD_DIR}" >&2
    print_build_dir_snapshot "${BUILD_DIR}"
    exit 1
fi

if [[ ! -f "${DELETE_BIN}" ]]; then
    echo "DeleteFolder executable not found in: ${DELETE_RELEASE_DIR} or ${DELETE_BUILD_DIR}" >&2
    print_build_dir_snapshot "${DELETE_BUILD_DIR}"
    exit 1
fi

rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}/lib" "${STAGE_DIR}/plugins"

cp -a "${SCRIPT_BIN}" "${STAGE_DIR}/CommAssistant.bin"
cp -a "${DELETE_BIN}" "${STAGE_DIR}/DeleteFolder"

copy_file_if_exists "${SCRIPT_RELEASE_DIR}/stylesheet.qss" "${STAGE_DIR}/stylesheet.qss"
copy_file_if_exists "${SCRIPT_RELEASE_DIR}/stylesheet.rcc" "${STAGE_DIR}/stylesheet.rcc"
copy_file_if_exists "${APP_ROOT}/documentation/Manual_ScriptCommunicator.pdf" "${STAGE_DIR}/Manual_ScriptCommunicator.pdf"

copy_tree_if_exists "${APP_ROOT}/config" "${STAGE_DIR}/config"
copy_tree_if_exists "${APP_ROOT}/templates" "${STAGE_DIR}/templates"
copy_tree_if_exists "${APP_ROOT}/ScriptEditor/apiFiles" "${STAGE_DIR}/apiFiles"

mkdir -p "${STAGE_DIR}/config"

find "${QT_INSTALL_LIBS}" -maxdepth 1 \( -type f -o -type l \) -name "libQt6*.so*" -exec cp -a {} "${STAGE_DIR}/lib/" \;

for plugin_dir in \
    platforms \
    canbus \
    imageformats \
    iconengines \
    platformthemes \
    xcbglintegrations \
    wayland-decoration-client \
    wayland-graphics-integration-client \
    wayland-shell-integration \
    egldeviceintegrations; do
    copy_qt_plugin_dir "${QT_INSTALL_PLUGINS}" "${plugin_dir}"
done

cat > "${STAGE_DIR}/qt.conf" <<'EOF'
[Paths]
Prefix=.
Plugins=plugins
EOF

cat > "${STAGE_DIR}/CommAssistant" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

APP_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export LD_LIBRARY_PATH="${APP_DIR}/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
export QT_PLUGIN_PATH="${APP_DIR}/plugins"
export QT_QPA_PLATFORM_PLUGIN_PATH="${APP_DIR}/plugins/platforms"

exec "${APP_DIR}/CommAssistant.bin" "$@"
EOF
chmod +x "${STAGE_DIR}/CommAssistant"

cat > "${STAGE_DIR}/config/comm_profiles.json" <<'EOF'
{
  "rs422": {
    "port": "/dev/ttyUSB0",
    "baud": 115200
  },
  "can": {
    "type": "SocketCAN",
    "channel": "can0",
    "bitrate": 500000,
    "idType": "Standard",
    "id": "123"
  },
  "ethernet": {
    "protocol": "TCP",
    "role": "Client",
    "localIp": "0.0.0.0",
    "localPort": 5000,
    "remoteIp": "127.0.0.1",
    "remotePort": 5000
  }
}
EOF

cat > "${STAGE_DIR}/LINUX_RUNTIME_NOTES.txt" <<'EOF'
Target: Linux ARM64 (Raspberry Pi 64-bit)

This package includes:
- Comm Assistant application binaries
- Qt runtime libraries used by the package
- Qt plugins required for the main UI and SocketCAN
- Application resources and default configuration

This package expects the target system to provide:
- glibc and standard C/C++ runtime libraries
- X11 or Wayland related system libraries
- fontconfig, dbus, xkbcommon and related desktop libraries
- device permissions for serial ports and CAN interfaces

Recommended entrypoint:
./CommAssistant
EOF

set_rpath_if_possible "${STAGE_DIR}/CommAssistant.bin" '$ORIGIN/lib'
set_rpath_if_possible "${STAGE_DIR}/DeleteFolder" '$ORIGIN/lib'

while IFS= read -r plugin_file; do
    set_rpath_if_possible "${plugin_file}" '$ORIGIN/../../lib'
done < <(find "${STAGE_DIR}/plugins" -type f -name "*.so*")

while IFS= read -r qt_lib; do
    set_rpath_if_possible "${qt_lib}" '$ORIGIN'
done < <(find "${STAGE_DIR}/lib" -maxdepth 1 -type f -name "libQt6*.so*")

mkdir -p "$(dirname "${ARCHIVE_PATH}")"
rm -f "${ARCHIVE_PATH}"
tar -czf "${ARCHIVE_PATH}" -C "${PACKAGE_ROOT}" "$(basename "${STAGE_DIR}")"

echo "created package: ${ARCHIVE_PATH}"
