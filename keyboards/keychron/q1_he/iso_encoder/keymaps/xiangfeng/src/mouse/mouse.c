/* mouse.c — the mouse-animation engine (see include/mouse.h).
 *
 * Auto shape movers, the full-screen DVD bounce, and the "draw with the mouse"
 * glyph tracer (also driven by the pinyin IME). keymap.c calls the semantic
 * seams (mouse_shape_toggle / mouse_dvd_toggle / mouse_stop / mouse_task);
 * the IME calls draw_begin and shares carriage_x / draw_cx / draw_cy. */
#include "quantum.h"
#include "digitizer.h" // absolute pointer, for the full-screen DVD bounce
#include "include/mouse.h"
#include <math.h>

// Traversal speed follows the mouse-accel level (F1..F5).
extern uint8_t mousekey_get_offset(void);

#define SHP_INTERVAL 12 // ms per step
static uint8_t  shp_active = SHP_OFF;
static uint16_t shp_timer  = 0;
static float    shp_theta  = 0;
static float    shp_ax = 0, shp_ay = 0; // fractional movement accumulators
static bool     draw_on = false;        // "draw my name" (祥沣) active

// Full-screen DVD bounce (layer 1 · F9). Uses the absolute digitizer report:
// x/y are screen fractions [0,1], so it bounces off the REAL screen edges at any
// resolution (the firmware can't read the pixel size, but 0..1 spans the screen).
static bool     dvd_on = false;
static uint16_t dvd_timer = 0;
static float    dvd_px = 0.10f, dvd_py = 0.10f;   // position, screen fraction
static float    dvd_vx = 0.0060f, dvd_vy = 0.0043f; // velocity per tick

static void shp_poly_v(uint8_t n, float r, uint8_t k, float *vx, float *vy) {
    float a = -1.5708f + (6.28318f / n) * k; // top vertex at start (0,0), centre (0,r)
    *vx = r * cosf(a);
    *vy = r + r * sinf(a);
}

static void shp_pos(uint8_t s, float th, float *ox, float *oy) {
    if (s == SHP_INF) { // vertical figure-8
        *ox = 30.0f * sinf(2.0f * th);
        *oy = 95.0f * sinf(th);
        return;
    }
    if (s == SHP_INFH) { // horizontal infinity
        *ox = 95.0f * sinf(th);
        *oy = 30.0f * sinf(2.0f * th);
        return;
    }
    if (s == SHP_CIRCLE) { // circle looping downward from start
        *ox = 55.0f * sinf(th);
        *oy = 55.0f * (1.0f - cosf(th));
        return;
    }
    if (s == SHP_WAVE) {
        *ox = 90.0f * sinf(th);
        *oy = 45.0f * sinf(4.0f * th);
        return;
    }
    if (s == SHP_SPIRAL) { // grows outward over one loop
        float r = 60.0f * th / 6.28318f;
        float a = 3.0f * th;
        *ox = r * cosf(a);
        *oy = r * sinf(a);
        return;
    }
    if (s == SHP_STAR) { // 5-point star (10 alternating vertices)
        float   seg = th / (6.28318f / 10);
        uint8_t k   = (uint8_t)seg;
        float   f   = seg - (float)k;
        float   a0 = -1.5708f + 0.628318f * k, a1 = -1.5708f + 0.628318f * (k + 1);
        float   r0 = (k & 1) ? 28.0f : 70.0f, r1 = ((k + 1) & 1) ? 28.0f : 70.0f;
        float   x0 = r0 * cosf(a0), y0 = r0 * sinf(a0), x1 = r1 * cosf(a1), y1 = r1 * sinf(a1);
        *ox = x0 + (x1 - x0) * f;
        *oy = y0 + (y1 - y0) * f;
        return;
    }
    if (s == SHP_HEART) {
        float sx = sinf(th);
        *ox = 5.0f * (16.0f * sx * sx * sx);
        *oy = -5.0f * (13.0f * cosf(th) - 5.0f * cosf(2 * th) - 2.0f * cosf(3 * th) - cosf(4 * th));
        return;
    }
    if (s == SHP_ROSE) { // 4-petal rose
        float r = 60.0f * cosf(2.0f * th);
        *ox = r * cosf(th);
        *oy = r * sinf(th);
        return;
    }
    if (s == SHP_LISS) {
        *ox = 80.0f * sinf(3.0f * th);
        *oy = 80.0f * sinf(2.0f * th);
        return;
    }
    if (s == SHP_SPIRO) {
        *ox = 42.0f * cosf(th) + 25.0f * cosf(7.0f * th);
        *oy = 42.0f * sinf(th) - 25.0f * sinf(7.0f * th);
        return;
    }
    if (s == SHP_SQUARE) { // axis-aligned square, corner at start
        const float S = 95.0f;
        const float X[4] = {0, S, S, 0}, Y[4] = {0, 0, S, S};
        float       seg = th / (6.28318f / 4);
        uint8_t     k   = (uint8_t)seg;
        float       f   = seg - (float)k;
        *ox = X[k & 3] + (X[(k + 1) & 3] - X[k & 3]) * f;
        *oy = Y[k & 3] + (Y[(k + 1) & 3] - Y[k & 3]) * f;
        return;
    }
    // regular polygon: triangle / pentagon / hexagon
    uint8_t n   = (s == SHP_TRI) ? 3 : (s == SHP_PENTA) ? 5 : 6;
    float   r   = (s == SHP_TRI) ? 70.0f : 58.0f;
    float   seg = th / (6.28318f / n);
    uint8_t k   = (uint8_t)seg;
    float   f   = seg - (float)k;
    float   x0, y0, x1, y1;
    shp_poly_v(n, r, k % n, &x0, &y0);
    shp_poly_v(n, r, (k + 1) % n, &x1, &y1);
    *ox = x0 + (x1 - x0) * f;
    *oy = y0 + (y1 - y0) * f;
}

