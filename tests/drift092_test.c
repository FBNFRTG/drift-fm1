/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
static void tick(void){for(int i=0;i<40;i++)web_render(128);}
static void tap(int b){web_buttons(1u<<b);tick();web_buttons(0);tick();}
int main(void){
 web_nor_erase();web_boot();for(int i=0;i<400;i++)web_render(128);drift_load(0);tick();
 track_t *t=&trk[0];
 for(int mode=0;mode<4;mode++)for(int trig=0;trig<2;trig++){
  t->p[P_VOICE]=mode;t->p[P_LTRIG]=trig;t->p[P_LFADE]=80;
  for(int j=0;j<12;j++){
   trk_all_off(t);t->lfo_ph=0x789ABCDEu+j;t->lfo_fade=23456;
   trk_note_on(t,48+j,100);assert(t->lfo_ph==0x789ABCDEu+j && t->lfo_fade==23456);
   trk_note_on(t,65+j,90);assert(t->lfo_ph==0x789ABCDEu+j && t->lfo_fade==23456);
  }
 }
 trk_all_off(t);uint32_t ph=t->lfo_ph;tick();assert(t->lfo_ph!=ph);
 puts("PASS: LFO phase/fade survives new phrases and overlapping notes in all voice modes and legacy trigger settings; advances in silence.");
 drift_page=D_HOME;tap(B_SAVE);assert(name_on());
 char last[NM_LEN+1];str_cpy(last,nm.s,sizeof last);uint32_t rng=t->lfo_rnd;
 for(int i=0;i<1000;i++){
  drift_random_name();assert(nm.len<=NM_LEN && nm.len>0 && nm.s[nm.len]==0);assert(strcmp(last,nm.s));
  str_cpy(last,nm.s,sizeof last);assert(t->lfo_rnd==rng);
 }
 web_enc(EN_K3,1);tick();assert(strcmp(last,nm.s));str_cpy(last,nm.s,sizeof last);
 tap(B_SAVE);assert(!name_on()&&!drift_dialog);char saved[16];drift_name(drift_id(),saved);assert(!strcmp(saved,last));
 up_boot();drift_load(256);drift_name(drift_id(),saved);assert(!strcmp(saved,last));
 tap(B_SAVE);tap(B_SAVE);assert(drift_dialog==D_DIALOG_OVERWRITE&&!drift_yes);tap(B_HOME);assert(name_on());
 web_enc(EN_K3,-1);tick();assert(strcmp(last,nm.s));tap(B_HOME);assert(!name_on());
 puts("PASS: 1000 bounded distinct consecutive random names; real KNOB 3 both directions, save/reload, overwrite confirmation and cancel.");return 0;
}
