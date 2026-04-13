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
  --grn:#00e676;--red:#ff5252;--rad:10px;--tr:.18s ease;
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
  border-radius:7px;color:var(--acc);font-size:.8rem;font-weight:800;
}
.logo-text{font-size:.95rem;font-weight:700;letter-spacing:.07em;text-transform:uppercase;color:var(--acc)}
.logo-sub{font-size:.68rem;color:var(--dim);letter-spacing:.04em;margin-top:1px}
.hdr-r{display:flex;align-items:center;gap:10px}
.badge{
  font-size:.7rem;letter-spacing:.03em;padding:3px 9px;border-radius:20px;
  color:transparent;transition:color var(--tr),background var(--tr);
}
.badge.unsaved{color:var(--red);background:rgba(255,82,82,.1)}
.badge.saving{color:var(--acc);background:var(--acc2)}
.badge.saved{color:var(--grn);background:rgba(0,230,118,.08)}

main{max-width:1100px;margin:0 auto;padding:20px}

/* Global bar */
.gbar{
  display:flex;align-items:center;gap:14px;flex-wrap:wrap;
  background:var(--s1);border:1px solid var(--bdr);
  border-radius:var(--rad);padding:14px 18px;margin-bottom:14px;
}
.gbar label{font-size:.73rem;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:var(--dim)}
.gbar select{
  background:var(--s2);color:var(--txt);border:1px solid var(--bdr);border-radius:7px;
  padding:6px 28px 6px 10px;font-size:.83rem;outline:none;cursor:pointer;
  -webkit-appearance:none;
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23555'/%3E%3C/svg%3E");
  background-repeat:no-repeat;background-position:right 10px center;
}
.gbar select:focus{border-color:var(--acc)}
.sep{width:1px;height:22px;background:var(--bdr);flex-shrink:0}
.latch-row{display:flex;align-items:center;gap:8px;font-size:.75rem;color:var(--dim)}