static void dvd_stop(void) {
    if (dvd_on) {
        dvd_on = false;
        digitizer_in_range_off(); // lift the absolute pointer
    }
}

static void shp_start(uint8_t s) {
    shp_active = s;
    shp_theta  = 0;
    shp_ax = shp_ay = 0;
    shp_timer = timer_read();
    draw_on   = false; // shapes and name-drawing are mutually exclusive
    dvd_stop();        // ...and the full-screen bounce
}

// One shape per key: tap starts it, tap the same key again stops it.
void mouse_shape_toggle(uint8_t s) {
    if (shp_active == s)
        shp_active = SHP_OFF;
    else
        shp_start(s);
}

// Full-screen DVD bounce toggle (layer 1 · F9).
void mouse_dvd_toggle(void) {
    if (dvd_on) {
        dvd_stop();
    } else {
        shp_active = SHP_OFF; // one auto-mover at a time
        draw_on    = false;
        dvd_on     = true;
        dvd_timer  = timer_read();
        digitizer_in_range_on();
    }
}

// Stop any running mouse animation (shape / DVD / draw).
void mouse_stop(void) {
    shp_active = SHP_OFF;
    dvd_stop();
    if (draw_on) { draw_on = false; report_mouse_t rel = {0}; host_mouse_send(&rel); } // release the button mid-stroke
}

// --- Mouse character-drawing engine (drives the pinyin IME "confirm") ---
// Traces a glyph's stroke polylines with the mouse, holding the left button
// during a stroke and lifting it between strokes, so it draws in a paint app.
// The glyph is supplied by the IME (dr_* pointers) via draw_begin().
static uint8_t  draw_s = 0, draw_p = 0, draw_phase = 0; // phase 0=pen-up move, 1=drawing, 2=final release
static uint16_t draw_base = 0, draw_timer = 0;
float           draw_cx = 0, draw_cy = 0;         // continuous virtual pen pos (shared with the IME, see include/mouse.h)
static float    draw_fx = 0, draw_fy = 0;
static const int16_t *dr_x = NULL, *dr_y = NULL;
static const uint8_t *dr_len = NULL;
static uint8_t        dr_ns = 0;
// Sentence carriage: each dictionary glyph is centred on its own origin, so we
// place that origin at carriage_x and step it right after every confirmed
// character. draw_cx/cy stay continuous so the pen just travels to the next cell.
float        carriage_x = 0;   // x-origin for the next character (shared with the IME, see include/mouse.h)
static float draw_home_x = 0;  // carriage_x captured when this character started
#define CHAR_ADVANCE 130.0f    // horizontal step per character (glyphs are ~110 wide)

