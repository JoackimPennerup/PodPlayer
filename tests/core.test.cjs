const test = require('node:test');
const assert = require('node:assert/strict');
const {createPlayer} = require('../mock/core.js');
const episodes = require('../fixtures/episodes.json');
test('select by stable ID, not title; unknown ID rejected', () => {
  const p = createPlayer(episodes);
  assert.equal(p.select('missing'), false);
  assert.equal(p.select(episodes[1].id), true);
  assert.equal(p.state.episodeId, episodes[1].id);
});
test('play, pause, tick, completion and restart', () => {
  const p = createPlayer(episodes);
  const initial = p.state.progress[p.state.episodeId];
  p.tick(3); assert.equal(p.state.progress[p.state.episodeId], initial);
  p.toggle(); p.tick(3); assert.equal(p.state.progress[p.state.episodeId], initial+3);
  p.toggle(); p.tick(3); assert.equal(p.state.progress[p.state.episodeId], initial+3);
  p.seek(1e9); assert.equal(p.state.progress[p.state.episodeId], p.episode().duration);
  assert.equal(p.state.playing, false);
  p.toggle(); assert.equal(p.state.progress[p.state.episodeId], 0); assert.equal(p.state.playing, true);
});
test('seek boundaries and invalid deltas', () => {
  const p = createPlayer(episodes);
  p.seek(-1e9); assert.equal(p.state.progress[p.state.episodeId], 0);
  p.seek(10); assert.equal(p.state.progress[p.state.episodeId], 10);
  p.seek(-10); assert.equal(p.state.progress[p.state.episodeId], 0);
  assert.equal(p.seek(NaN), false); assert.equal(p.seek(Infinity), false);
});
test('saved position and volume are bounded, playback never autostarts', () => {
  const p = createPlayer(episodes, {volume:400, playing:true, progress:{[episodes[0].id]:1e9}});
  assert.equal(p.state.volume, 100); assert.equal(p.state.playing, false);
  assert.equal(p.state.progress[episodes[0].id], episodes[0].duration);
  const restored = createPlayer(episodes, JSON.parse(JSON.stringify(p.state)));
  assert.deepEqual(restored.state, p.state);
});
test('resume preference and invalid saved data', () => {
  const p = createPlayer(episodes, {resume:false, episodeId:'missing', volume:'bad'});
  p.select(episodes[0].id); assert.equal(p.state.progress[episodes[0].id], 0);
  assert.equal(p.state.volume,65);
});
