/* SPDX-License-Identifier: GPL-3.0-only
 * Six flash samples as ANALOG waveforms. Uses Felucca's IMA decoder and SVF.
 * Voice state: s0/1 decoder, s2/3 interpolator, s4 zone, s5/7 SVF, s6 end;
 * ph0/1 position, ph2 noise. MIX becomes sample level; DETUNE remains cents.
 */
static void drift_sample_on(track_t *t,voice_t *v){
 uint32_t k=(uint32_t)(t->p[P_E0]-7),i,zi=0xffff;
 for(i=0;k<SMP_USER_SLOTS && i<usr_nz[k];i++)if(v->note>=usr_zone[k][i].lo && v->note<=usr_zone[k][i].hi){zi=0x8000u|(k<<5)|i;break;}
 v->ph[0]=v->ph[1]=0;v->ph[2]=v->age+0x1234567u;
 for(i=0;i<8;i++)v->s[i]=0;
 v->s[4]=(int32_t)(zi==0xffff?0:zi);v->s[6]=zi==0xffff;
}
static void drift_sample_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m){
 const int16_t *p=t->p;const smp_zone_t *z=smp_zone((uint32_t)v->s[4]);
 uint32_t extra_inc=drift_osc_inc(t,m);
 uint32_t i,frac=v->ph[1],step;int32_t ic1=v->s[5],ic2=v->s[7],noise=(int32_t)v->ph[2];tsvf_t flt;
 int32_t cut=(p[P_E4]<<8)+m->cutoff+p[P_E7]*(v->pitch16-960)/16;
 int32_t pitch=clamp(m->pitch16+p[P_E1]*16/100-z->root16,-1536,576);
 uint32_t r=pow2_q16(pitch);step=(r>>8)*(z->rate>>8);
 if(v->s[6] || !z->n){v->active=0;return;}
 tsvf_coef(&flt,cut,p[P_E5]);
 if(!v->ph[0] && !v->ph[1] && !v->s[3])v->s[3]=sample_next(z,v,1);
 for(i=0;i<n;i++){
  int32_t s,y,a;frac+=step;
  while(frac>=65536u){frac-=65536u;v->s[2]=v->s[3];if(v->ph[0]>=z->n){v->s[6]=1;v->s[3]=0;frac&=65535u;break;}v->s[3]=sample_next(z,v,1);}
  s=v->s[2]+((v->s[3]-v->s[2])*(int32_t)(frac>>1)>>15);s=s*p[P_E2]/127;
  if(p[P_E3])s+=mulq15((int32_t)(noise32(&noise)>>17)-16384,p[P_E3]*200);
  s=drift_osc_mix(t,v,s,extra_inc);
  if(p[P_E6])s=softclip(((s>>2)*((32768+p[P_E6]*512)>>2))>>11);
  y=analog_tap(&flt,ANALOG_K(p[P_E5]),s>>1,&ic1,&ic2,(uint32_t)p[P_FTYPE]);a=y<0?-y:y;
  if(a>16000){a=16000+(softclip((a-16000)*2)>>1);y=y<0?-a:a;}
  out[i]+=voice_amp(y*2,m,i)*2;if(v->s[6])break;
 }
 v->ph[1]=frac;v->ph[2]=(uint32_t)noise;v->s[5]=ic1;v->s[7]=ic2;
}