/* Toggle */
.tog{position:relative;width:40px;height:22px;cursor:pointer;flex-shrink:0}
.tog input{opacity:0;width:0;height:0;position:absolute}
.tog-t{position:absolute;inset:0;background:#2a2a2a;border-radius:22px;transition:var(--tr)}
.tog-t::after{content:'';position:absolute;left:3px;top:3px;width:16px;height:16px;background:#666;border-radius:50%;transition:var(--tr)}
.tog input:checked~.tog-t{background:var(--acc)}
.tog input:checked~.tog-t::after{background:#fff;transform:translateX(18px);box-shadow:0 1px 4px rgba(0,0,0,.5)}

/* Preset bar */
.preset-bar{
  display:flex;align-items:center;gap:8px;flex-wrap:wrap;
  background:var(--s1);border:1px solid var(--bdr);
  border-radius:var(--rad);padding:12px 18px;margin-bottom:14px;
}
.preset-bar span{font-size:.73rem;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:var(--dim);margin-right:4px}
.pbtn{
  padding:5px 14px;border-radius:20px;border:1px solid var(--bdr);
  background:var(--s2);color:var(--dim);font-size:.75rem;font-weight:700;
  cursor:pointer;letter-spacing:.04em;transition:all var(--tr);
}
.pbtn:hover{border-color:#444;color:var(--txt)}
.pbtn.active{background:var(--acc2);border-color:rgba(0,217,255,.5);color:var(--acc)}
.save-btn{
  margin-left:auto;padding:5px 16px;border-radius:20px;
  border:1px solid rgba(0,217,255,.3);background:var(--acc2);
  color:var(--acc);font-size:.75rem;font-weight:700;cursor:pointer;
  letter-spacing:.04em;transition:all var(--tr);
}
.save-btn:hover{background:rgba(0,217,255,.2);border-color:var(--acc)}
.save-btn:active{transform:scale(.97)}

/* Pots bar */
.pots-bar{
  display:grid;grid-template-columns:1fr 1fr;gap:14px;
  background:var(--s1);border:1px solid var(--bdr);
  border-radius:var(--rad);padding:14px 18px;margin-bottom:18px;
}
.pot-item .plbl{font-size:.73rem;font-weight:600;text-transform:uppercase;letter-spacing:.08em;color:var(--dim);margin-bottom:6px}
.pot-row{display:flex;align-items:center;gap:8px}
.pot-val{font-size:.75rem;color:var(--acc);min-width:28px;text-align:right;font-variant-numeric:tabular-nums}

/* Footswitch grid */
.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:14px}
@media(max-width:860px){.grid{grid-template-columns:repeat(2,1fr)}}
@media(max-width:540px){.grid{grid-template-columns:1fr};main{padding:12px}}

.card{
  background:var(--s1);border:1px solid var(--bdr);
  border-radius:var(--rad);padding:18px;
  transition:border-color var(--tr),box-shadow var(--tr);
}
.card:hover{border-color:#333;box-shadow:0 4px 20px rgba(0,0,0,.5)}

.card-hdr{display:flex;align-items:center;gap:8px;margin-bottom:16px}
.card-name{font-size:.83rem;font-weight:700;text-transform:uppercase;letter-spacing:.08em;flex:1}
.chip{font-size:.62rem;padding:2px 7px;border-radius:20px;font-weight:600;text-transform:uppercase;letter-spacing:.05em}
.chip-on{background:var(--acc2);color:var(--acc);border:1px solid rgba(0,217,255,.22)}
.chip-ext{background:rgba(255,255,255,.03);color:var(--dim);border:1px solid var(--bdr)}

/* Trigger button */
.trig-btn{
  padding:3px 10px;border-radius:6px;border:1px solid var(--bdr);
  background:var(--s2);color:var(--dim);font-size:.7rem;cursor:pointer;
  transition:all var(--tr);user-select:none;-webkit-user-select:none;
}
.trig-btn:hover{border-color:#444;color:var(--txt)}
.trig-btn.on{background:rgba(0,230,118,.15);border-color:rgba(0,230,118,.4);color:var(--grn)}

.fld{margin-bottom:12px}
.fld:last-child{margin-bottom:0}
.lbl{font-size:.66rem;font-weight:600;text-transform:uppercase;letter-spacing:.07em;color:var(--dim);margin-bottom:5px;display:flex;align-items:center;gap:5px}
.hint{font-weight:400;color:#444;text-transform:none;letter-spacing:0;font-size:.64rem}
.fld select,.fld input[type=number]{
  width:100%;background:var(--s2);color:var(--txt);border:1px solid var(--bdr);
  border-radius:7px;padding:7px 10px;font-size:.82rem;outline:none;
  transition:border-color var(--tr);-webkit-appearance:none;
}
.fld select{
  cursor:pointer;background-color:var(--s2);
  background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23555'/%3E%3C/svg%3E");
  background-repeat:no-repeat;background-position:right 10px center;padding-right:26px;
}
.fld select:focus,.fld input[type=number]:focus{border-color:var(--acc)}
hr.div{border:none;border-top:1px solid var(--bdr);margin:12px 0}

/* Range sliders */
.ramp-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}
.rlbl{font-size:.65rem;font-weight:600;text-transform:uppercase;letter-spacing:.07em;color:var(--dim);margin-bottom:4px}
.srow{display:flex;align-items:center;gap:7px}
input[type=range]{
  -webkit-appearance:none;flex:1;height:3px;background:var(--bdr);border-radius:2px;outline:none;cursor:pointer;
}
input[type=range]::-webkit-slider-thumb{
  -webkit-appearance:none;width:14px;height:14px;border-radius:50%;
  background:var(--acc);cursor:pointer;box-shadow:0 0 5px rgba(0,217,255,.4);
}
.sval{font-size:.71rem;color:var(--acc);min-width:34px;text-align:right;font-variant-numeric:tabular-nums}
.ramp-wrap.hidden{display:none}

footer{text-align:center;padding:28px;font-size:.68rem;color:#333;letter-spacing:.04em}
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
  </div>
</header>

<main>
  <!-- Global channel + BPM + latching toggle -->
  <div class="gbar">
    <label>MIDI Channel</label>
    <select id="gch"></select>
    <div class="sep"></div>
    <label>BPM</label>
    <span id="bpmVal" style="font-size:.83rem;color:var(--acc);font-variant-numeric:tabular-nums;min-width:36px">--</span>
    <span id="bpmExt" style="font-size:.68rem;color:var(--dim);letter-spacing:.05em"></span>
    <div class="sep"></div>
    <div class="latch-row">
      <span>Latching</span>
      <label class="tog">
        <input type="checkbox" id="latchTog">
        <span class="tog-t"></span>
      </label>
    </div>
  </div>

  <!-- Preset selector bar -->
  <div class="preset-bar">
    <span>Preset</span>
    <div id="pbtns"></div>
    <button class="save-btn" id="saveBtn" onclick="savePreset()">Save Preset</button>
  </div>

  <!-- Pot virtual controls -->
  <div class="pots-bar">
    <div class="pot-item">
      <div class="plbl">POT1 — CC 20</div>
      <div class="pot-row">
        <input type="range" id="pot0" min="0" max="127" value="0" oninput="potMove(0,this.value)">
        <span class="sval" id="pv0">0</span>
      </div>
    </div>
    <div class="pot-item">
      <div class="plbl">POT2 — CC 21</div>
      <div class="pot-row">
        <input type="range" id="pot1" min="0" max="127" value="0" oninput="potMove(1,this.value)">
        <span class="sval" id="pv1">0</span>
      </div>
    </div>
  </div>

  <!-- Footswitch cards -->
  <div class="grid" id="grid"></div>
</main>

<footer>192.168.4.1 &nbsp;|&nbsp; MIDI Morpher v2.0</footer>

<script>
const MODES=[
  "PC","CC","CC Latch","Note",
  "Ramp","Ramp Latch","Ramp Inv","Ramp Inv L",
  "LFO Sine","LFO Sine L","LFO Tri","LFO Tri L","LFO Sq","LFO Sq L",
  "Step","Step Latch","Step Inv","Step Inv L",
  "Random","Random L","Random Inv","Random Inv L",
  "Helix Snap","QC Scene","Fractal Scene","Kemper Slot",
  "Tap Tempo"
];
// Indices of modulation modes (show ramp sliders)
const MOD=new Set([4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21]);
// Indices of latching modes (click-toggle trigger, not hold)
const LATCHING=new Set([2,5,7,9,11,13,15,17,19,21]);
const HINT={0:"PC#",3:"Note",22:"0-7",23:"0-7",24:"0-7",25:"CC 50-54",26:"tap"};

// Note-value labels — order must match noteValueNames[] in midiClock.h
const NOTE_VALUES=[
  "1/32T","1/32","1/16T","1/32.","1/16","1/8T","1/16.","1/8",
  "1/4T","1/8.","1/4","1/2T","1/4.","1/2","1/2.","1/1","2/1"
];
// Bit-31 flag — use addition (not |) because JS bitwise ops return signed 32-bit.
const CLOCK_SYNC_FLAG=2147483648;

let activePreset=0;
let uiDirty=false;
let potTimers=[null,null];
const badge=document.getElementById('badge');

function markDirty(){
  uiDirty=true;
  badge.className='badge unsaved';
  badge.textContent='● Unsaved';
}
function markClean(){
  uiDirty=false;
  badge.className='badge saved';
  badge.textContent='Saved';
  setTimeout(()=>{if(!uiDirty){badge.className='badge';badge.textContent='';}},2500);
}

// ── State load ────────────────────────────────────────────────────────────────
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
  activePreset=s.activePreset||0;

  // Global channel
  const g=document.getElementById('gch');
  g.innerHTML='';
  for(let i=1;i<=16;i++){
    const o=document.createElement('option');
    o.value=i-1;o.textContent='Ch '+i;
    if(i-1===s.channel)o.selected=true;
    g.appendChild(o);
  }
  g.onchange=()=>post('/api/channel',{channel:+g.value});

  // BPM readout
  document.getElementById('bpmVal').textContent=s.bpm||'--';
  document.getElementById('bpmExt').textContent=s.externalSync?'EXT':'';

  // Latching toggle
  const lt=document.getElementById('latchTog');
  lt.checked=!!s.latching;
  lt.onchange=()=>post('/api/latching',{latching:lt.checked});

  // Preset buttons
  const pb=document.getElementById('pbtns');
  pb.innerHTML='';
  for(let i=0;i<6;i++){
    const b=document.createElement('button');
    b.className='pbtn'+(i===activePreset?' active':'');
    b.textContent='P'+(i+1);
    b.onclick=()=>loadPreset(i);
    pb.appendChild(b);
  }

  // Footswitch cards
  const grid=document.getElementById('grid');
  grid.innerHTML='';
  s.buttons.forEach((b,i)=>grid.appendChild(mkCard(b,i)));

  // If server says dirty, reflect it
  if(s.presetDirty) markDirty();
  else if(!uiDirty){ badge.className='badge'; badge.textContent=''; }
}

// ── Preset actions ────────────────────────────────────────────────────────────
async function loadPreset(idx){
  await fetch('/api/preset/load/'+idx,{method:'POST'});
  uiDirty=false;
  await load();
}

async function savePreset(){
  badge.className='badge saving';
  badge.textContent='Saving...';
  await fetch('/api/preset/save/'+activePreset,{method:'POST'});
  markClean();
}

// ── Pot controls ──────────────────────────────────────────────────────────────
function potMove(id,val){
  document.getElementById('pv'+id).textContent=val;
  clearTimeout(potTimers[id]);
  potTimers[id]=setTimeout(()=>{
    fetch('/api/pot',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({id:id,value:+val})});
  },40);
}

// ── Footswitch card builder ───────────────────────────────────────────────────
function mkCard(b,i){
  const ext=i>=4;
  const isMod=MOD.has(b.modeIndex);
  const d=document.createElement('div');
  d.className='card';
  d.innerHTML=`
<div class="card-hdr">
  <span class="card-name">${b.name}</span>
  <span class="chip ${ext?'chip-ext':'chip-on'}">${ext?'Ext':'Onboard'}</span>
  <button class="trig-btn" id="trig${i}" title="Trigger footswitch">&#9654;</button>
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
      <div class="rlbl">Ramp Up
        <label class="tog" style="width:28px;height:16px;margin-left:4px;vertical-align:middle">
          <input type="checkbox" id="us${i}" ${b.rampUpSync?'checked':''}>
          <span class="tog-t" style="border-radius:16px"></span>
        </label>
        <span style="font-size:.58rem;color:var(--dim)">sync</span>
      </div>
      <div class="srow">
        <input type="range" id="u${i}" min="0" max="5000" step="50" value="${b.rampUpSync?0:b.rampUpMs}" ${b.rampUpSync?'style="display:none"':''}>
        <select id="un${i}" ${b.rampUpSync?'':'style="display:none"'}></select>
        <span class="sval" id="uv${i}"></span>
      </div>
    </div>
    <div>
      <div class="rlbl">Ramp Down
        <label class="tog" style="width:28px;height:16px;margin-left:4px;vertical-align:middle">
          <input type="checkbox" id="ds${i}" ${b.rampDownSync?'checked':''}>
          <span class="tog-t" style="border-radius:16px"></span>
        </label>
        <span style="font-size:.58rem;color:var(--dim)">sync</span>
      </div>
      <div class="srow">
        <input type="range" id="dn${i}" min="0" max="5000" step="50" value="${b.rampDownSync?0:b.rampDownMs}" ${b.rampDownSync?'style="display:none"':''}>
        <select id="dnn${i}" ${b.rampDownSync?'':'style="display:none"'}></select>
        <span class="sval" id="dv${i}"></span>
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

  // Ramp Up — populate note select + wire slider/select/sync toggle
  const uNoteSel=d.querySelector('#un'+i);
  NOTE_VALUES.forEach((n,ni)=>{
    const o=document.createElement('option');
    o.value=ni;o.textContent=n;
    if(b.rampUpSync&&ni===b.rampUpNoteIdx)o.selected=true;
    uNoteSel.appendChild(o);
  });
  d.querySelector('#uv'+i).textContent=b.rampUpSync?NOTE_VALUES[b.rampUpNoteIdx||0]:fmt(b.rampUpMs);
  d.querySelector('#u'+i).oninput=e=>{
    d.querySelector('#uv'+i).textContent=fmt(+e.target.value);sched(i);
  };
  uNoteSel.onchange=()=>{
    d.querySelector('#uv'+i).textContent=NOTE_VALUES[+uNoteSel.value];sched(i);
  };
  d.querySelector('#us'+i).onchange=e=>{
    const on=e.target.checked;
    d.querySelector('#u'+i).style.display=on?'none':'';
    uNoteSel.style.display=on?'':'none';
    d.querySelector('#uv'+i).textContent=on?NOTE_VALUES[+uNoteSel.value]:fmt(+d.querySelector('#u'+i).value);
    sched(i);
  };

  // Ramp Down — same structure
  const dNoteSel=d.querySelector('#dnn'+i);
  NOTE_VALUES.forEach((n,ni)=>{
    const o=document.createElement('option');
    o.value=ni;o.textContent=n;
    if(b.rampDownSync&&ni===b.rampDownNoteIdx)o.selected=true;
    dNoteSel.appendChild(o);
  });
  d.querySelector('#dv'+i).textContent=b.rampDownSync?NOTE_VALUES[b.rampDownNoteIdx||0]:fmt(b.rampDownMs);
  d.querySelector('#dn'+i).oninput=e=>{
    d.querySelector('#dv'+i).textContent=fmt(+e.target.value);sched(i);
  };
  dNoteSel.onchange=()=>{
    d.querySelector('#dv'+i).textContent=NOTE_VALUES[+dNoteSel.value];sched(i);
  };
  d.querySelector('#ds'+i).onchange=e=>{
    const on=e.target.checked;
    d.querySelector('#dn'+i).style.display=on?'none':'';
    dNoteSel.style.display=on?'':'none';
    d.querySelector('#dv'+i).textContent=on?NOTE_VALUES[+dNoteSel.value]:fmt(+d.querySelector('#dn'+i).value);
    sched(i);
  };

  // Trigger button — latching: click-toggle; momentary: hold
  const tb=d.querySelector('#trig'+i);
  const latching=LATCHING.has(b.modeIndex)||!MOD.has(b.modeIndex)&&b.isLatching;
  if(LATCHING.has(b.modeIndex)){
    // Latching mode: single click = press+release (toggles state)
    tb.onclick=()=>trigClick(i,tb);
  } else {
    // Momentary: hold = active, release = off
    tb.addEventListener('mousedown', e=>{ e.preventDefault(); trigPress(i,tb); });
    tb.addEventListener('mouseup',   ()=>trigRelease(i,tb));
    tb.addEventListener('touchstart',e=>{ e.preventDefault(); trigPress(i,tb); },{passive:false});
    tb.addEventListener('touchend',  ()=>trigRelease(i,tb));
  }

  return d;
}

// ── Trigger helpers ───────────────────────────────────────────────────────────
async function trigClick(idx,btn){
  btn.classList.add('on');
  await fetch('/api/button/'+idx+'/press',{method:'POST'});
  await fetch('/api/button/'+idx+'/release',{method:'POST'});
  setTimeout(()=>btn.classList.remove('on'),300);
}
async function trigPress(idx,btn){
  btn.classList.add('on');
  await fetch('/api/button/'+idx+'/press',{method:'POST'});
}
async function trigRelease(idx,btn){
  btn.classList.remove('on');
  await fetch('/api/button/'+idx+'/release',{method:'POST'});
}

// ── Config save ───────────────────────────────────────────────────────────────
let saveTimers={};
function sched(i){
  markDirty();
  clearTimeout(saveTimers[i]);
  saveTimers[i]=setTimeout(()=>applyBtn(i),600);
}

async function applyBtn(i){
  const uSync=document.getElementById('us'+i).checked;
  const dSync=document.getElementById('ds'+i).checked;
  const uVal=uSync
    ? CLOCK_SYNC_FLAG+(+document.getElementById('un'+i).value)
    : +document.getElementById('u'+i).value;
  const dVal=dSync
    ? CLOCK_SYNC_FLAG+(+document.getElementById('dnn'+i).value)
    : +document.getElementById('dn'+i).value;
  await post('/api/button/'+i,{
    modeIndex:+document.getElementById('m'+i).value,
    midiNumber:+document.getElementById('n'+i).value,
    fsChannel:+document.getElementById('c'+i).value,
    rampUpMs:uVal,
    rampDownMs:dVal
  });
}

async function post(url,data){
  await fetch(url,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(data)});
}

function setHint(i,mi){
  const el=document.getElementById('h'+i);
  if(el)el.textContent='('+(HINT[mi]||(MOD.has(mi)?'CC#':'CC#'))+')';
}

function fmt(ms){return(ms/1000).toFixed(1)+'s'}

load();
</script>
</body>
</html>
)rawliteral";
