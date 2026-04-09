#pragma once

static const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MIDI Morpher</title>
<style>
:root{
  --bg:#080808;--s1:#111;--s2:#191919;--s3:#222;
  --acc:#00d9ff;--acc2:rgba(0,217,255,0.1);
  --txt:#ddd;--dim:#555;--bdr:#252525;
  --grn:#00e676;--rad:10px;--tr:.18s ease;
}
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',system-ui,sans-serif;background:var(--bg);color:var(--txt);min-height:100vh}

header{
  background:var(--s1);border-bottom:1px solid var(--bdr);
  padding:14px 20px;display:flex;align-items:center;
  justify-content:space-between;position:sticky;top:0;z-index:99;
}
.logo{display:flex;align-items:center;gap:10px}
.logo-icon{
  width:30px;height:30px;display:flex;align-items:center;justify-content:center;
  background:var(--acc2);border:1px solid rgba(0,217,255,0.35);
  border-radius:7px;color:var(--acc);font-size:.8rem;font-weight:800;letter-spacing:-.02em;
}
.logo-text{font-size:.95rem;font-weight:700;letter-spacing:.07em;text-transform:uppercase;color:var(--acc)}
.logo-sub{font-size:.68rem;color:var(--dim);letter-spacing:.04em;margin-top:1px}

.hdr-r{display:flex;align-items:center;gap:16px}
.badge{
  font-size:.7rem;letter-spacing:.03em;padding:3px 9px;border-radius:20px;
  color:transparent;transition:color var(--tr),background var(--tr);
}
.badge.saving{color:var(--acc);background:var(--acc2)}
.badge.saved{color:var(--grn);background:rgba(0,230,118,.08)}

