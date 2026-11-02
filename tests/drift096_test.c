/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
int main(void){
 web_nor_erase();web_boot();for(int i=0;i<400;i++)web_render(128);
 drift_load(0);int16_t *p=trk[0].p;p[P_DPROB]=71;p[P_DPITCH]=5;p[P_DRHYTHM]=99;
 for(int bars=1;bars<=16;bars++)for(int count=1;count<=32&&count<=bars*16;count++){
  p[P_DLEN]=bars;p[P_DNOTES]=count;p[P_DDUR]=2;
  uint8_t notes[32],enabled[32];uint32_t prev=0;
  for(int time=0;time<=100;time++){
   p[P_DRATE]=time;drift_generate();assert(drift_gen.count==count);uint32_t deviation=0;
   if(!time){memcpy(notes,drift_gen.note,32);memcpy(enabled,drift_gen.enabled,32);}
   assert(!memcmp(notes,drift_gen.note,count));assert(!memcmp(enabled,drift_gen.enabled,count));
   for(int i=0;i<count;i++){
    uint32_t base=(i*bars*16/count)*65536u/(bars*16);
    assert(drift_gen.gate32[i]>0);if(i)assert(drift_gen.phase[i]>drift_gen.phase[i-1]);
    if(!time){assert(drift_gen.phase[i]==base);assert(drift_gen.gate32[i]==4);}
    deviation+=abs((int)drift_gen.phase[i]-(int)base);
    unsigned slot=((uint32_t)drift_gen.phase[i]*bars*32+65535)/65536;
    assert(slot*65536/(bars*32)==drift_gen.phase[i]);
   }
   assert(deviation>=prev);prev=deviation;
   uint16_t phases[32],gates[32];memcpy(phases,drift_gen.phase,sizeof phases);memcpy(gates,drift_gen.gate32,sizeof gates);
   drift_generate();assert(!memcmp(phases,drift_gen.phase,sizeof phases));assert(!memcmp(gates,drift_gen.gate32,sizeof gates));
  }
 }
 p[P_DLEN]=1;p[P_DNOTES]=8;p[P_DDUR]=2;p[P_DPROB]=100;
 for(int t=0;t<=100;t+=25){p[P_DRATE]=t;drift_generate();printf("TIME %d starts/gates (32nds):",t);for(int i=0;i<8;i++)printf(" %u/%u",drift_gen.phase[i]/2048,drift_gen.gate32[i]);puts("");}
 p[P_DRATE]=25;drift_generate();int dotted=0;for(int i=0;i<8;i++)dotted|=drift_gen.gate32[i]==6;assert(dotted);
 p[P_DRATE]=100;drift_generate();int rests=0;for(int i=0;i<8;i++){unsigned gap=((i==7?65536:drift_gen.phase[i+1])-drift_gen.phase[i])/2048;assert(drift_gen.gate32[i]<=gap);rests+=drift_gen.gate32[i]<gap;}assert(rests);
 song.playing=1;drift_gen.was_playing=1;drift_gen.pos=12345;uint32_t cycle=drift_gen.cycle;p[P_DRATE]=40;drift_tick(128);assert(drift_gen.pos==12473&&drift_gen.cycle==cycle);
 drift_release();drift_fire_range(0,1,drift_loop_samples());assert(drift_gen.off[drift_gen.note[0]]==beat_samples()*drift_gen.gate32[0]/8);
 song.playing=0;assert(!up_store(0,"TIME TEST"));up_boot();drift_load(256);assert(trk[0].p[P_DRATE]==40);
 puts("PASS TIME: all bars/counts/intensities; ordered grid, progressive variation, fixed pitch/probability, dotted gates/rests, repeatability, live transport and persistence.");return 0;
}
