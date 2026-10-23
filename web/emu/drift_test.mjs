// SPDX-License-Identifier: GPL-3.0-only
import fs from 'node:fs';
import assert from 'node:assert/strict';
import {storeSectors,restoreSectors} from './worklet.js';
const bytes=fs.readFileSync(process.argv[2]||'build/emu/felucca.wasm');
async function device(saved,withSamples=false){
 const {instance}=await WebAssembly.instantiate(bytes,{});const e=instance.exports;e._initialize();e.web_nor_erase();
 if(saved)restoreSectors(new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size()),saved);
 if(withSamples){const flash=new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size());for(let k=0;k<3;k++)flash.set(fs.readFileSync(`build/sample-fixtures/${k}.bin`),0xA0000+k*0x14000);}
 e.web_boot();let peak=0,energy=0,samples=0;
 const render=ms=>{for(let i=0;i<ms*44.1;i+=128){e.web_render(128);const a=new Float32Array(e.memory.buffer,e.web_out_l(),128);for(const v of a){assert(Number.isFinite(v));peak=Math.max(peak,Math.abs(v));energy+=v*v;samples++;}}};
 const tap=b=>{e.web_buttons(1<<b);render(40);e.web_buttons(0);render(40);};
 const turn=(k,n)=>{e.web_enc(k,n);render(40);};
 render(1000);
 return {e,render,tap,turn,stats:()=>({peak,rms:Math.sqrt(energy/samples)}),flash:()=>storeSectors(new Uint8Array(e.memory.buffer,e.web_nor(),e.web_nor_size()))};
}
const d=await device();assert.equal(d.e.web_sample_rate(),44100);assert.equal(d.e.web_playing(),0);
d.tap(11); // favorite
// distinct patch edits, then SAVE AS without any overwrite.
d.tap(2);d.turn(3,5);d.tap(7);d.turn(4,1);d.tap(7);assert.equal(d.e.web_test_user_preset(0),1);
const saved=d.flash();const reopened=await device(saved);assert.equal(reopened.e.web_test_user_preset(0),1);
assert(reopened.e.web_lit_buttons() & (1<<11),'favorite survived power cycle');
// Sustained generative audio, active transport, no NaN or clipping.
d.tap(10);assert.equal(d.e.web_playing(),1);let t=performance.now();d.render(15000);const seconds=(performance.now()-t)/1000;
assert(d.stats().peak>0.001 && d.stats().peak<=1);d.tap(10);assert.equal(d.e.web_playing(),0);d.render(1000);
// Two independent starts produce identical audio for a fixed seed and controls.
const a=await device(),b=await device();a.tap(10);b.tap(10);a.render(2000);b.render(2000);
assert.deepEqual(Array.from(new Float32Array(a.e.memory.buffer,a.e.web_out_l(),128)),Array.from(new Float32Array(b.e.memory.buffer,b.e.web_out_l(),128)));
console.log(JSON.stringify({result:'PASS',checks:['WASM boot','SAVE AS','flash reload in fresh instance','favorite persistence','15 seconds generative audio','finite and unclipped audio','stop','deterministic playback'],...d.stats(),render15SecondsWallTime:seconds},null,2));

for(let k=0;k<3;k++){
 const s=await device(undefined,true);s.tap(5);s.turn(3,-20);s.turn(3,7+k);s.e.web_keys(1);s.render(500);s.e.web_keys(0);s.render(500);assert(s.stats().peak>0.001 && s.stats().peak<=1,`sample ${k+1}`);
}
console.log('PASS: all three sample slots produce finite unclipped WASM audio.');
// Routed LFO, second oscillator active: every target renders finite audio.
for(let target=0;target<4;target++){
 const r=await device();r.tap(5);r.tap(5);r.turn(3,4); // GLO 2 / SINE
 r.tap(3);r.turn(5,target);r.turn(6,100); // LFO target and depth
 r.tap(10);r.render(3000);assert(r.stats().peak>0.001 && r.stats().peak<=1);
}
const oct=await device();oct.tap(12);assert(oct.e.web_dim_buttons()&(1<<12));oct.tap(12);assert(oct.e.web_mid_buttons()&(1<<12));oct.tap(13);oct.tap(13);assert(!((oct.e.web_lit_buttons()|oct.e.web_dim_buttons()|oct.e.web_mid_buttons())&((1<<12)|(1<<13))));
console.log('PASS: four LFO destinations with active oscillator 2; octave LED dim/mid/zero states.');
// Confirmations and persistence through fresh WebAssembly instances.
const p=await device();p.tap(7);p.turn(4,1);p.tap(7);assert.equal(p.e.web_test_user_preset(0),1);
const before=p.flash();p.tap(7);p.tap(7);p.tap(6);p.tap(6);assert.deepEqual(p.flash(),before,'overwrite No leaves flash intact');
p.tap(6);p.tap(6);assert.deepEqual(p.flash(),before,'delete No leaves flash intact');
p.tap(6);p.turn(3,1);p.tap(6);assert.equal(p.e.web_test_user_preset(0),0);
const deleted=await device(p.flash());assert.equal(deleted.e.web_test_user_preset(0),0,'deletion survives reboot');
// Both controls on FX page 3 are exercised through the same UI as the hardware.
let worst=0;
for(let dt=0;dt<3;dt++)for(let rt=0;rt<6;rt++){
 const f=await device();f.tap(0);f.turn(4,127);f.turn(5,127);f.tap(0);f.turn(3,127);f.turn(6,127);f.tap(0);
 f.turn(3,dt);f.turn(4,-127);f.turn(4,rt);f.turn(6,127);f.e.web_keys(0x5555);
 const start=performance.now();f.render(3000);worst=Math.max(worst,(performance.now()-start)/1000);
 assert(f.stats().peak>0.001&&f.stats().peak<=1);f.e.web_keys(0);
}
console.log(JSON.stringify({result:'PASS',checks:['overwrite No preserves flash','delete No preserves flash','delete Yes persists after reboot','all 18 FX combinations / eight held notes'],worstRender3SecondsWallTime:worst},null,2));
