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
  assert.match(picks[0].reason, /Action RPG/);
  assert.match(picks[0].reason, /Space opera/);
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
