/* SPDX-License-Identifier: GPL-3.0-only
 * DRIFT 0.2: deterministic Euclidean pattern, rebuilt only on setting changes.
 * Fixed-size, audio-thread owned. Loop units: 4/4 bars; duration: sixteenths.
 */
static struct {
 uint32_t rng,pos,cycle,sig,serial;
 uint16_t phase[32];
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
static void drift_generate(void){
 int16_t *p=trk[0].p;uint32_t i,steps=16u*(uint32_t)clamp(p[P_DLEN],1,16);
 drift_gen.rng=0xA53C916Du;drift_gen.pool_n=0;
 for(i=(uint32_t)p[P_DMIN];i<128u && i<=(uint32_t)p[P_DMAX];i++)
  if(SCALE_MASK[p[P_SCALE]&15]&(1u<<((i+12u-p[P_ROOT])%12u)))drift_gen.pool[drift_gen.pool_n++]=(uint8_t)i;
 drift_gen.count=(uint8_t)clamp(p[P_DNOTES],1,steps<32?steps:32);
 for(i=0;i<drift_gen.count;i++){
  uint32_t slot=i*steps/drift_gen.count;
  uint32_t begin=slot*65536u/steps,end=((i+1u)*steps/drift_gen.count)*65536u/steps;
  uint32_t direction=(uint32_t)clamp(p[P_DPITCH],0,21),interval=(uint32_t)clamp(p[P_DOCT],0,7);
  uint32_t depth=direction?((direction-1)%7)+1:0,mode=direction?(direction-1)/7:0;
  int root=-1,degree=0;
  for(uint32_t j=0;j<drift_gen.pool_n;j++)if(drift_gen.pool[j]%12==p[P_ROOT] && (root<0 || ((int)drift_gen.pool[j]>60?(int)drift_gen.pool[j]-60:60-(int)drift_gen.pool[j])<((int)drift_gen.pool[root]>60?(int)drift_gen.pool[root]-60:60-(int)drift_gen.pool[root])))root=(int)j;
  if(depth){uint32_t period=depth+1,step=i%period;degree=mode==0?(int)step:mode==1?-(int)step:(i%(2*depth+1)<=depth?(int)(i%(2*depth+1)):(int)depth-(int)(i%(2*depth+1)));}
  else if(interval)degree=(int)(i%4);
  degree*=interval?interval:1;
  drift_gen.note[i]=root>=0?drift_gen.pool[clamp(root+degree,0,drift_gen.pool_n-1)]:0;
  drift_gen.enabled[i]=(uint8_t)(root>=0 && drift_rand()%100u<(uint32_t)p[P_DPROB]);
  drift_gen.velocity[i]=96;drift_gen.phase[i]=(uint16_t)begin;
  uint32_t time=(uint32_t)clamp(p[P_DRATE],0,100),rnd=drift_rand();
  if(time){
   /* Swing, then irregular displacement, then clusters over the whole bar. */
   int offset=(int)(end-begin)*(int)time/220;
   if(time<34)offset=(i&1)?offset:0;
   else offset=(int)(rnd%(2u*(uint32_t)offset+1u))-offset;
   int pos=(int)begin+offset;
   if(time>66){uint32_t cluster=(rnd>>12)%8;int target=(int)((begin/16384u)*16384u+cluster*2048u);pos+=(target-pos)*(int)(time-66)/34;}
   drift_gen.phase[i]=(uint16_t)clamp(pos,0,65535);
  }
 }
 /* Chaos performs a seeded Fisher–Yates shuffle; it cannot merely transpose. */
 for(i=drift_gen.count;i>1;i--)if(drift_rand()%100u<(uint32_t)p[P_DRHYTHM]){uint32_t j=drift_rand()%i;uint8_t n=drift_gen.note[i-1];drift_gen.note[i-1]=drift_gen.note[j];drift_gen.note[j]=n;}
 /* Sort timed events while preserving their assigned pitch/probability. */
 for(i=1;i<drift_gen.count;i++)for(uint32_t j=i;j && drift_gen.phase[j]<drift_gen.phase[j-1];j--){
  uint16_t ph=drift_gen.phase[j];drift_gen.phase[j]=drift_gen.phase[j-1];drift_gen.phase[j-1]=ph;
  uint8_t n=drift_gen.note[j];drift_gen.note[j]=drift_gen.note[j-1];drift_gen.note[j-1]=n;
  n=drift_gen.enabled[j];drift_gen.enabled[j]=drift_gen.enabled[j-1];drift_gen.enabled[j-1]=n;
 }

 drift_gen.sig=drift_cfg_sig();drift_gen.serial++;
}
static void drift_fire_range(uint32_t start,uint32_t end,uint32_t len,uint32_t dur){
 uint32_t i;for(i=0;i<drift_gen.count;i++){
  uint32_t due=(uint32_t)(((uint64_t)drift_gen.phase[i]*len)>>16),nt=drift_gen.note[i];
  if(due>=start && due<end && drift_gen.enabled[i]){
   if(drift_gen.off[nt])trk_note_off(&trk[0],nt);
   trk_note_on(&trk[0],nt,drift_gen.velocity[i]);drift_gen.off[nt]=dur;last_note=(uint8_t)nt;
  }
 }
}
static void drift_tick(uint32_t n){
 uint32_t i,len,end,dur;
 if(!drift_gen.count || drift_gen.sig!=drift_cfg_sig()){drift_release();drift_generate();}
 if(!song.playing){if(drift_gen.was_playing)drift_release();drift_gen.was_playing=0;drift_gen.pos=0;return;}
 if(!drift_gen.was_playing)drift_gen.pos=0;drift_gen.was_playing=1;
 for(i=0;i<128;i++)if(drift_gen.off[i]){if(drift_gen.off[i]<=n){trk_note_off(&trk[0],i);drift_gen.off[i]=0;}else drift_gen.off[i]-=n;}
 len=drift_loop_samples();dur=beat_samples()*(uint32_t)clamp(trk[0].p[P_DDUR],1,64)/4u;
 if(drift_gen.pos>=len)drift_gen.pos%=len;
 end=drift_gen.pos+n;drift_fire_range(drift_gen.pos,end<len?end:len,len,dur);drift_gen.pos=end;
 if(end>=len){drift_gen.pos=end-len;drift_gen.cycle++;drift_fire_range(0,drift_gen.pos,len,dur);}
}
