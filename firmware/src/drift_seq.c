/* SPDX-License-Identifier: GPL-3.0-only
 * DRIFT 0.2: deterministic Euclidean pattern, rebuilt only on setting changes.
 * Fixed-size, audio-thread owned. Loop units: 4/4 bars; duration: sixteenths.
 */
static struct {
 uint32_t rng,pos,cycle,sig,serial;
 uint16_t phase[32],gate32[32];
 uint8_t note[32],enabled[32],velocity[32],pool[128],pool_n,count,was_playing;
 uint32_t off[128];
} drift_gen;
static uint32_t drift_rand(void){uint32_t x=drift_gen.rng?drift_gen.rng:0xA53C916Du;x^=x<<13;x^=x>>17;x^=x<<5;return drift_gen.rng=x;}
static uint32_t drift_cfg_sig(void){
 static const uint8_t f[]={P_ROOT,P_SCALE,P_DMIN,P_DMAX,P_DLEN,P_DNOTES,P_DDUR,P_DPROB,P_DPITCH,P_DOCT,P_DRHYTHM,P_DRATE};
 uint32_t h=2166136261u,i;for(i=0;i<sizeof f;i++){h^=(uint16_t)trk[0].p[f[i]];h*=16777619u;}return h;
}
static void drift_release(void){uint32_t i;for(i=0;i<128;i++)if(drift_gen.off[i]){trk_note_off(&trk[0],i);drift_gen.off[i]=0;}}
static void drift_restart(void){drift_release();drift_gen.pos=0;drift_gen.was_playing=0;}
static uint8_t drift_nearest(int note){uint32_t i,b=0;int best=999;for(i=0;i<drift_gen.pool_n;i++){int d=drift_gen.pool[i]-note;if(d<0)d=-d;if(d<best){best=d;b=i;}}return drift_gen.pool[b];}
static uint32_t drift_loop_samples(void){return beat_samples()*4u*(uint32_t)clamp(trk[0].p[P_DLEN],1,16);}
/* Scale-degree movement: append-only values keep old patch directions intact. */
static int drift_degree(uint32_t direction,uint32_t i){
 if(!direction)return 0;
 uint32_t depth=(direction-1)%7+1,mode=(direction-1)/7,step=i%(depth+1),q;
 switch(mode){
 case 0:return (int)step;
 case 1:return -(int)step;
 case 2:q=i%(2*depth+1);return q<=depth?(int)q:(int)depth-(int)q;
 case 3:q=i%(2*depth+1);return q<=depth?-(int)q:(int)q-(int)depth;
 case 4:q=i%(2*depth);return q<=depth?(int)q:(int)(2*depth-q);
 case 5:return step&1?-(int)((step+1)/2):(int)(step/2);
 case 6:return step&1?(int)((depth+1)/2-step/2):-(int)(depth/2-step/2);
 case 7:return (i&1)?(int)((i/2)%depth+1):0;
 default: { /* Seeded walk is regenerated once, never on each cycle. */
  int d=0;uint32_t h=0x71239ABDu;
  for(uint32_t k=0;k<i;k++){h^=h<<13;h^=h>>17;h^=h<<5;d+=(h&1)?1:-1;d=clamp(d,-(int)depth,(int)depth);}return d;
 }
 }
}
static int drift_interval_degree(int degree,uint32_t interval){
 static const uint8_t patterns[8][4]={{2,3,2,3},{2,4,2,4},{3,4,3,4},{1,2,1,2},{2,2,3,0},{4,3,4,3},{7,2,7,2},{2,3,4,0}};
 if(interval<8)return degree*(interval?interval:1);
 static const uint8_t lengths[8]={2,2,2,2,3,2,2,3};
 int sign=degree<0?-1:1,n=degree<0?-degree:degree,sum=0;
 for(int k=0;k<n;k++)sum+=patterns[clamp(interval-8,0,7)][k%lengths[clamp(interval-8,0,7)]];
 return sign*sum;
}
/* Musical rhythm, in 1/32 notes. TIME is intensity, never a random seed.
 * A fixed permutation progressively activates pairs. Pair anchors stay fixed;
 * their inner boundary moves to a shorter musical interval as intensity rises.
 * This keeps note count, order and loop length, and cannot create collisions.
 */
