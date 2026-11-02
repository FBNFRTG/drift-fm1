/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
#include <math.h>
static void tick(void){for(int i=0;i<40;i++)web_render(128);}
int main(void){
 web_nor_erase();web_boot();for(int i=0;i<400;i++)web_render(128);
 for(int wave=12;wave<=13;wave++){
  drift_load(25);tick();track_t *t=&trk[0];voice_t *v=&t->v[0];
  t->p[P_E0]=wave;t->p[P_E1]=64;t->p[P_E2]=80;t->p[P_E4]=100;t->p[P_D2W]=0;
  memset(v,0,sizeof *v);v->note=57;v->pitch16=57*16;v->vel=100;v->age=1;analog_note_on(t,v);
  vmod_t m={.inc=pitch_inc(57*16),.pitch16=57*16,.amp0=32767,.amp1=32767};double energy[20]={0};
  char path[64];snprintf(path,sizeof path,"build/sustained-%d.f32",wave);FILE *f=fopen(path,"wb");assert(f);
  for(int k=0;k<44100*20/CTL;k++){int32_t out[CTL]={0};analog_render(t,v,out,CTL,&m);for(int j=0;j<CTL;j++){float a=out[j]/32768.f;assert(isfinite(a)&&fabsf(a)<2);energy[(k*CTL+j)/44100]+=a*a;fwrite(&a,4,1,f);}}
  fclose(f);for(int i=2;i<20;i++)assert(energy[i]>.001);assert(energy[19]>energy[3]*.1);
  assert(!up_store(0,"AIR TEST"));up_boot();drift_load(256);assert(t->p[P_E0]==wave&&t->p[P_E1]==64&&t->p[P_E2]==80);
  printf("PASS wave %d: continuous 20-second excitation, late energy %.6f, save/reload.\n",wave,energy[19]);
 }
 for(int id=29;id<=32;id++){
  drift_load(id);tick();web_keys(1u<<4);double first=0,late=0;float peak=0;
  for(int k=0;k<44100*12/128;k++){web_render(128);for(int j=0;j<128;j++){float a=out_l[j];assert(isfinite(a)&&fabsf(a)<=1);if(k<50)first+=a*a;if(k>3000)late+=a*a;if(fabsf(a)>peak)peak=fabsf(a);}}
  assert(late>.001&&peak>.001);web_keys(0);for(int k=0;k<44100*14/128;k++)web_render(128);
  printf("PASS sustained patch %d: peak %.4f, late energy %.4f; long hold and release.\n",id,peak,late);
 }
 // Full bank plus all favourites exceeds the old fixed-size patch list.
 for(int i=0;i<UP_SLOTS;i++){drift_load(0);assert(!up_store(i,"LIST TEST"));favorite_set(NENGINES,i,1);}
 for(int i=0;i<ENG_ANALOG.npresets;i++)favorite_set(0,i,1);
 drift_list_build();assert(drift_list_n==2*(ENG_ANALOG.npresets+UP_SLOTS));
 puts("PASS: expanded patch list includes every factory/user patch and its favourite duplicate.");return 0;
}
