const assert = require("node:assert/strict");
const fs = require("node:fs");
const path = require("node:path");
const test = require("node:test");
const vm = require("node:vm");

function makeNode(tagName = "div") {
  const attributes = new Map();
  const children = [];
  return {
    tagName: tagName.toUpperCase(),
    id: "",
    className: "",
    dataset: {},
    children,
    classList: {add() {}, remove() {}, toggle() {}},
    addEventListener() {},
    append(...items) { children.push(...items); },
    appendChild(item) { children.push(item); },
    replaceChildren(...items) { children.splice(0, children.length, ...items); },
    setAttribute(name, value) { attributes.set(name, String(value)); },
    getAttribute(name) { return attributes.get(name) ?? null; },
    removeAttribute(name) { attributes.delete(name); },
    focus() {},
    remove() {},
    showModal() { this.open = true; },
    close() { this.open = false; },
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
    querySelector(selector) {
      if (!nodes.has(selector)) nodes.set(selector, makeNode());
      return nodes.get(selector);
    },
    querySelectorAll() { return []; },
    createElement(tagName) {
      const node = makeNode(tagName);
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
