/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
int main(void){
 web_nor_erase();web_boot();
 int16_t *p=trk[0].p;p[P_ROOT]=0;p[P_SCALE]=2;p[P_DMIN]=24;p[P_DMAX]=96;p[P_DLEN]=4;p[P_DNOTES]=32;p[P_DPROB]=100;p[P_DRATE]=0;
 for(int dir=0;dir<64;dir++)for(int iv=0;iv<16;iv++){
  p[P_DPITCH]=dir;p[P_DOCT]=iv;p[P_DRHYTHM]=999;drift_generate();
  for(int i=0;i<32;i++){int n=drift_gen.note[i];assert(n>=24&&n<=96);assert(SCALE_MASK[2]&(1u<<(n%12)));}
 }
 p[P_DPITCH]=p[P_DOCT]=p[P_DRHYTHM]=0;drift_generate();for(int i=0;i<32;i++)assert(drift_gen.note[i]%12==0);
 assert(drift_interval_degree(1,8)==2&&drift_interval_degree(2,8)==5&&drift_interval_degree(3,8)==7);
 assert(drift_interval_degree(3,12)==7&&drift_interval_degree(6,12)==14);
 p[P_DPITCH]=7;p[P_DOCT]=1;uint32_t hashes[999];int unique=0;
 for(int c=1;c<=999;c++){
  p[P_DRHYTHM]=c;drift_generate();uint8_t notes[32];memcpy(notes,drift_gen.note,32);drift_generate();assert(!memcmp(notes,drift_gen.note,32));
  uint32_t h=2166136261u;for(int i=0;i<32;i++)h=(h^notes[i])*16777619u;
  int found=0;for(int i=0;i<unique;i++)found|=h==hashes[i];if(!found)hashes[unique++]=h;
 }
 assert(unique>980);printf("PASS: %d distinct deterministic CHAOS patterns / 999; all 64 DIR x 16 INTERVAL options stay in scale/range.\n",unique);
 p[P_DRHYTHM]=999;p[P_DPITCH]=63;p[P_DOCT]=15;song.g[G_DTIME]=17;drift_global_store();assert(!up_store(0,"NEW RANGE"));
 drift_load(0);drift_load(256);assert(p[P_DRHYTHM]==999 && p[P_DPITCH]==63 && p[P_DOCT]==15 && song.g[G_DTIME]==17);
 up_boot();drift_load(256);assert(p[P_DRHYTHM]==999);puts("PASS: extended patch values survive flash reload without changing record size.");
 // Delay timing: actual audio, including longest values at slowest tempo, all types.
 int values[]={0,6,7,8,9,13,14,15,16,17};
 for(int dt=0;dt<3;dt++)for(int v=0;v<10;v++){
  memset(&fx,0,sizeof fx);memset(&df,0,sizeof df);memset(dly_buf,0,sizeof dly_buf);memset(cho_buf,0,sizeof cho_buf);memset(rev_comb,0,sizeof rev_comb);memset(&rev_u,0,sizeof rev_u);
  song.g[G_BPM]=40;song.g[G_DTIME]=values[v];song.g[G_DFDBK]=80;song.g[G_DMIX]=127;song.g[G_RTYPE]=0;p[P_DDELAY]=dt;
  uint32_t expected=delay_samples(),first=0;int32_t zero[CTL]={0},in[CTL],out[CTL];
  for(uint32_t base=0;base<expected*(dt==2?2:1)+4096;base+=CTL){
   for(int i=0;i<CTL;i++)in[i]=(base+i<256)?12000:0;
   fx_buses(zero,in,zero,out,CTL);
   for(int i=0;i<CTL;i++){assert(out[i]>-262144&&out[i]<262144);if(!first && (out[i]>4||out[i]<-4))first=base+i;}
  }
  assert(first);if(dt<2)assert(first>=expected-128&&first<expected+256);
  else assert(first>=expected-128); // Reverse plays the previous whole grain backwards.
  printf("delay type %d time %d: requested %u samples, first %u, rate divisor %u\n",dt,values[v],expected,first,df.factor);
 }
 puts("PASS: long delay audio onset and bounds.");return 0;
}
