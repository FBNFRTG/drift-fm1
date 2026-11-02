/* SPDX-License-Identifier: GPL-3.0-only
 * DRIFT adapter for Felucca's MIT DaisySP-derived string and modal models.
 * State reuses ENG_MEM, three slots. Wave IDs 0..9 and patch IDs remain unchanged.
 * E1 = material, E2 = damping (higher = shorter/darker); common filter and ADSR.
 */
static void drift_physical_on(track_t *t,voice_t *v){
 phys_slot_t *s=phys_slot_of(t,v);if(!s){v->active=0;return;}
 uint32_t md=(t->p[P_E0]==10 || t->p[P_E0]==12)?PM_STRING:PM_MODAL;
 phys_reset(s,md,0x65A94D31u^(v->age*2654435761u)^(v->note*8191u));
 if(md==PM_STRING)s->u.s.trig=1;else s->u.m.trig=1;
 v->s[0]=v->s[1]=0;
}
static void drift_physical_render(track_t *t,voice_t *v,int32_t *out,uint32_t n,const vmod_t *m){
 phys_slot_t *s=phys_slot_of(t,v);if(!s){v->active=0;return;}
 const int16_t *p=t->p;uint32_t md=(p[P_E0]==10 || p[P_E0]==12)?PM_STRING:PM_MODAL;
 if(s->model!=md){v->active=0;return;}
 if(n>CTL)n=CTL;
 int32_t y[CTL],exc[CTL];
 int32_t material=p[P_E1]*516,damping=(127-p[P_E2])*516;
 int32_t brightness=clamp(48000-p[P_E2]*140,12000,65536),accent=clamp(v->vel*280,0,65536);
 int sustained=p[P_E0]>=12;
 int32_t bow=0;
 if(sustained){
  material=17544; /* harmonic body, no metallic strike */
  brightness=clamp(51000-p[P_E2]*220,18000,65536);
  damping=43000; /* continuous excitation, not a one-shot decay */
  bow=6000+p[P_E1]*170;
 }
 if(md==PM_STRING){
  px_string_blk_t b;
  px_string_block(&b,&s->u.s,m->inc,material,brightness,damping,accent,bow,18000);
  px_string_excite(&b,&s->u.s,exc,n);px_string_run(&b,&s->u.s,exc,y,n);
 }else{
  px_modal_blk_t b;
  px_modal_block(&b,&s->u.m,m->inc,material,brightness,damping,accent,2000,bow);
  px_modal_run(&b,&s->u.m,y,exc,n);
 }
 tsvf_t flt;int32_t ic1=v->s[0],ic2=v->s[1];
 int32_t cut=(p[P_E4]<<8)+m->cutoff+p[P_E7]*(v->pitch16-960)/16;
 tsvf_coef(&flt,cut,p[P_E5]);uint32_t extra_inc=drift_osc_inc(t,m);
 for(uint32_t i=0;i<n;i++){
  int32_t body=y[i];
  if(sustained)body+=px_m(exc[i],p[P_E0]==13?2500+p[P_E1]*50:1000+p[P_E1]*20,15);
  int32_t a=px_m(body,PHYS_GAIN[md]*(sustained?4:1),20);a=soft_knee(a,24000);
  a=drift_osc_mix(t,v,a,extra_inc);
  if(p[P_E6])a=softclip(((a>>2)*((32768+p[P_E6]*512)>>2))>>11);
  a=soft_knee(analog_tap(&flt,ANALOG_K(p[P_E5]),a>>1,&ic1,&ic2,(uint32_t)p[P_FTYPE]),16000)*2;
  out[i]+=voice_amp(a,m,i)*2;
 }
 v->s[0]=ic1;v->s[1]=ic2;
}
