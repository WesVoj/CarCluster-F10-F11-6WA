// SPDX-FileCopyrightText: 2026 WesVoj
// SPDX-License-Identifier: GPL-3.0-only

#include "AudioVUWebPage.h"

// Audio analysis stays in the browser. The ESP32 only serves this page and
// receives the resulting speed/RPM values through /api/state.
static const char AUDIO_VU_PAGE[] = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Spotify / Voicemeeter VU</title>
  <style>
    :root{color-scheme:light;--bg:#f4f7fb;--panel:#fff;--line:#dbe3ee;--text:#172033;--muted:#667085;--blue:#1769d2;--blue-soft:#eaf2ff;--green:#18794e;--red:#c53030}
    *{box-sizing:border-box}body{font-family:Arial,sans-serif;margin:0;background:var(--bg);color:var(--text)}
    header{background:var(--panel);border-bottom:1px solid var(--line)}.header-inner,main{max-width:920px;margin:0 auto;padding:20px}
    h1{font-size:27px;margin:0 0 6px}h2{font-size:17px;margin:0 0 14px}p,li{line-height:1.45}.lead,.muted{color:var(--muted)}.lead{margin:0}
    a{color:var(--blue);text-decoration:none;font-weight:700}section{background:var(--panel);border:1px solid var(--line);border-radius:9px;padding:17px;margin:14px 0;box-shadow:0 5px 18px rgba(25,42,70,.05)}
    .notice{border-left:4px solid var(--blue)}.warning{border-left-color:#d97706;background:#fffaf0}.hidden{display:none}ol{margin:8px 0 0;padding-left:22px}
    code{background:#f2f5f9;border:1px solid var(--line);border-radius:5px;padding:2px 5px}.grid,.readouts{display:grid;grid-template-columns:1fr 1fr;gap:15px}.field{display:flex;flex-direction:column;gap:7px}
    label{font-size:13px;font-weight:700}select,input[type=range]{width:100%}select{min-height:40px;padding:8px;color:var(--text);background:#fff;border:1px solid #c9d4e2;border-radius:7px}
    input[type=range],input[type=checkbox]{accent-color:var(--blue)}.inline{display:flex;align-items:center;gap:9px;min-height:40px}.inline label{font-weight:400}.buttons{display:flex;flex-wrap:wrap;gap:9px;margin-top:16px}
    button{border:1px solid #c9d4e2;border-radius:7px;padding:10px 13px;font-weight:700;cursor:pointer;background:#fff;color:var(--text)}button.primary{background:var(--blue);border-color:var(--blue);color:#fff}button.stop{color:var(--red);border-color:#efb3b3}button:disabled{opacity:.45;cursor:not-allowed}
    .readout{padding:14px;background:#f8fafc;border:1px solid var(--line);border-radius:8px}.readout-head{display:flex;justify-content:space-between;gap:10px;margin-bottom:10px;font-weight:700}.value{font-variant-numeric:tabular-nums;color:var(--blue)}
    .meter{height:16px;overflow:hidden;background:#e4eaf2;border-radius:999px}.fill{height:100%;width:0;background:linear-gradient(90deg,#71a8ee,var(--blue));transition:width 35ms linear}
    #status{margin:12px 0 0;min-height:18px;color:var(--muted);font-size:13px}#status.ok{color:var(--green)}#status.error{color:var(--red)}.back{display:inline-block;margin-top:5px}
    @media(max-width:650px){.header-inner,main{padding:15px}.grid,.readouts{grid-template-columns:1fr}}
  </style>
</head>
<body>
<header><div class="header-inner"><h1>Spotify / Voicemeeter VU needles</h1><p class="lead">The speedometer is the left channel and the tachometer is the right channel. Beat mode emphasizes bass so both needles move with the rhythm.</p></div></header>
<main>
  <section class="notice">
    <h2>Voicemeeter routing</h2>
    <ol>
      <li>Set the Windows output device for Spotify to <code>VoiceMeeter Input</code> or <code>VoiceMeeter AUX Input</code>.</li>
      <li>On that Voicemeeter strip, enable <code>A1</code> so you can hear the music and <code>B1</code> so the browser can capture it.</li>
      <li>Load the input list below and select <code>VoiceMeeter Output (VB-Audio VoiceMeeter VAIO)</code>. For AUX/B2, select the corresponding VoiceMeeter AUX Output device.</li>
    </ol>
    <p class="muted">This page does not access your Spotify account or song metadata. Audio is analysed locally in your browser and is never uploaded.</p>
  </section>

  <section id="secureWarning" class="notice warning hidden">
    <h2>The browser is blocking audio capture</h2>
    <p>Microphone and virtual-input capture require a secure browser context. For a local ESP32 test in Chrome, open <code>chrome://flags/#unsafely-treat-insecure-origin-as-secure</code>, add the exact ESP32 origin (for example <code>http://192.168.1.50</code>), enable the flag, and restart Chrome. Edge provides the equivalent flag under <code>edge://flags</code>.</p>
  </section>

  <section class="notice">
    <h2>Background operation</h2>
    <p>After starting VU mode, you may switch to Spotify or minimize Chrome. The audio-processing clock continues driving the cluster without relying on visible-tab animation frames. Keep this VU tab and the browser open: closing the tab, reloading it, or exiting the browser stops audio capture.</p>
  </section>

  <section>
    <h2>Audio input and needle response</h2>
    <div class="grid">
      <div class="field" style="grid-column:1/-1"><label for="device">Recording device</label><select id="device"><option value="">Load audio inputs first</option></select></div>
      <div class="field"><label for="mode">Analyzer mode</label><select id="mode"><option value="bass">Beat / bass (35-180 Hz)</option><option value="vu">Classic stereo VU / full range</option></select></div>
      <div class="field"><label for="sensitivity">Sensitivity: <span id="sensitivityValue">1.40x</span></label><input id="sensitivity" type="range" min="0.25" max="4" step="0.05" value="1.4"></div>
      <div class="field"><label for="attack">Attack: <span id="attackValue">18 ms</span></label><input id="attack" type="range" min="5" max="120" step="1" value="18"></div>
      <div class="field"><label for="release">Release: <span id="releaseValue">100 ms</span></label><input id="release" type="range" min="0" max="600" step="5" value="100"></div>
      <div class="inline"><input id="linked" type="checkbox"><label for="linked">Link both needles to the stronger channel</label></div>
      <div class="inline"><input id="swap" type="checkbox"><label for="swap">Swap left and right channels</label></div>
      <div class="inline"><input id="ignition" type="checkbox" checked><label for="ignition">Keep ignition on while VU mode is running</label></div>
    </div>
    <div class="buttons"><button id="devicesButton" type="button">Allow and load inputs</button><button id="startButton" type="button" class="primary">Start VU</button><button id="stopButton" type="button" class="stop" disabled>Stop and restore state</button></div>
    <p id="status">Ready. Load the input list and select a Voicemeeter output.</p>
  </section>

  <section>
    <h2>Live response</h2>
    <div class="readouts">
      <div class="readout"><div class="readout-head"><span>LEFT - speedometer</span><span id="speedValue" class="value">0 / -- km/h</span></div><div class="meter"><div id="leftFill" class="fill"></div></div></div>
      <div class="readout"><div class="readout-head"><span>RIGHT - tachometer</span><span id="rpmValue" class="value">0 / -- RPM</span></div><div class="meter"><div id="rightFill" class="fill"></div></div></div>
    </div>
    <p class="muted">Audio samples and cluster targets are updated up to about 50 times per second. Set Release to 0 ms for an immediate software drop; the physical needles still have their own mechanical and cluster/CAN response time. Requests are coalesced, and Stop restores the speed, RPM, and ignition state that were active before the test.</p>
  </section>
  <a class="back" href="/test">Back to experimental tests</a>
</main>

<script>
(() => {
  'use strict';
  const byId=id=>document.getElementById(id);
  const ui={device:byId('device'),mode:byId('mode'),sensitivity:byId('sensitivity'),attack:byId('attack'),release:byId('release'),linked:byId('linked'),swap:byId('swap'),ignition:byId('ignition'),devicesButton:byId('devicesButton'),startButton:byId('startButton'),stopButton:byId('stopButton'),status:byId('status'),leftFill:byId('leftFill'),rightFill:byId('rightFill'),speedValue:byId('speedValue'),rpmValue:byId('rpmValue'),secureWarning:byId('secureWarning')};
  let audioContext=null,stream=null,meterNode=null,silentGain=null,fallbackTimer=0,running=false,originalState=null,maxSpeed=260,maxRPM=6000,analysers=[],timeBuffers=[],frequencyBuffers=[],envelopes=[0,0],lastFallbackTime=0,lastSendTime=0,pendingState=null,writePromise=null;
  const SEND_INTERVAL_MS=20;
  const METER_WORKLET=`
class CarClusterMeterProcessor extends AudioWorkletProcessor {
  constructor(){
    super();
    this.samples=0;
    this.fullPower=[0,0];
    this.bassPower=[0,0];
    this.previousInput=[0,0];
    this.highPass=[0,0];
    this.lowPass=[0,0];
    this.reportSamples=Math.max(128,Math.round(sampleRate/50));
    const dt=1/sampleRate;
    this.highPassAlpha=1/(1+2*Math.PI*35*dt);
    this.lowPassAlpha=(2*Math.PI*180*dt)/(1+2*Math.PI*180*dt);
  }
  process(inputs,outputs){
    const input=inputs[0];
    const left=input&&input[0];
    const right=input&&(input[1]||left);
    const output=outputs[0]&&outputs[0][0];
    const frames=left?left.length:128;
    for(let i=0;i<frames;i+=1){
      const samples=[left?(left[i]||0):0,right?(right[i]||0):0];
      if(output)output[i]=(samples[0]+samples[1])*.5;
      for(let channel=0;channel<2;channel+=1){
        const value=samples[channel];
        const high=this.highPassAlpha*(this.highPass[channel]+value-this.previousInput[channel]);
        const low=this.lowPass[channel]+this.lowPassAlpha*(high-this.lowPass[channel]);
        this.previousInput[channel]=value;
        this.highPass[channel]=high;
        this.lowPass[channel]=low;
        this.fullPower[channel]+=value*value;
        this.bassPower[channel]+=low*low;
      }
    }
    this.samples+=frames;
    if(this.samples>=this.reportSamples){
      this.port.postMessage({
        full:[Math.sqrt(this.fullPower[0]/this.samples),Math.sqrt(this.fullPower[1]/this.samples)],
        bass:[Math.sqrt(this.bassPower[0]/this.samples),Math.sqrt(this.bassPower[1]/this.samples)],
        elapsedMs:this.samples*1000/sampleRate
      });
      this.samples=0;
      this.fullPower=[0,0];
      this.bassPower=[0,0];
    }
    return true;
  }
}
registerProcessor('carcluster-meter',CarClusterMeterProcessor);
`;
  const setStatus=(message,kind='')=>{ui.status.textContent=message;ui.status.className=kind};
  function audioConstraints(deviceId){const audio={echoCancellation:false,noiseSuppression:false,autoGainControl:false,channelCount:{ideal:2},sampleRate:{ideal:48000}};if(deviceId)audio.deviceId={exact:deviceId};return{audio,video:false}}
  async function fetchState(){const response=await fetch('/api/state',{cache:'no-store'});if(!response.ok)throw new Error('ESP API returned HTTP '+response.status);const state=await response.json();maxSpeed=Math.max(1,Number(state.maximumSpeed)||260);maxRPM=Math.max(1,Number(state.maximumRPM)||6000);updateReadouts(0,0,0,0);return state}
  async function postStateNow(state){const response=await fetch('/api/state',{method:'POST',headers:{'Content-Type':'application/json'},cache:'no-store',body:JSON.stringify(state)});if(!response.ok)throw new Error('ESP write failed: HTTP '+response.status)}
  function scheduleState(state){pendingState=state;if(writePromise)return;writePromise=(async()=>{while(pendingState){const next=pendingState;pendingState=null;await postStateNow(next)}})().catch(error=>{pendingState=null;setStatus(error.message,'error')}).finally(()=>{writePromise=null;if(pendingState)scheduleState(pendingState)})}
  async function discoverDevices(requestPermission){if(!navigator.mediaDevices||!navigator.mediaDevices.enumerateDevices)throw new Error('Audio input is unavailable because this page is not a secure browser context.');let permissionStream=null,activeDeviceId='';try{if(requestPermission){setStatus('Waiting for permission to use the microphone / virtual input...');permissionStream=await navigator.mediaDevices.getUserMedia(audioConstraints(''));const track=permissionStream.getAudioTracks()[0];if(track&&track.getSettings)activeDeviceId=track.getSettings().deviceId||''}const devices=(await navigator.mediaDevices.enumerateDevices()).filter(item=>item.kind==='audioinput');let saved='';try{saved=localStorage.getItem('carcluster-vu-device')||''}catch(_){}const previous=ui.device.value||saved||activeDeviceId;ui.device.replaceChildren();if(!devices.length){const option=document.createElement('option');option.value='';option.textContent='No recording device found';ui.device.appendChild(option);return}devices.forEach((device,index)=>{const option=document.createElement('option');option.value=device.deviceId;option.textContent=device.label||('Audio input '+(index+1));ui.device.appendChild(option)});if([...ui.device.options].some(option=>option.value===previous))ui.device.value=previous;setStatus('Inputs loaded. Select VoiceMeeter Output and start VU.','ok')}finally{if(permissionStream)permissionStream.getTracks().forEach(track=>track.stop())}}
  function analyserAmplitude(channel,bassMode){const analyser=analysers[channel];if(bassMode){const buffer=frequencyBuffers[channel];analyser.getFloatFrequencyData(buffer);const binHz=audioContext.sampleRate/analyser.fftSize,first=Math.max(1,Math.floor(35/binHz)),last=Math.min(buffer.length-1,Math.ceil(180/binHz));let power=0,count=0;for(let i=first;i<=last;i+=1){power+=Math.pow(10,buffer[i]/10);count+=1}return Math.sqrt(power/Math.max(1,count))}const buffer=timeBuffers[channel];analyser.getFloatTimeDomainData(buffer);let power=0;for(let i=0;i<buffer.length;i+=1)power+=buffer[i]*buffer[i];return Math.sqrt(power/buffer.length)}
  function targetLevel(amplitude,bassMode){const db=20*Math.log10(Math.max(Number(amplitude)||0,.000001)),adjustedDb=db+20*Math.log10(Math.max(.01,Number(ui.sensitivity.value))),floor=bassMode?-68:-52,ceiling=bassMode?-16:-5;return Math.max(0,Math.min(1,(adjustedDb-floor)/(ceiling-floor)))}
  function smoothEnvelope(index,target,elapsedMs){const timeMs=target>envelopes[index]?Number(ui.attack.value):Number(ui.release.value);if(timeMs<=0){envelopes[index]=target}else{envelopes[index]+=(target-envelopes[index])*(1-Math.exp(-elapsedMs/timeMs))}if(envelopes[index]<.004)envelopes[index]=0;return envelopes[index]}
  function updateReadouts(left,right,speed,rpm){ui.leftFill.style.width=(Math.max(0,Math.min(1,left))*100).toFixed(1)+'%';ui.rightFill.style.width=(Math.max(0,Math.min(1,right))*100).toFixed(1)+'%';ui.speedValue.textContent=speed+' / '+maxSpeed+' km/h';ui.rpmValue.textContent=rpm+' / '+maxRPM+' RPM'}
  function processLevels(full,bass,elapsedMs){if(!running)return;const bassMode=ui.mode.value==='bass',source=bassMode?bass:full;let leftTarget=targetLevel(source[0],bassMode),rightTarget=targetLevel(source[1],bassMode);if(ui.linked.checked)leftTarget=rightTarget=Math.max(leftTarget,rightTarget);if(ui.swap.checked)[leftTarget,rightTarget]=[rightTarget,leftTarget];const elapsed=Math.min(100,Math.max(1,Number(elapsedMs)||20)),left=smoothEnvelope(0,leftTarget,elapsed),right=smoothEnvelope(1,rightTarget,elapsed),speed=Math.round(left*maxSpeed),rpm=Math.round(right*maxRPM),now=performance.now();updateReadouts(left,right,speed,rpm);if(now-lastSendTime>=SEND_INTERVAL_MS){const nextState={speed,rpm};if(ui.ignition.checked)nextState.ignition=true;scheduleState(nextState);lastSendTime=now}}
  function processFallback(){if(!running||!analysers.length)return;const now=performance.now(),elapsed=Math.min(100,Math.max(1,now-(lastFallbackTime||now-20)));lastFallbackTime=now;processLevels([analyserAmplitude(0,false),analyserAmplitude(1,false)],[analyserAmplitude(0,true),analyserAmplitude(1,true)],elapsed)}
  async function startAudioClock(source){if(audioContext.audioWorklet&&window.AudioWorkletNode){let moduleUrl='';try{moduleUrl=URL.createObjectURL(new Blob([METER_WORKLET],{type:'application/javascript'}));await audioContext.audioWorklet.addModule(moduleUrl);meterNode=new AudioWorkletNode(audioContext,'carcluster-meter',{numberOfInputs:1,numberOfOutputs:1,outputChannelCount:[1]});silentGain=audioContext.createGain();silentGain.gain.value=.000001;meterNode.port.onmessage=event=>{const data=event.data||{};processLevels(data.full||[0,0],data.bass||[0,0],data.elapsedMs)};source.connect(meterNode);meterNode.connect(silentGain);silentGain.connect(audioContext.destination);return true}catch(error){console.warn('AudioWorklet unavailable; using timer fallback.',error)}finally{if(moduleUrl)URL.revokeObjectURL(moduleUrl)}}const splitter=audioContext.createChannelSplitter(2);source.connect(splitter);analysers=[audioContext.createAnalyser(),audioContext.createAnalyser()];analysers.forEach(analyser=>{analyser.fftSize=1024;analyser.smoothingTimeConstant=0;analyser.minDecibels=-100;analyser.maxDecibels=-6});splitter.connect(analysers[0],0);splitter.connect(analysers[1],1);timeBuffers=analysers.map(analyser=>new Float32Array(analyser.fftSize));frequencyBuffers=analysers.map(analyser=>new Float32Array(analyser.frequencyBinCount));fallbackTimer=window.setInterval(processFallback,20);return false}
  async function start(){if(running)return;if(!window.isSecureContext||!navigator.mediaDevices||!navigator.mediaDevices.getUserMedia){ui.secureWarning.classList.remove('hidden');throw new Error('The browser blocked audio capture. Mark this ESP32 origin as secure first.')}if(!ui.device.options.length||!ui.device.value)await discoverDevices(true);originalState=await fetchState();stream=await navigator.mediaDevices.getUserMedia(audioConstraints(ui.device.value));const AudioContextClass=window.AudioContext||window.webkitAudioContext;audioContext=new AudioContextClass({latencyHint:'interactive'});await audioContext.resume();envelopes=[0,0];lastFallbackTime=0;lastSendTime=0;running=true;ui.startButton.disabled=true;ui.stopButton.disabled=false;ui.devicesButton.disabled=true;ui.device.disabled=true;try{localStorage.setItem('carcluster-vu-device',ui.device.value)}catch(_){}const backgroundClock=await startAudioClock(audioContext.createMediaStreamSource(stream));setStatus(backgroundClock?'VU is running. You may switch to Spotify or minimize Chrome; keep this tab open.':'VU is running with the compatibility timer. Keep this tab visible for the smoothest response.','ok')}
  async function stop(restoreState){const wasRunning=running;running=false;if(fallbackTimer)clearInterval(fallbackTimer);fallbackTimer=0;pendingState=null;if(meterNode){meterNode.port.onmessage=null;try{meterNode.disconnect()}catch(_){}}meterNode=null;if(silentGain){try{silentGain.disconnect()}catch(_){}}silentGain=null;if(stream)stream.getTracks().forEach(track=>track.stop());stream=null;if(audioContext){try{await audioContext.close()}catch(_){}}audioContext=null;analysers=[];timeBuffers=[];frequencyBuffers=[];try{if(writePromise)await writePromise;if(restoreState&&originalState)await postStateNow({speed:Number(originalState.speed)||0,rpm:Number(originalState.rpm)||0,ignition:Boolean(originalState.ignition)});if(wasRunning)setStatus('VU stopped and the original cluster state was restored.','ok')}finally{originalState=null;envelopes=[0,0];updateReadouts(0,0,0,0);ui.startButton.disabled=false;ui.stopButton.disabled=true;ui.devicesButton.disabled=false;ui.device.disabled=false}}
  function describeError(error){if(error&&error.name==='NotAllowedError')return'Audio-input permission was denied. Allow microphone access for this page and try again.';if(error&&error.name==='NotFoundError')return'The selected input was not found. Check Voicemeeter and reload the inputs.';return(error&&error.message)||'Unknown audio-input error.'}
  ui.devicesButton.addEventListener('click',()=>discoverDevices(true).catch(error=>setStatus(describeError(error),'error')));ui.startButton.addEventListener('click',()=>start().catch(async error=>{await stop(false);setStatus(describeError(error),'error')}));ui.stopButton.addEventListener('click',()=>stop(true).catch(error=>setStatus(describeError(error),'error')));ui.device.addEventListener('change',()=>{try{localStorage.setItem('carcluster-vu-device',ui.device.value)}catch(_){}});
  [['sensitivity','sensitivityValue',value=>Number(value).toFixed(2)+'x'],['attack','attackValue',value=>value+' ms'],['release','releaseValue',value=>value+' ms']].forEach(([inputId,outputId,format])=>{const input=byId(inputId),output=byId(outputId);input.addEventListener('input',()=>{output.textContent=format(input.value)})});
  window.addEventListener('pagehide',()=>{if(!running||!originalState||!navigator.sendBeacon)return;running=false;const restore=JSON.stringify({speed:Number(originalState.speed)||0,rpm:Number(originalState.rpm)||0,ignition:Boolean(originalState.ignition)});navigator.sendBeacon('/api/state',new Blob([restore],{type:'application/json'}))});
  if(!window.isSecureContext||!navigator.mediaDevices)ui.secureWarning.classList.remove('hidden');fetchState().catch(error=>setStatus(error.message,'error'));if(window.isSecureContext&&navigator.mediaDevices)discoverDevices(false).catch(()=>{});
})();
</script>
</body>
</html>
)HTML";

void webDashboardAudioVU(HttpConnection *connection, int event, void *eventData) {
  if (event != HTTP_EVENT_REQUEST) return;

  HttpMessage *message = (HttpMessage *)eventData;
  if (httpStringCompare(message->method, httpString("GET")) != 0) {
    httpReply(connection, 405, "Allow: GET\r\nConnection: close\r\n", "Method not allowed\n");
    connection->is_draining = true;
    return;
  }

  httpReply(
    connection,
    200,
    "Content-Type: text/html; charset=utf-8\r\n"
    "Cache-Control: no-store\r\n"
    "Permissions-Policy: microphone=(self)\r\n"
    "Connection: close\r\n",
    "%s",
    AUDIO_VU_PAGE);
  connection->is_draining = true;
}
