# PSP Go Air-Gapped Hardware TOTP & Seed Vault

[![Licencia: MIT](https://img.shields.io/badge/Licencia-MIT-blue.svg)](LICENSE)
[![Lenguaje: C99](https://img.shields.io/badge/Lenguaje-C99%20Puro-00599C.svg)](#arquitectura-del-proyecto)
[![Plataforma: Sony PSP Go (PSP-N1000)](https://img.shields.io/badge/Hardware-PSP%20Go%20(MIPS%20R4000)-black.svg)](#especificaciones-de-hardware-y-entorno-embebido)
[![Criptografía: ChaCha20 + PBKDF2-HMAC-SHA256](https://img.shields.io/badge/Cripto-ChaCha20%20%2B%20HMAC--SHA256-success.svg)](#modelo-de-seguridad-y-arquitectura-criptográfica)
[![Tests: 89/89 Superados](https://img.shields.io/badge/Tests%20RFC-89%2F89%20OK-brightgreen.svg)](#ejecución-de-pruebas-unitarias)

> 🇬🇧 **Read in English**: [README.md](README.md) · 📐 **Especificación Técnica Completa**: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) · 🛠️ **Guía de Toolchain y Despliegue**: [docs/TOOLCHAIN_SETUP.md](docs/TOOLCHAIN_SETUP.md)

**PSP Go Air-Gapped Hardware TOTP & Seed Vault** transforma una consola **Sony PlayStation Portable Go (PSP-N1000)** con el interruptor físico WLAN apagado en un **token criptográfico hardware 100% fuera de línea (*air-gapped*)**.

Proporciona dos funciones críticas de seguridad en un único dispositivo físico de bolsillo:
1. **Códigos de Segundo Factor Temporales y por Evento**: Generador estándar de códigos **TOTP (RFC 6238)** y **HOTP (RFC 4226)** compatible con servicios como GitHub, AWS, Google, Proton, Cloudflare y autenticadores corporativos.
2. **Cofre Cifrado de Semillas Críticas y Contraseñas**: Custodia de alta seguridad para **frases de recuperación BIP-39 (12/24 palabras)**, **claves privadas y huellas SSH/GPG**, códigos de rescate y contraseñas maestras.

La aplicación está diseñada tanto para **uso portátil convencional** como para operar sobre un **dock de escritorio con la pantalla plegada (slider cerrado)**: renderiza dígitos vectoriales gigantes de 7 segmentos a **60 FPS** acelerados por hardware en la eDRAM de la PSP, incorpora una barra de progreso de 30 segundos sincronizada al segundo con el reloj en tiempo real (RTC) del sistema, y permite cambiar de cuenta con una sola mano mediante los gatillos **L** y **R** sin necesidad de abrir la consola.

---

## Especificaciones de Hardware y Entorno Embebido

* **Dispositivo Objetivo**: Sony PSP Go (PSP-N1000) con Custom Firmware 6.60 (p. ej. PROMOD / Infinity 2.0).
* **Procesador**: MIPS Allegrex R4000 (32 bits, little-endian) a frecuencia nominal de **333 MHz** (bus de sistema a 166 MHz).
* **Memoria Principal**: 64 MB DDR SDRAM.
* **Memoria de Vídeo (eDRAM)**: 2 MB de memoria embebida ultrarrápida configurada en doble búfer RGBA8888 de 32 bits ($512 \times 272 \times 4\text{ bytes}$).
* **Pantalla LCD**: Panel TFT panorámico de 3,8 pulgadas (formato 16:9, resolución nativa de $480 \times 272$ píxeles a 60 Hz).
* **Reloj en Tiempo Real (RTC)**: RTC interno alimentado por la batería de la consola con precisión de microsegundos, convertido automáticamente a **época Unix UTC pura**.
* **Almacenamiento de Usuario**: Memoria flash interna eMMC de 16 GB montada como `ef0:/` (o tarjeta M2 externa como `ms0:/`).
* **Medidas de Seguridad Crítica Anti-Brick**:
  * **Modo Usuario Estricto (*User Mode*)**: Declarado formalmente mediante `PSP_MODULE_INFO("TotpVault", 0, 1, 1)`. La segmentación MIPS KSEG protege por hardware el kernel de Sony e imposibilita tocar particiones críticas de la flash interna (`flash0:`).
  * **Pila Principal Ampliada (512 KB)**: `PSP_MAIN_THREAD_STACK_SIZE_KB(512)` erradica cualquier riesgo de desbordamiento de pila durante las 10.000 rondas de derivación PBKDF2.
  * **Escritura Atómica en Disco**: El re-cifrado en la consola escribe primero en un archivo temporal (`vault.enc.tmp`), verifica su integridad y renombra atómicamente a `vault.enc`, impidiendo la corrupción de datos ante agotamiento repentino de batería.

---

## Modelo de Seguridad y Arquitectura Criptográfica

### 1. Aislamiento Físico Absoluto (*Air-Gap*)
El ejecutable `EBOOT.PBP` está desarrollado en **C99 puro** sin dependencias externas y no enlaza ninguna biblioteca de red (`libpspnet`, `libpspnet_inet`, `libpspwlan` no existen en el binario). Al mantener el interruptor WLAN en posición de apagado, el hardware de radio permanece desenergizado.

### 2. Cifrado Autenticado (*Verify-then-Decrypt*)
Toda la información confidencial se almacena en un único archivo determinista: `vault.enc` (tamaño fijo e invariante de 15.372 bytes para impedir ataques por análisis de tamaño de archivo).

```
+-------------------------------------------------------------------------+
| Cabecera Pública (32 Bytes): Firma "PSPV", Longitud PIN, Sal, Nonce     |
+-------------------------------------------------------------------------+
| Carga Útil Cifrada (15.308 Bytes): Registros con ChaCha20 (2FA/Semillas)|
+-------------------------------------------------------------------------+
| Etiqueta HMAC (32 Bytes): HMAC-SHA256(K_mac, Cabecera || Carga Útil)    |
+-------------------------------------------------------------------------+
```

* **Derivación de Clave desde Botones Físicos**: La secuencia de pulsaciones físicas ($2$, $4$ u $8$ botones) se combina con una sal aleatoria de $128\text{ bits}$ mediante **PBKDF2-HMAC-SHA256 (RFC 8018)** a lo largo de **10.000 iteraciones** para derivar un bloque maestro de 64 bytes:
  * Primeros 32 bytes ($K_{\text{enc}}$): Clave simétrica de 256 bits para ChaCha20.
  * Últimos 32 bytes ($K_{\text{mac}}$): Clave de autenticación de 256 bits para HMAC-SHA256.
* **Cifrador de Flujo ChaCha20 (RFC 8439)**: 20 rondas ARX (*Add-Rotate-Xor*) en 32 bits con nonce de 96 bits. Ejecución en tiempo constante en la ALU MIPS, sin tablas S-Box e inmune a ataques de temporización por caché.
* **Verificación en Tiempo Constante**: La etiqueta HMAC-SHA256 se valida bit a bit en tiempo constante ($\mathcal{O}(1)$) **antes** de realizar cualquier descifrado en RAM. Si el PIN es erróneo o el archivo fue alterado, el descifrador ChaCha20 jamás llega a ejecutarse.

### 3. Descifrado Exclusivo en RAM y Purgado Activo (`crypto_zeroize`)
Los secretos en claro existen únicamente en la sección estática `.bss` de la memoria RAM mientras la sesión está abierta:
* **Temporizador de Inactividad**: Auto-bloqueo y purgado tras **120 segundos** sin pulsaciones.
* **Bloqueo de Emergencia (*Panic Lock*)**: Mantener pulsados los gatillos **L + R** simultáneamente durante $>1.0\text{ segundo}$ bloquea la bóveda al instante.
* **Sobrescritura Volátil (`crypto_zeroize`)**: Todos los registros, búferes de hash y claves derivadas se sobrescriben con ceros mediante punteros `volatile`, impidiendo que el compilador descarte la limpieza de memoria (*Dead Store Elimination*).

---

## Pantallas y Motor Visual de la Aplicación

La interfaz gráfica utiliza comandos directos de GPU `libpspgu` con 3 esquemas cromáticos de alto contraste adaptados al panel LCD de la consola:
* **Dark Cyberpunk**: Fondo azul noche profundo (`#0D1117`), acentos cian (`#58A6FF`) y dígitos en verde terminal.
* **Amber Hardware**: Negro mate (`#0A0A0A`) con fósforo ámbar vintage (`#FFB000`).
* **OLED Stealth**: Negro puro (`#000000`) con tipografía blanco tiza y sutiles acentos dorados.

### Las 6 Pantallas del Sistema

```
+----------------------------------------------------------------------------------+
| 1. Puerta de Seguridad (LOCKED) | 2. Modo Dock Gigante (DOCK_TOTP)               |
|   Introduce PIN Físico:         |    GITHUB                                      |
|   [ * ] [ * ] [ * ] [ * ]       |    8 4 9  2 0 1                                |
|   Longitud: 4 Botones           |    [========================      ] 18s        |
+---------------------------------+------------------------------------------------+
| 3. Lista de Cuentas (ACCOUNTS)  | 4. Visor Secreto (SECRET_NOTE)                 |
|   > [2FA] GitHub (developer)    |    FRASE SEMILLA BIP-39 (Mantén X presionado)  |
|     [2FA] AWS Root Account      |    abandon abandon ability able about above    |
|     [KEY] Bitcoin Cold Wallet   |    absent absorb abstract absurd abuse access  |
+---------------------------------+------------------------------------------------+
| 5. Ajustes de PIN (SETTINGS)    | 6. Modal de Ayuda (HELP_MODAL)                 |
|   Selecciona Longitud de PIN:   |    L/R: Cambiar cuenta    Cuadrado: Tema color |
|   < 2 >   [ 4 ]   < 8 >         |    SELECT: Ver lista      Mantener L+R: Panic  |
|   Pulsa X para grabar secuencia |    Mantener X: Revelar    START: Ajustar PIN   |
+----------------------------------------------------------------------------------+
```

1. **Puerta de Seguridad (`STATE_LOCKED`)**: Pantalla inicial tras el encendido o tras un bloqueo. Requiere pulsar la secuencia física configurada.
2. **Modo Dock Gigante (`STATE_DOCK_TOTP`)**: Optimizado para reposar en base de escritorio. Renderiza los 6 u 8 dígitos TOTP con el motor vectorial de 7 segmentos junto a la barra de progreso de 30 segundos.
3. **Directorio de Cuentas y Cofre (`STATE_LIST_ACCOUNTS`)**: Lista desplazable con todas las entradas `[2FA]`, `[HOTP]` y `[KEY]` con iconos y metadatos.
4. **Visor Secreto con Pulsador de Hombre Muerto (`STATE_SECRET_NOTE`)**: Visor para frases BIP-39 de 12/24 palabras, claves SSH o contraseñas maestras. Los datos permanecen ocultos (`****`) salvo mientras se mantenga presionado físicamente el botón **Cruz (`X`)**.
5. **Configuración de PIN en Consola (`STATE_PIN_SETTINGS`)**: Permite seleccionar la longitud deseada ($2$, $4$ u $8$ pulsaciones) y registrar una nueva combinación con re-cifrado atómico inmediato.
6. **Modal de Ayuda Rápida (`STATE_HELP_MODAL`)**: Resumen emergente en 2 columnas con los atajos operativos disponibles.

---

## Tabla Completa de Controles Físicos

| Acción | Modo Dock con Slider Cerrado | Modo Consola Abierta |
|---|---|---|
| **Gatillo L** | Cuenta anterior / Decrementar | Cuenta anterior / Subir página |
| **Gatillo R** | Cuenta siguiente / Incrementar | Cuenta siguiente / Bajar página |
| **Acorde L + R (>1.0s)** | **Panic Lock**: Purgar RAM y bloquear | **Panic Lock**: Purgar RAM y bloquear |
| **Cruceta Arriba / Abajo** | N/A (Pantalla cerrada) | Moverse por elementos de listas |
| **Cruceta Izq. / Der.** | N/A (Pantalla cerrada) | Modificar longitud de PIN / ciclar temas |
| **Botón Cruz (X)** | N/A (Pantalla cerrada) | Seleccionar / **Mantener para ver secreto** |
| **Botón Círculo (O)** | N/A (Pantalla cerrada) | Volver / Cancelar / Borrar pulsación |
| **Botón Cuadrado** | N/A (Pantalla cerrada) | Alternar tema visual (Cyberpunk / Amber / OLED) |
| **Botón Triángulo** | N/A (Pantalla cerrada) | Abrir / Cerrar ayuda en pantalla |
| **Botón SELECT** | N/A (Pantalla cerrada) | Alternar entre Vista Dock y Lista completa |
| **Botón START** | N/A (Pantalla cerrada) | Abrir Ajustes de PIN / Salir al menú XMB |

---

## Configuración y Carga de Claves (`mis_claves.txt`)

Para evitar la tediosa introducción manual de largas semillas Base32 o frases de 24 palabras con el pad de la consola, el proyecto incluye una utilidad nativa para ordenador (`tools/vault_cli.c`).

### 1. Sintaxis de la Plantilla (`mis_claves.txt`)
Las líneas que comienzan por `#` son comentarios:

```ini
# Secuencia física de desbloqueo (2, 4 u 8 pulsaciones)
# Botones válidos: U (Arriba), D (Abajo), L (Izquierda), R (Derecha), < (Gatillo L), > (Gatillo R), X (Cruz), O (Círculo), S (Cuadrado), T (Triángulo)
PIN = LRLR

# Cuentas 2FA Temporales (TOTP)
# Formato: TOTP | Proveedor | Cuenta / Etiqueta | Semilla_Base32 | Algoritmo (SHA1/SHA256) | Dígitos (6/8) | Periodo (30)
TOTP | GitHub   | developer@example.com | JBSWY3DPEHPK3PXP                 | SHA1   | 6 | 30
TOTP | AWS      | root-production       | GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ | SHA1   | 6 | 30
TOTP | Google   | security-team         | JBSWY3DPEHPK3PXP                 | SHA256 | 8 | 30

# Cuentas 2FA por Eventos (HOTP)
# Formato: HOTP | Proveedor | Cuenta / Etiqueta | Semilla_Base32 | Algoritmo (SHA1/SHA256) | Dígitos (6/8) | Contador_Inicial
HOTP | BackupSlot | admin-token-1       | GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ | SHA1   | 6 | 0

# Cofre de Semillas BIP-39, Claves SSH y Notas Cifradas
# Formato: NOTE | Categoría | Identificador | Secreto (hasta 239 caracteres)
NOTE | BIP-39   | BTC Cold Storage Seed | abandon abandon ability able about above absent absorb abstract absurd abuse access
NOTE | SSH Key  | Prod Cluster Key      | SHA256:4vK9L2mN8vQ4rT6wY1zB3cF5gH7jK0lM9nP2qR4sT6u (Passphrase: CorrectHorseBatteryStaple)
```

### 2. Uso de la Utilidad CLI (`vault_cli`)
Compilación inmediata en macOS o Linux con cualquier compilador estándar de C:

```bash
make cli
```

* **Empaquetar y Cifrar**:
  ```bash
  ./tools/vault_cli pack mis_claves.txt vault.enc
  ```
* **Inspeccionar Cabecera (Seguro, no revela secretos)**:
  ```bash
  ./tools/vault_cli inspect vault.enc
  ```
* **Verificar Descifrado con PIN**:
  ```bash
  ./tools/vault_cli verify vault.enc LRLR
  ```

> 🔒 **Recomendación de Seguridad**: Puedes utilizar un archivo local ignorado por Git (p. ej. `claves_reales.txt`, ya incluido en `.gitignore`), generar el archivo `vault.enc`, transferirlo a la consola y eliminar el archivo en claro de tu disco.

---

## Compilación y Empaquetado

### Opción A: Compilación Reproducible con Docker (Recomendado)
Sin necesidad de instalar toolchains ni dependencias en tu ordenador:

```bash
# 1. Construir la imagen del compilador
docker build -t psp-totp-builder .

# 2. Compilar EBOOT.PBP en el contenedor
docker run --rm -v "$PWD":/src psp-totp-builder
```

### Opción B: Toolchain Local de PSPDev
Si dispones de `pspdev` instalado localmente:

```bash
source ./activate.sh
make clean && make
deactivate_psp
```

---

## Despliegue Físico por USB en PSP Go

La memoria flash interna de la PSP Go está formateada en FAT32. En macOS, el sistema operativo genera archivos ocultos de metadatos (`._*`) que provocan que el menú XMB muestre iconos de **"Datos dañados"**. Sigue este procedimiento seguro:

```bash
# 1. Conectar la PSP Go por USB (con el interruptor WLAN en OFF)
mkdir -p "/Volumes/PSP GO/PSP/GAME/TotpVault"

# 2. Copiar el ejecutable y la bóveda cifrada
cp EBOOT.PBP vault.enc "/Volumes/PSP GO/PSP/GAME/TotpVault/"

# 3. Purgar metadatos ocultos AppleDouble y vaciar buffers de disco
dot_clean -m "/Volumes/PSP GO"
sync
```

Expulsa el volumen `"PSP GO"` en el Finder o terminal antes de desconectar el cable físico.

> 🕒 **Sincronización del Reloj**: Comprueba que la hora y tu zona horaria estén correctamente configuradas en el menú `Ajustes -> Ajustes de fecha y hora` de la consola. La aplicación traduce automáticamente los microsegundos del RTC a época Unix UTC, cuadrando al segundo exacto con los servidores TOTP mundiales.

---

## Ejecución de Pruebas Unitarias

El núcleo criptográfico y la lógica de negocio son C99 puro independiente de plataforma y cuentan con una batería de **89 pruebas unitarias automatizadas** que validan los vectores de prueba oficiales de **FIPS 180-4**, **RFC 2104**, **RFC 8018**, **RFC 8439**, **RFC 4648**, **RFC 4226** y **RFC 6238**:

```bash
make test
```

Salida esperada:
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

## Estructura de Directorios del Repositorio

```text
.
├── .gitignore                   # Excluye artefactos, claves privadas y archivos del sistema
├── Dockerfile                   # Contenedor reproducible de 10 líneas (pspdev/pspdev:latest)
├── LICENSE                      # Licencia de código abierto MIT
├── Makefile                     # Build system dual (EBOOT.PBP para PSP + test_core y vault_cli en host)
├── README.md                    # Documentación en inglés
├── README_ES.md                 # Documentación en español
├── activate.sh                  # Activador de entorno local para pspdev
├── main.c                       # Punto de entrada en User Mode (512 KB stack) y bucle a 60 FPS
├── mis_claves.txt               # Plantilla sintética de ejemplo con vectores RFC públicos
├── vault.enc                    # Bóveda cifrada de demostración (PIN: LRLR)
├── core/
│   ├── app_state.h / .c         # Máquina de estados finita (6 vistas) y ciclo de sesión
│   ├── base32.h / .c            # Decodificador Base32 conforme al RFC 4648
│   ├── crypto_chacha20.h / .c   # Cifrador de flujo ChaCha20 de 256 bits (RFC 8439)
│   ├── crypto_hmac.h / .c       # Motores HMAC y derivación PBKDF2-HMAC-SHA256 (RFC 8018)
│   ├── crypto_sha.h / .c        # Funciones hash SHA-1 y SHA-256 + purgado volatile zeroize
│   ├── input_logic.h / .c       # Reconocedor de PIN físico, gestos L/R y auto-lock de 120s
│   ├── otp.h / .c               # Motores de truncado dinámico HOTP (RFC 4226) y TOTP (RFC 6238)
│   └── vault.h / .c             # Contenedor binario (Verify-then-Decrypt y re-cifrado atómico)
├── ui/
│   ├── font_7seg.h / .c         # Motor vectorial de 7 segmentos para dígitos gigantes
│   ├── font_8x8.h / .c          # Tipografía bitmap 8x8 con soporte para tildes y caracteres españoles
│   ├── graphics.h / .c          # Renderizado a 60 FPS con libpspgu en doble búfer eDRAM
│   ├── screens.h / .c           # Vistas de la aplicación, barra de 30s y visor Hold-to-Reveal
│   └── theme_engine.h / .c      # 3 paletas cromáticas de hardware (Cyberpunk, Amber, OLED)
├── platform/
│   ├── psp_fs.h / .c            # I/O seguro en User Mode para ef0:/ y ms0:/ con renombrado atómico
│   ├── psp_input.h / .c         # Sondeo de botones digitales por flancos de subida (sceCtrl)
│   ├── psp_power.h / .c         # Telemetría de batería y pulso anti-suspensión scePowerTick
│   └── psp_rtc.h / .c           # Conversión de ticks del RTC a tiempo Unix UTC real
├── tools/
│   └── vault_cli.c              # Herramienta de host para empaquetar, inspeccionar y validar bóvedas
├── tests/
│   └── test_core.c              # Suite con 89 aserciones unitarias contra vectores RFC
└── docs/
    ├── ARCHITECTURE.md          # Especificación exhaustiva de arquitectura y seguridad a nivel de byte
    └── TOOLCHAIN_SETUP.md       # Manual de compilación cruzada, Docker y despliegue físico USB
```

---

## Licencia

Este proyecto se distribuye bajo los términos de la **Licencia MIT**. Consulta el archivo [LICENSE](LICENSE) para más detalles.
