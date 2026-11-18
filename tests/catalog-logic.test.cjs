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

test("age classification prefers PEGI while retaining the source territory", () => {
  const context = loadCatalogContext();
  const game = {
    ageRatings: [
      {system: "ESRB", value: "T", territory: "US"},
      {system: "PEGI", value: "12", territory: "Europe"}
    ]
  };
  const chosen = context.preferredAgeRating(game, {});
  assert.equal(chosen.system, "PEGI");
  assert.equal(chosen.value, "12");
  assert.equal(chosen.territory, "Europe");
});

test("external score indicator prefers OpenCritic and preserves its own scale", () => {
  const context = loadCatalogContext();
  const game = {
    externalScores: [
      {provider: "GameSpot", value: 8, scale: 10},
      {provider: "OpenCritic", value: 84, scale: 100},
      {provider: "Metacritic", value: 81, scale: 100}
    ]
  };
  const chosen = context.preferredExternalScore(game, {});
  assert.equal(chosen.provider, "OpenCritic");
  assert.equal(context.formatScoreValue(chosen), "84/100");
});

test("score formatting does not invent a scale or turn missing data into zero", () => {
  const context = loadCatalogContext();
  assert.equal(context.formatScoreValue({value: 8.5}), "8.5");
  assert.equal(context.formatScoreValue({value: null, scale: 10}), "");
  assert.equal(context.formatScoreValue({value: 8, scale: 0}, true), "");
});

test("curated review copy and rating descriptors are searchable", () => {
  const context = loadCatalogContext();
  const game = {
    gameId: "reviewed-game",
    title: "Example",
    platforms: ["psvita"],
    releases: [],
    ageRatings: [],
    externalScores: []
  };
  context.testGame = game;
  vm.runInContext(`
    state.overrides = {
      "reviewed-game": {
        ageRatings: [{system: "PEGI", value: "16", territory: "Europe", descriptors: ["Violence"]}],
        editorial: {
          standfirst: "A lost cyberpunk classic",
          review: {headline: "Still electric", body: "Excellent pacing and atmosphere", verdict: "Worth revisiting."},
          curiosities: ["Built for late-night sessions"]
        }
      }
    };
  `, context);
  const searchText = context.gameSearchText(context.testGame);
  assert.match(searchText, /cyberpunk classic/i);
  assert.match(searchText, /violence/i);
  assert.match(searchText, /late-night sessions/i);
});

test("curated artwork overrides merge per role and take priority over catalog assets", () => {
  const context = loadCatalogContext();
  const game = {
    artwork: {
      cover: {url: "https://cdn.example.org/old-cover.jpg", alt: "Catalog cover"},
      hero: {url: "https://cdn.example.org/hero.jpg", alt: "Catalog hero"},
      screenshots: [{url: "https://cdn.example.org/shot-1.jpg", alt: "Shot one"}]
    }
  };
  const override = {
    artwork: {
      cover: {url: "https://cdn.example.org/new-cover.jpg", alt: "Curated cover"},
      screenshots: [{url: "https://cdn.example.org/shot-2.jpg", alt: "Shot two"}]
    }
  };
  context.testGame = game;
  context.testOverride = override;
  const merged = vm.runInContext("mergeGameArtwork(testGame, testOverride)", context);
  assert.equal(merged.cover.alt, "Curated cover");
  assert.equal(merged.hero.alt, "Catalog hero");
  assert.deepEqual(Array.from(merged.screenshots, item => item.alt), ["Shot two", "Shot one"]);
});

test("pending curated cover falls back to an existing upstream cover", () => {
  const context = loadCatalogContext();
  context.testGame = {
    artwork: {cover: {url: "https://cdn.example.org/catalog-cover.jpg", alt: "Upstream cover"}}
  };
  context.testOverride = {
    artwork: {cover: {url: "https://cdn.example.org/pending-cover.jpg", alt: "Pending cover", rightsStatus: "pending-review"}}
  };
  const merged = vm.runInContext("mergeGameArtwork(testGame, testOverride)", context);
  assert.equal(merged.cover.alt, "Upstream cover");
});

test("artwork is displayed only from HTTPS and approved curated rights states", () => {
  const context = loadCatalogContext();
  const asset = {url: "https://cdn.example.org/cover.jpg", alt: "Cover"};
  context.testAsset = asset;
  assert.equal(vm.runInContext("isDisplayableArtworkAsset(testAsset)", context), true);
  assert.equal(vm.runInContext("isDisplayableArtworkAsset({...testAsset, rightsStatus:'pending-review'})", context), false);
  assert.equal(vm.runInContext("isDisplayableArtworkAsset({...testAsset, rightsStatus:'rejected'})", context), false);
  assert.equal(vm.runInContext("isDisplayableArtworkAsset({...testAsset, rightsStatus:'permission-granted'})", context), true);
  assert.equal(vm.runInContext("isDisplayableArtworkAsset({...testAsset, url:'http://cdn.example.org/cover.jpg'})", context), false);
});

