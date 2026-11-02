/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
#include <math.h>
static void tick(void){for(int i=0;i<40;i++)web_render(128);}
static void tap(int b){web_buttons(1u<<b);tick();web_buttons(0);tick();}
static double capture(int blocks,FILE *f){double energy=0;for(int j=0;j<blocks;j++){web_render(128);for(int i=0;i<128;i++){assert(isfinite(out_l[i])&&fabsf(out_l[i])<=1.0f);energy+=out_l[i]*out_l[i];}if(f)fwrite(out_l,4,128,f);}return energy;}
int main(void){
 web_nor_erase();web_boot();for(int i=0;i<400;i++)web_render(128);drift_load(0);tick();
 assert(ENG_ANALOG.npresets>=29);assert(trk_nvoice(&trk[0])==NVOICE);
 drift_page=D_OSC;web_enc(EN_K1,-127);tick();web_enc(EN_K1,11);tick();assert(trk[0].p[P_E0]==11);assert(trk_nvoice(&trk[0])==3);
 web_enc(EN_K2,70);web_enc(EN_K3,-20);tick();int material=trk[0].p[P_E1],damp=trk[0].p[P_E2];
 assert(!up_store(0,"BODY TEST"));up_boot();drift_load(256);assert(trk[0].p[P_E0]==11&&trk[0].p[P_E1]==material&&trk[0].p[P_E2]==damp);
 for(int id=25;id<=28;id++){
  drift_load(id);tick();char path[64];snprintf(path,sizeof path,"build/physical-patch-%d.f32",id);FILE *f=fopen(path,"wb");assert(f);
  web_keys(1u<<7);double en=capture(700,f);assert(en>.001);web_keys(0);capture(500,f);fclose(f);
 }
 // Strings/resonators at low/high pitch and parameter extremes; independent voice slots.
 for(int wave=10;wave<=11;wave++)for(int note=24;note<=108;note+=28)for(int material=0;material<=127;material+=127)for(int damp=0;damp<=127;damp+=127){
  drift_load(25);tick();trk[0].p[P_E0]=wave;trk[0].p[P_E1]=material;trk[0].p[P_E2]=damp;
  for(int k=0;k<8;k++)trk_note_on(&trk[0],note+k,100);
  int active=0;for(int k=0;k<NVOICE;k++)active+=!!trk[0].v[k].active;assert(active<=3);
  capture(60,0);trk_all_off(&trk[0]);
 }
 // Physical waveform transitions and common oscillator 2/filter/effects path.
 drift_load(25);tick();trk[0].p[P_D2W]=4;trk[0].p[P_D2LV]=80;web_keys(0x5555);assert(capture(100,0)>.001);
 drift_page=D_OSC;web_enc(EN_K1,-127);tick();assert(trk[0].p[P_E0]==0&&trk_nvoice(&trk[0])==NVOICE);web_keys(0);tick();web_keys(1);assert(capture(100,0)>.001);web_keys(0);
 // Isolated source captures exclude delay/reverb tails and master processing.
 for(int wave=10;wave<=11;wave++)for(int note=57;note<=69;note+=12)for(int damp=0;damp<=127;damp+=127){
  drift_load(25);tick();track_t *t=&trk[0];voice_t *v=&t->v[0];
  t->p[P_E0]=wave;t->p[P_E1]=34;t->p[P_E2]=damp;t->p[P_E4]=127;t->p[P_E5]=0;t->p[P_E6]=0;t->p[P_E7]=0;t->p[P_D2W]=0;
  memset(v,0,sizeof *v);v->note=note;v->pitch16=note*16;v->vel=100;v->age=1;analog_note_on(t,v);
  vmod_t m={.inc=pitch_inc(note*16),.pitch16=note*16,.amp0=32767,.amp1=32767};
  char path[80];snprintf(path,sizeof path,"build/physical-%d-%d-%d.f32",wave,note,damp);FILE *f=fopen(path,"wb");assert(f);double en=0;
  for(int k=0;k<44100*4/CTL;k++){int32_t out[CTL]={0};analog_render(t,v,out,CTL,&m);for(int j=0;j<CTL;j++){float a=out[j]/32768.f;assert(isfinite(a)&&fabsf(a)<4);en+=a*a;fwrite(&a,4,1,f);}}
  fclose(f);assert(en>.001);
 }
 puts("PASS: STRING/RESONATOR UI selection, material/damping save and flash reload; four audible starter patches; low/high notes and parameter extremes; 3-voice cap and 8-voice return; oscillator 2 and waveform changes.");return 0;
}
