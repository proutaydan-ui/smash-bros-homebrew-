// Saute-Saute Arena - version 3DS (libctru, sans bibliotheque supplementaire)
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define W 800
#define H 450
#define G 0.5f

typedef struct { float x, y, w, h, vx, vy; int inv, cd; u32 c; } E;
typedef struct { float x, y, w, h; } P;

static int state = 0, level = 1, hits = 0, lives = 3, ne = 1;
static E p, es[3];
static P plat, tops[2];
static u8 *fb;
static int left, right;

static float rnd(void) { return rand() / (float)RAND_MAX; }

// dessine un rectangle (coordonnees du jeu 800x450 -> ecran 400x240)
static void rect(float x, float y, float w, float h, u32 c) {
    int x0 = (int)(x / 2), y0 = (int)(y / 2) + 7;
    int x1 = (int)((x + w) / 2), y1 = (int)((y + h) / 2) + 7;
    if (x1 <= x0) x1 = x0 + 1;
    if (y1 <= y0) y1 = y0 + 1;
    if (x0 < 0) x0 = 0; if (y0 < 0) y0 = 0;
    if (x1 > 400) x1 = 400; if (y1 > 240) y1 = 240;
    for (int i = x0; i < x1; i++)
        for (int j = y0; j < y1; j++) {
            u8 *q = fb + (i * 240 + (239 - j)) * 3;
            q[0] = c & 0xFF; q[1] = (c >> 8) & 0xFF; q[2] = c >> 16;
        }
}

static E mk(float x, u32 c) { E e = {x, 100, 30, 30, 0, 0, 0, 0, c}; return e; }

static void setup(void) {
    float pw = fmaxf(180, 520 - (level - 1) * 25);
    plat = (P){(W - pw) / 2, 300, pw, 18};
    tops[0] = (P){plat.x + 15, 175, 110, 14};
    tops[1] = (P){plat.x + plat.w - 125, 175, 110, 14};
    p = mk(W / 2 - 15, 0xff5a5f);
    ne = level >= 20 ? 3 : level >= 7 ? 2 : 1;
    for (int i = 0; i < ne; i++) es[i] = mk(W / 2 + 60 + i * 80, i ? 0x8e44ad : 0x2c3e50);
    hits = 0; lives = 3;
}

static void respawn(E *e) { e->x = W / 2 - 15; e->y = -40; e->vx = 0; e->vy = 0; e->inv = 90; }

static void land(E *e, P *q, float py) {
    if (e->vy >= 0 && py <= q->y + 4 && e->y + e->h >= q->y &&
        e->x + e->w > q->x && e->x < q->x + q->w) { e->y = q->y - e->h; e->vy = 0; }
}

static void physics(E *e) {
    float py = e->y + e->h;
    e->vy += G; e->vx *= 0.9f; e->x += e->vx; e->y += e->vy;
    if (e->inv > 0) e->inv--;
    if (e->cd > 0) e->cd--;
    land(e, &plat, py); land(e, &tops[0], py); land(e, &tops[1], py);
    if (e->x < 8) { e->x = 8; e->vx = fabsf(e->vx) * 0.6f; }
    if (e->x + e->w > W - 8) { e->x = W - 8 - e->w; e->vx = -fabsf(e->vx) * 0.6f; }
    if (e->y < 8) { e->y = 8; if (e->vy < 0) e->vy = 0; }
}

static void ai(E *e) {
    float s = fminf(1.0f + level * 0.18f, 4.5f), jr = fminf(0.012f + level * 0.007f, 0.12f);
    int off = e->y > plat.y + 10 || e->x + e->w < plat.x || e->x > plat.x + plat.w;
    float tx = off ? W / 2 : p.x;
    e->vx += (tx > e->x ? 1 : -1) * s * 0.25f;
    if (e->vx > s * 2.4f) e->vx = s * 2.4f;
    if (e->vx < -s * 2.4f) e->vx = -s * 2.4f;
    if (off && e->y > plat.y - 20 && e->cd == 0 && e->vy > 0) { e->vy = -10; e->cd = 14; }
    else if (e->cd == 0 && (rnd() < jr ||
             (p.y < e->y - 30 && fabsf(p.x - e->x) < 60 && rnd() < fminf(0.05f + level * 0.01f, 0.3f)))) {
        e->vy = -9 - fminf(level * 0.15f, 3); e->cd = 10;
    }
}

