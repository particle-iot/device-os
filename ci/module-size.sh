#!/bin/bash
set -euxo pipefail

log() {
    echo "[module-size] $*"
}

fail() {
    echo "[module-size] error: $*" >&2
    exit 1
}

platform="$1"
release_dir="$2"
module_type="$3"
device_os_dir="${4:-$PWD}"

log "platform=${platform} module_type=${module_type} release_dir=${release_dir} device_os_dir=${device_os_dir}"

bin_path="$(find "$release_dir" -name "*-${module_type}@*.bin" | head -n 1)"
if [ -z "$bin_path" ]; then
    fail "no module binary matching *-${module_type}@*.bin found under ${release_dir}"
fi

used_bytes="$(wc -c < "$bin_path" | tr -d '[:space:]')"
log "bin_path=${bin_path}"
log "used_bytes=${used_bytes}"

platform_mcu="$(
    awk -v platform="$platform" '
        index($0, "ifeq (\"$(PLATFORM)\",\"" platform "\")") == 1 {
            want_platform_id = 1
            next
        }
        want_platform_id && $1 ~ /^PLATFORM_ID=/ {
            split($1, parts, "=")
            platform_id = parts[2]
            want_platform_id = 0
            next
        }
        platform_id != "" && index($0, "ifeq (\"$(PLATFORM_ID)\",\"" platform_id "\")") == 1 {
            want_platform_mcu = 1
            next
        }
        want_platform_mcu && $1 ~ /^PLATFORM_MCU=/ {
            split($1, parts, "=")
            print parts[2]
            exit
        }
    ' "$device_os_dir/build/platform-id.mk"
)"

if [ -z "$platform_mcu" ]; then
    fail "failed to resolve PLATFORM_MCU for ${platform}"
fi
log "platform_mcu=${platform_mcu}"

platform_mcu_dir="$(printf '%s' "$platform_mcu" | tr '[:upper:]' '[:lower:]')"
flash_ld="$device_os_dir/build/arm/linker/$platform_mcu_dir/platform_flash.ld"
if [ ! -f "$flash_ld" ]; then
    fail "platform flash layout not found: ${flash_ld}"
fi
log "flash_ld=${flash_ld}"

available_bytes="$(
    awk '
        $1 == "platform_system_part1_flash_size" {
            size = $3
            sub(/;/, "", size)
            if (size ~ /K$/) {
                sub(/K$/, "", size)
                print size * 1024
            } else if (size ~ /M$/) {
                sub(/M$/, "", size)
                print size * 1024 * 1024
            }
        }
    ' "$flash_ld"
)"

if [ -z "$available_bytes" ]; then
    fail "failed to parse platform_system_part1_flash_size from ${flash_ld}"
fi
log "available_bytes=${available_bytes}"

mkdir -p "$release_dir"
printf 'used=%s\navailable=%s\n' "$used_bytes" "$available_bytes" > "$release_dir/${module_type}.size"
log "wrote ${release_dir}/${module_type}.size"
