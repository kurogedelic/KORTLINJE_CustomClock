#pragma once
#include <Arduino.h>

// Embedded copy of ../../web/index.html. Keep both files identical.
static const char KORTLINJE_WEB_UI[] PROGMEM = R"KORTLINJE(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<title>KORTLINJE</title>
<style>
:root{--bg:#fff;--fg:#000;color-scheme:light dark;font:16px -apple-system,BlinkMacSystemFont,system-ui,sans-serif;color:var(--fg);background:var(--bg)}
@media(prefers-color-scheme:dark){:root{--bg:#000;--fg:#fff}}
*{box-sizing:border-box}
html,body{width:100%;height:100%;margin:0;overflow:hidden;background:var(--bg);color:var(--fg)}
button,input{font:inherit;color:inherit}
button{appearance:none;display:grid;place-items:center;min-width:52px;min-height:52px;padding:0;background:var(--bg);border:2px solid var(--fg);border-radius:0;touch-action:manipulation;cursor:pointer}
button.on{background:var(--fg);color:var(--bg)}
button:focus-visible,input:focus-visible{outline:2px solid var(--fg);outline-offset:3px}
button:disabled{cursor:default}
svg{width:24px;height:24px;fill:none;stroke:currentColor;stroke-width:2;stroke-linecap:round;stroke-linejoin:round;flex:none}
.shell{width:100%;max-width:760px;margin:auto;height:100dvh;display:flex;flex-direction:column;padding:calc(env(safe-area-inset-top) + 12px) 16px 0;gap:16px;overflow:hidden}
.top{height:32px;display:flex;justify-content:space-between;align-items:center;flex:none}
.top svg{width:24px;height:24px}
.readout{font-size:32px;line-height:1;font-variant-numeric:tabular-nums;letter-spacing:.02em}
.connection{display:grid;place-items:center;width:32px;height:32px}
.connection.off:after{content:"";position:absolute;width:27px;border-top:2px solid var(--fg);transform:rotate(-45deg)}
.matrix{border:2px solid var(--fg);display:grid;place-items:center;padding:12px;width:100%;flex:none;min-height:98px;aspect-ratio:42/10;background:#000}
canvas{display:block;width:100%;height:auto;aspect-ratio:42/9;image-rendering:pixelated}
.page{flex:1;min-height:0;display:none;flex-direction:column;gap:16px;overflow:hidden}
.page.on{display:flex}
.mode-grid{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px;align-content:start}
.mode-grid button{width:100%;min-height:68px;display:flex;flex-direction:column;justify-content:center;gap:4px}
.mode-label,.small-label{font:700 10px/1 ui-monospace,SFMono-Regular,Menlo,monospace;letter-spacing:.06em;text-transform:uppercase}
.page-clock .mode-grid{margin-top:4px}
.meta-grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:12px}
.meta-cell{border:2px solid var(--fg);padding:10px;min-height:64px;display:flex;flex-direction:column;justify-content:space-between;min-width:0}
.meta-value{font:700 16px/1 ui-monospace,SFMono-Regular,Menlo,monospace;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.value-panel{border:2px solid var(--fg);min-height:94px;display:flex;align-items:center;justify-content:center;gap:8px;flex:none}.value-panel[hidden]{display:none}
.value{font-size:32px;line-height:1;font-variant-numeric:tabular-nums}
.unit{font-size:16px;line-height:1}
.controls{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px;align-content:start}
.controls:empty{display:none}
.controls button{width:100%}
.controls input[type=number]{width:100%;height:100%;border:0;border-radius:0;background:var(--bg);text-align:center;font-size:32px;min-width:0;-moz-appearance:textfield}
.controls input::-webkit-inner-spin-button,.controls input::-webkit-outer-spin-button{-webkit-appearance:none;margin:0}
.controls .number{grid-column:span 2;display:flex;align-items:center;gap:8px;height:52px;padding-right:12px;border:2px solid var(--fg)}
.controls .number input{flex:1}
.h{margin:0 0 -8px;font-size:12px;font-weight:600;line-height:1;letter-spacing:.08em;text-transform:uppercase}
.setting-tabs{display:grid;grid-template-columns:repeat(3,1fr);gap:12px}
.setting-tabs button{min-width:0;font:700 11px/1 ui-monospace,SFMono-Regular,Menlo,monospace}
.setting-panel{display:none;flex:1;min-height:0;flex-direction:column;gap:14px}
.setting-panel.on{display:flex}
.binding-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}
.binding{display:flex;flex-direction:column;gap:5px;min-width:0}
.binding select,.weather-grid input{width:100%;height:52px;background:var(--bg);color:var(--fg);border:2px solid var(--fg);border-radius:0;padding:0 8px;font:700 14px ui-monospace,SFMono-Regular,Menlo,monospace}
.binding select:disabled{border-style:dashed}
.weather-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}
.source{border:2px solid var(--fg);padding:12px;display:flex;justify-content:space-between;align-items:center;min-height:58px;font:700 14px ui-monospace,SFMono-Regular,Menlo,monospace}
.status-line{font:700 12px ui-monospace,SFMono-Regular,Menlo,monospace}
.range{display:flex;align-items:center;gap:16px;min-height:52px}
.range input{flex:1;min-width:0;accent-color:var(--fg)}
.style-group{display:grid;grid-template-columns:repeat(4,minmax(0,1fr));gap:12px}
.style-group button{width:100%}
.style-group svg{width:24px;height:24px}
.settings-spacer{flex:1;min-height:0}
.settings-actions{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px;margin-bottom:16px}
.settings-actions button{width:100%}
.tabs{margin:0 -16px;padding:8px 16px calc(env(safe-area-inset-bottom) + 8px);border-top:2px solid var(--fg);display:grid;grid-template-columns:repeat(3,1fr);gap:12px;flex:none;background:var(--bg)}
.tabs button{width:100%;min-height:52px}
@media(max-height:690px){.shell{gap:10px;padding-top:calc(env(safe-area-inset-top) + 8px)}.page{gap:10px}.matrix{min-height:80px;padding:8px}.mode-grid,.controls,.style-group{gap:8px}.value-panel{min-height:72px}.settings-actions{margin-bottom:8px}}
@media(max-height:560px){.matrix{aspect-ratio:auto;height:80px}.value-panel{min-height:60px}}
@media(max-height:700px){.mode-grid button{min-height:56px}.meta-cell{min-height:52px}.binding-grid{gap:8px}.setting-panel{gap:8px}.binding select{height:48px}}
</style>
</head>
<body>
<div class="shell">
  <div class="top"><div class="readout" id="readout" aria-live="polite"></div><div class="connection off" id="connection" aria-label="Disconnected"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M2 8a15 15 0 0 1 20 0M5 12a10 10 0 0 1 14 0M8 16a5 5 0 0 1 8 0"/><circle cx="12" cy="20" r="1"/></svg></div></div>
  <div class="matrix"><canvas id="matrix" width="840" height="180" role="img" aria-label="42 by 9 LED matrix"></canvas></div>
  <section class="page page-clock on" id="clockPage" aria-label="Clock">
    <div class="mode-grid" id="clockModes"></div>
    <div class="meta-grid">
      <div class="meta-cell"><span class="small-label">DATE</span><span class="meta-value" id="metaDate"></span></div>
      <div class="meta-cell"><span class="small-label">MODE</span><span class="meta-value" id="metaMode"></span></div>
      <div class="meta-cell"><span class="small-label">RSSI</span><span class="meta-value" id="metaRssi"></span></div>
    </div>
    <div class="settings-spacer"></div>
  </section>
  <section class="page page-apps" id="appsPage" aria-label="Apps"><div class="mode-grid" id="appModes"></div><div class="value-panel" id="valuePanel" hidden><span class="value" id="appValue"></span><span class="unit" id="appUnit"></span></div><div class="controls" id="controls"></div><div class="settings-spacer"></div></section>
  <section class="page page-settings" id="settingsPage" aria-label="Settings">
    <div class="setting-tabs"><button class="on" data-settings-tab="display">DISPLAY</button><button data-settings-tab="buttons">KEYS</button><button data-settings-tab="weather">WEATHER</button></div>
    <div class="setting-panel on" id="displaySettings">
      <h2 class="h">Font</h2><div class="style-group" id="fonts"></div>
      <h2 class="h">Effect</h2><div class="style-group" id="effects"></div>
      <h2 class="h">Brightness</h2><div class="range"><svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="4"/><path d="M12 2v2m0 16v2M2 12h2m16 0h2M5 5l1.5 1.5M17.5 17.5 19 19M19 5l-1.5 1.5M6.5 17.5 5 19"/></svg><input id="brightness" type="range" min="0" max="7" value="7" aria-label="Brightness"><span class="value" id="brightnessValue">7</span></div>
      <div class="settings-spacer"></div><div class="settings-actions"><button id="invert" aria-label="Invert"><svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="9"/><path d="M12 3v18"/></svg></button><button id="preview" aria-label="Preview"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="m8 5 11 7-11 7z"/></svg></button><button id="save" aria-label="Save"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M4 3h14l3 3v15H3V3h1zM7 3v6h10V3M7 21v-8h10v8"/></svg></button><button id="lock" aria-label="Unlock at device" disabled><svg viewBox="0 0 24 24" aria-hidden="true"><rect x="5" y="10" width="14" height="11"/><path d="M8 10V7a4 4 0 0 1 8 0v3"/></svg></button></div>
    </div>
    <div class="setting-panel" id="buttonsSettings"><div class="binding-grid" id="bindings"></div><button id="openCalibrate">CALIBRATE</button><div class="status-line" id="buttonStatus"></div><div class="settings-spacer"></div></div>
    <div class="setting-panel" id="weatherSettings"><div class="source"><span>SOURCE</span><span>OPEN-METEO</span></div><div class="weather-grid"><label class="binding"><span class="small-label">LAT</span><input id="weatherLat" inputmode="decimal" aria-label="Latitude"></label><label class="binding"><span class="small-label">LON</span><input id="weatherLon" inputmode="decimal" aria-label="Longitude"></label></div><button id="saveWeather">SAVE LOCATION</button><div class="status-line" id="weatherStatus"></div><div class="settings-spacer"></div></div>
  </section>
  <nav class="tabs" aria-label="Navigation"><button class="on" data-tab="clock" aria-label="Clock"><svg viewBox="0 0 24 24" aria-hidden="true"><circle cx="12" cy="12" r="9"/><path d="M12 7v5l4 2"/></svg></button><button data-tab="apps" aria-label="Apps"><svg viewBox="0 0 24 24" aria-hidden="true"><rect x="3" y="3" width="7" height="7"/><rect x="14" y="3" width="7" height="7"/><rect x="3" y="14" width="7" height="7"/><rect x="14" y="14" width="7" height="7"/></svg></button><button data-tab="settings" aria-label="Settings"><svg viewBox="0 0 24 24" aria-hidden="true"><path d="M3 6h18M3 12h18M3 18h18"/><circle cx="8" cy="6" r="2"/><circle cx="16" cy="12" r="2"/><circle cx="10" cy="18" r="2"/></svg></button></nav>
</div>
<script>
const $=id=>document.getElementById(id);
const paths={clock:'<circle cx="12" cy="12" r="9"/><path d="M12 7v5l4 2"/>',seconds:'<circle cx="12" cy="12" r="9"/><path d="M12 5v7l5 3"/><circle cx="12" cy="12" r="1"/>',date:'<rect x="3" y="5" width="18" height="16"/><path d="M3 10h18M7 3v4m10-4v4"/>',calendar:'<rect x="3" y="5" width="18" height="16"/><path d="M3 10h18M7 3v4m10-4v4M7 14h2m3 0h2m3 0h2M7 18h2m3 0h2"/>',life:'<rect x="3" y="3" width="5" height="5"/><rect x="10" y="3" width="5" height="5"/><rect x="17" y="10" width="5" height="5"/><rect x="10" y="17" width="5" height="5"/>',snow:'<path d="M12 2v20M2 12h20M5 5l14 14M19 5 5 19M9 5l3 3 3-3M9 19l3-3 3 3"/>',rain:'<path d="M5 3v7m7-5v11m7-13v8M5 16v5m14-5v5"/>',dither:'<circle cx="5" cy="5" r="1"/><circle cx="12" cy="5" r="1"/><circle cx="19" cy="5" r="1"/><circle cx="5" cy="12" r="1"/><circle cx="12" cy="12" r="1"/><circle cx="19" cy="12" r="1"/><circle cx="5" cy="19" r="1"/><circle cx="12" cy="19" r="1"/><circle cx="19" cy="19" r="1"/>',snake:'<path d="M3 5h8v6h8v8h-7"/><circle cx="12" cy="19" r="1"/>',pong:'<path d="M3 5v14m18-14v14M10 8l4 4-4 4"/>',tempo:'<path d="M12 3v18M7 7h10M7 21h10M16 3l3 18"/>',pomodoro:'<circle cx="12" cy="13" r="8"/><path d="M12 5V2m-4 0h8M12 13l4-3"/>',stopwatch:'<circle cx="12" cy="13" r="8"/><path d="M12 5V2m-4 0h8M12 13v-5"/>',countdown:'<circle cx="12" cy="12" r="9"/><path d="M12 7v5l-4 4M7 3l-3 3"/>',sky:'<circle cx="9" cy="8" r="3"/><path d="M4 18h15a3 3 0 0 0 0-6h-2a5 5 0 0 0-9 3H6a3 3 0 0 0-2 3z"/>',up:'<path d="m5 15 7-7 7 7"/>',down:'<path d="m5 9 7 7 7-7"/>',left:'<path d="m15 5-7 7 7 7"/>',right:'<path d="m9 5 7 7-7 7"/>',play:'<path d="m8 5 11 7-11 7z"/>',pause:'<path d="M8 5v14m8-14v14"/>',reset:'<path d="M4 11a8 8 0 1 1 2 6M4 5v6h6"/>',tap:'<circle cx="12" cy="12" r="8"/><path d="M12 8v8m-4-4h8"/>',skip:'<path d="m4 5 8 7-8 7zM12 5l8 7-8 7z"/>',lap:'<path d="M4 18h16M12 3v12m-5-5 5 5 5-5"/>',sun:'<circle cx="12" cy="12" r="4"/><path d="M12 2v2m0 16v2M2 12h2m16 0h2"/>',cloud:'<path d="M4 18h15a3 3 0 0 0 0-6h-2a5 5 0 0 0-9 3H6a3 3 0 0 0-2 3z"/>',thin:'<path d="M7 3v18M17 3v18"/>',segment:'<path d="M7 3h10M7 12h10M7 21h10M5 5v5m14-5v5M5 14v5m14-5v5"/>',tiny5:'<path d="M8 4h8v6H8zM8 14h8v6H8z"/>',wipe:'<path d="M4 4h16M4 10h12M4 16h8M4 22h4"/>',roll:'<path d="M4 7h16M4 12h16M4 17h16M17 4l3 3-3 3M7 14l-3 3 3 3"/>',dissolve:'<circle cx="5" cy="5" r="1"/><circle cx="17" cy="6" r="1"/><circle cx="10" cy="12" r="1"/><circle cx="20" cy="17" r="1"/><circle cx="5" cy="20" r="1"/>',off:'<path d="M4 4l16 16M5 12a7 7 0 0 1 12-5m2 5a7 7 0 0 1-12 5"/>'};
paths.calibrate='<circle cx="12" cy="12" r="9"/><path d="M12 6v12M6 12h12"/>';
const icon=name=>`<svg viewBox="0 0 24 24" aria-hidden="true">${paths[name]}</svg>`;
const clockModes=['clock','seconds','date','calendar'],appModes=['countdown','pomodoro','stopwatch','tempo','sky','life','snow','rain','dither','snake','pong','calibrate'];
const fonts=['pixel','thin','segment','tiny5'],effects=['off','wipe','roll','dissolve'];
const keyNames=['snooze','brightness','mode','bell','up','down','set','alarm'];
const bindModes=['default',...clockModes,...appModes];
const fontIcon={pixel:'dither',thin:'thin',segment:'segment',tiny5:'tiny5'};
$('clockModes').innerHTML=clockModes.map(name=>`<button data-mode="${name}" aria-label="${name}">${icon(name)}<span class="mode-label">${name}</span></button>`).join('');
$('fonts').innerHTML=fonts.map(name=>`<button data-font="${name}" aria-label="${name}">${icon(fontIcon[name])}</button>`).join('');
$('effects').innerHTML=effects.map(name=>`<button data-effect="${name}" aria-label="${name}">${icon(name)}</button>`).join('');
const state={token:'',mode:'clock',font:'pixel',effect:'off',brightness:7,invert:false,unlocked:false,tab:'clock',status:{},bits:'0'.repeat(378),buttonMap:{},buttons:{},weatherLat:'',weatherLon:''};
let weatherDirty=false;
$('bindings').innerHTML=keyNames.map(key=>`<label class="binding"><span class="small-label">${key}</span><select data-key="${key}" aria-label="${key} mode">${bindModes.map(name=>`<option value="${name}">${name}</option>`).join('')}</select></label>`).join('');
function settingsTab(name){for(const b of document.querySelectorAll('[data-settings-tab]'))b.classList.toggle('on',b.dataset.settingsTab===name);for(const [n,id] of [['display','displaySettings'],['buttons','buttonsSettings'],['weather','weatherSettings']])$(id).classList.toggle('on',n===name)}
let appPage=0;
function renderAppModes(){const start=appPage*6;let visible=appModes.slice(start,start+6);$('appModes').innerHTML=visible.map(name=>`<button data-mode="${name}" aria-label="${name}">${icon(name)}<span class="mode-label">${name}</span></button>`).join('')+`<button data-page="-1" aria-label="Previous apps">${icon('left')}<span class="mode-label">PREV</span></button><button data-page="1" aria-label="Next apps">${icon('right')}<span class="mode-label">NEXT</span></button>`;active()}
if(appModes.indexOf(state.mode)>=6)appPage=1;renderAppModes();
const canvas=$('matrix'),ctx=canvas.getContext('2d');
function paint(){ctx.fillStyle='#000';ctx.fillRect(0,0,840,180);for(let y=0;y<9;y++)for(let x=0;x<42;x++){ctx.fillStyle=state.bits[y*42+x]==='1'?'#ff8c1a':'#2a1705';ctx.fillRect(x*20+1,y*20+1,18,18)}}
function tab(name){state.tab=name;for(const b of document.querySelectorAll('[data-tab]'))b.classList.toggle('on',b.dataset.tab===name);for(const [n,id] of [['clock','clockPage'],['apps','appsPage'],['settings','settingsPage']])$(id).classList.toggle('on',n===name)}
function active(){for(const b of document.querySelectorAll('[data-mode]'))b.classList.toggle('on',b.dataset.mode===state.mode);for(const b of document.querySelectorAll('[data-font]'))b.classList.toggle('on',b.dataset.font===state.font);for(const b of document.querySelectorAll('[data-effect]'))b.classList.toggle('on',b.dataset.effect===state.effect);$('invert').classList.toggle('on',state.invert);$('save').disabled=!state.unlocked;$('lock').classList.toggle('on',state.unlocked);$('saveWeather').disabled=!state.unlocked;for(const select of document.querySelectorAll('[data-key]')){select.disabled=!state.unlocked;if(document.activeElement!==select)select.value=state.buttonMap[select.dataset.key]||'default'}$('brightness').value=state.brightness;$('brightnessValue').textContent=state.brightness;const b=state.buttons;$('buttonStatus').textContent=(state.unlocked?'EDIT | ':'SET TO EDIT | ')+(b.back_key||'-').toUpperCase()+' '+(b.back_last_press_mv||0)+'mV / '+(b.top_key||'-').toUpperCase()+' '+(b.top_last_press_mv||0)+'mV';$('weatherStatus').textContent=state.weatherLat!==''&&state.weatherLon!==''?'LOCATION SAVED · FORECAST NOT ACTIVE':'ADD LOCATION'}
async function get(path){const r=await fetch(path,{cache:'no-store'});if(!r.ok)throw Error(r.status);return r.json()}
async function post(path,data){if(!state.token)throw Error('offline');const r=await fetch(path,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:new URLSearchParams({...data,token:state.token})});if(!r.ok)throw Error(r.status);return r.json()}
function setConnection(ok){$('connection').classList.toggle('off',!ok);$('connection').setAttribute('aria-label',ok?'Connected':'Disconnected')}
async function refresh(){try{const [settings,status,frame,buttons]=await Promise.all([get('/api/settings'),get('/status'),get('/api/frame'),get('/buttons')]);const first=!state.token;state.token=settings.token;state.unlocked=settings.unlocked;state.status=status;state.buttons=buttons;state.mode=frame.mode;state.bits=frame.bits;state.font=settings.font;state.effect=settings.effect;state.brightness=settings.brightness;state.invert=settings.invert;state.buttonMap=settings.button_map||{};state.weatherLat=settings.weather_lat||'';state.weatherLon=settings.weather_lon||'';if(!weatherDirty){$('weatherLat').value=state.weatherLat;$('weatherLon').value=state.weatherLon}if(first&&appModes.indexOf(state.mode)>=6){appPage=1;renderAppModes()}$('readout').textContent=status.clock;$('metaDate').textContent=status.date;$('metaMode').textContent=status.mode.toUpperCase();$('metaRssi').textContent=status.wifi?status.rssi+' dBm':'OFF';setConnection(true);paint();active();renderControls()}catch(e){setConnection(false)}}
async function updateFrame(){try{const frame=await get('/api/frame');state.bits=frame.bits;state.mode=frame.mode;paint();active()}catch(e){setConnection(false)}}
function appValue(){const s=state.status,m=state.mode;let v='',u='';if(m==='countdown'){let n=s.countdown_remaining_s||0;v=String(Math.floor(n/60)).padStart(2,'0')+':'+String(n%60).padStart(2,'0')}else if(m==='pomodoro'){let n=s.pomodoro_remaining_s||0;v=String(Math.floor(n/60)).padStart(2,'0')+':'+String(n%60).padStart(2,'0')}else if(m==='stopwatch'){v=((s.stopwatch_ms||0)/1000).toFixed(1);u='s'}else if(m==='tempo'){v=s.tempo_bpm||0;u='BPM'}else if(m==='snake'){v=s.snake_score||0}else if(m==='pong'){v=(s.pong_player||0)+':'+(s.pong_cpu||0)}else if(m==='calibrate'){v=s.calibration_label||'PRESS'}$('valuePanel').hidden=v==='';$('appValue').textContent=v;$('appUnit').textContent=u}
function controlButton(action,shape,label){return `<button data-action="${action}" aria-label="${label}">${icon(shape)}</button>`}
function renderControls(){const m=state.mode,c=$('controls');let html='';if(m==='countdown')html=controlButton('toggle',state.status.countdown_running?'pause':'play','Start or pause')+controlButton('reset','reset','Reset')+'<div class="number"><input id="seconds" type="number" min="10" max="5999" aria-label="Seconds"><span class="unit">s</span></div>'+controlButton('configure','play','Set seconds');else if(m==='pomodoro')html=controlButton('toggle',state.status.pomodoro_running?'pause':'play','Start or pause')+controlButton('reset','reset','Reset')+controlButton('skip','skip','Skip')+controlButton('demo','seconds','Demo');else if(m==='stopwatch')html=controlButton('toggle',state.status.stopwatch_running?'pause':'play','Start or pause')+controlButton('lap','lap','Lap')+controlButton('reset','reset','Reset');else if(m==='tempo')html=controlButton('tap','tap','Tap')+controlButton('meter','segment','Meter')+controlButton('reset','reset','Reset');else if(m==='sky')html=['sun','cloud','rain','snow'].map(name=>`<button data-condition="${name}" aria-label="${name}">${icon(name)}</button>`).join('');else if(m==='snake')html=['up','left','down','right'].map(name=>controlButton(name,name,name)).join('')+controlButton('restart','reset','Restart');else if(m==='pong')html=controlButton('up','up','Up')+controlButton('down','down','Down')+controlButton('restart','reset','Restart');else if(m==='calibrate')html=`<button data-mode="clock" aria-label="Exit calibration">${icon('clock')}<span class="mode-label">EXIT</span></button>`;if(c.dataset.mode!==m){c.innerHTML=html;c.dataset.mode=m;if(m==='countdown')$('seconds').value=state.status.countdown_preset_s||300}const toggle=c.querySelector('[data-action=toggle]');if(toggle)toggle.innerHTML=icon((m==='countdown'?state.status.countdown_running:m==='pomodoro'?state.status.pomodoro_running:state.status.stopwatch_running)?'pause':'play');appValue()}
async function mode(name){try{await post('/api/mode',{mode:name});state.mode=name;active();renderControls();await updateFrame()}catch(e){setConnection(false)}}
async function action(name){try{const m=state.mode,path=['snake','pong'].includes(m)?'/api/game':'/api/app',data={action:name};if(m==='countdown'&&name==='configure')data.seconds=$('seconds').value;await post(path,data);await refresh()}catch(e){setConnection(false)}}
async function previewStyle(){try{await post('/api/style-preview',{font:state.font,effect:state.effect,brightness:state.brightness,invert:state.invert?'1':'0'});await updateFrame()}catch(e){setConnection(false)}}
let previewTimer=0;function queueStyle(){active();clearTimeout(previewTimer);previewTimer=setTimeout(previewStyle,120)}
document.addEventListener('click',e=>{const b=e.target.closest('button');if(!b)return;if(b.dataset.tab)tab(b.dataset.tab);else if(b.dataset.settingsTab)settingsTab(b.dataset.settingsTab);else if(b.id==='openCalibrate')mode('calibrate');else if(b.dataset.page){appPage=(appPage+Number(b.dataset.page)+2)%2;renderAppModes()}else if(b.dataset.mode)mode(b.dataset.mode);else if(b.dataset.font){state.font=b.dataset.font;queueStyle()}else if(b.dataset.effect){state.effect=b.dataset.effect;queueStyle()}else if(b.dataset.action)action(b.dataset.action);else if(b.dataset.condition)post('/api/app',{action:'set',condition:b.dataset.condition}).then(refresh).catch(()=>setConnection(false))});
document.addEventListener('change',e=>{if(!e.target.matches('[data-key]'))return;const key=e.target.dataset.key,mode=e.target.value;post('/api/button-map',{key,mode}).then(settings=>{state.buttonMap=settings.button_map;active()}).catch(()=>refresh())});
for(const id of ['weatherLat','weatherLon'])$(id).addEventListener('input',()=>{weatherDirty=true});
$('saveWeather').addEventListener('click',()=>post('/api/weather-config',{lat:$('weatherLat').value,lon:$('weatherLon').value}).then(()=>{weatherDirty=false;refresh()}).catch(()=>{$('weatherStatus').textContent='INVALID LOCATION'}));
$('brightness').addEventListener('input',e=>{state.brightness=Number(e.target.value);queueStyle()});
$('invert').addEventListener('click',()=>{state.invert=!state.invert;queueStyle()});
$('preview').addEventListener('click',()=>post('/api/preview',{font:state.font,effect:state.effect}).then(updateFrame).catch(()=>setConnection(false)));
$('save').addEventListener('click',()=>post('/api/settings',{mode:clockModes.includes(state.mode)?state.mode:'clock',font:state.font,effect:state.effect,brightness:state.brightness,invert:state.invert?'1':'0'}).then(refresh).catch(()=>setConnection(false)));
paint();active();refresh();setInterval(()=>{if(document.hidden)return;refresh()},2500);setInterval(()=>{if(!document.hidden)updateFrame()},350);
</script>
</body>
</html>
)KORTLINJE";
