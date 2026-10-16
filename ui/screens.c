#include "screens.h"
#include "graphics.h"
#include "font_7seg.h"
#include "font_8x8.h"
#include "theme_engine.h"
#include "otp.h"
#include "psp_rtc.h"
#include "psp_power.h"
#include <stdio.h>
#include <string.h>

/* Helper to render the shared HUD Top Bar */
static void render_hud_top(const theme_palette_t *theme, const char *left_tag,
                           uint64_t now_utc_sec) {
    /* Top bar background */
    gfx_draw_rect_fill(0, 0, SCREEN_WIDTH, 24, theme->card_bg);
    gfx_draw_rect_fill(0, 24, SCREEN_WIDTH, 1, theme->accent);

    /* Left tag */
    font_draw_string(10, 8, left_tag, 1, theme->accent);

    /* Middle UTC Clock */
    char time_str[32];
    psp_rtc_format_time(now_utc_sec, time_str, sizeof(time_str), 1);
    char clock_buf[48];
    snprintf(clock_buf, sizeof(clock_buf), "UTC %s", time_str);
    font_draw_string_centered(8, clock_buf, 1, theme->text_primary);

    /* Right Battery indicator */
    int bat_pct = psp_power_get_battery_percent();
    int is_charging = psp_power_is_charging();
    char bat_buf[32];
    if (is_charging) {
        snprintf(bat_buf, sizeof(bat_buf), "[CHG] %d%%", bat_pct);
    } else {
        snprintf(bat_buf, sizeof(bat_buf), "[%d%%]", bat_pct);
    }
    uint32_t bat_color = (bat_pct > 20) ? theme->text_primary : theme->critical;
    int bat_w = font_string_width(bat_buf, 1);
    font_draw_string(SCREEN_WIDTH - bat_w - 10, 8, bat_buf, 1, bat_color);
}

/* =========================================================================
 * 1. Screen: LOCKED (Authentication / PIN sequence entry)
 * ========================================================================= */
static void render_screen_locked(AppState *app, uint64_t now_ticks_ms) {
    const theme_palette_t *theme = theme_get(app->theme_idx);
    gfx_clear(theme->bg);

    /* Header */
    font_draw_string_centered(30, "HARDWARE TOTP VAULT", 2, theme->accent);
    font_draw_string_centered(54, "AIR-GAPPED SECURITY TOKEN", 1, theme->text_secondary);

    /* Penalty cooldown message or unlock prompt */
    if (now_ticks_ms < app->pin_input.cooldown_until_ms) {
        uint64_t rem_sec = (app->pin_input.cooldown_until_ms - now_ticks_ms + 999) / 1000;
        char err_buf[64];
        snprintf(err_buf, sizeof(err_buf), "TAMPER COOLDOWN: ESPERA %llu s", rem_sec);
        gfx_draw_rect_fill(40, 84, 400, 22, theme->critical);
        font_draw_string_centered(91, err_buf, 1, COLOR_RGB(255, 255, 255));
    } else {
        font_draw_string_centered(91, "INTRODUCE SECUENCIA FISICA DE BOTONES", 1, theme->text_primary);
    }

    /* Render PIN Slots */
    uint8_t total_slots = app->pin_input.max_len;
    if (total_slots == 0) total_slots = 4;

    int slot_w = 40;
    int slot_h = 44;
    int slot_gap = 12;
    int total_w = (total_slots * slot_w) + ((total_slots - 1) * slot_gap);
    int start_x = (SCREEN_WIDTH - total_w) / 2;
    int slot_y = 125;

    for (int i = 0; i < total_slots; i++) {
        int x = start_x + (i * (slot_w + slot_gap));
        int filled = (i < app->pin_input.count);

        uint32_t border_col = filled ? theme->accent : theme->ghost_7seg;
        gfx_draw_rect_fill(x, slot_y, slot_w, slot_h, theme->card_bg);
        gfx_draw_rect_outline(x, slot_y, slot_w, slot_h, border_col);

        if (filled) {
            /* Draw solid bullet dot */
            int dot_size = 14;
            int dot_x = x + (slot_w - dot_size) / 2;
            int dot_y = slot_y + (slot_h - dot_size) / 2;
            gfx_draw_rect_fill(dot_x, dot_y, dot_size, dot_size, theme->accent);
        }
    }

    /* Instructions in footer */
    gfx_draw_rect_fill(0, 244, SCREEN_WIDTH, 1, theme->card_bg);
    if (app->file_header.flags & VAULT_FLAG_LR_ONLY) {
        font_draw_string_centered(252, "[L/R] Desbloqueo en Dock (Slider Cerrado)  [L+R] Borrar", 1, theme->text_secondary);
    } else {
        font_draw_string_centered(252, "[D-Pad / L / R / Botones]  [O] Borrar  [SELECT] Limpiar", 1, theme->text_secondary);
    }
}

