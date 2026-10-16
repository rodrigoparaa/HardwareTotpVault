# Guía de Toolchain, Empaquetado de Claves y Despliegue Seguro (`04_HardwareTotp_Vault`)

> **Instrucciones paso a paso para compilar el proyecto mediante Docker o el SDK nativo `pspdev`, ejecutar la batería de 89 pruebas criptográficas en el equipo anfitrión, aprovisionar nuevas bóvedas `vault.enc` y desplegar de forma segura en la memoria flash de la PSP Go.**

---

## 1. Compilación de `EBOOT.PBP` (Dos Alternativas)

El proyecto puede compilarse tanto mediante un contenedor **Docker** reproducible (sin necesidad de instalar herramientas en el sistema anfitrión) como mediante una instalación local del toolchain oficial **PSPDev**.

### Opción A: Compilación Reproducible con Docker (Recomendado en cualquier OS)
El archivo `Dockerfile` de la raíz utiliza la imagen oficial `pspdev/pspdev:latest`:

```bash
# 1. Construir la imagen del compilador cruzado
docker build -t psp-totp-builder .

# 2. Compilar el proyecto montando el directorio actual (genera EBOOT.PBP en la raíz)
docker run --rm -v "$PWD":/src psp-totp-builder
```

### Opción B: Compilación Nativa con `pspdev` (`activate.sh`)
Si dispones del SDK de PSP instalado en `/usr/local/pspdev`, `/opt/pspdev` o definido en la variable de entorno `$PSPDEV`:

```bash
# 1. Activar el entorno aislado en la sesión actual de terminal
source ./activate.sh

# 2. Limpiar artefactos previos y compilar EBOOT.PBP
make clean && make

# 3. Restaurar el PATH original al finalizar
deactivate_psp
```

---

## 2. Ejecución de la Suite de Pruebas Unitarias en Host (`make test`)

Toda la capa `core/` está escrita en **C99 puro independiente de plataforma** y cuenta con una batería de **89 pruebas unitarias** (`tests/test_core.c`) que verifican:
* Vectores oficiales **FIPS 180-4** para `SHA-1` y `SHA-256`.
* Vectores **RFC 2104 / RFC 4231** para `HMAC-SHA1` y `HMAC-SHA256`.
* Vectores **RFC 8018** para `PBKDF2-HMAC-SHA256`.
* Vector oficial **RFC 8439 §2.4.2** para el cifrador de flujo `ChaCha20`.
* Vectores **RFC 4648** para el decodificador `Base32`.
* Los 10 vectores oficiales **RFC 4226 Apéndice D** para `HOTP`.
* Los vectores oficiales **RFC 6238 Apéndice B** para `TOTP` (`SHA-1` y `SHA-256`, 6 y 8 dígitos).
* Empaquetado, verificación de integridad en tiempo constante (*Verify-then-Decrypt*), rechazo de manipulación de 1 bit y re-cifrado atómico de `vault.enc`.

Para compilar y ejecutar las pruebas con el compilador nativo de tu ordenador (`clang` o `gcc`):

```bash
make test
```

---

## 3. Gestión y Empaquetado de Claves con `vault_cli`

Para evitar tener que escribir semillas Base32 de 32 caracteres o frases mnemotécnicas largas con el pad direccional de la consola, el proyecto incluye la herramienta nativa de línea de comandos `tools/vault_cli.c`.

### 3.1. Compilar la utilidad `vault_cli`
```bash
make cli
```

### 3.2. Formato del archivo de configuración (`mis_claves.txt`)
El archivo de entrada utiliza un formato declarativo sencillo donde las líneas que comienzan por `#` son comentarios ignorados:

```ini
# 1. Secuencia física de desbloqueo (2, 4 u 8 botones)
# Códigos: U=Arriba, D=Abajo, L=Izquierda, R=Derecha, <=Gatillo L, >=Gatillo R, X=Cruz, O=Círculo, S=Cuadrado, T=Triángulo
PIN = UUDDLRLR

# 2. Cuentas 2FA Temporales (TOTP)
# Formato: TOTP | Proveedor | Cuenta | Semilla_Base32 | Algoritmo (SHA1/SHA256) | Dígitos (6/8) | Periodo (30)
TOTP | GitHub    | developer@example.com | JBSWY3DPEHPK3PXP                 | SHA1   | 6 | 30
TOTP | AWS       | root-prod-account     | GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ | SHA1   | 6 | 30

# 3. Cuentas 2FA por Contador de Eventos (HOTP)
# Formato: HOTP | Proveedor | Cuenta | Semilla_Base32 | Algoritmo (SHA1/SHA256) | Dígitos (6/8) | Contador_Inicial
HOTP | YubiKey   | backup-slot-1         | GEZDGNBVGY3TQOJQGEZDGNBVGY3TQOJQ | SHA1   | 6 | 0

# 4. Cofre de Notas Criptográficas, Semillas BIP-39 y Claves SSH/GPG
# Formato: NOTE | Categoría | Identificador | Texto Secreto (hasta 239 caracteres)
NOTE | SSH Key   | prod-cluster-ed25519  | SHA256:xK9pL2mN8vQ4rT6wY1zB3cF5gH7jK0lM9nP2qR4sT6u (Pass: CorrectHorseBattery)
```

### 3.3. Generar e inspeccionar `vault.enc`
```bash
# Empaquetar y cifrar mis_claves.txt hacia vault.enc
./tools/vault_cli pack mis_claves.txt vault.enc

# Inspeccionar la cabecera pública de vault.enc (sin revelar ningún secreto)
./tools/vault_cli inspect vault.enc

# Verificar el descifrado pasando el PIN físico por consola
./tools/vault_cli verify vault.enc UUDDLRLR
```

> **Recomendación de Seguridad**: Nunca subas tu archivo personal de claves en texto plano a un repositorio Git. Utiliza un archivo temporal ignorado por `.gitignore` (por ejemplo, `claves_reales.txt`), genera `vault.enc`, transfiérelo a la PSP Go y elimina el archivo en texto plano.

---

## 4. Protocolo de Despliegue Seguro por USB en PSP Go (macOS / Linux)

La memoria flash interna de 16 GB de la PSP Go (`ef0:`) utiliza un sistema de archivos **FAT32**. En macOS, el Finder crea archivos ocultos de metadatos AppleDouble (`._EBOOT.PBP`, `._vault.enc`) que el menú XMB de Sony muestra como iconos de **"Datos dañados"**.

Sigue siempre estos comandos al conectar la PSP Go en **Modo USB**:

```bash
# 1. Crear el directorio de la aplicación en la memoria interna de la PSP Go
mkdir -p "/Volumes/PSP GO/PSP/GAME/TotpVault"

# 2. Copiar el binario compilado y la bóveda cifrada
cp EBOOT.PBP vault.enc "/Volumes/PSP GO/PSP/GAME/TotpVault/"

# 3. Limpiar archivos ocultos AppleDouble (macOS) y vaciar los buffers de disco
dot_clean -m "/Volumes/PSP GO"
sync
```

Una vez finalizado `sync`, expulsa el volumen `"PSP GO"` de forma segura antes de desconectar el cable USB.
