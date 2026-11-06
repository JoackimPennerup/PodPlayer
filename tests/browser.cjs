// Optional test dependency: an already installed Playwright, never downloaded here.
const assert = require('node:assert/strict');
const path = require('node:path');
const {pathToFileURL} = require('node:url');
const {chromium} = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
let browser;
(async () => {
  browser = await chromium.launch({executablePath:process.env.CHROMIUM_PATH || '/usr/bin/chromium',headless:true,args:['--no-sandbox','--disable-dev-shm-usage']});
  const page = await browser.newPage({viewport:{width:1400,height:1000}});
  const errors=[], network=[];
  page.on('pageerror', e => errors.push(e.message));
  page.on('request', r => { if (/^https?:/.test(r.url())) network.push(r.url()); });
  const file=process.env.MOCK_BASE_URL || pathToFileURL(path.resolve(__dirname,'../mock/index.html')).href;
  await page.goto(file);
  await page.locator('#reset').click();
  const screen=page.locator('#screen');
  assert.deepEqual(await screen.evaluate(e=>[e.clientWidth,e.clientHeight]),[240,320]);
  await page.getByRole('button',{name:'Verklig storlek'}).click();
  assert.equal(Math.round((await screen.boundingBox()).width),240);
  await page.getByRole('button',{name:'Förstorad · 150 %'}).click();
  assert.equal(Math.round((await screen.boundingBox()).width),360);
  await page.screenshot({path:path.resolve(__dirname,'../test-results/library.png')});
  await page.getByRole('button',{name:'Öppna Mellan raderna',exact:true}).click();
  assert.equal(await page.locator('.episode-row').count(),4);
  await page.locator('.episode-row').first().click();
  assert.equal(await page.locator('#elapsed').textContent(),'7:22');
  await page.getByRole('button',{name:'Hoppa framåt 10 sekunder'}).click();
  assert.equal(await page.locator('#elapsed').textContent(),'7:32');
  await page.getByRole('button',{name:'Hoppa bakåt 10 sekunder'}).click();
  assert.equal(await page.locator('#elapsed').textContent(),'7:22');
  await page.getByRole('button',{name:'Spela',exact:true}).click();
  await page.waitForTimeout(1300);
  assert.notEqual(await page.locator('#elapsed').textContent(),'7:22');
  await page.getByRole('button',{name:'Pausa',exact:true}).click();
  const paused=await page.locator('#elapsed').textContent();
  await page.waitForTimeout(1100);
  assert.equal(await page.locator('#elapsed').textContent(),paused);
  await page.screenshot({path:path.resolve(__dirname,'../test-results/player.png')});
  // All screen buttons remain in the 320px viewport and have finger-sized targets.
  const bounds=await screen.boundingBox();
  for(const button of await screen.locator('button').all()) {
    const b=await button.boundingBox(); assert.ok(b.width>=44*1.5 && b.height>=44*1.5);
    assert.ok(b.y>=bounds.y && b.y+b.height<=bounds.y+bounds.height+1);
  }
  await page.getByRole('button',{name:'Inställningar',exact:true}).click();
  await page.locator('#volume').fill('27');
  assert.equal(await page.locator('#volume-value').textContent(),'27 %');
  await page.getByRole('switch',{name:'Kom ihåg position'}).click();
  assert.equal(await page.getByRole('switch').getAttribute('aria-checked'),'false');
  await page.screenshot({path:path.resolve(__dirname,'../test-results/settings.png')});
  await page.getByRole('button',{name:'Tillbaka',exact:true}).click();
  assert.match(await page.locator('[data-volume]').textContent(),/27/);
  for(const [state,text] of [['loading','Hämtar avsnitt'],['empty','Inga avsnitt ännu'],['offline','Offline'],['error','Ljudet kunde inte spelas'],['format','Ljudformatet stöds inte']]) {
    await page.locator('#scenario').selectOption(state);
    assert.ok((await screen.textContent()).includes(text));
    await page.screenshot({path:path.resolve(__dirname,`../test-results/${state}.png`)});
  }
  await page.locator('#scenario').selectOption('normal');
  await page.getByRole('button',{name:'Inställningar',exact:true}).click();
  await page.locator('#volume').fill('38');
  await page.reload();
  await page.getByRole('button',{name:'Inställningar',exact:true}).click();
  assert.equal(await page.locator('#volume-value').textContent(),'38 %');
  // Offline mode rejects un-cached episodes but permits the single cached demo.
  await page.locator('#scenario').selectOption('offline');
  await page.locator('.episode-row').nth(1).click();
  assert.ok((await screen.textContent()).includes('Ingen nätanslutning'));
  // State persistence unavailable: app must remain functional.
  const noStorage = await browser.newContext();
  await noStorage.addInitScript(() => {
    Object.defineProperty(window,'localStorage',{get(){throw new Error('Storage disabled');}});
  });
  const privatePage=await noStorage.newPage();
  await privatePage.goto(file);
  assert.match(await privatePage.locator('#storage-note').textContent(),/kan inte spara/);
  await privatePage.getByRole('button',{name:'Öppna Mellan raderna',exact:true}).click();
  assert.equal(await privatePage.locator('.episode-row').count(),4);
  await page.locator('#reset').click();
  await page.setViewportSize({width:390,height:844});
  await page.waitForTimeout(300);
  assert.ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
  await page.screenshot({path:path.resolve(__dirname,'../test-results/mobile.png'),fullPage:true});
  const external=network.filter(url=> !process.env.MOCK_BASE_URL || new URL(url).origin!==new URL(file).origin);
  assert.deepEqual(external,[]); assert.deepEqual(errors,[]);
  console.log(`PASS: Chromium ${process.env.MOCK_BASE_URL ? 'localhost' : 'file://'} navigation, playback, seeking, pause, volume, persistence, 5 scenarios, storage denied, mobile layout; 0 external requests.`);
  await browser.close();
})().catch(async e=>{console.error(e);await browser?.close();process.exitCode=1;});