/* =========================================================================
 * 2. Screen: TOTP_DOCK (Main Desktop Air-Gapped Authenticator)
 * ========================================================================= */
static void render_screen_totp_dock(AppState *app, uint64_t now_utc_sec, uint64_t now_utc_ms, uint64_t now_ticks_ms) {
    (void)now_ticks_ms;
    const theme_palette_t *theme = theme_get(app->theme_idx);
    gfx_clear(theme->bg);

    /* 1. HUD Top */
    char hud_left[32];
    if (app->vault.otp_count > 0) {
        snprintf(hud_left, sizeof(hud_left), "[OK] %02d/%02d",
                 app->current_account_idx + 1, app->vault.otp_count);
    } else {
        snprintf(hud_left, sizeof(hud_left), "[OK] VACIO");
    }
    render_hud_top(theme, hud_left, now_utc_sec);

    if (app->vault.otp_count == 0) {
        font_draw_string_centered(120, "NO HAY CUENTAS 2FA CONFIGURADAS", 1, theme->text_secondary);
        font_draw_string_centered(140, "Usa tools/vault_cli en tu PC para agregar cuentas", 1, theme->accent);
        return;
    }

    TotpAccount *acc = &app->vault.otp_accounts[app->current_account_idx];

    /* 2. Issuer & Account Label */
    font_draw_string(24, 36, acc->issuer, 2, theme->text_primary);

    char label_buf[64];
    snprintf(label_buf, sizeof(label_buf), "%s  ·  %s-%s (%us)",
             acc->label,
             (acc->type == OTP_TYPE_TOTP) ? "TOTP" : "HOTP",
             (acc->algo == OTP_ALGO_SHA256) ? "SHA256" : "SHA1",
             (unsigned int)acc->period);
    font_draw_string_truncated(24, 58, label_buf, 432, 1, theme->text_secondary);

    /* 3. Compute Code */
    uint32_t code = 0;
    uint32_t rem_sec = 0;
    float progress = 1.0f;

    if (acc->type == OTP_TYPE_TOTP) {
        uint64_t target_sec = now_utc_sec;
        if (app->preview_next_window) {
            target_sec += acc->period;
        }
        code = totp_generate(acc->secret, acc->secret_len, target_sec, acc->period, acc->digits, (otp_algo_t)acc->algo);
        rem_sec = totp_remaining_seconds(now_utc_sec, acc->period);
        progress = totp_progress_ratio(now_utc_ms, acc->period);
    } else {
        /* HOTP */
        code = hotp_generate(acc->secret, acc->secret_len, acc->counter, acc->digits, (otp_algo_t)acc->algo);
        rem_sec = 0;
        progress = 1.0f;
    }

    /* Next window banner if previewing */
    if (app->preview_next_window) {
        font_draw_string_centered(82, ">> SIGUIENTE VENTANA (+30s) <<", 1, theme->warning);
    }

    /* 4. Giant 7-Segment Display */
    char otp_fmt[16];
    otp_format_code(code, acc->digits, otp_fmt, sizeof(otp_fmt), 1);
    uint32_t otp_color = app->preview_next_window ? theme->warning : theme->accent;

    font_7seg_draw_otp(240, 96, otp_fmt, acc->digits, 78, otp_color, theme->ghost_7seg);

    /* 5. Progress Bar & Countdown Timer (for TOTP) */
    if (acc->type == OTP_TYPE_TOTP) {
        int bar_x = 32;
        int bar_y = 202;
        int bar_w = 416;
        int bar_h = 10;

        gfx_draw_rect_fill(bar_x, bar_y, bar_w, bar_h, theme->card_bg);
        gfx_draw_rect_outline(bar_x, bar_y, bar_w, bar_h, theme->ghost_7seg);

        uint32_t bar_color = theme->accent;
        if (rem_sec < 5) {
            bar_color = theme->critical;
        } else if (rem_sec < 10) {
            bar_color = theme->warning;
        }

        int fill_w = (int)(progress * (float)bar_w);
        if (fill_w > bar_w) fill_w = bar_w;
        if (fill_w > 0) {
            gfx_draw_rect_fill(bar_x, bar_y, fill_w, bar_h, bar_color);
        }

        char rem_buf[16];
        snprintf(rem_buf, sizeof(rem_buf), "%2us", (unsigned int)rem_sec);
        font_draw_string(SCREEN_WIDTH - 60, 218, rem_buf, 1, bar_color);
    }

    /* 6. Footer Controls */
    gfx_draw_rect_fill(0, 244, SCREEN_WIDTH, 1, theme->card_bg);
    font_draw_string(14, 252, "[L/R] Cuenta  [Hold R] +30s  [Hold L+R] Bloquear  [/\\\\] Cofre", 1, theme->text_secondary);
}