static int interact(E *a, E *b, int playerIsA) {
    if (a->x < b->x + b->w && a->x + a->w > b->x && a->y < b->y + b->h && a->y + a->h > b->y) {
        if (a->vy > 0 && a->y + a->h - b->y < a->vy + 14 && b->inv == 0 && a->inv == 0) {
            a->vy = -11; b->vy = -5; b->vx = (b->x > a->x ? 1 : -1) * 7; b->inv = 70;
            if (playerIsA) { hits++; if (hits >= 3) state = 2; }
            else { lives--; b->inv = 100; if (lives <= 0) state = 3; }
            return 1;
        }
    }
    return 0;
}

static void update(void) {
    if (left) p.vx -= 0.9f;
    if (right) p.vx += 0.9f;
    if (p.vx > 6) p.vx = 6;
    if (p.vx < -6) p.vx = -6;
    physics(&p);
    for (int i = 0; i < ne; i++) { ai(&es[i]); physics(&es[i]); }
    if (p.y > H - 20) { lives--; respawn(&p); if (lives <= 0) state = 3; }
    for (int i = 0; i < ne && state == 1; i++) {
        E *e = &es[i];
        if (e->y > H - 20) respawn(e);
        if (!interact(&p, e, 1)) interact(e, &p, 0);
        if (fabsf(p.x - e->x) < 28 && fabsf(p.y - e->y) < 28) {
            float d = p.x < e->x ? -1 : 1; p.vx += d * 1.5f; e->vx -= d * 1.5f;
        }
    }
}

static void drawE(E *e) {
    if (e->inv > 0 && (e->inv / 5) % 2) return;
    rect(e->x, e->y, e->w, e->h, e->c);
    rect(e->x + (e->vx >= 0 ? 16 : 4), e->y + 8, 8, 8, 0xffffff);
}

static void draw(void) {
    rect(0, 0, W, H, 0x5ec8ff);                       // ciel
    rect(650, 50, 80, 80, 0xffe066);                  // soleil
    rect(120, 220, 120, 230, 0x7aa6c9);               // montagnes
    rect(420, 190, 160, 260, 0x5b8db3);
    rect(0, H - 20, W, 20, 0xe8441a);                 // lave
    rect(plat.x, plat.y, plat.w, plat.h, 0x6b4a2b);
    rect(plat.x, plat.y, plat.w, 6, 0x3fae49);
    for (int i = 0; i < 2; i++) {
        rect(tops[i].x, tops[i].y, tops[i].w, tops[i].h, 0x6b4a2b);
        rect(tops[i].x, tops[i].y, tops[i].w, 5, 0x3fae49);
    }
    drawE(&p);
    for (int i = 0; i < ne; i++) drawE(&es[i]);
    rect(0, 0, 8, H, 0x2d2d44); rect(W - 8, 0, 8, H, 0x2d2d44); rect(0, 0, W, 8, 0x2d2d44);
}

static void hud(void) {
    printf("\x1b[1;1H\x1b[2J");
    printf("  SAUTE-SAUTE ARENA\n\n");
    printf("  Niveau : %d\n  Vies : %d\n  Ecrasements : %d/3\n\n", level, lives, hits);
    if (state == 0) printf("  Appuie sur A pour jouer\n");
    if (state == 2) printf("  Niveau reussi ! A = suivant\n");
    if (state == 3) printf("  GAME OVER (niveau %d)\n  A = rejouer\n", level);
    printf("\n  Croix : bouger\n  A / Haut : sauter\n  START : quitter\n");
}

int main(void) {
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);
    gfxSetDoubleBuffering(GFX_TOP, true);
    setup();
    int last = -1;
    while (aptMainLoop()) {
        hidScanInput();
        u32 d = hidKeysDown(), h = hidKeysHeld();
        if (d & KEY_START) break;
        left = h & KEY_LEFT; right = h & KEY_RIGHT;
        if (state == 1) {
            if (d & (KEY_A | KEY_UP | KEY_B)) p.vy = -10;
            update();
        } else if (d & KEY_A) {
            if (state == 2) level++; else level = 1;
            setup(); state = 1;
        }
        fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
        draw();
        int sig = state * 1000 + level * 10 + lives + hits * 100000;
        if (sig != last) { hud(); last = sig; }
        gfxFlushBuffers(); gfxSwapBuffers(); gspWaitForVBlank();
    }
    gfxExit();
    return 0;
}