static uint16_t drift_rhythm_value(uint32_t limit){
 static const uint16_t values[]={1,2,3,4,6,8,12,16,24,32,48,64,96,128,192,256,384,512};
 uint16_t value=1;for(uint32_t i=0;i<NELEM(values) && values[i]<=limit;i++)value=values[i];return value;
}
static uint32_t drift_rhythm_rand(uint32_t *state){
 uint32_t x=*state;x^=x<<13;x^=x>>17;x^=x<<5;return *state=x;
}
static void drift_rhythm(uint32_t steps,uint32_t time){
 uint32_t count=drift_gen.count,grid=steps*2u,base=(uint32_t)clamp(trk[0].p[P_DDUR],1,64)*2u;
 uint16_t at[32];uint8_t order[16];
 for(uint32_t i=0;i<count;i++){at[i]=(uint16_t)(2u*(i*steps/count));drift_gen.gate32[i]=(uint16_t)base;}
 if(!time)return;
 uint32_t pairs=count/2u,seed=0x86D12B49u^(steps<<16)^count;
 for(uint32_t i=0;i<pairs;i++)order[i]=(uint8_t)i;
 for(uint32_t i=pairs;i>1;i--){uint32_t j=drift_rhythm_rand(&seed)%i;uint8_t v=order[i-1];order[i-1]=order[j];order[j]=v;}
 for(uint32_t rank=0;rank<pairs;rank++){
  uint32_t pair=order[rank],a=pair*2u,b=a+1u;
  uint32_t end=b+1u<count?at[b+1u]:grid,span=end-at[a];
  uint32_t left=at[b]-at[a],right=end-at[b],small=left<right?left:right;
  uint32_t bits=drift_rhythm_rand(&seed);
  if(time*pairs<=rank*100u)continue;
  uint32_t divisor=time<34?2u:time<67?4u:8u;
  uint32_t short_gap=drift_rhythm_value(small/divisor);
  uint32_t gap0=(bits&1u)?short_gap:span-short_gap,gap1=span-gap0;
  at[b]=(uint16_t)(at[a]+gap0);
  uint32_t gaps[2]={gap0,gap1};
  for(uint32_t k=0;k<2;k++){
   uint32_t gap=gaps[k],limit=gap>short_gap?base*3u/2u:(base+1u)/2u;
   if(limit>gap)limit=gap;
   /* Shorten the long leg to leave an actual note-off/rest. Higher TIME
    * introduces rests in more pairs; no probability/pitch random numbers used. */
   if(gap>short_gap && time>=34u+rank*66u/(pairs?pairs:1u)){
    uint32_t rest_limit=gap/(time<67?2u:4u);if(limit>rest_limit)limit=rest_limit;
   }
   drift_gen.gate32[a+k]=drift_rhythm_value(limit);
  }
 }
 /* A single note (or the last odd note) can vary its gate without moving
  * the loop anchor. At TIME 100 every gate fits its slot, leaving clear gaps. */
 if((count&1u) && time>=34){
  uint32_t last=count-1u,gap=grid-at[last],limit=time<67?base:base/2u;
  if(limit>gap/2u)limit=gap/2u;drift_gen.gate32[last]=drift_rhythm_value(limit);
 }
 for(uint32_t i=0;i<count;i++)drift_gen.phase[i]=(uint16_t)((uint32_t)at[i]*65536u/grid);
}
static void drift_generate(void){
 int16_t *p=trk[0].p;uint32_t i,steps=16u*(uint32_t)clamp(p[P_DLEN],1,16);
 drift_gen.rng=0xA53C916Du;drift_gen.pool_n=0;
 for(i=(uint32_t)p[P_DMIN];i<128u && i<=(uint32_t)p[P_DMAX];i++)
  if(SCALE_MASK[p[P_SCALE]&15]&(1u<<((i+12u-p[P_ROOT])%12u)))drift_gen.pool[drift_gen.pool_n++]=(uint8_t)i;
 drift_gen.count=(uint8_t)clamp(p[P_DNOTES],1,steps<32?steps:32);
 for(i=0;i<drift_gen.count;i++){
  uint32_t slot=i*steps/drift_gen.count;
  uint32_t begin=slot*65536u/steps;
  uint32_t direction=(uint32_t)clamp(p[P_DPITCH],0,63),interval=(uint32_t)clamp(p[P_DOCT],0,15);
  int root=-1,degree=0;
  for(uint32_t j=0;j<drift_gen.pool_n;j++)if(drift_gen.pool[j]%12==p[P_ROOT] && (root<0 || ((int)drift_gen.pool[j]>60?(int)drift_gen.pool[j]-60:60-(int)drift_gen.pool[j])<((int)drift_gen.pool[root]>60?(int)drift_gen.pool[root]-60:60-(int)drift_gen.pool[root])))root=(int)j;
  degree=drift_degree(direction,i);
  if(!direction && interval)degree=i%4;
  degree=drift_interval_degree(degree,interval);
  if(root>=0 && (direction>21 || interval>=8)){int n=drift_gen.pool_n;degree=((root+degree)%n+n)%n-root;}
  drift_gen.note[i]=root>=0?drift_gen.pool[clamp(root+degree,0,drift_gen.pool_n-1)]:0;
  drift_gen.enabled[i]=(uint8_t)(root>=0 && drift_rand()%100u<(uint32_t)p[P_DPROB]);
  drift_gen.velocity[i]=96;drift_gen.phase[i]=(uint16_t)begin;
  (void)drift_rand(); /* preserve existing probability draws */
 }
 /* Values 1..999 choose independently seeded full permutations. No transport RNG. */
 if(p[P_DRHYTHM]>0){
  drift_gen.rng=0x9E3779B9u*(uint32_t)p[P_DRHYTHM]^0xA53C916Du;
  for(i=drift_gen.count;i>1;i--){uint32_t j=drift_rand()%i;uint8_t n=drift_gen.note[i-1];drift_gen.note[i-1]=drift_gen.note[j];drift_gen.note[j]=n;}
 }
 drift_rhythm(steps,(uint32_t)clamp(p[P_DRATE],0,100));

 drift_gen.sig=drift_cfg_sig();drift_gen.serial++;
}
static void drift_fire_range(uint32_t start,uint32_t end,uint32_t len){
 uint32_t i;for(i=0;i<drift_gen.count;i++){
  uint32_t due=(uint32_t)(((uint64_t)drift_gen.phase[i]*len)>>16),nt=drift_gen.note[i];
  if(due>=start && due<end && drift_gen.enabled[i]){
   if(drift_gen.off[nt])trk_note_off(&trk[0],nt);
   trk_note_on(&trk[0],nt,drift_gen.velocity[i]);drift_gen.off[nt]=beat_samples()*(uint32_t)drift_gen.gate32[i]/8u;last_note=(uint8_t)nt;
  }
 }
}
static void drift_tick(uint32_t n){
 uint32_t i,len,end;
 if(!drift_gen.count || drift_gen.sig!=drift_cfg_sig()){drift_release();drift_generate();}
 if(!song.playing){if(drift_gen.was_playing)drift_release();drift_gen.was_playing=0;drift_gen.pos=0;return;}
 if(!drift_gen.was_playing)drift_gen.pos=0;drift_gen.was_playing=1;
 for(i=0;i<128;i++)if(drift_gen.off[i]){if(drift_gen.off[i]<=n){trk_note_off(&trk[0],i);drift_gen.off[i]=0;}else drift_gen.off[i]-=n;}
 len=drift_loop_samples();
 if(drift_gen.pos>=len)drift_gen.pos%=len;
 end=drift_gen.pos+n;drift_fire_range(drift_gen.pos,end<len?end:len,len);drift_gen.pos=end;
 if(end>=len){drift_gen.pos=end-len;drift_gen.cycle++;drift_fire_range(0,drift_gen.pos,len);}
}