/* =========================================================================
 * 3. Screen: VAULT_LIST (Critical Secrets List)
 * ========================================================================= */
static void render_screen_vault_list(AppState *app) {
    const theme_palette_t *theme = theme_get(app->theme_idx);
    gfx_clear(theme->bg);

    char hud_left[32];
    snprintf(hud_left, sizeof(hud_left), "[COFRE] %02d/%02d",
             app->current_secret_idx + 1, app->vault.secret_count);
    render_hud_top(theme, hud_left, 0);

    if (app->vault.secret_count == 0) {
        font_draw_string_centered(120, "EL COFRE NO CONTIENE CLAVES", 1, theme->text_secondary);
        font_draw_string_centered(140, "[O] Volver al generador TOTP", 1, theme->accent);
        return;
    }

    int start_y = 36;
    int row_h = 48;
    int max_rows = 4;

    /* Simple scrolling window */
    int page_start = 0;
    if (app->current_secret_idx >= max_rows) {
        page_start = app->current_secret_idx - max_rows + 1;
    }

    for (int i = 0; i < max_rows && (page_start + i) < app->vault.secret_count; i++) {
        int idx = page_start + i;
        VaultSecret *sec = &app->vault.secrets[idx];
        int y = start_y + (i * row_h);

        int is_selected = (idx == app->current_secret_idx);
        uint32_t box_bg = is_selected ? theme->card_bg : theme->bg;
        uint32_t box_border = is_selected ? theme->accent : theme->ghost_7seg;

        gfx_draw_rect_fill(14, y, SCREEN_WIDTH - 28, row_h - 4, box_bg);
        gfx_draw_rect_outline(14, y, SCREEN_WIDTH - 28, row_h - 4, box_border);

        /* Category badge */
        const char *cat_str = "[KEY]";
        switch (sec->category) {
            case SEC_CAT_SSH_KEY:    cat_str = "[SSH]"; break;
            case SEC_CAT_GPG:        cat_str = "[GPG]"; break;
            case SEC_CAT_SEED_BIP39: cat_str = "[SEED]"; break;
            case SEC_CAT_PASSWORD:   cat_str = "[PASS]"; break;
            case SEC_CAT_RECOVERY:   cat_str = "[RECOV]"; break;
        }

        font_draw_string(24, y + 6, cat_str, 1, theme->accent);
        font_draw_string(76, y + 6, sec->title, 1, theme->text_primary);
        font_draw_string(24, y + 24, sec->subtitle, 1, theme->text_secondary);
        font_draw_string(SCREEN_WIDTH - 120, y + 14, "•••• ••••", 1, theme->ghost_7seg);
    }

    /* Footer */
    gfx_draw_rect_fill(0, 244, SCREEN_WIDTH, 1, theme->card_bg);
    font_draw_string(14, 252, "[^/v] Elegir  [X] Abrir Clave  [O] Volver  [Hold L+R] Bloquear", 1, theme->text_secondary);
}

/* =========================================================================
 * 4. Screen: VAULT_DETAIL (Hold-to-Reveal Protected View)
 * ========================================================================= */
