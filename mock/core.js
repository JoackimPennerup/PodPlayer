(function (root) {
  'use strict';
  const clamp = (n, min, max) => Math.max(min, Math.min(max, n));
  function createPlayer(episodes, saved = {}) {
    const progress = {};
    for (const e of episodes) {
      const p = saved.progress?.[e.id];
      progress[e.id] = Number.isFinite(p) ? clamp(p, 0, e.duration) : e.position || 0;
    }
    const state = {
      episodeId: episodes.some(e => e.id === saved.episodeId) ? saved.episodeId : episodes[0].id,
      progress, playing: false, volume: Number.isFinite(saved.volume) ? clamp(saved.volume, 0, 100) : 65,
      resume: saved.resume !== false, wifi: saved.wifi !== false
    };
    const episode = () => episodes.find(e => e.id === state.episodeId);
    function select(id) {
      if (!episodes.some(e => e.id === id)) return false;
      state.episodeId = id; state.playing = false;
      if (!state.resume) state.progress[id] = 0;
      return true;
    }
    function seek(delta) {
      if (!Number.isFinite(delta)) return false;
      state.progress[state.episodeId] = clamp(state.progress[state.episodeId] + delta, 0, episode().duration);
      if (state.progress[state.episodeId] === episode().duration) state.playing = false;
      return true;
    }
    function toggle() {
      if (state.progress[state.episodeId] >= episode().duration) state.progress[state.episodeId] = 0;
      state.playing = !state.playing;
    }
    function tick(seconds) { if (state.playing && Number.isFinite(seconds) && seconds >= 0) seek(seconds); }
    return { state, episode, select, seek, toggle, tick };
  }
  const api = { createPlayer, clamp };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  root.PodCore = api;
})(globalThis);
