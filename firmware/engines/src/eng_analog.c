/* SPDX-License-Identifier: MIT
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* ANALOG: two band-limited oscillators (saw / square / tri / sine / PWM),
 * noise, drive and a trapezoidal state-variable filter: low-pass, or (1.5, TYPE) band-pass / high-pass.
 * WAVE SYNC and SUB (#104, appended: WAVE 0..4 play as they always did) are two more oscillator pairs (analog_osc_x):
 *   SYNC  OSC 2 a saw hard-synced to OSC 1 (a saw): it restarts with OSC 1's every cycle, band-limited at the restart
 *         (polyBLEP scaled by the step the restart makes). DTN (shown SYNC) sets OSC 2 above OSC 1, 3/16 semitone a
 *         step (127: +23.8 semitones), and the SHP routings sweep it (ENV DEST / LFO DEST SHP, the matrix's SHP: 3/16
 *         semitone a step of shape, as PWM takes them for its width): the sync sweep. MIX between the two as for
 *         every pair (OSC 2 alone at 127)
 *   SUB   OSC 1 a saw, OSC 2 a square an octave below it (detuned by DTN in cents as before): MIX (shown SUB) is the
 *         sub oscillator's level */
static const char *const N_ANALOG_WAVE[] = {"SAW", "SQR", "TRI", "SIN", "PWM", "SYNC", "SUB", "SAMPLE 1", "SAMPLE 2", "SAMPLE 3"};
enum { AW_SYNC = 5, AW_SUB = 6 };
static const param_desc_t ANALOG_SYNC = {"SYNC", F_INT, 0, 127, 10, 0, 0};   /* DTN's range and default */
static const param_desc_t ANALOG_SUB = {"SUB", F_PCT, 0, 127, 64, 0, 0};      /* MIX's */

/* the filter's TYPE (1.5, #104: track parameter P_FTYPE, EDIT > FILTER; ANALOG's 8 values are full): the SVF's
 * outputs (dsp.c tsvf_lp, written out here with the taps: Simper's trapezoidal state-variable filter), 0 LP the
 * low-pass as always (the same steps, bit for bit), 1 BP its band-pass v1, 2 HP in - k v1 - v2. At the cutoff the three peak alike (1/k, k the damping
 * RES sets: 2 at RES 0, ~0.15 at 127), so the soft knee after them and the level stay as LP's */
static inline int32_t analog_tap(const tsvf_t *c, int32_t k, int32_t in, int32_t *ic1, int32_t *ic2, uint32_t ft)
{
    int32_t v3 = in - *ic2;
    int32_t v1 = (c->a1 * *ic1 + c->a2 * v3) >> 13;
    int32_t v2 = *ic2 + ((c->a2 * *ic1 + c->a3 * v3) >> 13);
    *ic1 = clamp(2 * v1 - *ic1, -150000, 150000);
    *ic2 = clamp(2 * v2 - *ic2, -150000, 150000);
    if (!ft)
        return v2;                                    /* LP: tsvf_lp's (the same steps) */
    return ft == 1u ? v1 : in - ((k * v1) >> 12) - v2;
}
#define ANALOG_K(res) (8192 - (res) * 7600 / 127)     /* tsvf_coef's damping, Q12 */

static const param_desc_t *analog_desc(const track_t *t, uint32_t k)
{
    if (k == 1u && t->p[P_E0] == AW_SYNC)
        return &ANALOG_SYNC;
    if (k == 2u && t->p[P_E0] == AW_SUB)
        return &ANALOG_SUB;
    return 0;
}

static void analog_note_on(track_t *t, voice_t *v)
{
#ifdef DRIFT_SAMPLES
    drift_osc_phase[t-trk][v-t->v]=0;
    drift_wave_tag[t-trk][v-t->v]=(uint8_t)(t->p[P_E0]+1);
    if(t->p[P_E0]>=7){drift_sample_on(t,v);return;}
#endif
    (void)t;
    v->ph[1] = v->ph[0] + 0x40000000u;
    v->s[0] = v->s[1] = 0;                            /* filter */
    if (!v->s[2])
        v->s[2] = 0x1234567 + (int32_t)v->age;        /* noise state */
    v->s[3] = 0;                                      /* SYNC: no restart pending */
}

/* WAVE SYNC / SUB: OSC 1 a saw, OSC 2 the synced saw / the sub square; then noise, drive and the filter as
 * analog_render. Its own function: analog_render's loop stays what it was */
