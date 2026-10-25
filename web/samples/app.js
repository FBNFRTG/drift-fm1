/* SPDX-License-Identifier: GPL-3.0-only */
import {RATE,MAX_FRAMES,SLOT_SIZE,LOCAL_STORE,buildSample,parseInfo,uploadSample} from './codec.js';
const $=id=>document.getElementById(id),say=t=>$('status').textContent=t;
let access,input,output,pending,verified=false,busy=false,slots=Array.from({length:3},()=>({zones:0,name:''}));
const drafts=Array.from({length:3},()=>({pcm:null}));
function base64(bytes){let s='';for(let i=0;i<bytes.length;i+=8192)s+=String.fromCharCode(...bytes.subarray(i,i+8192));return btoa(s);}
function localInfo(){let o=JSON.parse(localStorage.getItem(LOCAL_STORE)||'{}');return slots=Array.from({length:3},(_,i)=>({zones:o[i]?1:0,name:o[i]?.name||''}));}
function update(){
 $('access').hidden=$('target').value!=='usb';$('ports').hidden=$('target').value!=='usb'||!access;
 document.querySelectorAll('input,select,button').forEach(el=>el.disabled=busy);
 drafts.forEach((d,k)=>{const card=$('s'+k);card.querySelector('.saved').textContent=slots[k].zones?slots[k].name:'EMPTY';const b=card.querySelector('.upload');b.disabled=busy||!d.pcm||($('target').value==='usb'&&!verified);b.textContent=(slots[k].zones?'Replace':'Save')+($('target').value==='usb'?' → FM-1':' → Webplayer');});
}
function fail(e){say(e?.message||String(e));}
function disconnect(){verified=false;if(pending){clearTimeout(pending.timer);pending.reject(new Error('MIDI connection interrupted.'));pending=null;}update();}
function request(cmd,args){return new Promise((resolve,reject)=>{
 if(!input||!output||input.state==='disconnected'||output.state==='disconnected')return reject(new Error('No connected device.'));
 if(pending)return reject(new Error('A request is already running.'));
 const timer=setTimeout(()=>{pending=null;verified=false;reject(new Error('No device response. Check the connection; upload is incomplete.'));},4000);
 pending={cmd,resolve,reject,timer};try{output.send([240,125,70,76,cmd,...args,247]);}catch(e){clearTimeout(timer);pending=null;reject(e);}
});}
function onMessage(e){const a=Array.from(e.data);if(!pending||a.length<6||a[0]!==240||a[1]!==125||a[2]!==70||a[3]!==76||a.at(-1)!==247||a[4]!==pending.cmd)return;
 const p=pending;pending=null;clearTimeout(p.timer);p.resolve(a.slice(5,-1));}
function portLists(){for(const [id,ports] of [['input',access.inputs],['output',access.outputs]]){const el=$(id),old=el.value;el.replaceChildren();for(const p of ports.values())if(p.state!=='disconnected'){const o=document.createElement('option');o.value=p.id;o.textContent=p.name||p.id;el.append(o);}if([...el.options].some(o=>o.value===old))el.value=old;else{const candidate=[...el.options].find(o=>/fm-?1|felucca|drift|sinco/i.test(o.textContent));if(candidate)el.value=candidate.value;}}}
$('access').onclick=async()=>{try{if(!navigator.requestMIDIAccess)throw new Error('Web MIDI is unavailable. Use Chrome or Edge.');access=await navigator.requestMIDIAccess({sysex:true});portLists();access.onstatechange=()=>{disconnect();portLists();say('MIDI devices changed. Verify the connection again.');};say('Select FM-1 input and output, then check device.');update();}catch(e){fail(e);}};
$('input').onchange=$('output').onchange=()=>{disconnect();say('Please check the selected connection.');};
$('connect').onclick=async()=>{try{
 if(input)input.onmidimessage=null;input=access.inputs.get($('input').value);output=access.outputs.get($('output').value);if(!input||!output)throw new Error('Select input and output.');
 await input.open();await output.open();input.onmidimessage=onMessage;busy=true;update();
 const info=await request(1,[]),end=info.indexOf(0),name=String.fromCharCode(...info.slice(0,end));
 if(end<0||!/^FELUCCA DRIFT 0\.(?:[34578]|9(?:\.1)?|6(?:\.1)?)$/.test(name))throw new Error('Expected DRIFT 0.3–0.9.1. Found: '+name+'. No samples written.');
 slots=parseInfo(await request(15,[]));verified=true;say('Connected: '+name+' · 3 slots available.');
 }catch(e){verified=false;fail(e);}finally{busy=false;update();}};
