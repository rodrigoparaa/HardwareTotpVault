# PSP Go Air-Gapped Hardware TOTP & Seed Vault

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Language: C99](https://img.shields.io/badge/Language-C99-00599C.svg)](#architecture)
[![Platform: Sony PSP Go (PSP-N1000)](https://img.shields.io/badge/Hardware-PSP%20Go%20(MIPS%20R4000)-black.svg)](#hardware-specifications--embedded-runtime)
[![Crypto: ChaCha20 + PBKDF2-HMAC-SHA256](https://img.shields.io/badge/Crypto-ChaCha20%20%2B%20HMAC--SHA256-success.svg)](#cryptographic-architecture)
[![Tests: 89/89 Passing](https://img.shields.io/badge/RFC%20Unit%20Tests-89%2F89%20Passing-brightgreen.svg)](#running-unit-tests)

> 🇪🇸 **Leer en Español**: [README_ES.md](README_ES.md) · 📐 **Especificación Técnica Completa**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) · 🛠️ **Guía de Toolchain y Despliegue**: [docs/TOOLCHAIN_SETUP.md](docs/TOOLCHAIN_SETUP.md)

**PSP Go Air-Gapped Hardware TOTP & Seed Vault** transforms a **Sony PlayStation Portable Go (PSP-N1000)** with the physical WLAN switch turned off into a dedicated, **100% offline (air-gapped) cryptographic hardware token**.

It provides two core security functions in one pocket-sized physical device:
1. **Time-Based & Counter-Based One-Time Passwords**: Standard **TOTP (RFC 6238)** and **HOTP (RFC 4226)** compatible with services such as GitHub, AWS, Google, Proton, Cloudflare, and custom enterprise 2FA logins.
2. **Encrypted Critical Seed & Password Vault**: High-security storage for **BIP-39 recovery phrases (12/24 words)**, **SSH/GPG private keys and fingerprints**, recovery codes, and master credentials.

The app supports both **portable handheld use** and **closed-slider desk dock operation**, featuring a custom hardware-accelerated 7-segment vector renderer running at **60 FPS** on the PSP's eDRAM, a 30-second progress bar synchronized to the hardware real-time clock (RTC), and single-handed account navigation via the **L** and **R** shoulder triggers even with the console screen slid shut.

---

## Hardware Specifications & Embedded Runtime

* **Target Device**: Sony PSP Go (PSP-N1000) running Custom Firmware 6.60 (e.g., PROMOD / Infinity 2.0).
* **Processor**: MIPS Allegrex R4000 (32-bit little-endian) running at nominal **333 MHz** (system bus at 166 MHz).
* **System Memory**: 64 MB DDR SDRAM.
* **Graphics VRAM**: 2 MB ultra-fast embedded DRAM (eDRAM) configured in double-buffered 32-bit RGBA8888 mode ($512 \times 272 \times 4\text{ bytes}$).
* **Display**: 3.8-inch TFT LCD (16:9 ratio, $480 \times 272$ native pixels, 60 Hz vertical refresh).
* **Hardware Clock**: Internal battery-backed Real-Time Clock (RTC) delivering microsecond precision converted automatically to **pure UTC Unix Epoch**.
* **Storage Mounts**: Internal 16 GB eMMC flash storage mounted as `ef0:/` (or external M2 card as `ms0:/`).
* **Anti-Brick & Safety Directives**:
  * **Strict User Mode**: Declared via `PSP_MODULE_INFO("TotpVault", 0, 1, 1)`. Hardware-level MIPS segmentation prevents any accidental access to system kernel areas or flash partitions (`flash0:`).
  * **Expanded 512 KB Stack**: `PSP_MAIN_THREAD_STACK_SIZE_KB(512)` guarantees zero stack overflows during 10,000-round PBKDF2 key derivation.
  * **Atomic File Writes**: Re-keying operations on-device write to a temporary file (`vault.enc.tmp`), verify integrity, and atomically rename to `vault.enc`, eliminating corruption risks if power is interrupted.

---

## Security Model & Cryptographic Architecture

### 1. Absolute Air-Gap Protection
The application binary (`EBOOT.PBP`) is built in pure **C99** and links zero networking libraries (`libpspnet`, `libpspnet_inet`, `libpspwlan` are completely absent). The hardware WLAN slider switch should remain switched off, guaranteeing that no wireless radio or network interface can ever be energized.

### 2. Authenticated Encryption (Verify-then-Decrypt)
All sensitive data on disk resides inside a single deterministic file: `vault.enc` (fixed size of 15,372 bytes to prevent metadata leakage based on entry count).

```
+-------------------------------------------------------------------------+
| Public Header (32 Bytes): Magic "PSPV", PIN Length, Salt (16B), Nonce   |
+-------------------------------------------------------------------------+
| Ciphertext (15,308 Bytes): ChaCha20 Encrypted Records (TOTP/HOTP/Notes) |
+-------------------------------------------------------------------------+
| Authentication Tag (32 Bytes): HMAC-SHA256(K_mac, Header || Ciphertext) |
+-------------------------------------------------------------------------+
```

* **Physical PIN Key Derivation**: The user's physical button sequence ($2$, $4$, or $8$ presses) is combined with a 128-bit random salt via **PBKDF2-HMAC-SHA256 (RFC 8018)** using **10,000 iterations** to derive a 64-byte master key block:
  * First 32 bytes ($K_{\text{enc}}$): Used as the 256-bit symmetric key for ChaCha20 encryption.
  * Last 32 bytes ($K_{\text{mac}}$): Used as the 256-bit authentication key for HMAC-SHA256.
* **Stream Cipher (ChaCha20 — RFC 8439)**: 20 rounds of ARX operations (Add-Rotate-Xor) in 32 bits with 96-bit nonce. Invariant to cache-timing attacks with constant-time execution on the MIPS ALU.
* **Constant-Time Verification**: The authentication tag is computed and validated bit-by-bit in constant time ($\mathcal{O}(1)$) *prior* to decrypting a single byte in RAM. If the PIN is wrong or the file was tampered with, decryption is never executed.

### 3. RAM-Only Decryption & Active Zeroization
Plaintext keys and seeds exist exclusively in the volatile static `.bss` section of the PSP's RAM during an active session:
* **Auto-Lock Timer**: An inactivity timer locks the vault and purges memory after **120 seconds** of idle time.
* **Panic Lock**: Holding down **L + R** simultaneously for $>1.0\text{ second}$ instantly locks the vault.
* **Volatile Zeroization (`crypto_zeroize`)**: All plaintext records, intermediate hash buffers, and derived keys are overwritten with zeros using volatile pointers, preventing compiler dead-store elimination.

---

## Application Screens & Visual Engine

The graphical interface is built on direct `libpspgu` GPU commands with 3 distinct high-contrast hardware color palettes:
* **Dark Cyberpunk**: Deep navy background (`#0D1117`), cyan accents (`#58A6FF`), terminal green digits.
* **Amber Hardware**: Matte black (`#0A0A0A`) with vintage amber phosphorescent glow (`#FFB000`).
* **OLED Stealth**: Pitch black (`#000000`) with crisp chalk-white typography and subtle gold accents.

### The 6 Core Views

```
+----------------------------------------------------------------------------------+
| 1. Security Gate (LOCKED)       | 2. Giant Dock View (DOCK_TOTP)                 |
|   Enter Physical Button PIN:    |    GITHUB                                      |
|   [ * ] [ * ] [ * ] [ * ]       |    8 4 9  2 0 1                                |
|   Length: 4 Buttons             |    [========================      ] 18s        |
+---------------------------------+------------------------------------------------+
| 3. Account List (LIST_ACCOUNTS) | 4. Secret Viewer (SECRET_NOTE)                 |
|   > [2FA] GitHub (developer)    |    BIP-39 RECOVERY PHRASE (Hold X to reveal)   |
|     [2FA] AWS Root Account      |    abandon abandon ability able about above    |
|     [KEY] Bitcoin Cold Wallet   |    absent absorb abstract absurd abuse access  |
+---------------------------------+------------------------------------------------+
| 5. PIN Settings (PIN_SETTINGS)  | 6. Help Overlay (HELP_MODAL)                   |
|   Select PIN Length:            |    L/R: Switch Accounts   Square: Color Theme  |
|   < 2 >   [ 4 ]   < 8 >         |    SELECT: View List      Hold L+R: Panic Lock |
|   Press X to record sequence    |    Hold X: Reveal Secret  START: PIN Settings  |
+----------------------------------------------------------------------------------+
```

1. **Security Gate (`STATE_LOCKED`)**: The entry screen upon boot or lock. Requires pressing the configured physical button combination.
2. **Giant Dock View (`STATE_DOCK_TOTP`)**: Optimized for desktop dock cradles. Renders the 6 or 8 TOTP digits using a custom mathematical 7-segment vector engine, along with account label and live 30-second depleting progress bar.
3. **Account & Vault Directory (`STATE_LIST_ACCOUNTS`)**: Scrollable master list displaying all `[2FA]`, `[HOTP]`, and `[KEY]` entries with icons and metadata.
4. **Secret Vault Viewer (`STATE_SECRET_NOTE`)**: Dedicated inspection view for multi-line notes, SSH keys, or 12/24-word seed phrases. Includes a **Hold-to-Reveal** dead-man's switch: secrets remain masked (`****`) until the **Cross (`X`)** button is held down.
5. **On-Device PIN Settings (`STATE_PIN_SETTINGS`)**: Allows changing the physical PIN length ($2$, $4$, or $8$ presses) and recording a new sequence directly on the console with atomic re-encryption.
6. **Controls Reference Modal (`STATE_HELP_MODAL`)**: Two-column HUD overlay displaying active shortcuts.

---

## Complete Controls Mapping

| Action | Closed-Slider Dock Mode | Open-Slider Handheld Mode |
|---|---|---|
| **L Trigger** | Previous account / Decrement | Previous account / Scroll page up |
| **R Trigger** | Next account / Increment | Next account / Scroll page down |
| **Hold L + R ($>1.0\text{s}$)** | **Panic Lock**: Wipe RAM & lock | **Panic Lock**: Wipe RAM & lock |
| **D-Pad Up / Down** | N/A (Slider closed) | Navigate list items / options |
| **D-Pad Left / Right** | N/A (Slider closed) | Change PIN length / cycle themes |
| **Cross (X)** | N/A (Slider closed) | Select entry / **Hold to reveal secret** |
| **Circle (O)** | N/A (Slider closed) | Back / Cancel / Delete last PIN input |
| **Square** | N/A (Slider closed) | Cycle color themes (Cyberpunk / Amber / OLED) |
| **Triangle** | N/A (Slider closed) | Open / Close Controls Help Modal |
| **SELECT** | N/A (Slider closed) | Toggle between Giant Dock View and Account List |
| **START** | N/A (Slider closed) | Open PIN Settings / Exit from lock screen |

---

## Configuration & Key Provisioning (`mis_claves.txt`)

To provision accounts without tedious manual entry on a gamepad, the project includes a declarative text configuration format and an offline host CLI tool (`tools/vault_cli.c`).

### 1. Plaintext Template Syntax (`mis_claves.txt`)
Lines beginning with `#` are comments:

```ini
# Physical Unlock PIN Sequence (2, 4, or 8 buttons)
# Valid codes: U (Up), D (Down), L (Left), R (Right), < (Trigger L), > (Trigger R), X (Cross), O (Circle), S (Square), T (Triangle)
PIN = LRLR

# Time-Based 2FA Accounts (TOTP)
# Format: TOTP | Issuer | Account / Label | Base32_Secret | Hash_Algo (SHA1/SHA256) | Digits (6/8) | Period (30)
TOTP | GitHub   | developer@example.com | JBSWY3DPEHPK3PXP                 | SHA1   | 6 | 30
TOTP | AWS      | root-production       | GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ | SHA1   | 6 | 30
TOTP | Google   | security-team         | JBSWY3DPEHPK3PXP                 | SHA256 | 8 | 30

# Counter-Based 2FA Accounts (HOTP)
# Format: HOTP | Issuer | Account / Label | Base32_Secret | Hash_Algo (SHA1/SHA256) | Digits (6/8) | Initial_Counter
HOTP | BackupSlot | admin-token-1       | GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ | SHA1   | 6 | 0

# Encrypted Secrets, BIP-39 Seed Phrases & SSH Keys
# Format: NOTE | Category | Identifier | Secret Text (up to 239 characters)
NOTE | BIP-39   | BTC Cold Storage Seed | abandon abandon ability able about above absent absorb abstract absurd abuse access
NOTE | SSH Key  | Prod Cluster Key      | SHA256:4vK9L2mN8vQ4rT6wY1zB3cF5gH7jK0lM9nP2qR4sT6u (Passphrase: CorrectHorseBatteryStaple)
```

### 2. Using the Host CLI Tool (`vault_cli`)
Compile the tool on macOS or Linux with any standard C compiler:

```bash
make cli
```

* **Pack & Encrypt**:
  ```bash
  ./tools/vault_cli pack mis_claves.txt vault.enc
  ```
* **Inspect Header (Safe, no secrets revealed)**:
  ```bash
  ./tools/vault_cli inspect vault.enc
  ```
* **Verify Decryption against a PIN**:
  ```bash
  ./tools/vault_cli verify vault.enc LRLR
  ```

> 🔒 **Security Tip**: Store your personal plaintext keys in a local file ignored by git (e.g. `claves_reales.txt`), pack it into `vault.enc`, deploy to your PSP Go, and delete the plaintext file.

---

## Building & Installation

### Option A: Reproducible Docker Build (Recommended)
Zero host dependencies required beyond Docker:

```bash
# 1. Build the compiler image
docker build -t psp-totp-builder .

# 2. Compile EBOOT.PBP inside container
docker run --rm -v "$PWD":/src psp-totp-builder
```

### Option B: Native PSPDev Toolchain
If you have `pspdev` installed locally:

```bash
source ./activate.sh
make clean && make
deactivate_psp
```

---

## Safe USB Deployment to PSP Go

The PSP Go's internal storage is formatted as FAT32. On macOS, the operating system creates hidden metadata files (`._*`) that cause the PSP XMB menu to display **"Corrupted Data"** icons. Follow this safe transfer procedure:

```bash
# 1. Connect PSP Go via USB (ensure WLAN physical switch is OFF)
mkdir -p "/Volumes/PSP GO/PSP/GAME/TotpVault"

# 2. Copy the executable and encrypted vault
cp EBOOT.PBP vault.enc "/Volumes/PSP GO/PSP/GAME/TotpVault/"

# 3. Purge macOS AppleDouble metadata and flush disk buffers
dot_clean -m "/Volumes/PSP GO"
sync
```

Eject the drive safely in Finder or terminal before disconnecting the USB cable.

> 🕒 **Clock Synchronization**: Ensure the PSP Go's internal date, time, and timezone are set correctly in `Settings -> Date & Time Settings`. The application converts local hardware ticks to UTC Unix Epoch automatically, matching world-standard TOTP clocks to the second.

---

## Running Unit Tests

The core cryptography and business logic are platform-independent C99 and include a test suite with **89 automated assertions** validating official test vectors from **FIPS 180-4**, **RFC 2104**, **RFC 8018**, **RFC 8439**, **RFC 4648**, **RFC 4226**, and **RFC 6238**:

```bash
make test
```

Expected output:
```text
====================================================
  04_HardwareTotp_Vault - Cryptographic Verification 
====================================================
[*] Testing SHA-1 (RFC 3174)...
[*] Testing SHA-256 (FIPS 180-4)...
[*] Testing HMAC-SHA1 & HMAC-SHA256 (RFC 2202 / 4231)...
[*] Testing PBKDF2-HMAC-SHA256 (RFC 6070)...
[*] Testing ChaCha20 (RFC 8439 Sec 2.4.2)...
[*] Testing Base32 (RFC 4648)...
[*] Testing HOTP (RFC 4226)...
[*] Testing TOTP (RFC 6238)...
[*] Testing Vault Container (vault.c)...
[*] Testing Input Logic & Gestures (input_logic.c)...
[*] Testing App State Machine (app_state.c)...
====================================================
  Summary: 89 / 89 tests passed successfully.
====================================================
```

---

## Project Directory Structure

```text
.
├── .gitignore                   # Excludes build artifacts, private keys, and macOS files
├── Dockerfile                   # 10-line reproducible build container (pspdev/pspdev:latest)
├── LICENSE                      # MIT Open Source License
├── Makefile                     # Dual build system (PSP EBOOT.PBP + Host test_core & vault_cli)
├── README.md                    # English documentation
├── README_ES.md                 # Spanish documentation
├── activate.sh                  # Portable shell activation for local pspdev installations
├── main.c                       # User-Mode initialization (512 KB stack) and 60 FPS loop
├── mis_claves.txt               # Synthetic example key configuration (RFC vectors only)
├── vault.enc                    # Synthetic demo encrypted vault (PIN: LRLR)
├── core/
│   ├── app_state.h / .c         # 6-state finite state machine & session management
│   ├── base32.h / .c            # RFC 4648 Base32 decoder
│   ├── crypto_chacha20.h / .c   # Pure C99 ChaCha20 256-bit stream cipher (RFC 8439)
│   ├── crypto_hmac.h / .c       # Pure C99 HMAC & PBKDF2-HMAC-SHA256 (RFC 8018)
│   ├── crypto_sha.h / .c        # Pure C99 SHA-1 & SHA-256 + constant-time zeroize
│   ├── input_logic.h / .c       # PIN recognizer, dock triggers, and 120s auto-lock
│   ├── otp.h / .c               # HOTP (RFC 4226) & TOTP (RFC 6238) dynamic truncation
│   └── vault.h / .c             # Binary vault container (Verify-then-Decrypt & Re-keying)
├── ui/
│   ├── font_7seg.h / .c         # Procedural 7-segment vector renderer for giant digits
│   ├── font_8x8.h / .c          # 8x8 bitmap font with Spanish accent sanitization
│   ├── graphics.h / .c          # 60 FPS hardware libpspgu double-buffered eDRAM renderer
│   ├── screens.h / .c           # Screen views, 30s progress bar & Hold-to-Reveal card
│   └── theme_engine.h / .c      # 3 hardware color palettes (Cyberpunk, Amber, OLED)
├── platform/
│   ├── psp_fs.h / .c            # Safe User-Mode I/O for ef0:/ and ms0:/ with atomic rename
│   ├── psp_input.h / .c         # Edge-triggered digital button polling (sceCtrl)
│   ├── psp_power.h / .c         # Battery telemetry and scePowerTick keep-awake pulse
│   └── psp_rtc.h / .c           # Pure UTC Unix epoch conversion from hardware RTC ticks
├── tools/
│   └── vault_cli.c              # Host CLI utility to pack, inspect, and verify vaults
├── tests/
│   └── test_core.c              # 89 unit assertions with official RFC test vectors
└── docs/
    ├── ARCHITECTURE.md          # In-depth technical architecture and byte-level spec
    └── TOOLCHAIN_SETUP.md       # Toolchain, Docker, and USB deployment manual
```

---

## License

This project is licensed under the terms of the **MIT License**. See the [LICENSE](LICENSE) file for complete details.
