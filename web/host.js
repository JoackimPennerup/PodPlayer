/* Browser hardware adapter only: pixels, pointer events and local persistence.
 * Every on-device widget, navigation and playback transition runs in C++/LVGL. */
(async () => {
 'use strict';
 const canvas=document.getElementById('screen'),status=document.getElementById('boot-status');
 const note=document.getElementById('storage-note'),context=canvas.getContext('2d',{alpha:false});
 const key='podplayer.lvgl.v1';
 const surface={context,image:context.createImageData(240,320)};
 try {
  if(typeof createPodPlayer!=='function')throw new Error('WebAssembly-bygget saknas. Kör python3 scripts/build-web.py.');
  const app=await createPodPlayer({surface,printErr:message=>console.error(message),onAbort:message=>{status.hidden=false;status.textContent='LVGL kunde inte köras: '+message;}});
  const snapshot=()=>JSON.parse(app.UTF8ToString(app._demo_snapshot()));
  let storageAvailable=true;
  app.saveState=()=>{
   try{localStorage.setItem(key,JSON.stringify({version:1,...snapshot()}));}
   catch{storageAvailable=false;}
   note.textContent=storageAvailable?'Dina val sparas i den här webbläsaren.':'Webbläsaren kan inte spara. Dina val gäller tills sidan stängs.';
  };
  try {
   const saved=JSON.parse(localStorage.getItem(key)||'null');
   if(saved?.version===1&&Array.isArray(saved.positions)&&Array.isArray(saved.ids)&&saved.positions.length<=40){
    const current=snapshot();
    saved.positions.forEach((p,i)=>{if(Number.isInteger(p)&&p>=0&&p<=2147483647){const index=current.ids.indexOf(saved.ids[i]);if(index>=0)app._demo_restore_episode(index,p);}});
    const selected=current.ids.indexOf(saved.episodeId);
    if(selected>=0&&Number.isInteger(saved.volume)&&saved.volume>=0&&saved.volume<=100&&typeof saved.resume==='boolean'&&typeof saved.wifi==='boolean')app._demo_restore_settings(selected,saved.volume,+saved.resume,+saved.wifi);
   }
  } catch {storageAvailable=false;}
  app.saveState();
  let held=false;
  const pointer=(e,down)=>{const r=canvas.getBoundingClientRect();app._demo_pointer(Math.floor((e.clientX-r.left)*240/r.width),Math.floor((e.clientY-r.top)*320/r.height),+down);};
  canvas.addEventListener('pointerdown',e=>{if(e.button!==0)return;e.preventDefault();held=true;canvas.setPointerCapture(e.pointerId);pointer(e,true);});
  canvas.addEventListener('pointermove',e=>{if(held)pointer(e,true);});
  canvas.addEventListener('pointerup',e=>{pointer(e,false);held=false;});
  canvas.addEventListener('pointercancel',e=>{pointer(e,false);held=false;});
  const release=()=>{held=false;app._demo_pointer(0,0,0);};
  window.addEventListener('blur',release);
  document.addEventListener('visibilitychange',()=>{if(document.hidden){release();app.saveState();}});
  window.addEventListener('pagehide',()=>{release();app.saveState();});
  document.getElementById('scenario').addEventListener('change',e=>app._demo_scenario(e.target.selectedIndex));
  document.getElementById('reset').addEventListener('click',()=>{app._demo_reset();document.getElementById('scenario').value='normal';});
  document.querySelectorAll('[data-scale]').forEach(button=>button.addEventListener('click',()=>{
   document.documentElement.style.setProperty('--scale',button.dataset.scale);
   document.querySelectorAll('[data-scale]').forEach(b=>b.setAttribute('aria-pressed',String(b===button)));
  }));
  // Read-only diagnostics for integration tests; no DOM replacement or fake controls.
  window.PodPlayerLVGL={snapshot,version:'8.3.10',get flushCount(){return app.flushCount||0;}};
  status.hidden=true;
 } catch(error){status.hidden=false;status.textContent='Kunde inte starta LVGL. '+error.message+' Öppna via en lokal HTTP-server.';console.error(error);}
})();
