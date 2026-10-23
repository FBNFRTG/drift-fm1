/* SPDX-License-Identifier: GPL-3.0-only
 * IMA/CRC/pack7 from Felucca by Leo Kuroshita; DRIFT slot format 0.3. */
function pack7(b) {
  const out = [];
  for (let i = 0; i < b.length; i += 7) {
    const g = Array.from(b.slice(i, i + 7));
    out.push(g.reduce((m, x, j) => m | (((x >> 7) & 1) << j), 0));
    for (const x of g) out.push(x & 0x7F);
  }
  return out;
}
function unpack7(a) {
  const out = [];
  for (let i = 0; i < a.length; i += 8) {
    const top = a[i];
    for (let j = 0; j < 7 && i + 1 + j < a.length; j++) out.push(a[i + 1 + j] | (((top >> j) & 1) << 7));
  }
  return Uint8Array.from(out);
}

const CRC_TABLE = (() => {
  const t = new Uint32Array(256);
  for (let n = 0; n < 256; n++) { let c = n; for (let k = 0; k < 8; k++) c = c & 1 ? 0xEDB88320 ^ (c >>> 1) : c >>> 1; t[n] = c >>> 0; }
  return t;
})();
function crc32(b) { let c = 0xFFFFFFFF; for (let i = 0; i < b.length; i++) c = CRC_TABLE[(c ^ b[i]) & 0xFF] ^ (c >>> 8); return (c ^ 0xFFFFFFFF) >>> 0; }

const IMA_STEP = [7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88,
  97, 107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
  724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660,
  4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
  18500, 20350, 22385, 24623, 27086, 29794, 32767];
const IMA_IDX = [-1, -1, -1, -1, 2, 4, 6, 8];
/* IMA ADPCM, 4 bit, low nibble first, from predictor 0 / index 0 -> {data, pred, idx} (state at loopStart) */
function imaEncode(s, loopStart = 0) {
  let pred = 0, idx = 0, atLoop = [0, 0];
  const nib = new Uint8Array(s.length + (s.length & 1));
  for (let n = 0; n < s.length; n++) {
    if (n === loopStart) atLoop = [pred, idx];
    const step = IMA_STEP[idx];
    let diff = s[n] - pred, code = 0;
    if (diff < 0) { code = 8; diff = -diff; }
    let vd = step >> 3;
    if (diff >= step) { code |= 4; diff -= step; vd += step; }
    if (diff >= step >> 1) { code |= 2; diff -= step >> 1; vd += step >> 1; }
    if (diff >= step >> 2) { code |= 1; vd += step >> 2; }
    pred = Math.max(-32768, Math.min(32767, code & 8 ? pred - vd : pred + vd));
    idx = Math.max(0, Math.min(88, idx + IMA_IDX[code & 7]));
    nib[n] = code;
  }
  const out = new Uint8Array(nib.length >> 1);
  for (let k = 0; k < nib.length; k += 2) out[k >> 1] = nib[k] | (nib[k + 1] << 4);
  return { data: out, pred: atLoop[0], idx: atLoop[1] };
}


export {pack7, crc32, imaEncode};
export const SLOT_SIZE=0x14000, DATA_OFFSET=512, RATE=22050, MAX_FRAMES=(SLOT_SIZE-DATA_OFFSET)*2;
export const LOCAL_STORE='drift-03-sample-slots';
export function buildSample(pcm,name,root=60,loop=false){
 if(!(pcm instanceof Int16Array)||pcm.length<2||pcm.length>MAX_FRAMES)throw new Error('Sample must contain 2 to '+MAX_FRAMES+' frames.');
 if(!Number.isInteger(root)||root<0||root>127)throw new Error('Root note must be between 0 and 127.');
 const data=imaEncode(pcm).data,header=new Uint8Array(480),v=new DataView(header.buffer);
 const nm=String(name).toUpperCase().replace(/[^A-Z0-9 _-]/g,'').trim().slice(0,8);if(!nm)throw new Error('Enter a sample name.');
 v.setUint32(0,0x33504D44,true);v.setUint16(4,1,true);header[6]=1;
 for(let i=0;i<nm.length;i++)header[8+i]=nm.charCodeAt(i);
 v.setUint32(16,data.length,true);v.setUint32(20,crc32(data),true);
 [0,pcm.length,0,pcm.length-1,32768].forEach((x,i)=>v.setUint32(32+i*4,x,true));
 v.setInt16(52,root*16,true);header[58]=127;header[59]=loop?1:0;
 const slot=new Uint8Array(SLOT_SIZE).fill(255);slot.set(header);slot.set(data,512);
 return {header,data,slot,name:nm};
}
export function parseInfo(a){
 let i=0;const take=()=>{if(i>=a.length)throw new Error('Incomplete device response');return a[i++];};
 const count=take(),kib=take();if(count!==3||kib!==80)throw new Error('DRIFT 0.3 with 3 × 80 KiB required.');
 const slots=[];for(let k=0;k<count;k++){const zones=take();let name='',c;while((c=take())){if(name.length>=8)throw new Error('Invalid sample name');name+=String.fromCharCode(c);}slots.push({zones,name,kib:take()});}
 if(i!==a.length)throw new Error('Unexpected sample info');return slots;
}
export async function uploadSample(request,k,built,onProgress=()=>{}){
 if(!Number.isInteger(k)||k<0||k>2)throw new Error('Invalid slot');
 let a=await request(11,[k]);if(a.length!==2||a[0]!==k||a[1])throw new Error('Could not prepare sample (BEGIN).');
 for(let off=0;off<built.data.length;off+=256){
  const address=512+off,chunk=built.data.slice(off,off+256);
  a=await request(12,[k,address&127,(address>>7)&127,(address>>14)&127,...pack7(chunk)]);
  if(a.length!==5||a[0]!==k||(a[1]|a[2]<<7|a[3]<<14)!==address||a[4])throw new Error('Write failed at byte '+address+'. Slot remains incomplete.');
  onProgress(Math.min(1,(off+chunk.length)/built.data.length));
 }
 a=await request(13,[k,...pack7(built.header)]);if(a.length!==2||a[0]!==k||a[1])throw new Error('CRC/header check failed (END '+a[1]+').');
 const slots=parseInfo(await request(15,[]));if(slots[k].zones!==1||slots[k].name!==built.name)throw new Error('Could not verify the uploaded sample.');
 return slots;
}
