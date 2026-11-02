/* SPDX-License-Identifier: GPL-3.0-only
 * DRIFT tape colour: stereo, fixed-point, no allocation or audio-rate division.
 * Shared delay modulation preserves the stereo image. Zero controls bypass exactly.
 * 512 stereo int16 frames = 2 KiB; Q2 samples retain four times full-scale headroom.
 */
static int16_t drift_tape_buf[512][2] __attribute__((section(".pool")));
static struct {uint32_t pos, wow_ph, flutter_ph, warm_ph;int32_t smooth[4],mix,lp[2];} dtape;
static int32_t drift_tape_smooth(int32_t x,int32_t target){
 int32_t d=target-x;if(d>-512 && d<512)return target;return x+d/512;
}
static int32_t drift_tape_mul(int32_t a,int32_t b){return (int32_t)(((int64_t)a*b)>>15);}
static int32_t drift_tape_soft(int32_t x){
 x=clamp(x,-32767,32767);
 return (x-mulq15(mulq15(x,x),x)/3)*3/2;
}
static void drift_tape_sample(int32_t *left,int32_t *right){
 const int16_t *p=trk[0].p;
 const uint8_t ids[]={P_DTAPE,P_DWOW,P_DFLUT,P_DWARM};
 for(uint32_t j=0;j<4;j++)dtape.smooth[j]=drift_tape_smooth(dtape.smooth[j],clamp(p[ids[j]],0,127)*258);
 int32_t tape=dtape.smooth[0],wow=dtape.smooth[1],flutter=dtape.smooth[2],warm=dtape.smooth[3];
 dtape.wow_ph+=31165u; /* 0.32 Hz at 44100 Hz */
 dtape.flutter_ph+=662262u; /* 6.8 Hz */
 dtape.warm_ph+=76840u; /* 0.789 Hz, independent of musical LFO */
 if(!(tape|wow|flutter|warm|dtape.mix)){
  drift_tape_buf[dtape.pos][0]=(int16_t)clamp(*left/4,-32767,32767);
  drift_tape_buf[dtape.pos][1]=(int16_t)clamp(*right/4,-32767,32767);
  dtape.lp[0]=clamp(*left,-131068,131068);dtape.lp[1]=clamp(*right,-131068,131068);
  dtape.pos=(dtape.pos+1)&511u;return;
 }
 int32_t target=(p[P_DWOW]||p[P_DFLUT]||p[P_DWARM])?32767:0;
 dtape.mix=drift_tape_smooth(dtape.mix,target);
 int32_t depth=(mulq15(osc_sine(dtape.wow_ph),wow)*80
               +mulq15(osc_sine(dtape.flutter_ph),flutter)*8
               +mulq15(osc_sine(dtape.warm_ph),warm)*2)/128; /* Q8 samples */
 uint32_t rd=((dtape.pos<<8)-(128u<<8)-(uint32_t)depth)&((512u<<8)-1u);
 uint32_t index=rd>>8,frac=rd&255u;
 int32_t strength=clamp(tape+warm/3,0,32767),gain=4096+strength/4,comp=32767-strength/2;
 int32_t input[2]={*left,*right},output[2];
 for(uint32_t ch=0;ch<2;ch++){
  int32_t x=clamp(input[ch],-131068,131068),y=x;
  if(strength){
   int32_t driven=(int32_t)(((int64_t)x*gain)>>12);
   int32_t soft=mulq15(drift_tape_soft(driven),comp);
   y=x+drift_tape_mul(soft-x,strength);
  }
  dtape.lp[ch]+=drift_tape_mul(y-dtape.lp[ch],12288);
  y+=drift_tape_mul(dtape.lp[ch]-y,warm/2);
  drift_tape_buf[dtape.pos][ch]=(int16_t)clamp(y/4,-32767,32767);
  int32_t a=(int32_t)drift_tape_buf[index][ch]*4;
  int32_t b=(int32_t)drift_tape_buf[(index+1)&511u][ch]*4;
  int32_t delayed=a+(int32_t)(((int64_t)(b-a)*frac)>>8);
  output[ch]=y+(int32_t)(((int64_t)(delayed-y)*dtape.mix)>>15);
  if(!(tape|wow|flutter|warm|dtape.mix))output[ch]=input[ch];
 }
 dtape.pos=(dtape.pos+1)&511u;*left=output[0];*right=output[1];
}