.wifi-row{display:flex;align-items:center;gap:7px;font-size:.75rem;color:var(--dim)}
.wdot{width:7px;height:7px;border-radius:50%;background:#2a2a2a;transition:var(--tr)}
.wdot.on{background:var(--acc);box-shadow:0 0 7px var(--acc)}

.tog{position:relative;width:40px;height:22px;cursor:pointer;flex-shrink:0}
.tog input{opacity:0;width:0;height:0;position:absolute}
.tog-t{position:absolute;inset:0;background:#2a2a2a;border-radius:22px;transition:var(--tr)}
.tog-t::after{
  content:'';position:absolute;left:3px;top:3px;
  width:16px;height:16px;background:#666;border-radius:50%;transition:var(--tr);
}
.tog input:checked~.tog-t{background:var(--acc)}
.tog input:checked~.tog-t::after{background:#fff;transform:translateX(18px);box-shadow:0 1px 4px rgba(0,0,0,.5)}

main{max-width:1100px;margin:0 auto;padding:20px}

.gbar{
  display:flex;align-items:center;gap:10px;
  background:var(--s1);border:1px solid var(--bdr);
  border-radius:var(--rad);padding:14px 18px;margin-bottom:18px;
}
.gbar label{font-size:.73rem;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:var(--dim)}
.gbar select{
  margin-left:auto;background:var(--s2);color:var(--txt);
  border:1px solid var(--bdr);border-radius:7px;
  padding:6px 10px;font-size:.83rem;min-width:120px;
  outline:none;cursor:pointer;transition:border-color var(--tr);
  -webkit-appearance:none;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23555'/%3E%3C/svg%3E");
  background-repeat:no-repeat;background-position:right 10px center;padding-right:26px;
}
.gbar select:focus{border-color:var(--acc)}

.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:14px}
@media(max-width:860px){.grid{grid-template-columns:repeat(2,1fr)}}
@media(max-width:540px){.grid{grid-template-columns:1fr};main{padding:12px}}

.card{
  background:var(--s1);border:1px solid var(--bdr);
  border-radius:var(--rad);padding:18px;
  transition:border-color var(--tr),box-shadow var(--tr);
}
.card:hover{border-color:#333;box-shadow:0 4px 20px rgba(0,0,0,.5)}

.card-hdr{display:flex;align-items:center;justify-content:space-between;margin-bottom:16px}
.card-name{font-size:.83rem;font-weight:700;text-transform:uppercase;letter-spacing:.08em}
.chip{
  font-size:.62rem;padding:2px 7px;border-radius:20px;
  font-weight:600;text-transform:uppercase;letter-spacing:.05em;
}
.chip-on{background:var(--acc2);color:var(--acc);border:1px solid rgba(0,217,255,.22)}
.chip-ext{background:rgba(255,255,255,.03);color:var(--dim);border:1px solid var(--bdr)}

.fld{margin-bottom:12px}
.fld:last-child{margin-bottom:0}
.lbl{
  font-size:.66rem;font-weight:600;text-transform:uppercase;
  letter-spacing:.07em;color:var(--dim);margin-bottom:5px;
  display:flex;align-items:center;gap:5px;
}
.hint{font-weight:400;color:#444;text-transform:none;letter-spacing:0;font-size:.64rem}

.fld select,.fld input[type=number]{
  width:100%;background:var(--s2);color:var(--txt);
  border:1px solid var(--bdr);border-radius:7px;
  padding:7px 10px;font-size:.82rem;outline:none;
  transition:border-color var(--tr);-webkit-appearance:none;
}
.fld select{
  cursor:pointer;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23555'/%3E%3C/svg%3E");
  background-repeat:no-repeat;background-position:right 10px center;
  padding-right:26px;background-color:var(--s2);
}
.fld select:focus,.fld input[type=number]:focus{border-color:var(--acc)}
.fld input[type=number]{cursor:text}

hr.div{border:none;border-top:1px solid var(--bdr);margin:12px 0}

.ramp-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.rlbl{font-size:.65rem;font-weight:600;text-transform:uppercase;letter-spacing:.07em;color:var(--dim);margin-bottom:4px}
.srow{display:flex;align-items:center;gap:7px}
input[type=range]{
  -webkit-appearance:none;flex:1;height:3px;
  background:var(--bdr);border-radius:2px;outline:none;cursor:pointer;
}
input[type=range]::-webkit-slider-thumb{
  -webkit-appearance:none;width:14px;height:14px;border-radius:50%;
  background:var(--acc);cursor:pointer;box-shadow:0 0 5px rgba(0,217,255,.4);
  transition:box-shadow var(--tr);
}
input[type=range]:active::-webkit-slider-thumb{box-shadow:0 0 10px var(--acc)}
.sval{font-size:.71rem;color:var(--acc);min-width:34px;text-align:right;font-variant-numeric:tabular-nums}
.ramp-wrap.hidden{display:none}

footer{
  text-align:center;padding:28px;
  font-size:.68rem;color:#333;letter-spacing:.04em;
}
</style>
</head>
<body>
<header>
  <div class="logo">
    <div class="logo-icon">MM</div>
    <div>
      <div class="logo-text">MIDI Morpher</div>
      <div class="logo-sub">Configuration</div>
    </div>
  </div>
  <div class="hdr-r">
    <span id="badge" class="badge"></span>
    <div class="wifi-row">
      <div class="wdot" id="wdot"></div>
      <span>WiFi</span>
      <label class="tog">
        <input type="checkbox" id="wifiTog">
        <span class="tog-t"></span>
      </label>
    </div>
  </div>
</header>

<main>
  <div class="gbar">
    <label>Global MIDI Channel</label>
    <select id="gch"></select>
  </div>
  <div class="grid" id="grid"></div>
</main>

<footer>192.168.4.1 &nbsp;|&nbsp; MIDI Morpher v1.0</footer>

<script>
const MODES=[
  "PC","CC","CC Latch","Note",
  "Ramp","Ramp Latch","Ramp Inv","Ramp Inv L",
  "LFO Sine","LFO Sine L","LFO Tri","LFO Tri L","LFO Sq","LFO Sq L",
  "Step","Step Latch","Step Inv","Step Inv L",
  "Random","Random L","Random Inv","Random Inv L",
  "Helix Snap","QC Scene","Fractal Scene","Kemper Slot"
];
const MOD=new Set([4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21]);
const HINT={0:"PC#",3:"Note",22:"0-7",23:"0-7",24:"0-7",25:"CC 50-54"};

let saveTimer=null;
const badge=document.getElementById('badge');
const wdot=document.getElementById('wdot');

function setStatus(s){
  badge.className='badge '+s;
  badge.textContent=s==='saving'?'Saving...':s==='saved'?'Saved':'';
}

async function load(){
  try{
    const r=await fetch('/api/state');
    const s=await r.json();
    render(s);
  }catch(e){
    document.getElementById('grid').innerHTML='<p style="color:#555;padding:20px">Could not load state.</p>';
  }
}

function render(s){
  const g=document.getElementById('gch');
  g.innerHTML='';
  for(let i=1;i<=16;i++){
    const o=document.createElement('option');
    o.value=i-1;o.textContent='Ch '+i;
    if(i-1===s.channel)o.selected=true;
    g.appendChild(o);
  }
  g.onchange=()=>post('/api/channel',{channel:+g.value});

  const wt=document.getElementById('wifiTog');
  wt.checked=s.wifiEnabled;
  wdot.className='wdot'+(s.wifiEnabled?' on':'');
  wt.onchange=()=>toggleWifi(wt.checked);

  const grid=document.getElementById('grid');
  grid.innerHTML='';
  s.buttons.forEach((b,i)=>grid.appendChild(mkCard(b,i)));
}

function mkCard(b,i){
  const ext=i>=4;
  const isMod=MOD.has(b.modeIndex);
  const d=document.createElement('div');
  d.className='card';
  d.innerHTML=`
<div class="card-hdr">
  <span class="card-name">${b.name}</span>
  <span class="chip ${ext?'chip-ext':'chip-on'}">${ext?'External':'Onboard'}</span>
</div>
<div class="fld">
  <div class="lbl">Mode</div>
  <select id="m${i}"></select>
</div>
<div class="fld">
  <div class="lbl">MIDI # <span class="hint" id="h${i}"></span></div>
  <input type="number" id="n${i}" min="0" max="127" value="${b.midiNumber}">
</div>
<div class="fld">
  <div class="lbl">Channel</div>
  <select id="c${i}"></select>
</div>
<hr class="div">
<div class="ramp-wrap${isMod?'':' hidden'}" id="r${i}">
  <div class="ramp-grid">
    <div>
      <div class="rlbl">Ramp Up</div>
      <div class="srow">
        <input type="range" id="u${i}" min="0" max="5000" step="50" value="${b.rampUpMs}">
        <span class="sval" id="uv${i}">${fmt(b.rampUpMs)}</span>
      </div>
    </div>
    <div>
      <div class="rlbl">Ramp Down</div>
      <div class="srow">
        <input type="range" id="dn${i}" min="0" max="5000" step="50" value="${b.rampDownMs}">
        <span class="sval" id="dv${i}">${fmt(b.rampDownMs)}</span>
      </div>
    </div>
  </div>
</div>`;

  // Mode dropdown
  const ms=d.querySelector('#m'+i);
  MODES.forEach((m,mi)=>{
    const o=document.createElement('option');
    o.value=mi;o.textContent=m;
    if(mi===b.modeIndex)o.selected=true;
    ms.appendChild(o);
  });
  setHint(i,b.modeIndex);
  ms.onchange=()=>{
    const mi=+ms.value;
    setHint(i,mi);
    d.querySelector('#r'+i).classList.toggle('hidden',!MOD.has(mi));
    sched(i);
  };

  // Channel dropdown
  const cs=d.querySelector('#c'+i);
  const og=document.createElement('option');
  og.value=255;og.textContent='Global';
  if(b.fsChannel===255)og.selected=true;
  cs.appendChild(og);
  for(let c=1;c<=16;c++){
    const o=document.createElement('option');
    o.value=c-1;o.textContent='Ch '+c;
    if(c-1===b.fsChannel)o.selected=true;
    cs.appendChild(o);
  }

  d.querySelector('#n'+i).oninput=()=>sched(i);
  cs.onchange=()=>sched(i);
  d.querySelector('#u'+i).oninput=e=>{
    d.querySelector('#uv'+i).textContent=fmt(+e.target.value);sched(i);
  };
  d.querySelector('#dn'+i).oninput=e=>{
    d.querySelector('#dv'+i).textContent=fmt(+e.target.value);sched(i);
  };
  return d;
}

function setHint(i,mi){
  const el=document.getElementById('h'+i);
  if(el)el.textContent='('+(HINT[mi]||(MOD.has(mi)?'CC#':'CC#'))+')';
}

function fmt(ms){return(ms/1000).toFixed(1)+'s'}

function sched(i){
  setStatus('saving');
  clearTimeout(saveTimer);
  saveTimer=setTimeout(()=>saveBtn(i),600);
}

async function saveBtn(i){
  await post('/api/button/'+i,{
    modeIndex:+document.getElementById('m'+i).value,
    midiNumber:+document.getElementById('n'+i).value,
    fsChannel:+document.getElementById('c'+i).value,
    rampUpMs:+document.getElementById('u'+i).value,
    rampDownMs:+document.getElementById('dn'+i).value
  });
}

async function post(url,data){
  await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});
  setStatus('saved');
  setTimeout(()=>setStatus(''),2500);
}

async function toggleWifi(on){
  if(!on){
    const ok=confirm('Disable WiFi?\n\nTo re-enable: hold FS1 + FS2 together for 3 seconds.');
    if(!ok){document.getElementById('wifiTog').checked=true;return;}
  }
  wdot.className='wdot'+(on?' on':'');
  await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:on})});
}

load();
</script>
</body>
</html>
)rawliteral";
