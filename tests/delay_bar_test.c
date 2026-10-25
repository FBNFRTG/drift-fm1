/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
static void tick(void){for(int i=0;i<40;i++)web_render(128);}
static int height(void){int h=0;for(int y=102;y<196;y++)h+=web_fb[y*240+88]==65535;return h;}
int main(void){
 web_nor_erase();web_boot();for(int i=0;i<400;i++)web_render(128);
 drift_load(0);drift_page=D_FX;web_enc(EN_K2,-127);tick();
 int previous=-1;uint32_t samples=0;
 for(int i=0;i<18;i++){
  int h=height();assert(h>previous);assert(h==i*94/17);assert(delay_samples()>samples);
  previous=h;samples=delay_samples();if(i<17){web_enc(EN_K2,1);tick();}
 }
 for(int i=16;i>=0;i--){web_enc(EN_K2,-1);tick();assert(height()==i*94/17);}
 web_enc(EN_K2,11);tick();int value=trk[0].p[P_DR2];int h=height();
 assert(!up_store(0,"BAR TEST"));drift_load(0);drift_load(256);drift_page=D_FX;ui.force=1;tick();
 assert(trk[0].p[P_DR2]==value && height()==h);
 puts("PASS: all 18 Delay Time framebuffer bar heights increase with duration; reverse traversal and patch persistence preserve mapping.");
 return 0;
}