static void render_screen_vault_detail(AppState *app) {
    const theme_palette_t *theme = theme_get(app->theme_idx);
    gfx_clear(theme->bg);

    VaultSecret *sec = &app->vault.secrets[app->current_secret_idx];

    /* HUD Top */
    render_hud_top(theme, "[DETALLE DE CLAVE]", 0);

    /* Title & Category */
    font_draw_string(20, 34, sec->title, 2, theme->text_primary);
    font_draw_string(20, 56, sec->subtitle, 1, theme->text_secondary);

    /* Protected Secret Box */
    int box_x = 20;
    int box_y = 74;
    int box_w = SCREEN_WIDTH - 40;
    int box_h = 130;

    gfx_draw_rect_fill(box_x, box_y, box_w, box_h, theme->card_bg);
    gfx_draw_rect_outline(box_x, box_y, box_w, box_h, app->reveal_secret ? theme->accent : theme->ghost_7seg);

    if (app->reveal_secret) {
        /* Render secret in clear */
        /* Word wrap simple renderer for multi-line secrets or seeds */
        char val_buf[256];
        strncpy(val_buf, sec->value, sizeof(val_buf) - 1);
        val_buf[sizeof(val_buf) - 1] = '\0';

        int line_y = box_y + 12;
        int max_chars_per_line = 50;
        char *line = val_buf;

        while (*line && line_y < box_y + box_h - 14) {
            char temp[64];
            size_t len = strlen(line);
            if (len > (size_t)max_chars_per_line) {
                strncpy(temp, line, max_chars_per_line);
                temp[max_chars_per_line] = '\0';
                font_draw_string(box_x + 12, line_y, temp, 1, theme->accent);
                line += max_chars_per_line;
            } else {
                font_draw_string(box_x + 12, line_y, line, 1, theme->accent);
                break;
            }
            line_y += 14;
        }
    } else {
        /* Masked state */
        font_draw_string_centered(box_y + 40, "••••••••  ••••••••  ••••••••  ••••••••", 1, theme->ghost_7seg);
        font_draw_string_centered(box_y + 70, "MANTEN PRESIONADO [X] O [R] PARA VER EN CLARO", 1, theme->warning);
    }

    /* Footer */
    gfx_draw_rect_fill(0, 244, SCREEN_WIDTH, 1, theme->card_bg);
    font_draw_string(14, 252, "[Hold X / R] Revelar Secreto   [O] Volver   [Hold L+R] Bloquear", 1, theme->text_secondary);
}

/* =========================================================================
 * 5. Screen: SETTINGS
 * ========================================================================= */
static void render_screen_settings(AppState *app, uint64_t now_utc_sec) {
    const theme_palette_t *theme = theme_get(app->theme_idx);
    gfx_clear(theme->bg);

    render_hud_top(theme, "[CONFIGURACION]", now_utc_sec);

    font_draw_string(24, 38, "AJUSTES DE DISPOSITIVO", 2, theme->text_primary);

    char drift_buf[64];
    snprintf(drift_buf, sizeof(drift_buf), "Compensacion Deriva RTC:  %+d segundos", app->vault.time_drift_sec);
    font_draw_string(24, 76, drift_buf, 1, theme->text_primary);

    char lock_buf[64];
    snprintf(lock_buf, sizeof(lock_buf), "Tiempo de Auto-Bloqueo:   %u minutos", app->vault.auto_lock_minutes);
    font_draw_string(24, 102, lock_buf, 1, theme->text_primary);

    char theme_buf[64];
    snprintf(theme_buf, sizeof(theme_buf), "Tema Visual Activo:       %s", theme->name);
    font_draw_string(24, 128, theme_buf, 1, theme->text_primary);

    font_draw_string_centered(180, "Para cambiar contrasena o agregar cuentas,", 1, theme->text_secondary);
    font_draw_string_centered(196, "usa la herramienta CLI tools/vault_cli en tu Mac.", 1, theme->accent);

    /* Footer */
    gfx_draw_rect_fill(0, 244, SCREEN_WIDTH, 1, theme->card_bg);
    font_draw_string(14, 252, "[O] Volver al Dock   [Hold L+R] Bloquear", 1, theme->text_secondary);
}

void screens_render(AppState *app, uint64_t now_utc_sec, uint64_t now_utc_ms, uint64_t now_ticks_ms) {
    if (!app) return;

    switch (app->screen) {
        case SCREEN_LOCKED:
            render_screen_locked(app, now_ticks_ms);
            break;
        case SCREEN_TOTP_DOCK:
            render_screen_totp_dock(app, now_utc_sec, now_utc_ms, now_ticks_ms);
            break;
        case SCREEN_VAULT_LIST:
            render_screen_vault_list(app);
            break;
        case SCREEN_VAULT_DETAIL:
            render_screen_vault_detail(app);
            break;
        case SCREEN_SETTINGS:
            render_screen_settings(app, now_utc_sec);
            break;
    }
}
