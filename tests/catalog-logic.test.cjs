const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const test = require("node:test");
const vm = require("node:vm");

function makeNode() {
  const attributes = new Map();
  return {
    className: "",
    dataset: {},
    classList: { add() {}, remove() {}, toggle() {} },
    addEventListener() {},
    append() {},
    appendChild() {},
    replaceChildren() {},
    setAttribute(name, value) { attributes.set(name, String(value)); },
    getAttribute(name) { return attributes.get(name) ?? null; },
    removeAttribute(name) { attributes.delete(name); },
    focus() {},
    remove() {},
    showModal() {},
    close() {},
    hidden: false,
    open: false,
    value: "",
    childElementCount: 0
  };
}

function loadCatalogContext() {
  const nodes = new Map();
  const document = {
    querySelector(selector) {
      if (!nodes.has(selector)) nodes.set(selector, makeNode());
      return nodes.get(selector);
    },
    querySelectorAll() { return []; },
    createElement() { return makeNode(); },
    addEventListener() {}
  };
  const context = {
    document,
    console: {error() {}},
    fetch() { return new Promise(() => {}); },
    Set,
    Map,
    Date,
    Math,
    Number,
    String,
    Promise
  };
  const source = fs.readFileSync(path.join(__dirname, "../web/catalog.js"), "utf8");
  vm.createContext(context);
  vm.runInContext(source, context, {filename: "catalog.js"});
  return context;
}

test("empty source arrays fall back to curated editorial metadata on both sides", () => {
  const context = loadCatalogContext();
  const source = {
    gameId: "source",
    title: "Source Game",
    platforms: ["psvita"],
    releases: [],
    genres: [],
    themes: [],
    features: []
  };
  const candidate = {
    gameId: "candidate",
    title: "Candidate Game",
    platforms: ["psp"],
    releases: [],
    genres: [],
    themes: [],
    features: []
  };
  const sourceOverride = {genres: ["Action RPG"], themes: ["Space opera"]};
  context.testSource = source;
  context.testCandidate = candidate;
  context.testOverride = sourceOverride;
  vm.runInContext("state.catalog = {games: [testSource, testCandidate]}; state.overrides = {source: testOverride, candidate: {genres: ['Action RPG'], themes: ['Space opera']}};", context);

  const picks = context.getRecommendations(context.testSource, context.testOverride);
  assert.equal(picks.length, 1);
  assert.equal(picks[0].game.gameId, "candidate");
  assert.equal(picks[0].label, "METADATA MATCH");
  assert.match(picks[0].reason, /action rpg/i);
  assert.match(picks[0].reason, /space opera/i);
});

test("empty in-record relations fall back to curated override relations", () => {
  const context = loadCatalogContext();
  const source = {
    gameId: "source",
    title: "Source Game",
    platforms: ["psvita"],
    releases: [],
    relations: []
  };
  const candidate = {
    gameId: "candidate",
    title: "Candidate Game",
    platforms: ["ps1"],
    releases: []
  };
  const override = {
    relations: [{
      gameId: "candidate",
      relationType: "same-series",
      confidence: 0.95,
      reason: "Same series, different platform."
    }]
  };

  context.testSource = source;
  context.testCandidate = candidate;
  vm.runInContext("state.catalog = {games: [testSource, testCandidate]};", context);
  const picks = context.getRecommendations(context.testSource, override);
  assert.equal(picks.length, 1);
  assert.equal(picks[0].game.gameId, "candidate");
  assert.equal(picks[0].label, "SAME SERIES");
  assert.equal(picks[0].reason, "Same series, different platform.");
});


test("platform filters include release-derived platforms and search release identifiers", () => {
  const context = loadCatalogContext();
  const games = [
    {
      gameId: "vita-game",
      title: "Alpha",
      platforms: ["psvita"],
      releases: [{platform: "psvita", titleId: "PCSE00001", region: "EU"}]
    },
    {
      gameId: "psp-game",
      title: "Beta",
      platforms: [],
      releases: [{platform: "psp", titleId: "ULUS12345", contentId: "NPUH10001", region: "US"}]
    },
    {
      gameId: "ps1-game",
      title: "Gamma",
      platforms: ["ps1"],
      releases: [{platform: "ps1", titleId: "SCUS90001", region: "JP"}]
    }
  ];
  context.testGames = games;
  vm.runInContext("state.catalog = {games: testGames}; state.platform = 'psp'; state.query = ''; state.sort = 'title';", context);
  assert.deepEqual(Array.from(context.filteredGames(), game => game.gameId), ["psp-game"]);

  vm.runInContext("state.platform = 'all'; state.query = 'ULUS12345';", context);
  assert.deepEqual(Array.from(context.filteredGames(), game => game.gameId), ["psp-game"]);

  vm.runInContext("state.query = 'JP';", context);
  assert.deepEqual(Array.from(context.filteredGames(), game => game.gameId), ["ps1-game"]);
});

test("size sorting uses the largest release package size and remains deterministic", () => {
  const context = loadCatalogContext();
  context.testGames = [
    {gameId: "small", title: "Alpha", platforms: ["psvita"], releases: [{packageSizeBytes: 100}]},
    {gameId: "large", title: "Beta", platforms: ["psp"], releases: [{packageSizeBytes: 500}, {packageSizeBytes: 900}]},
    {gameId: "medium", title: "Gamma", platforms: ["ps1"], releases: [{packageSizeBytes: 400}]}
  ];
  vm.runInContext("state.catalog = {games: testGames}; state.platform = 'all'; state.query = ''; state.sort = 'size';", context);
  assert.deepEqual(Array.from(context.filteredGames(), game => game.gameId), ["large", "medium", "small"]);
});