static __attribute__((noinline)) void analog_render_x(track_t *t, voice_t *v, int32_t *out, uint32_t n,
                                                      const vmod_t *m)
{
    const int16_t *p = t->p;
    uint32_t sub = p[P_E0] == AW_SUB, i;
    int32_t det = p[P_E1], mix = p[P_E2], noise = p[P_E3];
    int32_t cut = (p[P_E4] << 8) + m->cutoff + (p[P_E7] * (v->pitch16 - 60 * 16) >> 4);
    tsvf_t flt;
    int32_t drive = 32768 + p[P_E6] * 512;
    int32_t m2 = mix * 258, m1 = 32767 - m2;
    int32_t nz = noise * 200, drv = p[P_E6];
    uint32_t extra_inc=drift_osc_inc(t,m);
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1], inc1 = m->inc, inc2;
    int32_t ic1 = v->s[0], ic2 = v->s[1], nst = v->s[2];
    int32_t jl = v->s[3];                             /* SYNC: 1 + the step of a restart a sample ago (16 bits), 0 none */
    if (sub) {                                        /* an octave below OSC 1, DTN as OSC 2 always had it */
        inc2 = (det ? cents_inc(m->pitch16, det, 0) : inc1) >> 1;
    } else {                                          /* DTN and the shape above OSC 1, 3/16 semitone a step */
        inc2 = pitch_inc(clamp(m->pitch16 + det * 3 + (((m->shape - (64 << 8)) * 3) >> 8), 0, 2047));
        if (m->fine)
            inc2 += (uint32_t)((int32_t)(inc2 >> 12) * m->fine);
    }
    uint32_t ft = (uint32_t)p[P_FTYPE];
    int32_t fk = ANALOG_K(p[P_E5]);
    tsvf_coef(&flt, cut, p[P_E5]);
    for (i = 0; i < n; i++) {
        int32_t a = osc_saw(ph0, inc1), b, s;
        if (sub) {
            b = osc_pulse(ph1, inc2, 0x80000000u);
        } else {
            int32_t bl = blep(ph1, inc2);
            if (jl) {                                 /* just restarted: the step was jl - 1, not a whole cycle */
                bl = (bl * ((jl - 1) >> 1)) >> 15;
                jl = 0;
            }
            b = (int32_t)(ph1 >> 16) - 32768 - bl;
            if (ph0 > 0xFFFFFFFFu - inc1)             /* OSC 1 restarts it before the next sample: round that step */
                b -= (blep(ph0, inc1) * (int32_t)(ph1 >> 17)) >> 15;
        }
        ph0 += inc1;
        ph1 += inc2;
        if (!sub && ph0 < inc1) {                     /* OSC 1 wrapped: OSC 2 restarts at the same point */
            uint32_t f = ph0 / ((inc1 >> 16) | 1u);   /* the part of a sample since, 0..65535 */
            uint32_t at = ph1 - f * (inc2 >> 16);     /* OSC 2 at the wrap */
            jl = (int32_t)(at >> 16) + 1;
            ph1 = f * (inc2 >> 16);
        }
        s = mulq15(a, m1) + mulq15(b, m2);
        if (nz)
            s += mulq15((int32_t)(noise32(&nst) >> 17) - 16384, nz);
        s=drift_osc_mix(t,v,s,extra_inc);
        if (drv)
            s = softclip(((s >> 2) * (drive >> 2)) >> 11);
        s = soft_knee(analog_tap(&flt, fk, s >> 1, &ic1, &ic2, ft), 16000) << 1;
        out[i] += voice_amp(s, m, i) << 1;
    }
    v->ph[0] = ph0;
    v->ph[1] = ph1;
    v->s[0] = ic1;
    v->s[1] = ic2;
    v->s[2] = nst;
    v->s[3] = jl;
}

/* WAVE 0..4 through the filter TYPE ft: analog_render's loop with ft 0 (a constant: LP, the code as it always was),
 * analog_render_f's with BP / HP (its own copy: the LP loop gains nothing) */
