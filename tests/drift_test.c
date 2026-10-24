/* SPDX-License-Identifier: GPL-3.0-only */
#include "../web/emu/felucca_web.c"
#include <assert.h>
static void advance_ms(uint32_t ms){uint32_t i;for(i=0;i<ms*441u/10u;i+=128)web_render(128);}
static void tap(uint32_t b){web_buttons(1u<<b);advance_ms(40);web_buttons(0);advance_ms(40);}
static void test_home(void){if(name_on() || drift_dialog || drift_page!=D_HOME)tap(B_HOME);}
static void screen_file(const char *name){FILE *f=fopen(name,"wb");assert(f);uint32_t i;fprintf(f,"P6\n240 240\n255\n");for(i=0;i<240*240;i++){uint16_t c=web_fb[i];c=(c>>8)|(c<<8);uint8_t rgb[3]={(c>>11)*255/31,((c>>5)&63)*255/63,(c&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);}
/* DRIFT 0.7 behavioral checks: actual UI edges and persistent records. */
static int used_count(void){int n=0;for(int k=0;k<UP_SLOTS;k++)n+=!!up_used(k);return n;}
static void set_save_name(const char *s){str_cpy(nm.s,s,sizeof nm.s);nm.len=nm.cur=str_len(nm.s);}
static void patch_tests(void){
 test_home();drift_load(256);drift_list_build();drift_select_id(256);
 int count=used_count();up_rec_t old=*up_rec(0);trk[0].p[P_DMOD]=99;
 tap(B_SAVE);tap(B_SAVE);assert(drift_dialog==D_DIALOG_OVERWRITE && !drift_yes);
 screen_file("build/drift-overwrite.ppm");tap(B_HOME);assert(name_on() && !drift_dialog && !memcmp(&old,up_rec(0),sizeof old));
 tap(B_SAVE);web_enc(EN_K1,1);advance_ms(40);assert(drift_yes);tap(B_HOME);
 assert(!name_on()&&!drift_dialog&&used_count()==count&&trk[0].p[P_DMOD]==99);
 drift_load(0);drift_load(256);assert(trk[0].p[P_DMOD]==99);
 tap(B_SAVE);set_save_name(" my ambient ");tap(B_SAVE);assert(drift_dialog==D_DIALOG_OVERWRITE&&drift_target==256&&!drift_yes);tap(B_HOME);tap(B_HOME);
 // New FX fields and old format migration preserve the previously saved sound.
 trk[0].p[P_DDELAY]=2;trk[0].p[P_DCOLOR]=110;trk[0].p[P_DMOD]=72;song.g[G_RTYPE]=5;drift_global_store();assert(!up_store(0,"MY AMBIENT"));
 drift_load(0);drift_load(256);assert(trk[0].p[P_DDELAY]==2&&trk[0].p[P_DCOLOR]==110&&trk[0].p[P_DMOD]==72&&song.g[G_RTYPE]==5);
 trk[0].p[P_DFMT]=0;trk[0].p[P_D2W]=4;trk[0].p[P_DLED]=1;assert(!up_store(2,"OLD FORMAT"));drift_load(258);
 assert(trk[0].p[P_DFMT]==1&&trk[0].p[P_D2W]==4&&trk[0].p[P_DLED]&&trk[0].p[P_DDELAY]==0&&trk[0].p[P_DMOD]==32);
 // Delete defaults to No, including a new dialog after a previous Yes.
 drift_load(256);drift_list_build();drift_select_id(256);count=used_count();tap(B_HOME);
 assert(drift_dialog==D_DIALOG_DELETE&&!drift_yes);screen_file("build/drift-delete.ppm");tap(B_HOME);assert(up_used(0)&&used_count()==count);
 tap(B_REC);assert(drift_fav(256));tap(B_HOME);web_enc(EN_K1,1);advance_ms(40);
 // Running transport cannot erase; can stop directly inside the dialog.
 tap(B_PLAY);assert(song.playing);tap(B_HOME);assert(up_used(0)&&drift_dialog);tap(B_PLAY);tap(B_HOME);
 assert(!up_used(0)&&!drift_fav(256)&&used_count()==count-1&&!drift_dialog);
 // Factory delete is a persistent tombstone; no user-bank slot is lost.
 drift_load(1);drift_page=D_HOME;drift_list_build();drift_select_id(1);count=used_count();tap(B_HOME);assert(!drift_yes);web_enc(EN_K1,1);advance_ms(40);tap(B_HOME);
 assert(favorite_has(1,1)&&used_count()==count);settings_init();drift_list_build();for(unsigned k=0;k<drift_list_n;k++)assert(drift_list[k]!=1);
 // Same-name factory save becomes a user patch in place of the factory entry.
 drift_load(2);drift_select_id(2);tap(B_REC);tap(B_SAVE);tap(B_SAVE);assert(drift_dialog==D_DIALOG_OVERWRITE&&drift_target==2&&!drift_yes);
 web_enc(EN_K1,1);advance_ms(40);tap(B_HOME);assert(favorite_has(1,2)&&trk[0].user&&drift_fav(drift_id()));
 // A full bank still allows overwrite, while a new name cannot erase a slot.
 for(int k=0;k<UP_SLOTS;k++)if(!up_used(k)){char name[13];sprintf(name,"FILL %02d",k);assert(!up_store(k,name));}
 drift_load(256);count=used_count();tap(B_SAVE);tap(B_SAVE);assert(drift_dialog==D_DIALOG_OVERWRITE);web_enc(EN_K1,1);advance_ms(40);tap(B_HOME);assert(used_count()==count);
 old=*up_rec(0);tap(B_SAVE);set_save_name("NEW FULL");tap(B_SAVE);assert(name_on()&&!drift_dialog&&used_count()==count&&!memcmp(&old,up_rec(0),sizeof old));tap(B_HOME);
 printf("PASS: patch overwrite/No/case/trim, deletion/No/favorites, transport guard, persistent factory hide/replace, full-bank overwrite and old-format migration.\n");
}
static void fx_reset_test(void){memset(&fx,0,sizeof fx);memset(&hl,0,sizeof hl);memset(&df,0,sizeof df);memset(dly_buf,0,sizeof dly_buf);memset(cho_buf,0,sizeof cho_buf);rev_clear();memset(&pf,0,sizeof pf);}
static void fx_tests(void){
 int32_t zero[CTL]={0},in[CTL],out[CTL];uint64_t hashes[6];
 drift_load(0);song.g[G_RSIZE]=100;song.g[G_RDAMP]=60;song.g[G_DMIX]=127;trk[0].p[P_DMOD]=80;
 for(int rt=0;rt<6;rt++){
  fx_reset_test();fx.rtype=rt;song.g[G_RTYPE]=rt;uint64_t hash=1469598103934665603ull;long long energy=0;
  for(int block=0;block<1200;block++){
   memset(in,0,sizeof in);if(!block)in[0]=30000;fx_buses(zero,zero,in,out,CTL);
   for(int i=0;i<CTL;i++){assert(abs(out[i])<262144);energy+=(long long)out[i]*out[i];hash=(hash^(uint32_t)out[i])*1099511628211ull;}
  }
  assert(energy>10000);hashes[rt]=hash;for(int j=0;j<rt;j++)assert(hash!=hashes[j]);
 }
 // Ping-pong first echo exclusively left, second exclusively right.
 fx_reset_test();trk[0].p[P_DDELAY]=1;song.g[G_RTYPE]=0;song.g[G_BPM]=120;song.g[G_DTIME]=3;song.g[G_DFDBK]=90;
 uint32_t len=delay_samples();long long l1=0,r1=0,l2=0,r2=0;
 for(uint32_t b=0;b<3*len/CTL+2;b++){
  memset(in,0,sizeof in);if(!b)in[0]=20000;fx_buses(zero,in,zero,out,CTL);
  for(unsigned i=0;i<CTL;i++){unsigned t=b*CTL+i;int l=out[i]+df.side[i],r=out[i]-df.side[i];if(t>=len&&t<len+16){l1+=abs(l);r1+=abs(r);}if(t>=2*len&&t<2*len+16){l2+=abs(l);r2+=abs(r);}}
 }
 assert(l1>5000&&r1<20&&r2>2000&&l2<20);
 // Reverse really reverses a ramp, with the edge fade excluded.
 fx_reset_test();trk[0].p[P_DDELAY]=2;song.g[G_DFDBK]=0;int prev=30000,seen=0;
 for(uint32_t b=0;b<2*len/CTL+2;b++){
  for(unsigned i=0;i<CTL;i++){unsigned t=b*CTL+i;in[i]=t<len?(int)t*4:0;}fx_buses(zero,in,zero,out,CTL);
  for(unsigned i=0;i<CTL;i++){unsigned t=b*CTL+i;if(t>len+250&&t<2*len-250){assert(out[i]<=prev);prev=out[i];seen++;}}
 }assert(seen>1000);
 // Pitch heads produce the requested octave: strongest sine correlation wins.
 for(int down=0;down<2;down++){
  memset(&df,0,sizeof df);double power[3]={0},re[3]={0},im[3]={0};int hz[3]={220,440,880};
  for(int i=0;i<44100;i++){int x=(int)(10000*sin(2*3.141592653589793*440*i/44100));int y=drift_pitch(x,down);if(i>4096)for(int j=0;j<3;j++){double a=2*3.141592653589793*hz[j]*i/44100;re[j]+=y*cos(a);im[j]+=y*sin(a);}}
  for(int j=0;j<3;j++)power[j]=re[j]*re[j]+im[j]*im[j];assert(power[down?0:2]>power[1]*10);
 }
 // UI page 3 routes every knob and saves the reverb type.
 drift_page=D_HOME;tap(B_FX);assert(drift_page==D_FX);
 web_enc(EN_K1,127);web_enc(EN_K3,127);web_enc(EN_K4,127);advance_ms(40);
 assert(trk[0].p[P_DLY]==127 && song.g[G_DFDBK]==120 && trk[0].p[P_DDELAY]==2);screen_file("build/drift-delay.ppm");
 tap(B_FX);assert(drift_page==D_SPACE);web_enc(EN_K1,127);web_enc(EN_K2,127);web_enc(EN_K3,127);web_enc(EN_K4,127);advance_ms(40);
 assert(trk[0].p[P_REV]==127 && song.g[G_RSIZE]==127 && song.g[G_RTYPE]==5 && trk[0].p[P_DCOLOR]==127);screen_file("build/drift-reverb.ppm");
 tap(B_FX);assert(drift_page==D_TYPES);web_enc(EN_K1,127);web_enc(EN_K2,127);web_enc(EN_K3,127);web_enc(EN_K4,127);advance_ms(40);
 assert(trk[0].p[P_CHOR]==127 && song.g[G_RDAMP]==127 && trk[0].p[P_DMOD]==127 && trk[0].p[P_LEVEL]==127);screen_file("build/drift-misc.ppm");
 // Worst controls, eight voices, all 18 type combinations: bounded audio.
 for(int dt=0;dt<3;dt++)for(int rt=0;rt<6;rt++){
  drift_load(17);trk[0].p[P_DDELAY]=dt;song.g[G_RTYPE]=rt;trk[0].p[P_DMOD]=127;song.g[G_RSIZE]=127;song.g[G_DFDBK]=120;trk[0].p[P_REV]=127;trk[0].p[P_DLY]=127;
  web_keys(0x5555);advance_ms(1200);for(int i=0;i<128;i++)assert(isfinite(out_l[i])&&fabsf(out_l[i])<=1&&isfinite(out_r[i])&&fabsf(out_r[i])<=1);web_keys(0);advance_ms(100);
 }
 printf("PASS: six distinct reverb impulse responses, stereo ping-pong, backwards delay ramp, octave pitch spectra, FX 3 routing, bounded eight-voice audio across 18 type combinations.\n");
}
int main(void){
 web_nor_erase();
 for(int k=0;k<3;k++){char path[80];sprintf(path,"build/sample-fixtures/%d.bin",k);FILE *f=fopen(path,"rb");assert(f);assert(fread(nor+SMP_USER_BASE+k*SMP_USER_SIZE,1,SMP_USER_SIZE,f)==SMP_USER_SIZE);fclose(f);}
 web_boot();advance_ms(1000);assert(drift_page==D_HOME&&song.sel==0&&!song.playing);
 assert(song.g[G_BPM]==80 && !trk[0].p[P_D2W] && trk[0].p[P_DLV1]==127);
 ui.force=1;drift_draw();uint32_t draws=web_draws;ui.force=1;drift_draw();assert(web_draws==draws);
 trk[0].p[P_DONLY]=1;assert(kb_map(&trk[0],7)==60);assert(kb_map(&trk[0],8)==KB_SILENT);
 for(int root=0;root<12;root++){trk[0].p[P_ROOT]=root;for(int n=0;n<128;n++)assert(drift_scale_contains(&trk[0],n)==!!(SCALE_MASK[2]&(1u<<((n+12-root)%12))));}
 trk[0].p[P_ROOT]=0;trk[0].p[P_DONLY]=0;
 tap(B_GLO);tap(B_GLO);assert(drift_page==D_OSC2);screen_file("build/drift-osc2-off.ppm");
 web_enc(EN_K1,4);advance_ms(40);assert(trk[0].p[P_D2W]==4);screen_file("build/drift-osc2.ppm");
 trk[0].p[P_DLV1]=0;trk[0].p[P_ATK]=0;trk[0].p[P_SUS]=127;trk[0].p[P_E4]=100;
 web_keys(1u<<7);advance_ms(500);float peak2=0;for(int n=0;n<128;n++){float a=fabsf(out_l[n]);if(a>peak2)peak2=a;}assert(peak2>0.0001f);web_keys(0);
 tap(B_SCL);tap(B_SCL);assert(drift_page==D_SCL2);web_enc(EN_K1,1);web_enc(EN_K2,1);advance_ms(40);assert(trk[0].p[P_DLED] && trk[0].p[P_DONLY]);screen_file("build/drift-scale2.ppm");
 drift_leds();for(int k=0;k<27;k++){uint8_t q=led_pos[k+14];if(q!=255)assert(!!(fm1_led[q>>3]&(1u<<(q&7)))==drift_scale_contains(&trk[0],53+k));}
 last_note=60;input_on(&trk[0],61,100);assert(last_note==60);
 trk[0].p[P_D2W]=4;trk[0].p[P_D2T]=71;drift_global_store();up_store(3,"DUAL TEST");drift_load(259);assert(trk[0].p[P_D2W]==4 && trk[0].p[P_D2T]==71 && trk[0].p[P_DLED] && trk[0].p[P_DONLY]);
 trk[0].p[P_DFMT]=127;up_store(4,"LEGACY");drift_load(260);assert(!trk[0].p[P_D2W] && trk[0].p[P_DLV1]==127 && !trk[0].p[P_DONLY]);

 drift_load(0);test_home();assert(!trk[0].p[P_D2W]);
 // Octave button brightness and zero state, in both directions.
 for(int dir=-1;dir<=1;dir+=2){
  uint32_t button=dir<0?B_OCTDN:B_OCTUP,bit=1u<<button;
  for(int n=1;n<=3;n++){tap(button);assert(song.octave==dir*n);assert(!!(web_dim_buttons()&bit)==(n==1));assert(!!(web_mid_buttons()&bit)==(n==2));assert(!!(web_lit_buttons()&bit)==(n==3));}
  screen_file(dir<0?"build/drift-oct-minus.ppm":"build/drift-oct-plus.ppm");
  for(int n=0;n<3;n++)tap(dir<0?B_OCTUP:B_OCTDN);
  assert(song.octave==0);uint32_t both=(1u<<B_OCTUP)|(1u<<B_OCTDN);assert(!((web_lit_buttons()|web_dim_buttons()|web_mid_buttons())&both));
 }
 // Modulation destination changes only the requested route; base patch values stay fixed.
 tap(B_LFO);web_enc(EN_K3,1);web_enc(EN_K4,100);advance_ms(40);assert(trk[0].p[P_DDEST]==D_LFO_DELAY && trk[0].p[P_LD_AMP]==100);
 trk[0].p[P_DLY]=64;trk[0].lfo_fade=32767;trk[0].lfo_val=32767;int high=drift_delay_send(&trk[0]);trk[0].lfo_val=-32768;int low=drift_delay_send(&trk[0]);assert(high>low && trk[0].p[P_DLY]==64);
 vmod_t vm={0};vm.pitch16=60*16;trk[0].p[P_D2T]=64;trk[0].p[P_DDEST]=D_LFO_OSC2_PITCH;trk[0].lfo_val=32767;uint32_t up=drift_osc_inc(&trk[0],&vm);trk[0].lfo_val=-32768;uint32_t down=drift_osc_inc(&trk[0],&vm);assert(up>down);
 trk[0].p[P_DDEST]=D_LFO_OSC2_DETUNE;uint32_t detdown=drift_osc_inc(&trk[0],&vm);trk[0].lfo_val=32767;uint32_t detup=drift_osc_inc(&trk[0],&vm);assert(detup>detdown && detup-detdown<up-down);
 trk[0].p[P_LD_AMP]=0;assert(drift_osc_inc(&trk[0],&vm)==pitch_inc(vm.pitch16));
 trk[0].p[P_DDEST]=D_LFO_VOLUME;assert(drift_delay_send(&trk[0])==64*258);
 trk[0].p[P_DDEST]=D_LFO_OSC2_DETUNE;trk[0].p[P_LD_AMP]=80;ui.force=1;drift_draw();screen_file("build/drift-lfo-assign.ppm");
 drift_global_store();up_store(3,"LFO TEST");drift_load(259);assert(trk[0].p[P_DDEST]==D_LFO_OSC2_DETUNE && trk[0].p[P_LD_AMP]==80);
 drift_load(0);test_home();
 assert(SMP_USER_BASE+3*SMP_USER_SIZE==0xDC000u);
 for(int i=1;i<4;i++)assert(trk[i].p[P_MUTE]);for(int i=0;i<3;i++)assert(usr_nz[i]==1);
 assert(drift_list_n==23);tap(B_REC);assert(drift_fav(0)&&drift_list_n==24);screen_file("build/drift-home.ppm");
 tap(B_GLO);assert(drift_page==D_OSC);screen_file("build/drift-osc.ppm");tap(B_EDIT);assert(drift_page==D_FILTER);
 tap(B_ENV);assert(drift_page==D_ENV);screen_file("build/drift-env.ppm");
 tap(B_FX);assert(drift_page==D_FX);screen_file("build/drift-fx.ppm");tap(B_FX);assert(drift_page==D_SPACE);screen_file("build/drift-space.ppm");
 tap(B_ARP);assert(drift_page==D_SEQ1);screen_file("build/drift-seq1.ppm");tap(B_SEQ);assert(drift_page==D_SEQ2);screen_file("build/drift-seq2.ppm");
 assert(trk[0].p[P_DLEN]==4&&drift_loop_samples()==beat_samples()*16);
 for(int i=0;i<8;i++){assert(drift_gen.enabled[i]);assert(drift_gen.phase[i]==i*8192);}

 for(int i=0;i<drift_gen.count;i++)assert(drift_gen.note[i]%12==trk[0].p[P_ROOT]);
 tap(B_SCL);screen_file("build/drift-scale.ppm");trk[0].p[P_LD_FLT]=55;trk[0].p[P_LD_PIT]=-40;trk[0].p[P_LD_AMP]=110;tap(B_LFO);screen_file("build/drift-lfo.ppm");tap(B_EDIT);screen_file("build/drift-filter.ppm");
 web_enc(EN_ALGO,-4);advance_ms(40);assert(drift_overlay==1);screen_file("build/drift-overlay.ppm");advance_ms(600);assert(!drift_overlay && drift_page==D_FILTER);
 web_enc(EN_SELECT,4);advance_ms(40);assert(drift_overlay==2);advance_ms(600);assert(!drift_overlay);
 tap(B_PLAY);advance_ms(700);uint32_t prior=drift_gen.pos;trk[0].p[P_DPITCH]=5;advance_ms(50);assert(drift_gen.pos>prior);tap(B_PLAY);
 // Every settings combination is bounded, ordered and deterministic.
 trk[0].p[P_ROOT]=2;trk[0].p[P_SCALE]=2;trk[0].p[P_DMIN]=48;trk[0].p[P_DMAX]=84;
 for(int trial=0;trial<500;trial++){
  trk[0].p[P_DPITCH]=trial%22;trk[0].p[P_DOCT]=(trial*3)%8;trk[0].p[P_DRHYTHM]=(trial*7)%101;trk[0].p[P_DRATE]=100;drift_generate();
  for(int i=0;i<drift_gen.count;i++){int nt=drift_gen.note[i];assert(nt>=48&&nt<=84);assert(SCALE_MASK[2]&(1u<<((nt+10)%12)));if(i)assert(drift_gen.phase[i]>=drift_gen.phase[i-1]);}
 }
 trk[0].p[P_DPITCH]=7;trk[0].p[P_DOCT]=1;trk[0].p[P_DRHYTHM]=0;trk[0].p[P_DRATE]=0;drift_generate();
 uint8_t ordered[32];memcpy(ordered,drift_gen.note,32);trk[0].p[P_DRHYTHM]=100;drift_generate();assert(memcmp(ordered,drift_gen.note,drift_gen.count));
 trk[0].p[P_DRATE]=100;drift_generate();int irregular=0;for(int i=0;i<drift_gen.count;i++)irregular|=drift_gen.phase[i]!=i*8192;assert(irregular);
 trk[0].p[P_DLEN]=1;trk[0].p[P_DPROB]=60;drift_generate();
 uint8_t notes[32],enabled[32],vel[32];uint16_t phase[32];
 memcpy(notes,drift_gen.note,32);memcpy(enabled,drift_gen.enabled,32);memcpy(vel,drift_gen.velocity,32);memcpy(phase,drift_gen.phase,64);
 uint32_t serial=drift_gen.serial;tap(B_PLAY);advance_ms(20000);assert(drift_gen.cycle>=5&&drift_gen.serial==serial);
 assert(!memcmp(notes,drift_gen.note,32)&&!memcmp(enabled,drift_gen.enabled,32)&&!memcmp(vel,drift_gen.velocity,32)&&!memcmp(phase,drift_gen.phase,64));
 tap(B_PLAY);tap(B_PLAY);advance_ms(50);assert(drift_gen.serial==serial);tap(B_PLAY);
 trk[0].p[P_DRHYTHM]=20;advance_ms(50);assert(drift_gen.serial==serial+1); // even while stopped
 trk[0].p[P_DMIN]=61;trk[0].p[P_DMAX]=61;trk[0].p[P_ROOT]=0;trk[0].p[P_SCALE]=1;advance_ms(50);assert(!drift_gen.pool_n);
 trk[0].p[P_DMIN]=48;trk[0].p[P_DMAX]=84;trk[0].p[P_DPROB]=0;tap(B_PLAY);advance_ms(500);for(int i=0;i<128;i++)assert(!drift_gen.off[i]);tap(B_PLAY);
 trk[0].p[P_DPROB]=100;trk[0].p[P_DRATE]=0;tap(B_PLAY);advance_ms(100);int held=0;for(int i=0;i<128;i++)held+=!!drift_gen.off[i];assert(held);tap(B_PLAY);for(int i=0;i<128;i++)assert(!drift_gen.off[i]);
 song.g[G_BPM]=77;song.g[G_RSIZE]=93;tap(B_SAVE);assert(name_on());str_cpy(nm.s,"MY AMBIENT",sizeof nm.s);nm.len=nm.cur=10;tap(B_SAVE);assert(up_used(0));
 song.g[G_BPM]=100;drift_load(256);assert(song.g[G_BPM]==77&&song.g[G_RSIZE]==93);
 tap(B_SAVE);assert(nm.slot==1);test_home();assert(!up_used(1));
 // All six slots sound through the analog waveform dispatch, including slots 5/6.
 for(int k=0;k<3;k++){
  panic_req=15;trk[0].p[P_E0]=7+k;trk[0].p[P_E2]=100;trk[0].p[P_E4]=100;trk[0].p[P_ATK]=0;trk[0].p[P_SUS]=127;trk[0].p[P_REL]=0;advance_ms(200);
  voice_t v={0};v.note=60;v.age=1;drift_sample_on(&trk[0],&v);assert((v.s[4]&0x8000)&&!v.s[6]);assert(smp_zone(v.s[4])==&usr_zone[k][0]);
  web_keys(1u<<7);advance_ms(300);float peak=0;for(int i=0;i<128;i++){float a=out_l[i]<0?-out_l[i]:out_l[i];if(a>peak)peak=a;assert(a<=1.0f);}assert(peak>0.0001f);web_keys(0);advance_ms(200);
 }
 tap(B_GLO);screen_file("build/drift-sample3.ppm");
 drift_load(16);advance_ms(100);assert(trk[0].p[P_E0]==6 && trk[0].p[P_DMAX]==47 && trk[0].p[P_VOICE]==V_LEGATO);
 test_home();screen_file("build/drift-sub.ppm");web_keys(1u<<7);advance_ms(200);FILE *bass=fopen("build/drift-sub.f32","wb");assert(bass);
 for(int j=0;j<345;j++){web_render(128);for(int i=0;i<128;i++)assert(out_l[i]>=-1 && out_l[i]<=1);fwrite(out_l,sizeof(float),128,bass);}fclose(bass);web_keys(0);
 drift_load(20);advance_ms(300);assert(trk[0].preset==20 && trk[0].p[P_D2W]==3 && trk[0].p[P_DDEST]==D_LFO_OSC2_DETUNE);
 test_home();screen_file("build/drift-air-salt.ppm");web_keys(1u<<7);advance_ms(700);float airpeak=0;FILE *air=fopen("build/drift-air-salt.f32","wb");assert(air);
 for(int j=0;j<1400;j++){web_render(128);for(int k=0;k<128;k++){assert(isfinite(out_l[k]) && fabsf(out_l[k])<=1);if(fabsf(out_l[k])>airpeak)airpeak=fabsf(out_l[k]);}fwrite(out_l,sizeof(float),128,air);}fclose(air);assert(airpeak>0.001f);web_keys(0);advance_ms(4000);
 patch_tests();fx_tests();
 printf("PASS: AIR & SALT audio/preset, octave LEDs at three brightness levels/zero, LFO target routing/depth/persistence, stable buffered display, 80 BPM, scale LED mask and key/MIDI gating, oscillator 2 OFF/wave/audio, new parameter persistence and legacy migration, single-part, favourites/save, routes, horizontal UI snapshots, bars, Euclidean grid, 500 bounded settings, fixed replay across cycles/restart, regeneration on edit, probability, empty range, stop, all three sample voices and non-overlapping flash slots.\n");return 0;
}
