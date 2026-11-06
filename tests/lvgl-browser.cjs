const assert=require('node:assert/strict');
const fs=require('node:fs');
const {chromium}=require(process.env.PLAYWRIGHT_MODULE||'playwright');
const base=process.env.LVGL_BASE_URL||'http://127.0.0.1:8766/web/';
let browser;
(async()=>{
 fs.mkdirSync('test-results',{recursive:true});
 browser=await chromium.launch({executablePath:process.env.CHROMIUM_PATH||'/usr/bin/chromium',headless:true,args:['--no-sandbox','--disable-dev-shm-usage']});
 const page=await browser.newPage({viewport:{width:1400,height:1000}});
 const errors=[],external=[],responses=[];
 page.on('pageerror',e=>errors.push(e.message));
 page.on('request',r=>{if(/^https?:/.test(r.url())&&new URL(r.url()).origin!==new URL(base).origin)external.push(r.url());});
 page.on('response',r=>{if(r.url().endsWith('.wasm'))responses.push(r.status());});
 const boot=async()=>{await page.goto(base);await page.waitForFunction(()=>window.PodPlayerLVGL?.flushCount>0);};
 const state=()=>page.evaluate(()=>PodPlayerLVGL.snapshot());
 const click=async(x,y)=>{const r=await page.locator('canvas').boundingBox();await page.mouse.click(r.x+x*r.width/240,r.y+y*r.height/320);await page.waitForTimeout(90);};
 const shot=name=>page.screenshot({path:`test-results/lvgl-${name}.png`});
 await boot();await page.locator('#reset').click();await page.waitForTimeout(100);
 assert.deepEqual(await page.locator('canvas').evaluate(e=>[e.width,e.height,e.childElementCount]),[240,320,0]);
 assert.equal((await state()).position,442);await shot('library');
 await click(100,110);assert.equal((await state()).screen,1);await shot('episodes');
 await click(100,100);assert.equal((await state()).screen,2);assert.equal((await state()).position,442);
 await click(193,254);assert.equal((await state()).position,452);
 await click(47,254);assert.equal((await state()).position,442);
 await click(120,254);assert.equal((await state()).playing,true);await page.waitForTimeout(1300);assert.ok((await state()).position>=443);
 await click(120,254);assert.equal((await state()).playing,false);const paused=(await state()).position;await page.waitForTimeout(1100);assert.equal((await state()).position,paused);
 await shot('player');
 // The slider is an LVGL widget: operate it using real canvas pointer input.
 await click(18,180);assert.equal((await state()).position,0);
 await click(222,180);assert.equal((await state()).position,2382);assert.equal((await state()).playing,false);
 await click(47,254);assert.equal((await state()).position,2372);
 await click(214,42);assert.equal((await state()).screen,3);
 await click(78,112);assert.ok((await state()).volume>=20&&(await state()).volume<=40);
 const volume=(await state()).volume;
 await click(206,220);assert.equal((await state()).resume,false);await click(206,220);assert.equal((await state()).resume,true);
 await shot('settings');
 await click(24,42);assert.equal((await state()).screen,2);
 await page.reload();await page.waitForFunction(()=>window.PodPlayerLVGL?.flushCount>0);assert.equal((await state()).volume,volume);assert.equal((await state()).position,2372);assert.equal((await state()).playing,false);
 await click(100,298);assert.equal((await state()).screen,2);
 for(const [scenario,value] of [['loading',1],['empty',2],['offline',3],['error',4],['format',5]]){
  await page.locator('#scenario').selectOption(scenario);await page.waitForTimeout(100);assert.equal((await state()).scenario,value);assert.equal((await state()).playing,false);await shot(scenario);
 }
 // Unsupported-format action returns to the episode list.
 await click(100,266);assert.equal((await state()).screen,1);assert.equal((await state()).scenario,0);
 // Drag the LVGL episode container to expose the last row (no DOM scroll area).
 let r=await page.locator('canvas').boundingBox();
 await page.mouse.move(r.x+226*r.width/240,r.y+241*r.height/320);await page.mouse.down();
 await page.mouse.move(r.x+226*r.width/240,r.y+95*r.height/320,{steps:15});await page.mouse.up();await page.waitForTimeout(350);
 await shot('scrolled');
 await click(100,238);assert.equal((await state()).screen,2);assert.equal((await state()).episode,3);
 // Storage-denied sessions remain usable.
 const privateContext=await browser.newContext();await privateContext.addInitScript(()=>Object.defineProperty(window,'localStorage',{get(){throw new Error('Disabled');}}));
 const privatePage=await privateContext.newPage();await privatePage.goto(base);await privatePage.waitForFunction(()=>window.PodPlayerLVGL?.flushCount>0);assert.match(await privatePage.locator('#storage-note').textContent(),/kan inte spara/);await privateContext.close();
 await page.locator('#reset').click();await page.getByRole('button',{name:'Verklig storlek'}).click();await page.waitForTimeout(250);assert.equal(Math.round((await page.locator('canvas').boundingBox()).width),240);
 await page.getByRole('button',{name:'Förstorad · 150 %'}).click();await page.waitForTimeout(250);assert.equal(Math.round((await page.locator('canvas').boundingBox()).width),360);
 await page.reload();await page.waitForFunction(()=>window.PodPlayerLVGL?.flushCount>0);await page.setViewportSize({width:390,height:844});await page.waitForTimeout(300);assert.ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));await page.screenshot({path:'test-results/lvgl-mobile.png',fullPage:true});
 assert.ok(responses.length>0&&responses.every(status=>status===200));assert.deepEqual(errors,[]);assert.deepEqual(external,[]);
 console.log('PASS: real LVGL/WASM canvas, rapid pointer clicks, navigation, playback/timing, ±10, slider boundaries, volume, resume, reload persistence, 5 scenarios, scrolling gesture, preview sizing, mobile layout, denied storage. Zero external requests.');
 await browser.close();
})().catch(async e=>{console.error(e);await browser?.close();process.exitCode=1;});
