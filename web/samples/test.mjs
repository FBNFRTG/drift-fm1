import assert from 'node:assert/strict';import fs from 'node:fs';
import {buildSample,uploadSample,pack7,crc32,parseInfo,MAX_FRAMES,SLOT_SIZE} from './codec.js';
const un7=a=>{const out=[];for(let i=0;i<a.length;){const mask=a[i++];for(let j=0;j<7&&i<a.length;j++)out.push(a[i++]|((mask>>j)&1)<<7);}return Uint8Array.from(out);};
for(let len=1;len<258;len++){const a=Uint8Array.from({length:len},(_,i)=>(i*91)&255);assert.deepEqual(un7(pack7(a)),a);}
assert.throws(()=>buildSample(new Int16Array(MAX_FRAMES+1),'LONG'));
assert.throws(()=>buildSample(new Int16Array(10),'TEST',128));
assert.throws(()=>parseInfo([6,40]));
fs.mkdirSync('build/sample-fixtures',{recursive:true});
for(let k=0;k<3;k++){
 const pcm=Int16Array.from({length:11025},(_,i)=>Math.round(Math.sin(i*(k+1)*220*2*Math.PI/22050)*18000));
 const b=buildSample(pcm,'SAMPLE'+(k+1),60,true);fs.writeFileSync(`build/sample-fixtures/${k}.bin`,b.slot);
 const mem=new Uint8Array(SLOT_SIZE).fill(255);let begun=false,committed=false;
 const info=()=>[3,80,...Array.from({length:3},(_,i)=>i===k?[1,...Buffer.from(b.name),0,Math.ceil(b.data.length/1024)]:[0,0,0]).flat()];
 const request=async(cmd,a)=>{
  assert.equal(a[0]??k,k);if(cmd===11){begun=true;return[k,0];}
  if(cmd===12){assert(begun);const off=a[1]|a[2]<<7|a[3]<<14,part=un7(a.slice(4));assert(off>=512&&off+part.length<=SLOT_SIZE);mem.set(part,off);return a.slice(0,4).concat(0);}
  if(cmd===13){const h=un7(a.slice(1));assert.equal(h.length,480);mem.set(h);const v=new DataView(mem.buffer);assert.equal(v.getUint32(0,true),0x33504D44);assert.equal(v.getUint32(20,true),crc32(mem.slice(512,512+v.getUint32(16,true))));committed=true;return[k,0];}
  assert(committed);return info();
 };
 await uploadSample(request,k,b);assert.deepEqual(mem,b.slot);
 let calls=0;await assert.rejects(()=>uploadSample(async(cmd,a)=>{calls++;return cmd===11?[k,0]:[k,0,0,0,3];},k,b));assert.equal(calls,2);
}
console.log('PASS: sample encoder, CRC, pack7 roundtrips, 3 slot uploads with matching ACKs, size/root/device guards, failure abort, generated audio fixtures.');