static inline __attribute__((always_inline)) void analog_osc(track_t *t, voice_t *v, int32_t *out, uint32_t n,
                                                             const vmod_t *m, uint32_t ft)
{
    const int16_t *p = t->p;
    uint32_t wave = (uint32_t)p[P_E0], i;
    int32_t det = p[P_E1], mix = p[P_E2], noise = p[P_E3];
    int32_t cut = (p[P_E4] << 8) + m->cutoff + (p[P_E7] * (v->pitch16 - 60 * 16) >> 4);
    tsvf_t flt;
    int32_t drive = 32768 + p[P_E6] * 512;                       /* 1x .. 3x */
    uint32_t inc1 = m->inc;
    uint32_t inc2 = det ? cents_inc(m->pitch16, det, 0) : inc1;      /* DTN in cents */
    uint32_t pw = 0x80000000u + (uint32_t)((m->shape - (64 << 8)) << 15);
    int32_t m2 = mix * 258, m1 = 32767 - m2;                    /* osc mix Q15 */
    int32_t nz = noise * 200, drv = p[P_E6];
    uint32_t extra_inc=drift_osc_inc(t,m);
    uint32_t ph0 = v->ph[0], ph1 = v->ph[1];                  /* state in locals: out[] may alias v->s[] */
    int32_t ic1 = v->s[0], ic2 = v->s[1], nst = v->s[2];
    int32_t fk = ft ? ANALOG_K(p[P_E5]) : 0;
    tsvf_coef(&flt, cut, p[P_E5]);
    for (i = 0; i < n; i++) {
        int32_t a, b, s;
        switch (wave) {
        case 1:
            a = osc_pulse(ph0, inc1, 0x80000000u);
            b = osc_pulse(ph1, inc2, 0x80000000u);
            break;
        case 2:
            a = osc_tri(ph0);
            b = osc_tri(ph1);
            break;
        case 3:
            a = osc_sine(ph0);
            b = osc_sine(ph1);
            break;
        case 4:
            a = osc_pulse(ph0, inc1, pw);
            b = osc_pulse(ph1, inc2, pw);
            break;
        default:
            a = osc_saw(ph0, inc1);
            b = osc_saw(ph1, inc2);
            break;
        }
        ph0 += inc1;
        ph1 += inc2;
        s = mulq15(a, m1) + mulq15(b, m2);
        if (nz)
            s += mulq15((int32_t)(noise32(&nst) >> 17) - 16384, nz);
        s=drift_osc_mix(t,v,s,extra_inc);
        if (drv)
            s = softclip(((s >> 2) * (drive >> 2)) >> 11);   /* pre-shifts: drive is up to 3x, no overflow */
        {   /* filter, linear up to half scale, then a soft knee (only resonance peaks saturate); soft_knee()
             * written out: the host compiler makes this loop 1 % slower with the call */
            int32_t y = ft ? analog_tap(&flt, fk, s >> 1, &ic1, &ic2, ft) : tsvf_lp(&flt, s >> 1, &ic1, &ic2);
            int32_t a = y < 0 ? -y : y;
            if (a > 16000) {
                a = 16000 + (softclip((a - 16000) * 2) >> 1);
                y = y < 0 ? -a : a;
            }
            s = y << 1;
        }
        out[i] += voice_amp(s, m, i) << 1;
    }
    v->ph[0] = ph0;
    v->ph[1] = ph1;
    v->s[0] = ic1;
    v->s[1] = ic2;
    v->s[2] = nst;
}
static __attribute__((noinline)) void analog_render_f(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
    analog_osc(t, v, out, n, m, (uint32_t)t->p[P_FTYPE]);
}

static void analog_render(track_t *t, voice_t *v, int32_t *out, uint32_t n, const vmod_t *m)
{
#ifdef DRIFT_SAMPLES
    if(drift_wave_tag[t-trk][v-t->v]!=(uint8_t)(t->p[P_E0]+1)){v->active=0;return;}
    if(t->p[P_E0]>=7){drift_sample_render(t,v,out,n,m);return;}
#endif
    if (t->p[P_E0] >= AW_SYNC)
        analog_render_x(t, v, out, n, m);
    else if (t->p[P_FTYPE])
        analog_render_f(t, v, out, n, m);
    else
        analog_osc(t, v, out, n, m, 0);
}