void draw_begin(const int16_t *x, const int16_t *y, const uint8_t *len, uint8_t ns) {
    shp_active = SHP_OFF; // don't run a shape or the bounce at the same time
    dvd_stop();
    dr_x = x; dr_y = y; dr_len = len; dr_ns = ns;
    draw_on = true;
    draw_s = draw_p = draw_phase = 0;
    draw_base = 0;
    draw_home_x = carriage_x; // draw this glyph at the current carriage position
    draw_fx = draw_fy = 0;    // keep draw_cx/cy continuous across characters
    draw_timer = timer_read();
}

void mouse_task(void) {
    if (dvd_on && timer_elapsed(dvd_timer) > SHP_INTERVAL) { // full-screen bounce
        dvd_timer   = timer_read();
        uint8_t off = mousekey_get_offset();
        if (off == 0) off = 1;
        float spd = 0.5f + off * 0.05f; // pace follows the mouse-accel level (F1..F5)
        dvd_px += dvd_vx * spd;
        dvd_py += dvd_vy * spd;
        if (dvd_px <= 0.0f)      { dvd_px = 0.0f; dvd_vx = -dvd_vx; }
        else if (dvd_px >= 1.0f) { dvd_px = 1.0f; dvd_vx = -dvd_vx; }
        if (dvd_py <= 0.0f)      { dvd_py = 0.0f; dvd_vy = -dvd_vy; }
        else if (dvd_py >= 1.0f) { dvd_py = 1.0f; dvd_vy = -dvd_vy; }
        digitizer_set_position(dvd_px, dvd_py); // absolute -> bounces off real edges
    }

    if (shp_active && timer_elapsed(shp_timer) > SHP_INTERVAL) {
        shp_timer   = timer_read();
        uint8_t off = mousekey_get_offset();
        if (off == 0) off = 1;
        float dth = 0.02f + off * 0.004f; // step size scales with accel level
        float t0 = shp_theta, t1 = t0 + dth;
        float x0, y0, x1, y1;
        shp_pos(shp_active, t0, &x0, &y0);
        shp_pos(shp_active, t1, &x1, &y1); // sinf/mod are periodic, so t1 > 2pi is fine
        shp_ax += (x1 - x0);
        shp_ay += (y1 - y0);
        shp_theta = (t1 > 6.28318f) ? t1 - 6.28318f : t1;
        int8_t mx = (int8_t)shp_ax, my = (int8_t)shp_ay;
        shp_ax -= mx;
        shp_ay -= my;
        if (mx || my) {
            report_mouse_t rep = {0};
            rep.x = mx;
            rep.y = my;
            host_mouse_send(&rep);
        }
    }

    if (draw_on && timer_elapsed(draw_timer) > SHP_INTERVAL) {
        draw_timer = timer_read();
        if (draw_phase == 2) { // finished: release the button
            report_mouse_t rel = {0};
            host_mouse_send(&rel);
            draw_on = false;
        } else {
            uint8_t off = mousekey_get_offset();
            if (off == 0) off = 1;
            float   step = 2.0f + off * 0.4f;
            uint8_t btn  = (draw_phase == 1) ? 0x01 : 0x00; // left button while drawing a stroke
            uint8_t len  = dr_len[draw_s];
            float   tx = draw_home_x + dr_x[draw_base + draw_p], ty = dr_y[draw_base + draw_p];
            float   ex = tx - draw_cx, ey = ty - draw_cy;
            float   dist = sqrtf(ex * ex + ey * ey);
            float   mvx, mvy;
            if (dist <= step + 0.01f) { // reached this point
                mvx = ex; mvy = ey;
                draw_cx = tx; draw_cy = ty;
                if (draw_phase == 0) { // at stroke start -> pen down
                    draw_phase = 1;
                    draw_p     = 1;
                } else {
                    draw_p++;
                    if (draw_p >= len) { // stroke finished -> lift, next stroke
                        draw_phase = 0;
                        draw_base += len;
                        draw_s++;
                        draw_p = 0;
                        if (draw_s >= dr_ns) { draw_phase = 2; carriage_x += CHAR_ADVANCE; } // done -> advance to next cell
                    }
                }
            } else {
                mvx = ex / dist * step;
                mvy = ey / dist * step;
                draw_cx += mvx;
                draw_cy += mvy;
            }
            draw_fx += mvx;
            draw_fy += mvy;
            int8_t rx = (int8_t)draw_fx, ry = (int8_t)draw_fy;
            draw_fx -= rx;
            draw_fy -= ry;
            report_mouse_t rep = {0};
            rep.x       = rx;
            rep.y       = ry;
            rep.buttons = btn;
            host_mouse_send(&rep);
        }
    }
}
