/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
#include <math.h>
static const uint8_t ids[]={P_DTAPE,P_DWOW,P_DFLUT,P_DWARM};
static void tick(void){for(int i=0;i<40;i++)web_render(128);}
static void tap(int b){web_buttons(1u<<b);tick();web_buttons(0);tick();}
static void reset_tape(void){memset(&dtape,0,sizeof dtape);memset(drift_tape_buf,0,sizeof drift_tape_buf);for(int j=0;j<4;j++)trk[0].p[ids[j]]=0;}
int main(void){
 web_nor_erase();web_boot();for(int i=0;i<400;i++)web_render(128);drift_load(0);tick();
 drift_page=D_HOME;tap(B_FX);assert(drift_page==D_FX);tap(B_FX);assert(drift_page==D_SPACE);tap(B_FX);assert(drift_page==D_TAPE);
 for(int j=0;j<4;j++){web_enc(EN_K1+j,127);tick();assert(trk[0].p[ids[j]]==127);}
 assert(!up_store(0,"TAPE TEST"));up_boot();drift_load(256);for(int j=0;j<4;j++)assert(trk[0].p[ids[j]]==127);
 up_rec_t old=*up_rec(0);old.packed[141]=0;int16_t v[P_COUNT];up_values(&old,v);for(int j=0;j<4;j++)assert(v[ids[j]]==0);
 drift_page=D_TAPE;tap(B_FX);assert(drift_page==D_TYPES);tap(B_FX);assert(drift_page==D_FX);
 reset_tape();for(int i=-200000;i<=200000;i+=17){int32_t l=i,r=-i;drift_tape_sample(&l,&r);assert(l==i&&r==-i);}
 puts("PASS: four-page routing, all four knobs, saved/reloaded values, old patch migration to OFF, exact bypass including headroom.");
 for(int mode=0;mode<8;mode++){
  reset_tape();if(mode>0&&mode<5)trk[0].p[ids[mode-1]]=127;if(mode==7)trk[0].p[P_DWARM]=127;if(mode==5)for(int j=0;j<4;j++)trk[0].p[ids[j]]=127;
  char path[64];snprintf(path,sizeof path,"build/tape-response-%d.f32",mode);FILE *f=fopen(path,"wb");assert(f);
  for(int i=0;i<44100*4;i++){
   int32_t l=(int32_t)(12000*sin(6.283185307179586*(mode>=6?9000:440)*i/44100)),r=l;
   drift_tape_sample(&l,&r);assert(l==r && l>=-131068 && l<=131068);float o=(float)l/32768;fwrite(&o,4,1,f);
  }fclose(f);
 }
 // Very hot alternating input and silence, controls and bypass transitions.
 reset_tape();for(int j=0;j<4;j++)trk[0].p[ids[j]]=127;
 for(int i=0;i<44100*2;i++){int32_t l=i%2?200000:-200000,r=-l;drift_tape_sample(&l,&r);assert(l>-200001&&l<200001&&r>-200001&&r<200001);}
 for(int j=0;j<4;j++)trk[0].p[ids[j]]=0;
 for(int i=0;i<44100;i++){int32_t l=0,r=0;drift_tape_sample(&l,&r);if(i>22050)assert(!l&&!r);}
 for(int id=21;id<25;id++){
  drift_load(id);tick();for(int j=0;j<4;j++)assert(trk[0].p[ids[j]]>0);
  trk_note_on(&trk[0],id==23?36:60,100);int peak=0;
  for(int i=0;i<800;i++){web_render(128);for(int k=0;k<128;k++){float a=out_l[k];assert(isfinite(a)&&fabsf(a)<=1.01f);if(fabsf(a)>0.001f)peak=1;}}
  assert(peak);trk_all_off(&trk[0]);
 }
 puts("PASS: stereo coherence, bounded full-scale DSP, effect transitions to silence, all four new patches render audio.");return 0;
}
