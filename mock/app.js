/* All content and assets are local. No network calls or audio devices are used. */
(() => {
  'use strict';
  const { podcasts, episodes } = POD_DATA;
  const key = 'podplayer.demo.v1';
  let storageAvailable = true;
  let saved = {};
  try { saved = JSON.parse(localStorage.getItem(key) || '{}') || {}; }
  catch { storageAvailable = false; }
  const player = PodCore.createPlayer(episodes, saved);
  let view = 'podcasts', podcastId = episodes.find(e => e.id === player.state.episodeId).podcastId;
  let scenario = 'normal';
  const screen = document.getElementById('screen');
  const icon = name => {
    const selected = POD_ICONS[name] || POD_ICONS.list;
    return `<svg viewBox="0 0 24 24" aria-hidden="true" focusable="false" data-lucide="${selected.name}">${selected.svg}</svg>`;
  };
  const escape = s => String(s).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  const fmt = n => `${Math.floor(n / 60)}:${String(Math.floor(n % 60)).padStart(2, '0')}`;
  const currentPod = () => podcasts.find(p => p.id === podcastId);
  const online = () => player.state.wifi && scenario !== 'offline';
  const save = () => {
    try { localStorage.setItem(key, JSON.stringify(player.state)); }
    catch { storageAvailable = false; }
    document.getElementById('storage-note').textContent = storageAvailable ? 'Dina val sparas i den här webbläsaren.' : 'Webbläsaren kan inte spara. Dina val gäller tills sidan stängs.';
  };
  function art(p, size) {
    const title = size === 'small' ? p.initials.split('\n').map(word => word[0]).join('') : p.initials;
    return `<div class="art ${size}" style="--art:${p.color}" aria-hidden="true"><b>${escape(title).replace('\n', '<br>')}</b><span>${size === 'small' ? 'PODD' : 'EN PODD · DEMO'}</span></div>`;
  }
  function toolbar(title, back = false) {
    return `<header class="toolbar ${back ? 'compact' : ''}">${back ? `<button class="icon-button" data-action="back" aria-label="Tillbaka">${icon('back')}</button>` : ''}<h2>${escape(title)}</h2>${view === 'settings' ? '' : `<button class="icon-button" data-action="settings" aria-label="Inställningar">${icon('settings')}</button>`}</header>`;
  }
  function mini() {
    return `<footer class="footer"><button class="mini-player" data-action="current" aria-label="Öppna aktuell uppspelning">${icon('volume')}<span class="mini-copy"><strong>${escape(player.episode().title)}</strong><small>Fortsätt lyssna · <span data-mini-time>${fmt(player.state.progress[player.state.episodeId])}</span></small></span></button><button class="icon-button" data-action="toggle" aria-label="${player.state.playing ? 'Pausa' : 'Spela'}">${icon(player.state.playing ? 'pause' : 'play')}</button></footer>`;
  }
  function message(title, body, type, action, label) {
    return `<div class="state-view" role="status">${type === 'loading' ? '<div class="loading-ring"></div>' : icon(type)}<h3>${title}</h3><p>${body}</p>${action ? `<button class="primary" data-action="${action}">${label}</button>` : ''}</div>`;
  }
  function render() {
    let html = `<div class="status"><span>PodPlayer</span><span class="status-right">${icon(online() ? 'wifi' : 'offline')}${icon('battery')}</span></div>`;
    if (view === 'podcasts') {
      html += toolbar('Mina poddar');
      if (!online()) html += '<div class="offline-banner">Offline · sparade avsnitt kan spelas</div>';
      html += '<div class="content"><div class="section-label">DITT BIBLIOTEK</div>' + podcasts.map(p => `<button class="podcast-row" data-podcast="${p.id}" aria-label="Öppna ${escape(p.title)}">${art(p,'small')}<span class="row-copy"><strong>${escape(p.title)}</strong><small>${episodes.filter(e => e.podcastId === p.id).length} avsnitt</small></span>${p.newCount ? `<span class="new-dot" aria-label="${p.newCount} nya">${p.newCount}</span>` : ''}${icon('chevron').replace('<svg ', '<svg class="row-arrow" ')}</button>`).join('') + '</div>' + mini();
    } else if (view === 'episodes') {
      html += toolbar(currentPod().title, true);
      if (scenario === 'loading') html += message('Hämtar avsnitt', 'Det tar bara en liten stund.', 'loading', 'retry', 'Visa demoavsnitt');
      else if (scenario === 'empty') html += message('Inga avsnitt ännu', 'Nya avsnitt visas här när podden publicerar dem.', 'list', 'retry', 'Försök igen');
      else {
        if (!online()) html += '<div class="offline-banner">Offline · 1 avsnitt sparat i demonstrationen</div>';
        html += '<div class="content">' + episodes.filter(e => e.podcastId === podcastId).map(e => {
          const pos = player.state.progress[e.id];
          const date = new Date(e.pubDate + 'T12:00:00').toLocaleDateString('sv-SE', { day:'numeric', month:'short' });
          return `<button class="episode-row" data-episode="${e.id}"><span class="row-copy"><strong>${escape(e.title)}</strong></span><span class="episode-meta">${date}<span>·</span>${Math.round(e.duration/60)} min${e.downloaded ? '<span>· Sparat</span>' : ''}</span>${pos > 0 ? `<span class="episode-meta partial">${pos >= e.duration ? 'Lyssnat' : `${fmt(e.duration-pos)} kvar`}<span class="tiny-progress"><i style="width:${pos/e.duration*100}%"></i></span></span>` : ''}</button>`;
        }).join('') + '</div>' + mini();
      }
    } else if (view === 'player') {
      const e = player.episode(), p = podcasts.find(p => p.id === e.podcastId);
      html += toolbar('Spelas nu', true);
      if (scenario === 'error') html += message('Ljudet kunde inte spelas', 'Försök igen. Din position är sparad.', 'error', 'retry', 'Försök igen');
      else if (scenario === 'format') html += message('Ljudformatet stöds inte', 'Välj ett annat avsnitt för att fortsätta lyssna.', 'error', 'episodes', 'Visa avsnitt');
      else if (!online() && !e.downloaded) html += message('Ingen nätanslutning', 'Det här avsnittet är inte sparat. Anslut till Wi-Fi eller välj ett sparat avsnitt.', 'offline', 'settings', 'Öppna inställningar');
      else html += `<div class="player">${art(p,'hero')}<div class="player-heading"><p class="pod-name">${escape(p.title)}</p><h3>${escape(e.title)}</h3></div><div class="player-status" data-player-status>${player.state.playing ? 'Spelar' : 'Pausad'}${!online() ? ' · Sparat avsnitt' : ' · Demo'}</div><input class="progress" id="progress" type="range" min="0" max="${e.duration}" value="${player.state.progress[e.id]}" aria-label="Uppspelningsposition"><div class="time-labels"><span id="elapsed">${fmt(player.state.progress[e.id])}</span><span id="remaining">−${fmt(e.duration-player.state.progress[e.id])}</span></div><div class="transport"><button class="skip" data-action="back10" aria-label="Hoppa bakåt 10 sekunder">${icon('back10')}<span>10</span></button><button class="play-button" data-action="toggle" aria-label="${player.state.playing ? 'Pausa' : 'Spela'}">${icon(player.state.playing ? 'pause' : 'play')}</button><button class="skip" data-action="forward10" aria-label="Hoppa framåt 10 sekunder">${icon('forward10')}<span>10</span></button></div><div class="player-bottom">${icon('volume')}<span data-volume>${player.state.volume} %</span> · Simulerat ljud</div></div>`;
    } else {
      html += toolbar('Inställningar', true) + `<div class="content settings"><div class="setting"><label for="volume">Volym <span id="volume-value">${player.state.volume} %</span></label><input id="volume" type="range" min="0" max="100" value="${player.state.volume}"></div><div class="setting"><button class="wifi-toggle" data-action="wifi" aria-label="Växla simulerad Wi-Fi-anslutning"><span>Wi-Fi</span><span class="wifi-value">${online() ? 'Ansluten' : 'Ej ansluten'}</span></button></div><div class="setting"><button class="switch" data-action="resume" role="switch" aria-checked="${player.state.resume}"><span>Kom ihåg position</span><span class="switch-track"></span></button></div><div class="setting"><span class="setting-title">Om spelaren</span><small>PodPlayer 0.1 · UI-prototyp<br>ESP32-S3 · 240 × 320<br>Wi-Fi och ljud simuleras.</small></div></div>`;
    }
    screen.innerHTML = html;
  }
  let previousView = 'podcasts';
  screen.addEventListener('click', event => {
    const button = event.target.closest('button'); if (!button) return;
    if (button.dataset.podcast) { podcastId = button.dataset.podcast; view = 'episodes'; }
    if (button.dataset.episode) { player.select(button.dataset.episode); view = 'player'; }
    const action = button.dataset.action;
    if (action === 'back') { view = view === 'settings' ? previousView : view === 'player' ? 'episodes' : 'podcasts'; }
    if (action === 'settings') { if (view !== 'settings') previousView = view; view = 'settings'; }
    if (action === 'current') { podcastId = player.episode().podcastId; view = 'player'; }
    if (action === 'episodes') { scenario = 'normal'; view = 'episodes'; }
    if (action === 'toggle') {
      if (!online() && !player.episode().downloaded) { view = 'player'; }
      else { player.toggle(); }
    }
    if (action === 'back10') player.seek(-10);
    if (action === 'forward10') player.seek(10);
    if (action === 'resume') player.state.resume = !player.state.resume;
    if (action === 'wifi') { player.state.wifi = !player.state.wifi; scenario = 'normal'; if (!player.state.wifi && !player.episode().downloaded) player.state.playing = false; }
    if (action === 'retry') scenario = 'normal';
    document.getElementById('scenario').value = scenario;
    save(); render();
  });
  screen.addEventListener('input', event => {
    if (event.target.id === 'progress') { player.seek(Number(event.target.value) - player.state.progress[player.state.episodeId]); updateProgress(); }
    if (event.target.id === 'volume') { player.state.volume = Number(event.target.value); document.getElementById('volume-value').textContent = `${player.state.volume} %`; }
    save();
  });
  function updateProgress() {
    const e = player.episode(), pos = player.state.progress[e.id];
    if (document.getElementById('elapsed')) {
      document.getElementById('elapsed').textContent = fmt(pos);
      document.getElementById('remaining').textContent = '−' + fmt(e.duration-pos);
      document.getElementById('progress').value = pos;
    }
    const miniTime = screen.querySelector('[data-mini-time]'); if (miniTime) miniTime.textContent = fmt(pos);
  }
  let last = performance.now(), lastSaved = 0;
  setInterval(() => {
    const now = performance.now(), wasPlaying = player.state.playing;
    player.tick(Math.min((now-last)/1000, 2)); last = now;
    if (wasPlaying) {
      if (!player.state.playing) { save(); render(); } else updateProgress();
      if (now-lastSaved > 15000) { save(); lastSaved = now; }
    }
  }, 250);
  document.querySelectorAll('[data-scale]').forEach(button => button.addEventListener('click', () => {
    document.documentElement.style.setProperty('--scale', button.dataset.scale);
    document.querySelectorAll('[data-scale]').forEach(b => b.setAttribute('aria-pressed', String(b === button)));
  }));
  document.getElementById('scenario').addEventListener('change', event => {
    scenario = event.target.value; player.state.playing = false;
    view = ['error', 'format'].includes(scenario) ? 'player' : scenario === 'normal' ? 'podcasts' : 'episodes';
    render(); save();
  });
  document.getElementById('reset').addEventListener('click', () => {
    const fresh = PodCore.createPlayer(episodes).state;
    Object.assign(player.state, fresh); view = 'podcasts'; scenario = 'normal'; podcastId = podcasts[0].id;
    document.getElementById('scenario').value = scenario; save(); render();
  });
  window.addEventListener('pagehide', save);
  render(); save();
})();
