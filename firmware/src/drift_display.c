/* SPDX-License-Identifier: GPL-3.0-only
 * Complete monochrome frames in RAM, then changed 8-row strips to SPI.
 * Four-bit luminance preserves font antialiasing. No visible clear pass.
 */
static uint8_t drift_pixels[240u*240u/2u] __attribute__((section(".pool")));
static uint16_t drift_strip[240u*8u] __attribute__((section(".pool")));
static uint32_t drift_hash[30];
static uint8_t drift_buffering,drift_frame_valid;
static void drift_pixel(uint32_t x,uint32_t y,uint16_t c){
 uint32_t i=y*240u+x,q=(c>>7)&15u;
 if(i&1u)drift_pixels[i>>1]=(uint8_t)((drift_pixels[i>>1]&15u)|(q<<4));
 else drift_pixels[i>>1]=(uint8_t)((drift_pixels[i>>1]&240u)|q);
}
static void drift_lcd_fill(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint16_t c){
 if(!drift_buffering){drift_frame_valid=0;lcd_fill(x,y,w,h,c);return;}
 if(x>=240 || y>=240)return;if(w>240-x)w=240-x;if(h>240-y)h=240-y;
 for(uint32_t j=y;j<y+h;j++)for(uint32_t i=x;i<x+w;i++)drift_pixel(i,j,c);
}
static void drift_lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *px){
 if(!drift_buffering){drift_frame_valid=0;lcd_blit(x,y,w,h,px);return;}
 for(uint32_t j=0;j<h && y+j<240;j++)for(uint32_t i=0;i<w && x+i<240;i++){
  uint16_t c=px[j*w+i];drift_pixel(x+i,y+j,(uint16_t)((c>>8)|(c<<8)));
 }
}
static void drift_frame_begin(void){lcd_sync();drift_buffering=1;memset(drift_pixels,0,sizeof drift_pixels);}
static void drift_frame_end(void){
 drift_buffering=0;
 for(uint32_t row=0;row<30;row++){
  uint32_t hash=2166136261u;const uint8_t *src=drift_pixels+row*960u;
  for(uint32_t i=0;i<960;i++){hash^=src[i];hash*=16777619u;}
  if(drift_frame_valid && hash==drift_hash[row])continue;
  lcd_sync();
  for(uint32_t i=0;i<1920;i++){uint32_t q=(src[i>>1]>>((i&1u)*4))&15u,g=q*17u;
   uint16_t c=(uint16_t)(((g>>3)<<11)|((g>>2)<<5)|(g>>3));drift_strip[i]=(uint16_t)((c>>8)|(c<<8));}
  lcd_blit(0,row*8,240,8,drift_strip);drift_hash[row]=hash;
 }
 lcd_sync();drift_frame_valid=1;
}
#define lcd_fill drift_lcd_fill
#define lcd_blit drift_lcd_blit
