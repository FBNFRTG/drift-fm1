/* SPDX-License-Identifier: GPL-3.0-only
 * DRIFT additions, 2026. One playable part; four legacy storage slots retained.
 */
static int up_store_cat(uint32_t k, const char *name, uint32_t cat);
static int drift_up_delete(uint32_t k);
enum { D_HOME, D_OSC, D_FILTER, D_ENV, D_LFO, D_FX, D_SPACE, D_SCL, D_SEQ1, D_SEQ2, D_OSC2, D_SCL2, D_TYPES };
static uint8_t drift_page, drift_dirty, drift_ready;
static int drift_row;
static uint32_t drift_draw_ms,drift_overlay_ms;
static uint8_t drift_overlay;
static const char *const D_TITLE[]={"PATCHES","OSCILLATOR 1","FILTER","ENVELOPE","MODULATION","EFFECTS","SPACE","SCALE","SEQUENCE 1","SEQUENCE 2","OSCILLATOR 2","SCALE KEYS","FX TYPES"};
static const uint8_t D_PARAMS[][4]={
 {P_E4,P_E5,P_ATK,P_REL},{P_E0,P_E1,P_E2,P_DLV1},{P_E4,P_E5,P_ED_FLT,P_E6},
 {P_ATK,P_DEC,P_SUS,P_REL},{P_LRATE,P_LD_FLT,P_DDEST,P_LD_AMP},
 {P_CHOR,P_DLY,P_REV,P_LEVEL},{G_RSIZE,G_RDAMP,G_DTIME,G_DFDBK},
 {P_ROOT,P_SCALE,P_DMIN,P_DMAX},{P_DLEN,P_DNOTES,P_DDUR,P_DPROB},
 {P_DPITCH,P_DOCT,P_DRHYTHM,P_DRATE},
 {P_D2W,P_D2T,P_D2LV,255},{P_DLED,P_DONLY,255,255},{P_DDELAY,P_DR5,P_DCOLOR,P_DMOD}
};
static const char *const D_LABEL[][4]={
 {"CUT","RES","ATK","REL"},{"WAVE","DETUNE","MIX","LEVEL"},
 {"CUT","RES","ENV","DRIVE"},{"ATTACK","DECAY","SUSTAIN","RELEASE"},
 {"RATE","FILTER","TARGET","DEPTH"},{"CHORUS","DELAY","REVERB","LEVEL"},
 {"SIZE","DAMP","TIME","FDBK"},{"ROOT","SCALE","MIN","MAX"},
 {"BARS","NOTES","DUR /16","PROB %"},{"DIR","INTERVAL","CHAOS","TIME"},
 {"WAVE","TUNE","LEVEL",""},{"LED","SCALE ONLY","",""},{"DELAY","REVERB","TONE","MOTION"}
};
static const char *const D_DELAYS[]={"CLASSIC","PING PONG","REVERSE"};
static const char *const D_WAVES[]={"OFF","SAW","SQUARE","TRI","SINE","PULSE"};
static const char *const D_LFO_TARGETS[]={"VOLUME","DELAY","O2 FREQ","O2 DET"};
static const param_desc_t *drift_desc(uint32_t id){
 static const param_desc_t delay={.fmt=F_ENUM,.min=0,.max=2,.names=D_DELAYS};
 if(id==P_DDELAY)return &delay;if(id==P_DR5)return &GP[G_RTYPE];
 static const param_desc_t target={.fmt=F_ENUM,.min=0,.max=3,.names=D_LFO_TARGETS};
 if(id==P_DDEST)return &target;
 static const param_desc_t led={.fmt=F_ENUM,.min=0,.max=1,.names=N_ONOFF};
 static const param_desc_t wave={.fmt=F_ENUM,.min=0,.max=5,.names=D_WAVES};
 static const param_desc_t level={.fmt=F_PCT,.min=0,.max=127};
 static const param_desc_t tune={.fmt=F_INT,.min=40,.max=88};
 if(id==P_DLED || id==P_DONLY)return &led;
 if(id==P_D2W)return &wave;if(id==P_D2T)return &tune;
 if(id==P_DLV1 || id==P_D2LV || id==P_DCOLOR || id==P_DMOD)return &level;
 return id>=P_E0?&ENG_ANALOG.edit[id-P_E0]:&TP[id];
}
static void drift_new_params(void){
 int16_t *p=trk[0].p;p[P_DLED]=0;p[P_DONLY]=0;p[P_DLV1]=127;
 p[P_DDEST]=D_LFO_VOLUME;p[P_D2W]=0;p[P_D2T]=64;p[P_D2LV]=80;p[P_DFMT]=1;p[P_DDELAY]=0;p[P_DCOLOR]=70;p[P_DMOD]=32;
}
static uint16_t drift_list[128];
static uint32_t drift_list_n;
static uint32_t drift_id(void){return trk[0].user ? 256u+trk[0].user-1u : trk[0].preset;}
static int drift_fav(uint32_t id){return favorite_has(id>=256?NENGINES:0,id>=256?id-256:id);}
static void drift_list_build(void) {
 uint32_t pass,i;drift_list_n=0;
 for(pass=0;pass<2;pass++) {
  for(i=0;i<ENG_ANALOG.npresets;i++)if(!favorite_has(1,i) && (pass || drift_fav(i)))drift_list[drift_list_n++]=(uint16_t)i;
  for(i=0;i<UP_SLOTS;i++)if(up_used(i) && up_engine(i)==0 && (pass || drift_fav(256+i)))drift_list[drift_list_n++]=(uint16_t)(256+i);
 }
 if(drift_row>=(int)drift_list_n)drift_row=(int)drift_list_n-1;
}
static void drift_name(uint32_t id,char *out) {
 if(id>=256)up_name(id-256,out);else str_cpy(out,ENG_ANALOG.presets[id%ENG_ANALOG.npresets].name,16);
}
static void drift_select_id(uint32_t id) { uint32_t i; for(i=0;i<drift_list_n;i++)if(drift_list[i]==id){drift_row=(int)i;break;} }
static const uint8_t D_GLOBALS[]={G_RSIZE,G_RDAMP,G_DTIME,G_DFDBK,G_BPM,G_RTYPE};
static void drift_global_store(void){uint32_t i;for(i=0;i<6;i++)trk[0].p[P_DR0+i]=song.g[D_GLOBALS[i]];}
static void drift_global_load(void){uint32_t i;for(i=0;i<6;i++)song.g[D_GLOBALS[i]]=(int16_t)clamp(trk[0].p[P_DR0+i],GP[D_GLOBALS[i]].min,GP[D_GLOBALS[i]].max);}
static void drift_load(uint32_t id) {
 song.sel=0; panic_req=15;
 if(id>=256)up_load(id-256);
 else {set_engine_of(&trk[0],0);apply_preset_to(&trk[0],id);trk[0].p[P_SCALE]=2;drift_new_params();if(id>=17 && id<=19){trk[0].p[P_D2W]=id==19?3:4;trk[0].p[P_D2T]=id==18?71:id==19?52:64;}if(id==16 || id==19){trk[0].p[P_DMIN]=24;trk[0].p[P_DMAX]=47;}}
 if(id==20){
  int16_t *p=trk[0].p;
  p[P_D2W]=3;p[P_D2T]=64;p[P_D2LV]=40;p[P_DLV1]=114;
  p[P_LRATE]=23;p[P_LD_FLT]=3;p[P_DDEST]=D_LFO_OSC2_DETUNE;p[P_LD_AMP]=9;
  p[P_ROOT]=2;p[P_SCALE]=1;
 }
 if(trk[0].p[P_DFMT]>1)drift_new_params();
 if(!trk[0].p[P_DFMT]){trk[0].p[P_DDELAY]=0;trk[0].p[P_DCOLOR]=70;trk[0].p[P_DMOD]=32;trk[0].p[P_DFMT]=1;}
 drift_global_load();
 trk[0].p[P_AMODE]=0; trk[0].p[P_CHRD]=0; drift_dirty=0; ui.force=1;
}
static void drift_init(void) {
 uint32_t i; song.sel=0;song.rec=0;song.playing=0;
 song.g[G_BPM]=80;song.g[G_ROUTE]=ROUTE_SEL;song.g[G_RTYPE]=2;
 song.g[G_RSIZE]=100;song.g[G_RDAMP]=72;
 for(i=0;i<NTRK;i++){track_defaults_steps(&trk[i]);trk[i].p[P_MUTE]=i!=0;trk[i].p[P_AMODE]=0;}
 set_engine_of(&trk[0],0);apply_preset_to(&trk[0],0);trk[0].p[P_SCALE]=2;
 drift_new_params();
 trk[0].engine=0;eng_mem_claim(&trk[0]);
 drift_page=D_HOME;drift_row=0;drift_dirty=0;drift_ready=1;
 drift_list_build();if(drift_list_n)drift_load(drift_list[0]);drift_select_id(drift_id());
}
enum { D_DIALOG_NONE, D_DIALOG_DELETE, D_DIALOG_OVERWRITE };
static uint8_t drift_dialog,drift_yes;
static uint32_t drift_target;
static char drift_save_name[NM_LEN+1];
static int drift_same_name(const char *a,const char *b){
 while(*a && *b){char x=*a++,y=*b++;if(x>='a'&&x<='z')x-=32;if(y>='a'&&y<='z')y-=32;if(x!=y)return 0;}return *a==*b;
}
static int drift_free_slot(void){uint32_t k;for(k=0;k<UP_SLOTS;k++)if(!up_used(k))return k;return -1;}
static int drift_named_patch(const char *name){
 char b[16];uint32_t k;
 for(k=0;k<UP_SLOTS;k++)if(up_used(k)&&up_engine(k)==0){up_name(k,b);if(drift_same_name(name,b))return 256+k;}
 for(k=0;k<ENG_ANALOG.npresets;k++)if(!favorite_has(1,k)&&drift_same_name(name,ENG_ANALOG.presets[k].name))return k;
 return -1;
}
static void drift_save_patch(int replace){
 int slot=replace && drift_target>=256?(int)drift_target-256:drift_free_slot();
 if(transport_busy()){ui_message("STOP TO SAVE");return;}
 if(slot<0){ui_message("PATCH BANK FULL");return;}
 drift_global_store();
 if(up_store_cat(slot,drift_save_name,nm.cat)){ui_message("SAVE FAILED");return;}
 if(replace && drift_target<256){
  favorite_set(NENGINES,slot,drift_fav(drift_target));favorite_set(0,drift_target,0);favorite_set(1,drift_target,1);settings_save();
 }
 name_close();drift_dialog=0;song.grid=0;drift_load(256+slot);drift_list_build();drift_select_id(drift_id());drift_page=D_HOME;ui.force=1;
}
static void drift_prepare_save(void){
 uint32_t a=0,z;nm_commit();
 for(z=nm.len;z&&nm.s[z-1]==' ';z--) {}
 while(a<z&&nm.s[a]==' ')a++;
 uint32_t n=z-a;for(uint32_t i=0;i<n;i++)drift_save_name[i]=nm.s[a+i];drift_save_name[n]=0;
 if(!n){ui_message("ENTER PATCH NAME");return;}
 int target=drift_named_patch(drift_save_name);
 if(target>=0){drift_target=target;drift_dialog=D_DIALOG_OVERWRITE;drift_yes=0;ui.force=1;return;}
 drift_save_patch(0);
}
static void drift_delete_patch(void){
 if(transport_busy()){ui_message("STOP TO DELETE");return;}
 if(drift_target>=256){if(drift_up_delete(drift_target-256)){ui_message("DELETE FAILED");return;}}
 else{favorite_set(1,drift_target,1);favorite_set(0,drift_target,0);settings_save();}
 drift_dialog=0;song.grid=0;drift_list_build();
 if(drift_list_n)drift_load(drift_list[clamp(drift_row,0,(int)drift_list_n-1)]);
 else{drift_load(15);trk[0].user=0;drift_row=0;} /* INIT remains playable when list is empty */
 drift_page=D_HOME;ui.force=1;
}
static void drift_input(void) {
 uint32_t pressed=fm1_input_edges(0),notes=fm1_input_note_edges(),b,k;int32_t s;
 song.sel=0; song.rec=0;
 settings_later_poll();
 if(drift_dialog){
  s=panel_enc(EN_K1);if(s){drift_yes=(uint8_t)!drift_yes;ui.force=1;}enc_drop();song.grid=2;
  if((pressed>>panel.btn[B_PLAY])&1u)transport_req=song.playing?2:1;
  if((pressed>>panel.btn[B_HOME])&1u){
   if(!drift_yes){drift_dialog=0;song.grid=name_on()?2:0;ui.force=1;}
   else if(drift_dialog==D_DIALOG_DELETE)drift_delete_patch();else drift_save_patch(1);
  }
  return;
 }
 if(name_on()) {
  uint32_t cancel=((pressed>>panel.btn[B_OCTDN])&1u)|((pressed>>panel.btn[B_HOME])&1u);
  uint32_t save=((pressed>>panel.btn[B_OCTUP])&1u)|((pressed>>panel.btn[B_SAVE])&1u);
  name_input(notes,cancel?1:0);
  if(name_on() && save)drift_prepare_save();
  if(!name_on()){song.grid=0;drift_page=D_HOME;ui.force=1;}
  return;
 }
 song.grid=0;
 for(b=0;b<NB;b++)if((pressed>>panel.btn[b])&1u){
  switch(b){
   case B_HOME: if(drift_page==D_HOME && drift_list_n){drift_target=drift_id();drift_dialog=D_DIALOG_DELETE;drift_yes=0;song.grid=2;}else{drift_page=D_HOME;drift_list_build();drift_select_id(drift_id());}break;
   case B_EDIT:drift_page=D_FILTER;break;
   case B_ENV:drift_page=D_ENV;break;
   case B_LFO:drift_page=D_LFO;break;
   case B_FX:drift_page=drift_page==D_FX?D_SPACE:drift_page==D_SPACE?D_TYPES:D_FX;break;
   case B_GLO:drift_page=drift_page==D_OSC?D_OSC2:D_OSC;break;
   case B_SCL:drift_page=drift_page==D_SCL?D_SCL2:D_SCL;break;
   case B_SEQ:drift_page=D_SEQ2;break;
   case B_ARP:drift_page=D_SEQ1;break;
   case B_PLAY:transport_req=song.playing?2:1;break;
   case B_REC:{uint32_t id=drift_id();favorite_set(id>=256?NENGINES:0,id>=256?id-256:id,!drift_fav(id));settings_save();drift_list_build();drift_select_id(id);break;}
   case B_SAVE:
    if(song.playing){ui_message("STOP TO SAVE");break;}
    for(k=0;k<UP_SLOTS && up_used(k);k++);
    if(k==UP_SLOTS)k=0; /* same-name overwrite still works in a full bank */
    drift_global_store();name_open(NK_USER_SAVE,k);{char label[16];drift_name(drift_id(),label);str_cpy(nm.s,label,sizeof nm.s);nm.len=nm.cur=(uint8_t)str_len(nm.s);}song.grid=2;break;
   case B_OCTDN:song.octave=(int8_t)clamp(song.octave-1,-3,3);break;
   case B_OCTUP:song.octave=(int8_t)clamp(song.octave+1,-3,3);break;
  }
  ui.force=1;
 }
 if(name_on() || drift_dialog)return;
 s=panel_enc(EN_PRESET);
 if(s && drift_list_n){drift_list_build();drift_row=clamp(drift_row+s,0,(int)drift_list_n-1);drift_load(drift_list[drift_row]);}
 s=panel_enc(EN_SELECT);if(s){song.g[G_BPM]=(int16_t)clamp(song.g[G_BPM]+s,40,240);drift_overlay=2;drift_overlay_ms=fm1_ms;drift_dirty=1;ui.force=1;}
 s=panel_enc(EN_ALGO);if(s){trk[0].p[P_FTYPE]=0;trk[0].p[P_E4]=(int16_t)clamp(trk[0].p[P_E4]+s,0,127);drift_overlay=1;drift_overlay_ms=fm1_ms;drift_dirty=1;ui.force=1;}
 for(k=0;k<4;k++)if((s=panel_enc(EN_K1+k))!=0){
  uint32_t id=D_PARAMS[drift_page][k];if(id==255)continue;
  int16_t *v=drift_page==D_SPACE?&song.g[id]:&trk[0].p[id];
  const param_desc_t *d=drift_page==D_SPACE?&GP[id]:drift_desc(id);
  *v=(int16_t)clamp(*v+s,d->min,d->max);
  if(id==P_DR5)song.g[G_RTYPE]=*v;
  if(id==P_DDEST)trk[0].p[P_LD_PIT]=0; /* legacy vibrato ends when a new target is chosen */
  if(id==P_E0 || id==P_DONLY || id==P_SCALE || id==P_ROOT)panic_req|=1; /* wave/sample state cannot be shared */
  if(trk[0].p[P_DNOTES]>trk[0].p[P_DLEN]*16)trk[0].p[P_DNOTES]=trk[0].p[P_DLEN]*16;
  if(id==P_DMIN && trk[0].p[P_DMIN]>trk[0].p[P_DMAX])trk[0].p[P_DMAX]=trk[0].p[P_DMIN];
  if(id==P_DMAX && trk[0].p[P_DMAX]<trk[0].p[P_DMIN])trk[0].p[P_DMIN]=trk[0].p[P_DMAX];
  drift_dirty=1;ui.force=1;
 }
}
static void drift_text(uint32_t y,const char *s,uint16_t fg,uint16_t bg) {draw_text_line(12,y,216,&AF_S,s,fg,bg,0);}
static void drift_heart(uint32_t x,uint32_t y,uint16_t c) {
 static const uint8_t bits[]={54,127,127,127,62,28,8};uint32_t r,k;
 for(r=0;r<7;r++)for(k=0;k<7;k++)if(bits[r]&(1u<<k))lcd_fill(x+k*2,y+r*2,2,2,c);
}
/* Small direct framebuffer primitives: monochrome, no retained colour theme. */
static void drift_line(int x,int y,int x1,int y1,uint16_t color){
 int dx=x1>x?x1-x:x-x1,sx=x<x1?1:-1,dy=y1>y?y-y1:y1-y,sy=y<y1?1:-1,err=dx+dy;
 for(;;){int e;if(x>=0 && x<240 && y>=0 && y<240)lcd_fill(x,y,1,1,color);if(x==x1 && y==y1)break;e=err*2;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}
}
/* Schematic logarithmic frequency-response view, not an FFT measurement. */
static void drift_filter_graph(int top,int bottom){
 int cut=12+216*trk[0].p[P_E4]/127,res=trk[0].p[P_E5],last=top+12;
 for(int x=12;x<=228;x++){
  int d=x-cut,y=top+12;
  if(d>0)y+=(bottom-top)*d/(d+24);
  int dist=d<0?-d:d;if(dist<12)y-=res*(12-dist)*(bottom-top)/127/48;
  y=clamp(y,top,bottom);if(x>12)drift_line(x-1,last,x,y,65535);last=y;
 }
 for(int y=top;y<=bottom;y+=4)lcd_fill(cut,y,1,1,65535);
 drift_line(12,bottom,228,bottom,65535);
}
static int32_t drift_wave_shape(uint32_t wave,uint32_t ph){
 if(wave==1 || wave==4)return ph<0x80000000u?24000:-24000;
 if(wave==2)return osc_tri(ph);
 if(wave==3)return osc_sine(ph);
 return (int32_t)(ph>>16)-32768;
}
static void drift_wave_graph(void){
 const int16_t *p=trk[0].p;uint32_t wave=p[P_E0];
 if(drift_page==D_OSC2){
  if(!p[P_D2W]){draw_text_line(12,143,216,&AF_M,"OSCILLATOR OFF",65535,0,1);return;}
  int last=150;for(int x=0;x<216;x++){int y=150-(int)((int64_t)drift_wave_shape(p[P_D2W]-1,(uint32_t)((uint64_t)x*0x100000000ull/108))*p[P_D2LV]*45/(32768*127));if(x)drift_line(11+x,last,12+x,y,65535);last=y;}return;
 }
 if(wave>=7){
  uint32_t k=wave-7;char label[16];
  if(k>=SMP_USER_SLOTS || !usr_nz[k]){draw_text_line(12,143,216,&AF_M,"EMPTY SAMPLE",65535,0,1);return;}
  const smp_user_hdr_t *h=(const smp_user_hdr_t *)smp_user_xip(k);
  static uint32_t crc,lastslot=999;static uint8_t peak[216];
  if(lastslot!=k || crc!=h->crc){
   voice_t v={0};const smp_zone_t *z=&usr_zone[k][0];
   for(uint32_t x=0;x<216;x++){int a=0;uint32_t end=(x+1)*z->n/216;
    while(v.ph[0]<end){int n=sample_next(z,&v,0);if(n<0)n=-n;if(n>a)a=n;}peak[x]=(uint8_t)(a*38/32768);
   }lastslot=k;crc=h->crc;
  }
  for(uint32_t x=0;x<216;x++){uint32_t a=peak[x]*p[P_E2]/127;drift_line(12+x,149-a,12+x,149+a,65535);}
  for(uint32_t i=0;i<8;i++)label[i]=h->name[i];label[8]=0;draw_text_line(12,185,216,&AF_S,label,65535,0,1);return;
 }
 int last=150;uint32_t seed=12345;
 for(uint32_t x=0;x<216;x++){
  uint64_t ph=(uint64_t)x*0x100000000ull/108u;
  uint32_t ph2=(uint32_t)(ph+ph*(uint32_t)p[P_E1]/1700u+0x40000000u);
  int32_t a=drift_wave_shape(wave,(uint32_t)ph),b=drift_wave_shape(wave,ph2);
  if(wave==6)b=drift_wave_shape(1,(uint32_t)(ph/2));
  if(wave==5)b=drift_wave_shape(0,(uint32_t)((ph&0xffffffffull)*(16u+3u*p[P_E1])/16u));
  int n=(a*(127-p[P_E2])+b*p[P_E2])/127;
  seed=seed*1664525u+1013904223u;n+=((int)(seed>>17)-16384)*p[P_E3]/127;
  int y=150-clamp(n,-32768,32767)*p[P_DLV1]/127*45/32768;
  if(x)drift_line(11+x,last,12+x,y,65535);last=y;
 }
 for(int x=12;x<228;x+=6)lcd_fill(x,150,1,1,65535);
}
static void drift_env_graph(void){
 const int16_t *p=trk[0].p;
 uint32_t a=TIME_MS_X10[p[P_ATK]&127]+20,d=TIME_MS_X10[p[P_DEC]&127]+20,r=TIME_MS_X10[p[P_REL]&127]+20;
 uint32_t total=a+d+r,aw=10+142u*a/total,dw=10+142u*d/total,rw=10+142u*r/total;
 int x=14,y=194,peak=101,sy=y-(y-peak)*p[P_SUS]/127;
 drift_line(12,196,228,196,65535);
 drift_line(x,y,x+aw,peak,65535);x+=aw;
 drift_line(x,peak,x+dw,sy,65535);x+=dw;
 drift_line(x,sy,x+40,sy,65535);x+=40;
 drift_line(x,sy,x+rw,y,65535);
}
static void drift_roll(void){
 uint32_t i,bars=(uint32_t)trk[0].p[P_DLEN],dur=(uint32_t)trk[0].p[P_DDUR];
 int lo=trk[0].p[P_DMIN],hi=trk[0].p[P_DMAX],span=hi-lo+1;
 /* only a bar grid and C guides; no heavy piano-key illustration */
 for(i=0;i<=bars;i++){uint32_t x=12+216u*i/bars;for(uint32_t y=103;y<198;y+=4)lcd_fill(x,y,1,1,65535);}
 for(i=0;i<(uint32_t)span;i++)if((lo+(int)i)%12==0){int y=194-88*(int)i/(span>1?span-1:1);for(int x=12;x<229;x+=6)lcd_fill(x,y,1,1,65535);}
 for(i=0;i<drift_gen.count;i++)if(drift_gen.enabled[i]){
  uint32_t x=12+216u*drift_gen.phase[i]/65536u,w=216u*dur/(bars*16u);int y=193-88*((int)drift_gen.note[i]-lo)/(span>1?span-1:1);
  if(w<2)w=2;if(w>216)w=216;uint32_t first=x+w>228?228-x:w;
  if(first)lcd_fill(x,y,first,3,65535);if(w>first)lcd_fill(12,y,w-first,3,65535);
 }
 if(song.playing){uint32_t len=drift_loop_samples(),x=12+216u*drift_gen.pos/len;drift_line(x,101,x,198,65535);}
 if(!drift_gen.pool_n)draw_text_line(12,145,216,&AF_S,"NO NOTES IN RANGE",65535,0,1);
}
static void drift_columns(void){
 uint32_t i;char b[32],v[32];
 for(i=0;i<4;i++){
  uint32_t id=D_PARAMS[drift_page][i],x=8+i*(drift_page==D_SCL2?112:56),width=drift_page==D_SCL2?110:54;if(id==255)continue;
  const param_desc_t *d=drift_page==D_SPACE?&GP[id]:drift_desc(id);
  int val=drift_page==D_SPACE?song.g[id]:trk[0].p[id];
  const char *label=D_LABEL[drift_page][i];
  if(drift_page==D_OSC && trk[0].p[P_E0]>=7 && i==2)label="LEVEL";
  draw_text_line(x,49,width,&AF_S,label,65535,0,1);
  if(id==P_D2T){fmt_int(v,val-64);str_cpy(v+str_len(v),"st",sizeof v-str_len(v));}
  else if(id==P_DDELAY)str_cpy(v,((const char *const[]){"CLASSIC","PINGPONG","REVERSE"})[clamp(val,0,2)],sizeof v);
  else if(id==P_DR5)str_cpy(v,((const char *const[]){"ROOM","SPRING","HALL","SPACE","SHIMMER","SUBMRN"})[clamp(val,0,5)],sizeof v);
  else if(id==P_DDEST)str_cpy(v,D_LFO_TARGETS[clamp(val,0,3)],sizeof v);
  else if(id==P_DMIN||id==P_DMAX){str_cpy(v,N_NOTE[val%12],sizeof v);fmt_int(b,val/12-1);str_cpy(v+str_len(v),b,sizeof v-str_len(v));}
  else if(id==P_DPITCH){if(!val)str_cpy(v,"ROOT",sizeof v);else{str_cpy(v,val<=7?"UP ":val<=14?"DN ":"UD ",sizeof v);fmt_int(b,(val-1)%7+1);str_cpy(v+3,b,sizeof v-3);}}
  else if(id==P_DOCT)str_cpy(v,((const char *const[]){"1ST","2ND","3RD","4TH","5TH","6TH","7TH","8VE"})[val&7],sizeof v);
  else if(id==P_E0 && val>=7){fmt_int(v,val-6);str_cpy(b,"SMP ",sizeof b);str_cpy(b+4,v,sizeof b-4);str_cpy(v,b,sizeof v);}
  else {const char *unit;param_format(d,val,v,&unit);if(str_len(v)+str_len(unit)<8)str_cpy(v+str_len(v),unit,sizeof v-str_len(v));}
  draw_text_line(x,69,width,&AF_S,v,65535,0,1);
  if(drift_page==D_FX || drift_page==D_SPACE || drift_page==D_TYPES || drift_page==D_FILTER){
   uint32_t h=d->max>d->min?(uint32_t)(val-d->min)*(drift_page==D_FILTER?42u:94u)/(d->max-d->min):0;
   drift_line(x+9,196,x+45,196,65535);if(h)lcd_fill(x+18,196-h,18,h,65535);
  }
 }
}
static void drift_octave_label(void){
 char b[16],n[8];str_cpy(b,song.octave>0?"OCT +":"OCT ",sizeof b);
 fmt_int(n,song.octave);str_cpy(b+str_len(b),n,sizeof b-str_len(b));
 draw_text_line(176,212,52,&AF_S,b,65535,0,2);
}
static void drift_draw(void){
 uint32_t i;char b[32];
 if(!drift_ready)return;if(!ui.force && fm1_ms-drift_draw_ms<80u)return;
 drift_draw_ms=fm1_ms;ui.force=0;drift_frame_begin();
 draw_text_line(12,8,216,&AF_S,drift_dialog?"CONFIRM":name_on()?"SAVE PATCH":D_TITLE[drift_page],65535,0,0);
 if(!drift_dialog && !name_on() && (drift_page==D_SEQ1 || drift_page==D_OSC || drift_page==D_SCL))draw_text_line(93,8,54,&AF_S,"1/2",65535,0,1);
 if(!drift_dialog && !name_on() && (drift_page==D_SEQ2 || drift_page==D_OSC2 || drift_page==D_SCL2))draw_text_line(93,8,54,&AF_S,"2/2",65535,0,1);
 if(!drift_dialog && !name_on() && (drift_page==D_FX || drift_page==D_SPACE || drift_page==D_TYPES))draw_text_line(93,8,54,&AF_S,drift_page==D_FX?"1/3":drift_page==D_SPACE?"2/3":"3/3",65535,0,1);
 fmt_int(b,song.g[G_BPM]);str_cpy(b+str_len(b)," BPM",sizeof b-str_len(b));draw_text_line(157,8,71,&AF_S,b,65535,0,2);
 lcd_fill(12,34,216,1,65535);
 if(drift_dialog){
  char label[32],patch[16];drift_name(drift_target,patch);
  str_cpy(label,drift_dialog==D_DIALOG_DELETE?"Delete ":"Overwrite ",sizeof label);str_cpy(label+str_len(label),patch,sizeof label-str_len(label));
  draw_text_line(12,72,216,text_w(&AF_M,label)>216?&AF_S:&AF_M,label,65535,0,1);
  draw_text_line(12,124,216,&AF_L,drift_yes?"Yes":"No",65535,0,1);
  draw_text_line(12,172,216,&AF_S,"KNOB 1: YES / NO",65535,0,1);
  draw_text_line(12,193,216,&AF_S,"HOME: CONFIRM",65535,0,1);
  if(ui.msg_t){draw_text_line(12,212,158,&AF_S,ui.msg,65535,0,0);ui.msg_t--;}
  drift_octave_label();drift_frame_end();return;
 }
 if(name_on()){
  /* SAVE AS is already in the header. */
  for(i=0;i<nm.len;i++){char ch[2]={nm.s[i],0};draw_text_line(12+i*16,60,16,&AF_M,ch,65535,0,1);}
  lcd_fill(12+nm.cur*16,85,12,2,65535);
  draw_text_line(12,110,104,&AF_S,"1 CURSOR",65535,0,1);draw_text_line(124,110,104,&AF_S,"2 LETTER",65535,0,1);
  drift_text(155,"SAVE / OCT+ : STORE",65535,0);drift_text(180,"HOME / OCT- : CANCEL",65535,0);drift_octave_label();drift_frame_end();return;
 }
 if(drift_overlay && fm1_ms-drift_overlay_ms<500u){
  drift_text(49,drift_overlay==1?"LOW PASS":"TEMPO",65535,0);
  if(drift_overlay==1){const char *unit;param_format(&ENG_ANALOG.edit[4],trk[0].p[P_E4],b,&unit);draw_text_line(12,112,216,&AF_S,unit,65535,0,1);}
  else{fmt_int(b,song.g[G_BPM]);str_cpy(b+str_len(b)," BPM",sizeof b-str_len(b));}
  draw_text_line(12,80,216,&AF_L,b,65535,0,1);
  if(drift_overlay==1)drift_filter_graph(126,193);
  drift_octave_label();drift_frame_end();return;
 }drift_overlay=0;
 if(drift_page==D_HOME){
  if(!drift_list_n)draw_text_line(12,110,216,&AF_M,"NO PATCHES",65535,0,1);
  int top=clamp(drift_row-2,0,drift_list_n>6?(int)drift_list_n-6:0);
  for(i=0;i<6 && top+(int)i<(int)drift_list_n;i++){
   uint32_t id=drift_list[top+i],y=48+i*25;uint16_t bg=top+(int)i==drift_row?65535:0,fg=bg?0:65535;
   lcd_fill(8,y-2,224,24,bg);drift_name(id,b);draw_text_line(14,y-2+CAP_IN(S,24),190,&AF_S,b,fg,bg,0);if(drift_fav(id))drift_heart(211,y+3,fg);
  }
 }else{
  drift_columns();
  if(drift_page==D_SEQ1 || drift_page==D_SEQ2)drift_roll();
  if(drift_page==D_ENV)drift_env_graph();
  if(drift_page==D_SCL){
   draw_text_line(12,113,216,&AF_L,N_NOTE[trk[0].p[P_ROOT]],65535,0,1);
   draw_text_line(12,155,216,&AF_M,((const char *const[]){"Chromatic","Major","Minor","Dorian","Mixolydian","Pentatonic","Minor pent.","Harm. minor","Phrygian","Lydian","Locrian","Mel. minor","Blues","Whole tone","Dim. H-W","Dim. W-H"})[trk[0].p[P_SCALE]&15],65535,0,1);
  }
  if(drift_page==D_LFO){
   const int depths[]={127,trk[0].p[P_LD_FLT],trk[0].p[P_LD_AMP]};
   for(uint32_t c=0;c<3;c++)for(uint32_t x=0;x<216;x++){
    int y=150-(int)((int64_t)lfo_wave(&trk[0],(uint32_t)(((uint64_t)x<<32)/216))*depths[c]*45/(32768*127));
    if(c==0 || x%(c+2)==0)lcd_fill(12+x,y,1,1,c?65535:0x4208);
   }
   uint32_t x=12+(uint32_t)(((uint64_t)trk[0].lfo_ph*216)>>32);drift_line(x,101,x,198,65535);

  }
  if(drift_page==D_FILTER)drift_filter_graph(96,145);
  if(drift_page==D_OSC || drift_page==D_OSC2)drift_wave_graph();
 }
 lcd_fill(12,205,216,1,65535);drift_name(drift_id(),b);if(drift_dirty)str_cpy(b+str_len(b)," *",sizeof b-str_len(b));draw_text_line(12,212,158,&AF_S,b,65535,0,0);
 if(song.playing){uint32_t len=drift_loop_samples();lcd_fill(12,237,216u*drift_gen.pos/len,2,65535);}
 if(ui.msg_t){lcd_fill(0,205,240,35,0);draw_text_line(12,212,158,&AF_S,ui.msg,65535,0,0);ui.msg_t--;}
 drift_octave_label();drift_frame_end();
}
static void drift_leds(void) {
 uint32_t b,k;uint8_t nl[FM1_NCOL]={0},nd[FM1_NCOL]={0},mid[FM1_NCOL]={0};static uint8_t ready;
 static const uint8_t btn[]={B_HOME,B_GLO,B_EDIT,B_ENV,B_LFO,B_FX,B_FX,B_SCL,B_ARP,B_SEQ,B_GLO,B_SCL,B_FX};
 if(!ready){led_pos_init();ready=1;}
 led_put(nl,panel.btn[btn[drift_page]],1);
 led_put(nl,panel.btn[B_REC],drift_fav(drift_id()));
 if(song.playing)nl[LED_PLAY_GREEN>>3]|=1u<<(LED_PLAY_GREEN&7);
 for(k=0;k<27;k++)led_put(nl,k+14,trk[0].p[P_DLED]?drift_scale_contains(&trk[0],clamp(53+(int)k+12*song.octave+trk[0].p[P_TRANS],0,127)):(fm1_in.notes>>k)&1u);
 if(song.octave){
  uint32_t oct=song.octave<0?-song.octave:song.octave,button=panel.btn[song.octave<0?B_OCTDN:B_OCTUP];
  led_put(oct==1?nd:oct==2?mid:nl,button,1);
 }
 for(b=0;b<FM1_NCOL;b++){fm1_led[b]=nl[b];fm1_led_dim[b]=nd[b];fm1_led_mid[b]=mid[b];fm1_led_breath[b]=0;}
}
