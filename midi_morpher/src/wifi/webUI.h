#pragma once

static const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html lang="en"><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"><title>MIDI Morpher</title>
<style>
:root{--bg:#0a0a0a;--s1:#111;--s2:#181818;--s3:#222;--a:#00d9ff;--a2:rgba(0,217,255,.08);--a3:rgba(0,217,255,.18);--t:#ddd;--d:#666;--d2:#444;--b:#252525;--g:#00e676;--g2:rgba(0,230,118,.12);--r:#ff5252;--r2:rgba(255,82,82,.1);--o:#ffab40;--o2:rgba(255,171,64,.1);--rd:10px;--tr:.18s ease}
*{box-sizing:border-box;margin:0;padding:0}body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',system-ui,sans-serif;background:var(--bg);color:var(--t);min-height:100vh;-webkit-tap-highlight-color:transparent}
header{background:var(--s1);border-bottom:1px solid var(--b);padding:12px 16px;display:flex;align-items:center;justify-content:space-between;position:sticky;top:0;z-index:99}
.lo{display:flex;align-items:center;gap:10px}.li{width:28px;height:28px;display:flex;align-items:center;justify-content:center;background:var(--a2);border:1px solid rgba(0,217,255,.3);border-radius:6px;color:var(--a);font-size:.7rem;font-weight:800}.lt{font-size:.88rem;font-weight:700;letter-spacing:.07em;text-transform:uppercase;color:var(--a)}
.bg{font-size:.68rem;padding:3px 9px;border-radius:20px;color:transparent;transition:color var(--tr),background var(--tr)}.bg.un{color:var(--r);background:var(--r2)}.bg.sv{color:var(--a);background:var(--a2)}.bg.sd{color:var(--g);background:var(--g2)}
main{max-width:1100px;margin:0 auto;padding:16px}
.st{font-size:.68rem;font-weight:700;text-transform:uppercase;letter-spacing:.1em;color:var(--d);margin:0 0 8px 2px}
.tb{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:12px}
.tc{background:var(--s1);border:1px solid var(--b);border-radius:var(--rd);padding:14px 16px}.tc .lb{font-size:.68rem;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:var(--d);margin-bottom:8px}
.br{display:flex;align-items:center;gap:8px}
.bi{width:68px;background:var(--s2);color:var(--a);border:1px solid var(--b);border-radius:7px;padding:7px 8px;font-size:1rem;font-weight:700;text-align:center;outline:none;font-variant-numeric:tabular-nums;-moz-appearance:textfield}.bi::-webkit-outer-spin-button,.bi::-webkit-inner-spin-button{-webkit-appearance:none}.bi:focus{border-color:var(--a)}.bi:disabled{opacity:.5;cursor:not-allowed}
.bb{width:32px;height:32px;border-radius:6px;border:1px solid var(--b);background:var(--s2);color:var(--d);font-size:1rem;cursor:pointer;display:flex;align-items:center;justify-content:center;transition:all var(--tr)}.bb:hover{border-color:#444;color:var(--t)}.bb:active{background:var(--a2);color:var(--a)}
.eb{font-size:.62rem;font-weight:700;padding:2px 7px;border-radius:10px;background:var(--o2);color:var(--o);border:1px solid rgba(255,171,64,.25);display:none}
.sl{background:var(--s2);color:var(--t);border:1px solid var(--b);border-radius:7px;padding:8px 30px 8px 10px;font-size:.85rem;outline:none;cursor:pointer;-webkit-appearance:none;width:100%;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23555'/%3E%3C/svg%3E");background-repeat:no-repeat;background-position:right 10px center}.sl:focus{border-color:var(--a)}
.pb{display:flex;align-items:center;gap:6px;flex-wrap:wrap;background:var(--s1);border:1px solid var(--b);border-radius:var(--rd);padding:10px 14px;margin-bottom:12px}
.pb .pl{font-size:.68rem;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:var(--d);margin-right:4px}
.pn{width:38px;height:32px;border-radius:6px;border:1px solid var(--b);background:var(--s2);color:var(--d);font-size:.78rem;font-weight:700;cursor:pointer;transition:all var(--tr)}.pn:hover{border-color:#444;color:var(--t)}.pn.ac{background:var(--a3);border-color:rgba(0,217,255,.5);color:var(--a);box-shadow:0 0 8px rgba(0,217,255,.15)}
.sb{margin-left:auto;padding:6px 16px;border-radius:6px;border:1px solid rgba(0,217,255,.3);background:var(--a2);color:var(--a);font-size:.75rem;font-weight:700;cursor:pointer;transition:all var(--tr)}.sb:hover{background:var(--a3);border-color:var(--a)}.sb:active{transform:scale(.97)}
.pB{display:grid;grid-template-columns:1fr 1fr;gap:12px;margin-bottom:16px}
.pC{background:var(--s1);border:1px solid var(--b);border-radius:var(--rd);padding:12px 14px}.pC .pk{font-size:.68rem;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:var(--d);margin-bottom:8px}
.pr{display:flex;align-items:center;gap:8px}.pv{font-size:.78rem;color:var(--a);min-width:28px;text-align:right;font-variant-numeric:tabular-nums;font-weight:600}
.tg{position:relative;width:36px;height:20px;cursor:pointer;flex-shrink:0}.tg input{opacity:0;width:0;height:0;position:absolute}.tt{position:absolute;inset:0;background:#2a2a2a;border-radius:20px;transition:var(--tr)}.tt::after{content:'';position:absolute;left:3px;top:3px;width:14px;height:14px;background:#555;border-radius:50%;transition:var(--tr)}.tg input:checked~.tt{background:var(--a)}.tg input:checked~.tt::after{background:#fff;transform:translateX(16px);box-shadow:0 1px 4px rgba(0,0,0,.5)}
.gr{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}
@media(max-width:860px){.gr{grid-template-columns:repeat(2,1fr)}}@media(max-width:540px){.gr{grid-template-columns:1fr}main{padding:10px}.tb{grid-template-columns:1fr}.pB{gap:8px}}
.cd{background:var(--s1);border:1px solid var(--b);border-radius:var(--rd);overflow:hidden;transition:border-color var(--tr)}.cd:hover{border-color:#333}
.ct{display:flex;align-items:center;gap:8px;padding:12px 14px 10px}.cn{font-size:.82rem;font-weight:700;text-transform:uppercase;letter-spacing:.06em;flex:1}
.ch{font-size:.58rem;padding:2px 6px;border-radius:10px;font-weight:600;text-transform:uppercase}.co{background:var(--a2);color:var(--a);border:1px solid rgba(0,217,255,.2)}.ce{background:rgba(255,255,255,.03);color:var(--d2);border:1px solid var(--b)}
.tb2{width:100%;padding:10px;border:none;border-top:1px solid var(--b);background:var(--s2);color:var(--d);font-size:.78rem;font-weight:700;cursor:pointer;transition:all var(--tr);user-select:none;-webkit-user-select:none;text-transform:uppercase;letter-spacing:.06em}.tb2:hover{background:var(--s3);color:var(--t)}.tb2:active{background:var(--a2)}.tb2.on{background:var(--g2);color:var(--g);border-top-color:rgba(0,230,118,.2)}
.cb{padding:0 14px 14px}.fd{margin-bottom:10px}.fd:last-child{margin-bottom:0}
.fl{font-size:.64rem;font-weight:600;text-transform:uppercase;letter-spacing:.07em;color:var(--d);margin-bottom:4px;display:flex;align-items:center;gap:5px}.hn{font-weight:400;color:var(--d2);text-transform:none;letter-spacing:0;font-size:.62rem}
.fd select,.fd input[type=number]{width:100%;background:var(--s2);color:var(--t);border:1px solid var(--b);border-radius:7px;padding:7px 10px;font-size:.82rem;outline:none;transition:border-color var(--tr);-webkit-appearance:none}
.fd select{cursor:pointer;background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23555'/%3E%3C/svg%3E");background-repeat:no-repeat;background-position:right 10px center;padding-right:26px}
.fd select:focus,.fd input[type=number]:focus{border-color:var(--a)}.fd input[type=number]:disabled{opacity:.4;cursor:not-allowed}
hr.dv{border:none;border-top:1px solid var(--b);margin:10px 0}
.rg{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.rl{font-size:.62rem;font-weight:600;text-transform:uppercase;letter-spacing:.07em;color:var(--d);margin-bottom:4px;display:flex;align-items:center;gap:6px}
.rl .sy{margin-left:auto;font-size:.56rem;color:var(--d);text-transform:none;letter-spacing:0;font-weight:400}
.rl .tg{width:24px;height:13px}.rl .tt{border-radius:13px}.rl .tt::after{top:2px;left:2px;width:9px;height:9px}.rl .tg input:checked~.tt::after{transform:translateX(11px)}
.sr{display:flex;align-items:center;gap:6px}
input[type=range]{-webkit-appearance:none;flex:1;height:3px;background:var(--b);border-radius:2px;outline:none;cursor:pointer}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;border-radius:50%;background:var(--a);cursor:pointer;box-shadow:0 0 6px rgba(0,217,255,.35)}
.sv2{font-size:.7rem;color:var(--a);min-width:32px;text-align:right;font-variant-numeric:tabular-nums;font-weight:600}
.rw.hd{display:none}
footer{text-align:center;padding:24px;font-size:.64rem;color:#333}
</style></head><body>
<header><div class="lo"><div class="li">MM</div><div class="lt">MIDI Morpher</div></div><div style="display:flex;align-items:center;gap:8px"><span id="badge" class="bg"></span></div></header>
<main>
<div class="tb"><div class="tc"><div class="lb">MIDI Channel</div><select id="gch" class="sl"></select></div>
<div class="tc"><div class="lb">Tempo</div><div class="br"><button class="bb" onclick="nudgeBpm(-1)">&minus;</button><input type="number" id="bpmVal" class="bi" min="20" max="300" value="120"><button class="bb" onclick="nudgeBpm(1)">&plus;</button><span style="font-size:.72rem;color:var(--d)">BPM</span><span id="bpmExt" class="eb">EXT</span></div></div></div>
<div class="pb"><span class="pl">Preset</span><div id="pbtns"></div><button class="sb" onclick="savePreset()">Save</button></div>
<div class="pB"><div class="pC"><div class="pk">POT 1 &mdash; CC 20</div><div class="pr"><input type="range" id="pot0" min="0" max="127" value="0" oninput="potMove(0,this.value)"><span class="pv" id="pv0">0</span></div></div>
<div class="pC"><div class="pk">POT 2 &mdash; CC 21</div><div class="pr"><input type="range" id="pot1" min="0" max="127" value="0" oninput="potMove(1,this.value)"><span class="pv" id="pv1">0</span></div></div></div>
<div class="st">Footswitches</div><div class="gr" id="grid"></div>
</main><footer>192.168.4.1 | MIDI Morpher</footer>
<script>
var M=["PC","CC","CC Latch","Note","Ramp","Ramp Latch","Ramp Inv","Ramp Inv L","LFO Sine","LFO Sine L","LFO Tri","LFO Tri L","LFO Sq","LFO Sq L","Step","Step Latch","Step Inv","Step Inv L","Random","Random L","Random Inv","Random Inv L","Helix Snap","Helix Scrl","QC Scene","QC Scrl","Fractal Scene","Fract Scrl","Kemper Slot","Kemper Scrl","Tap Tempo","System"];
var MG=[{l:"Basic",s:0,c:4},{l:"Ramper",s:4,c:4},{l:"LFO",s:8,c:6},{l:"Stepper",s:14,c:4},{l:"Random",s:18,c:4},{l:"Scenes",s:22,c:8},{l:"Utility",s:30,c:2}];
var MOD=new Set([4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21]);
var LAT=new Set([2,5,7,9,11,13,15,17,19,21]);
var HN={0:"PC#",3:"Note",22:"1-8",23:"max",24:"1-8",25:"max",26:"1-8",27:"max",28:"1-5",29:"max",30:"tap"};
var MX={22:8,23:8,24:8,25:8,26:8,27:8,28:5,29:5,31:8};
var TT=30,SY=31;
var SC=["Play","Stop","Continue","Record","Pause","Rewind","FFwd","GotoStart"];
function mx(i){return MX[i]||128}
var NV=["1/32T","1/32","1/16T","1/32.","1/16","1/8T","1/16.","1/8","1/4T","1/8.","1/4","1/2T","1/4.","1/2","1/2.","1/1","2/1"];
var CF=2147483648;
var aP=0,uD=false,pT=[null,null],bT=null;
var bdg=document.getElementById('badge');
function mD(){uD=true;bdg.className='bg un';bdg.textContent='\u25CF Unsaved';}
function mC(){uD=false;bdg.className='bg sd';bdg.textContent='Saved';setTimeout(function(){if(!uD){bdg.className='bg';bdg.textContent='';}},2500);}
function nudgeBpm(d){var inp=document.getElementById('bpmVal');var v=(parseInt(inp.value)||120)+d;v=Math.max(20,Math.min(300,v));inp.value=v;sBpm(v);}
function onBI(){clearTimeout(bT);bT=setTimeout(function(){var inp=document.getElementById('bpmVal');var v=parseInt(inp.value);if(isNaN(v))return;v=Math.max(20,Math.min(300,v));inp.value=v;sBpm(v);},600);}
function sBpm(v){fetch('/api/bpm',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({bpm:v})});}
async function load(){try{var r=await fetch('/api/state');var s=await r.json();ren(s);}catch(e){document.getElementById('grid').innerHTML='<p style="color:#555;padding:20px">Could not load state.</p>';}}
function ren(s){
aP=s.activePreset||0;
var g=document.getElementById('gch');g.innerHTML='';
for(var i=1;i<=16;i++){var o=document.createElement('option');o.value=i-1;o.textContent='Ch '+i;if(i-1===s.channel)o.selected=true;g.appendChild(o);}
g.onchange=function(){post('/api/channel',{channel:+g.value});};
var bi=document.getElementById('bpmVal');bi.value=s.bpm||120;bi.disabled=!!s.externalSync;bi.oninput=onBI;
document.getElementById('bpmExt').style.display=s.externalSync?'':'none';
document.querySelectorAll('.bb').forEach(function(b){b.disabled=!!s.externalSync;});
var pb=document.getElementById('pbtns');pb.innerHTML='';
for(var i=0;i<6;i++){var b=document.createElement('button');b.className='pn'+(i===aP?' ac':'');b.textContent='P'+(i+1);b.onclick=(function(x){return function(){lP(x);};})(i);pb.appendChild(b);}
var gr=document.getElementById('grid');gr.innerHTML='';
s.buttons.forEach(function(b,i){gr.appendChild(mkC(b,i));});
if(s.presetDirty)mD();else if(!uD){bdg.className='bg';bdg.textContent='';}
}
async function poll(){try{var r=await fetch('/api/poll');var s=await r.json();var bi=document.getElementById('bpmVal');if(document.activeElement!==bi)bi.value=s.bpm||120;var ex=!!s.externalSync;bi.disabled=ex;document.getElementById('bpmExt').style.display=ex?'':'none';document.querySelectorAll('.bb').forEach(function(b){b.disabled=ex;});if(s.activePreset!==aP){aP=s.activePreset;load();return;}if(s.presetDirty&&!uD)mD();else if(!s.presetDirty&&uD){uD=false;bdg.className='bg';bdg.textContent='';}}catch(e){}}
setInterval(poll,2000);
async function lP(i){await fetch('/api/preset/load/'+i,{method:'POST'});uD=false;await load();}
async function savePreset(){bdg.className='bg sv';bdg.textContent='Saving...';await fetch('/api/preset/save/'+aP,{method:'POST'});mC();}
function potMove(id,v){document.getElementById('pv'+id).textContent=v;clearTimeout(pT[id]);pT[id]=setTimeout(function(){fetch('/api/pot',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({id:id,value:+v})});},40);}
function bMS(sel,ci){MG.forEach(function(g){var og=document.createElement('optgroup');og.label=g.l;for(var j=0;j<g.c;j++){var mi=g.s+j;var o=document.createElement('option');o.value=mi;o.textContent=M[mi];if(mi===ci)o.selected=true;og.appendChild(o);}sel.appendChild(og);});}
function mkC(b,i){
var ext=i>=4,isMod=MOD.has(b.modeIndex);
var d=document.createElement('div');d.className='cd';
d.innerHTML='<div class="ct"><span class="cn">'+b.name+'</span><span class="ch '+(ext?'ce':'co')+'">'+(ext?'EXT':'ON')+'</span></div><div class="cb"><div class="fd"><div class="fl">Mode</div><select id="m'+i+'"></select></div><div class="fd"><div class="fl"><span id="lb'+i+'">MIDI #</span> <span class="hn" id="h'+i+'"></span></div><input type="number" id="n'+i+'" min="1" max="'+mx(b.modeIndex)+'" value="'+Math.min(b.midiNumber+1,mx(b.modeIndex))+'"'+(b.modeIndex===TT?' disabled':'')+(b.modeIndex===SY?' style="display:none"':'')+'><select id="ns'+i+'"'+(b.modeIndex===SY?'':' style="display:none"')+'></select></div><div class="fd"><div class="fl">Channel</div><select id="c'+i+'"></select></div><hr class="dv"><div class="rw'+(isMod?'':' hd')+'" id="r'+i+'"><div class="rg"><div><div class="rl">Up <span class="sy">sync</span><label class="tg"><input type="checkbox" id="us'+i+'"'+(b.rampUpSync?' checked':'')+'><span class="tt"></span></label></div><div class="sr"><input type="range" id="u'+i+'" min="0" max="5000" step="50" value="'+(b.rampUpSync?0:b.rampUpMs)+'"'+(b.rampUpSync?' style="display:none"':'')+'><select id="un'+i+'"'+(b.rampUpSync?'':' style="display:none"')+'></select><span class="sv2" id="uv'+i+'"></span></div></div><div><div class="rl">Down <span class="sy">sync</span><label class="tg"><input type="checkbox" id="ds'+i+'"'+(b.rampDownSync?' checked':'')+'><span class="tt"></span></label></div><div class="sr"><input type="range" id="dn'+i+'" min="0" max="5000" step="50" value="'+(b.rampDownSync?0:b.rampDownMs)+'"'+(b.rampDownSync?' style="display:none"':'')+'><select id="dnn'+i+'"'+(b.rampDownSync?'':' style="display:none"')+'></select><span class="sv2" id="dv'+i+'"></span></div></div></div></div></div><button class="tb2" id="trig'+i+'">Trigger</button>';
var ms=d.querySelector('#m'+i);bMS(ms,b.modeIndex);
var ss=d.querySelector('#ns'+i);SC.forEach(function(n,ci){var o=document.createElement('option');o.value=ci;o.textContent=n;if(b.modeIndex===SY&&ci===b.midiNumber)o.selected=true;ss.appendChild(o);});ss.onchange=function(){sch(i);};
sH(i,b.modeIndex,d);sFL(i,b.modeIndex,d);
ms.onchange=function(){var mi=+ms.value;sH(i,mi);sFL(i,mi);d.querySelector('#r'+i).classList.toggle('hd',!MOD.has(mi));var nI=d.querySelector('#n'+i);var iS=mi===SY;nI.style.display=iS?'none':'';ss.style.display=iS?'':'none';var m=mx(mi);nI.max=m;nI.disabled=mi===TT;if(+nI.value>m)nI.value=m;if(+nI.value<1)nI.value=1;bT2(d.querySelector('#trig'+i),i,mi);sch(i);};
var cs=d.querySelector('#c'+i);var og=document.createElement('option');og.value=255;og.textContent='Global';if(b.fsChannel===255)og.selected=true;cs.appendChild(og);
for(var c=1;c<=16;c++){var o=document.createElement('option');o.value=c-1;o.textContent='Ch '+c;if(c-1===b.fsChannel)o.selected=true;cs.appendChild(o);}
var nI=d.querySelector('#n'+i);nI.oninput=function(){sH(i,+ms.value);sch(i);};
nI.onblur=function(){var m=+nI.max||128;var v=+nI.value;if(isNaN(v)||v<1)v=1;if(v>m)v=m;if(+nI.value!==v){nI.value=v;sch(i);}sH(i,+ms.value);};
cs.onchange=function(){sch(i);};
var uN=d.querySelector('#un'+i);NV.forEach(function(n,ni){var o=document.createElement('option');o.value=ni;o.textContent=n;if(b.rampUpSync&&ni===b.rampUpNoteIdx)o.selected=true;uN.appendChild(o);});
d.querySelector('#uv'+i).textContent=b.rampUpSync?NV[b.rampUpNoteIdx||0]:fmt(b.rampUpMs);
d.querySelector('#u'+i).oninput=function(e){d.querySelector('#uv'+i).textContent=fmt(+e.target.value);sch(i);};
uN.onchange=function(){d.querySelector('#uv'+i).textContent=NV[+uN.value];sch(i);};
d.querySelector('#us'+i).onchange=function(e){var on=e.target.checked;d.querySelector('#u'+i).style.display=on?'none':'';uN.style.display=on?'':'none';d.querySelector('#uv'+i).textContent=on?NV[+uN.value]:fmt(+d.querySelector('#u'+i).value);sch(i);};
var dN=d.querySelector('#dnn'+i);NV.forEach(function(n,ni){var o=document.createElement('option');o.value=ni;o.textContent=n;if(b.rampDownSync&&ni===b.rampDownNoteIdx)o.selected=true;dN.appendChild(o);});
d.querySelector('#dv'+i).textContent=b.rampDownSync?NV[b.rampDownNoteIdx||0]:fmt(b.rampDownMs);
d.querySelector('#dn'+i).oninput=function(e){d.querySelector('#dv'+i).textContent=fmt(+e.target.value);sch(i);};
dN.onchange=function(){d.querySelector('#dv'+i).textContent=NV[+dN.value];sch(i);};
d.querySelector('#ds'+i).onchange=function(e){var on=e.target.checked;d.querySelector('#dn'+i).style.display=on?'none':'';dN.style.display=on?'':'none';d.querySelector('#dv'+i).textContent=on?NV[+dN.value]:fmt(+d.querySelector('#dn'+i).value);sch(i);};
bT2(d.querySelector('#trig'+i),i,b.modeIndex);
return d;}
function bT2(tb,i,mi){var nb=tb.cloneNode(true);tb.parentNode.replaceChild(nb,tb);if(LAT.has(mi)){nb.onclick=function(){tC(i,nb);};}else{nb.addEventListener('mousedown',function(e){e.preventDefault();tP(i,nb);});nb.addEventListener('mouseup',function(){tR(i,nb);});nb.addEventListener('mouseleave',function(){if(nb.classList.contains('on'))tR(i,nb);});nb.addEventListener('touchstart',function(e){e.preventDefault();tP(i,nb);},{passive:false});nb.addEventListener('touchend',function(){tR(i,nb);});}}
async function tC(x,b){b.classList.add('on');await fetch('/api/button/'+x+'/press',{method:'POST'});await fetch('/api/button/'+x+'/release',{method:'POST'});setTimeout(function(){b.classList.remove('on');},300);}
async function tP(x,b){b.classList.add('on');await fetch('/api/button/'+x+'/press',{method:'POST'});}
async function tR(x,b){b.classList.remove('on');await fetch('/api/button/'+x+'/release',{method:'POST'});}
var sT={};function sch(i){mD();clearTimeout(sT[i]);sT[i]=setTimeout(function(){aB(i);},600);}
async function aB(i){var mi=+document.getElementById('m'+i).value;var uS=document.getElementById('us'+i).checked;var dS=document.getElementById('ds'+i).checked;var uV=uS?CF+(+document.getElementById('un'+i).value):+document.getElementById('u'+i).value;var dV=dS?CF+(+document.getElementById('dnn'+i).value):+document.getElementById('dn'+i).value;var mN;if(mi===SY){mN=+document.getElementById('ns'+i).value;}else{var m=mx(mi);var n=+document.getElementById('n'+i).value;if(isNaN(n)||n<1)n=1;if(n>m)n=m;mN=n-1;}await post('/api/button/'+i,{modeIndex:mi,midiNumber:mN,fsChannel:+document.getElementById('c'+i).value,rampUpMs:uV,rampDownMs:dV});}
async function post(u,d){await fetch(u,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(d)});}
function sH(i,mi,r){var s=r||document;var e=s.querySelector('#h'+i);if(!e)return;if(mi===SY){e.textContent='';return;}e.textContent='('+(HN[mi]||'CC#')+')';}
function sFL(i,mi,r){var s=r||document;var e=s.querySelector('#lb'+i);if(!e)return;e.textContent=mi===SY?'Command':'MIDI #';}
function fmt(ms){return(ms/1000).toFixed(1)+'s';}
load();
</script></body></html>
)rawliteral";
