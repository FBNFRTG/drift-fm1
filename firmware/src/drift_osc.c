/* SPDX-License-Identifier: GPL-3.0-only
 * Extra independently tuned oscillator, summed before the common filter/ADSR.
 * Per-voice phase; all state is owned by the track's audio rendering core. */
static uint32_t drift_osc_phase[NPART][NVOICE];
enum { D_LFO_VOLUME, D_LFO_DELAY, D_LFO_OSC2_PITCH, D_LFO_OSC2_DETUNE };
/* Keep saved patch values unchanged: derive modulation once per audio block. */
static int32_t drift_lfo_amount(const track_t *t){
 return mulq15(t->lfo_val,t->lfo_fade)*t->p[P_LD_AMP]/127;
}
static int32_t drift_delay_send(const track_t *t){
 int32_t v=t->p[P_DLY];
 if(t->p[P_DDEST]==D_LFO_DELAY)v+=drift_lfo_amount(t)*127/32768;
 return clamp(v,0,127)*258;
}
static uint32_t drift_osc_inc(const track_t *t,const vmod_t *m){
 int32_t cents=0,amount=drift_lfo_amount(t);
 if(t->p[P_DDEST]==D_LFO_OSC2_PITCH)cents=amount*1200/32768;
 if(t->p[P_DDEST]==D_LFO_OSC2_DETUNE)cents=amount*50/32768;
 return cents_inc(clamp(m->pitch16+(t->p[P_D2T]-64)*16,0,2047),cents,m->fine);
}
static int32_t drift_osc_mix(track_t *t,voice_t *v,int32_t base,uint32_t inc){
 int32_t a=base*t->p[P_DLV1]/127;
 if(!t->p[P_D2W] || !t->p[P_D2LV])return a;
 uint32_t *ph=&drift_osc_phase[t-trk][v-t->v],p=*ph;*ph+=inc;int32_t b;
 switch(t->p[P_D2W]-1){
  case 1:b=osc_pulse(p,inc,0x80000000u);break;
  case 2:b=osc_tri(p);break;
  case 3:b=osc_sine(p);break;
  case 4:b=osc_pulse(p,inc,0x50000000u);break;
  default:b=osc_saw(p,inc);break;
 }
 /* Automatic summing headroom when both oscillator levels are raised. */
 int32_t sum=a+b*t->p[P_D2LV]/127,total=t->p[P_DLV1]+t->p[P_D2LV];
 return total>127?sum*127/total:sum;
}