static const preset_t ANALOG_PRESETS[] = { /* DRIFT ambient bank, 2026 */
    {"STILL WATER", {0, 14, 64, 0, 50, 9, 0, 32}, {84, 90, 110, 105}, 8, 0, FX(0, 45, 24, 86), PAT(0)},
    {"SOFT HORIZON", {4, 8, 50, 0, 66, 16, 0, 36}, {75, 90, 115, 100}, 12, 0, FX(0, 52, 20, 78), PAT(0)},
    {"GLASS TIDE", {3, 7, 64, 0, 115, 0, 0, 20}, {3, 90, 20, 95}, 0, 0, FX(0, 22, 58, 74), PAT(0)},
    {"DUST PLUCK", {0, 8, 50, 0, 32, 30, 0, 52}, {0, 82, 0, 76}, 48, 0, FX(0, 18, 50, 66), PAT(0)},
    {"MOON KEYS", {2, 5, 40, 0, 88, 6, 0, 32}, {4, 85, 35, 86}, 12, 0, FX(0, 28, 40, 65), PAT(0)},
    {"SLOW BLOOM", {0, 23, 64, 2, 52, 20, 0, 24}, {110, 100, 118, 118}, 18, 0, FX(0, 60, 22, 90), PAT(0)},
    {"PAPER STRINGS", {0, 20, 60, 0, 68, 10, 0, 32}, {68, 85, 110, 95}, 6, 0, FX(0, 56, 20, 74), PAT(0)},
    {"LOW CLOUD", {6, 4, 50, 0, 35, 10, 0, 20}, {60, 100, 100, 100}, 8, 0, FX(0, 30, 16, 72), PAT(0)},
    {"SINE DROPS", {3, 0, 0, 0, 127, 0, 0, 0}, {0, 75, 0, 85}, 0, 0, FX(0, 9, 64, 78), PAT(0)},
    {"WARM CURRENT", {1, 12, 60, 0, 48, 18, 0, 32}, {75, 95, 100, 103}, 10, 0, FX(0, 44, 24, 82), PAT(0)},
    {"FELT PLUCK", {2, 3, 40, 0, 50, 15, 0, 40}, {2, 70, 0, 65}, 24, 0, FX(0, 20, 48, 68), PAT(0)},
    {"NIGHT SIGNAL", {0, 8, 50, 0, 66, 24, 5, 48}, {10, 78, 75, 80}, 25, 0, FX(0, 24, 54, 62), PAT(0)},
    {"AIRFIELD", {4, 16, 44, 8, 42, 28, 0, 20}, {100, 100, 105, 115}, 16, 0, FX(0, 50, 16, 92), PAT(0)},
    {"EMBER", {6, 5, 32, 0, 45, 20, 8, 48}, {8, 85, 60, 80}, 25, 0, FX(0, 20, 32, 65), PAT(0)},
    {"SOFT BRASS", {0, 10, 64, 0, 48, 18, 5, 48}, {32, 78, 85, 78}, 32, 0, FX(0, 28, 24, 64), PAT(0)},
    {"INIT", {0, 0, 0, 0, 90, 0, 0, 32}, {5, 65, 90, 65}, 0, 0, FX(0, 0, 0, 30), PAT(0)},
    {"WARM SUB", {6, 0, 104, 0, 36, 4, 18, 12}, {10, 65, 112, 48}, 5, 1, FX(0, 0, 0, 4), PAT(0)},
    {"TWIN HORIZON", {0, 9, 64, 0, 58, 12, 0, 24}, {85, 90, 112, 108}, 8, 0, FX(0, 30, 22, 76), PAT(0)},
    {"FIFTH LIGHT", {2, 3, 45, 0, 82, 8, 0, 24}, {12, 82, 70, 92}, 12, 0, FX(0, 22, 40, 65), PAT(0)},
    {"DEEP PAIR", {3, 0, 0, 0, 48, 4, 10, 12}, {10, 70, 112, 54}, 4, 1, FX(0, 0, 0, 8), PAT(0)},
    {"AIR & SALT", {0, 4, 52, 0, 71, 22, 3, 24}, {69, 103, 88, 110}, 14, 0, FX(0, 18, 16, 64), PAT(0)},
};

static const engine_t ENG_ANALOG = {
    .name = "ANALOG",
    .page_title = {"OSC", "FLT"},
    .edit = {
        {"WAVE", F_ENUM, 0, 9, 0, N_ANALOG_WAVE, 0},
        {"DTN", F_INT, 0, 127, 10, 0, "ct"},
        {"MIX", F_PCT, 0, 127, 64, 0, 0},
        {"NOIS", F_PCT, 0, 127, 0, 0, 0},
        {"CUT", F_CUTOFF, 0, 127, 90, 0, 0},
        {"RES", F_PCT, 0, 127, 30, 0, 0},
        {"DRV", F_PCT, 0, 127, 0, 0, 0},
        {"KTR", F_PCT, 0, 127, 64, 0, 0},
    },
    .presets = ANALOG_PRESETS,
    .npresets = NELEM(ANALOG_PRESETS),
    .note_on = analog_note_on,
    .render = analog_render,
    .desc = analog_desc,
    .knob = {P_E4, P_E5, P_ATK, P_REL},
    .keep = 0x03,                /* the filter */
};
