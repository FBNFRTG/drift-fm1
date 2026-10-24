// SPDX-License-Identifier: GPL-3.0-only
import {Updater} from './fm1ota.js';
import {productOf,logicalImage,sha256hex} from './fm1pkg.js';
const button=document.querySelector('#install'),confirm=document.querySelector('#confirm'),status=document.querySelector('#status');
let busy=false;confirm.onchange=()=>button.disabled=busy||!confirm.checked;
window.addEventListener('beforeunload',e=>{if(busy){e.preventDefault();e.returnValue='';}});
button.onclick=async()=>{
 if(busy||!confirm.checked)return;busy=true;button.disabled=true;confirm.disabled=true;
 try{
  if(!navigator.requestMIDIAccess)throw new Error('Web MIDI is unavailable. Use Chrome or Edge over HTTPS or localhost.');
  status.textContent='Checking firmware…';
  const [fr,mr]=await Promise.all([fetch('drift-fm1-0.9.fwsc',{cache:'no-store'}),fetch('manifest.json',{cache:'no-store'})]);
  if(!fr.ok||!mr.ok)throw new Error('Firmware download failed. Nothing written.');
  const bytes=new Uint8Array(await fr.arrayBuffer()),manifest=await mr.json();
  if(bytes.length!==manifest.bytes||await sha256hex(bytes)!==manifest.sha256||productOf(bytes)!==manifest.product||!/^FM-1_\d+$/.test(manifest.product))throw new Error('Firmware validation failed. Nothing written.');
  const access=await navigator.requestMIDIAccess({sysex:true}),updater=new Updater(access),image=logicalImage(bytes);
  const progress=(step,value)=>{const labels={start:'FM-1 detected',verify:'Preparing update loader · requests',loader:'Waiting for update loader…',write:'Writing firmware · requests',reboot:'Waiting for reboot…',done:'Installation complete. Device identity verified'};status.textContent=(labels[step]||step)+(value!==undefined?' '+value:'');};
  status.textContent='Finding FM-1…';
  if(!await updater.resume(image,progress,manifest.product))await updater.install(image,manifest.product,progress);
 }catch(e){status.textContent='Installation not confirmed: '+e.message;}
 finally{busy=false;confirm.disabled=false;button.disabled=!confirm.checked;}
};
