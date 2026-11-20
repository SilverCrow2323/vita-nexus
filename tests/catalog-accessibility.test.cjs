const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const test = require("node:test");
const vm = require("node:vm");

function makeNode(tagName = "div", documentRef = null) {
  const attributes = new Map();
  const children = [];
  const listeners = new Map();
  return {
    tagName: tagName.toUpperCase(),
    id: "",
    className: "",
    dataset: {},
    children,
    isConnected: true,
    classList: {add() {}, remove() {}, toggle() {}},
    addEventListener(name, handler) {
      const list = listeners.get(name) || [];
      list.push(handler);
      listeners.set(name, list);
    },
    dispatchEvent(event) {
      (listeners.get(event.type) || []).forEach((handler) => handler(event));
    },
    append(...items) { children.push(...items); },
    appendChild(item) { children.push(item); },
    replaceChildren(...items) { children.splice(0, children.length, ...items); },
    setAttribute(name, value) {
      attributes.set(name, String(value));
      if (name === "open") this.open = true;
    },
    getAttribute(name) { return attributes.get(name) ?? null; },
    removeAttribute(name) {
      attributes.delete(name);
      if (name === "open") this.open = false;
    },
    focus() { if (documentRef) documentRef.activeElement = this; },
    remove() { this.isConnected = false; },
    showModal() { this.open = true; },
    close() { this.open = false; this.dispatchEvent({type: "close", target: this}); },
    hidden: false,
    open: false,
    value: "",
    textContent: "",
    get childElementCount() { return children.length; }
  };
}

function loadCatalogContext() {
  const nodes = new Map();
  const created = [];
  const document = {
    activeElement: null,
    querySelector(selector) {
      if (!nodes.has(selector)) nodes.set(selector, makeNode("div", document));
      return nodes.get(selector);
    },
    querySelectorAll() { return []; },
    createElement(tagName) {
      const node = makeNode(tagName, document);
      created.push(node);
      return node;
    },
    addEventListener() {}
  };
  const context = {
    document,
    created,
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

test("catalog search announces the result count, not every refreshed card", () => {
  const html = fs.readFileSync(path.join(__dirname, "../web/catalog.html"), "utf8");
  assert.match(html, /id="results-label" aria-live="polite"/);
  assert.match(html, /id="catalog-grid" class="catalog-grid" aria-live="off" aria-busy="true"/);
});

test("dynamic game dialog exposes its title and summary to assistive technology", () => {
  const html = fs.readFileSync(path.join(__dirname, "../web/catalog.html"), "utf8");
  assert.match(html, /<dialog id="game-dialog" class="game-dialog" aria-labelledby="game-dialog-title" aria-describedby="game-dialog-summary">/);

  const context = loadCatalogContext();
  context.testGame = {
    gameId: "accessibility-fixture",
    title: "Accessible Game Sheet",
    platforms: ["psvita"],
    releases: []
  };
  vm.runInContext("state.catalog = {games: [testGame]}; openGame(testGame);", context);

  const title = context.created.find((node) => node.id === "game-dialog-title");
  const summary = context.created.find((node) => node.id === "game-dialog-summary");
  assert.ok(title, "dialog title heading is created");
  assert.equal(title.tagName, "H2");
  assert.equal(title.textContent, "Accessible Game Sheet");
  assert.ok(summary, "dialog description is created");
  assert.equal(summary.tagName, "P");
  assert.match(summary.textContent, /release details/i);
  assert.equal(context.document.querySelector("#game-dialog").open, true);
});


test("opening a recommendation replaces focus with its heading and restores the original card", () => {
  const context = loadCatalogContext();
  const opener = context.document.createElement("button");
  opener.focus();
  context.testGame = {
    gameId: "focus-origin",
    title: "First Game",
    platforms: ["psvita"],
    releases: []
  };
  context.nextGame = {
    gameId: "focus-recommendation",
    title: "Recommended Game",
    platforms: ["psp"],
    releases: []
  };

  vm.runInContext("state.catalog = {games: [testGame, nextGame]}; openGame(testGame);", context);
  const firstTitle = context.created.filter((node) => node.id === "game-dialog-title").at(-1);
  assert.equal(context.document.activeElement, firstTitle, "the active sheet focuses its heading");

  vm.runInContext("openGame(nextGame);", context);
  const nextTitle = context.created.filter((node) => node.id === "game-dialog-title").at(-1);
  assert.equal(context.document.activeElement, nextTitle, "replacing a sheet never leaves focus on a removed recommendation card");

  context.document.querySelector("#game-dialog").close();
  assert.equal(context.document.activeElement, opener, "closing returns focus to the original catalog trigger");
});

test("dialog focus falls back to catalog search if its original trigger was removed", () => {
  const context = loadCatalogContext();
  const opener = context.document.createElement("button");
  opener.focus();
  context.testGame = {
    gameId: "removed-origin",
    title: "Detached Trigger Game",
    platforms: ["psvita"],
    releases: []
  };

  vm.runInContext("state.catalog = {games: [testGame]}; openGame(testGame);", context);
  opener.remove();
  context.document.querySelector("#game-dialog").close();
  assert.equal(
    context.document.activeElement,
    context.document.querySelector("#catalog-search"),
    "focus returns to the catalog search when the original trigger no longer exists"
  );
});