$('target').onchange=()=>{verified=false;if($('target').value==='local'){try{localInfo();say('Browser-Speicher. Nach dem Save den Webplayer neu laden.');}catch(e){fail(e);}}else{slots=Array.from({length:3},()=>({zones:0,name:''}));say('Connect USB, enable MIDI and check device.');}update();};
async function prepare(file,k,card){
 const revision=(drafts[k].revision||0)+1;drafts[k].revision=revision;drafts[k].pcm=null;update();if(!file)return;
 try{
 if(file.size>32*1024*1024)throw new Error('File too large. Trim to 7.383 seconds first.');
 const decoded=await new OfflineAudioContext(1,1,RATE).decodeAudioData(await file.arrayBuffer());
 const frames=Math.round(decoded.duration*RATE);if(frames<2||frames>MAX_FRAMES)throw new Error('Sample is '+decoded.duration.toFixed(3)+' s long. Maximum '+(MAX_FRAMES/RATE).toFixed(3)+' s. Please trim first.');
 const ctx=new OfflineAudioContext(1,frames,RATE),mono=ctx.createBuffer(1,decoded.length,decoded.sampleRate),sum=mono.getChannelData(0);
 for(let c=0;c<decoded.numberOfChannels;c++){const ch=decoded.getChannelData(c);for(let i=0;i<sum.length;i++)sum[i]+=ch[i]/decoded.numberOfChannels;}
 const src=ctx.createBufferSource();src.buffer=mono;src.connect(ctx.destination);src.start();const converted=await ctx.startRendering(),x=converted.getChannelData(0);
 const pcm=Int16Array.from(x,v=>Math.round(Math.max(-1,Math.min(32767/32768,Number.isFinite(v)?v:0))*32768));if(drafts[k].revision!==revision)return;drafts[k].pcm=pcm;
 card.querySelector('.name').value=file.name.replace(/\.[^.]+$/,'').toUpperCase().replace(/[^A-Z0-9 _-]/g,'').slice(0,8)||'SAMPLE'+(k+1);
 card.querySelector('.fileinfo').textContent=`${decoded.duration.toFixed(3)} s · ${decoded.numberOfChannels} channel(s) → mono · 22,050 Hz · ${Math.ceil(pcm.length/2)} / 81,408 Bytes`;
 const canvas=card.querySelector('canvas'),g=canvas.getContext('2d');g.clearRect(0,0,canvas.width,canvas.height);g.strokeStyle='#eee';g.beginPath();for(let i=0;i<canvas.width;i++){const a=Math.floor(i*pcm.length/canvas.width),b=Math.max(a+1,Math.floor((i+1)*pcm.length/canvas.width));let peak=0;for(let j=a;j<b;j++)peak=Math.max(peak,Math.abs(pcm[j])/32768);g.moveTo(i,26-peak*24);g.lineTo(i,26+peak*24);}g.stroke();
 say('Sample '+(k+1)+' prepared. Save to transfer the data.');
 }catch(e){if(drafts[k].revision!==revision)return;card.querySelector('.fileinfo').textContent=e.message;card.querySelector('canvas').getContext('2d').clearRect(0,0,600,52);fail(e);}finally{update();}
}
for(let k=0;k<3;k++){
 const card=document.createElement('section');card.className='slot';card.id='s'+k;
 card.innerHTML=`<div class="top"><h2>SAMPLE ${k+1}</h2><small class="saved">EMPTY</small></div><input type="file" aria-label="Audio file for sample ${k+1}" accept="audio/*,.wav"><canvas width="600" height="52" aria-label="Waveform"></canvas><div class="fileinfo">Choose audio · maximum 7.383 s</div><div class="row"><label>Name<input class="name" maxlength="8" value="SAMPLE${k+1}"></label><label>Root note (MIDI)<input class="root" type="number" min="0" max="127" step="1" value="60"></label></div><label class="loop"><input class="loopbox" type="checkbox">Loop · entire file</label><progress max="1" value="0" hidden></progress><button class="upload" disabled>Save → Webplayer</button>`;
 $('slots').append(card);card.querySelector('input[type=file]').onchange=e=>prepare(e.target.files[0],k,card);
 card.querySelector('.upload').onclick=async()=>{if(busy)return;try{
 const built=buildSample(drafts[k].pcm,card.querySelector('.name').value,Number(card.querySelector('.root').value),card.querySelector('.loopbox').checked);
 busy=true;update();const progress=card.querySelector('progress');progress.hidden=false;progress.value=0;
 if($('target').value==='local'){const saved=JSON.parse(localStorage.getItem(LOCAL_STORE)||'{}');saved[k]={name:built.name,data:base64(built.slot)};localStorage.setItem(LOCAL_STORE,JSON.stringify(saved));localInfo();progress.value=1;say('Sample '+(k+1)+' saved. Reload the player, then GLO → WAVE → SAMPLE '+(k+1)+'.');}
 else{if(!verified)throw new Error('Check device first.');slots=await uploadSample(request,k,built,p=>progress.value=p);say('Sample '+(k+1)+' saved and verified on the FM-1.');}
 }catch(e){fail(e);}finally{busy=false;update();}};
}
try{localInfo();}catch(e){fail(e);}update();
