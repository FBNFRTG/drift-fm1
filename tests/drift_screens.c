/* SPDX-License-Identifier: GPL-3.0-only */
/* Reproducible screenshots from DRIFT's actual framebuffer, no UI mockups. */
#include "../web/emu/felucca_web.c"
static void tick(void){for(int i=0;i<400;i++)web_render(128);}
static void shot(const char *path){FILE *f=fopen(path,"wb");fprintf(f,"P6\n240 240\n255\n");for(int i=0;i<240*240;i++){uint16_t c=web_fb[i];c=(c>>8)|(c<<8);unsigned char b[3]={(c>>11)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};fwrite(b,1,3,f);}fclose(f);}
int main(void){web_nor_erase();web_boot();tick();drift_load(0);drift_page=D_HOME;tick();shot("build/site-patches.ppm");drift_page=D_ENV;ui.force=1;tick();shot("build/site-envelope.ppm");trk[0].p[P_ROOT]=2;trk[0].p[P_SCALE]=2;drift_page=D_SCL;ui.force=1;tick();shot("build/site-scale.ppm");trk[0].p[P_DPITCH]=7;trk[0].p[P_DOCT]=2;trk[0].p[P_DRHYTHM]=34;trk[0].p[P_DRATE]=18;drift_page=D_SEQ2;ui.force=1;tick();shot("build/site-sequence.ppm");return 0;}
