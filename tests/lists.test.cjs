const assert = require("node:assert/strict");
require("../web/lists.js");
const Lists = globalThis.VitaNexusLists;
const test = require("node:test");

test("create list trims names and enforces a non-empty title", () => {
  const created = Lists.createList([], "  Vita essentials  ", "essentials", "2026-11-17T00:00:00.000Z");
  assert.equal(created[0].name, "Vita essentials");
  assert.equal(created[0].id, "essentials");
  assert.throws(() => Lists.createList([], "   "), /name/i);
});

test("adding games is idempotent and order can be changed or removed", () => {
  let lists = Lists.createList([], "Backlog", "backlog", "2026-11-17T00:00:00.000Z");
  lists = Lists.addGame(lists, "backlog", "game-a", "2026-11-17T00:01:00.000Z");
  lists = Lists.addGame(lists, "backlog", "game-b", "2026-11-17T00:02:00.000Z");
  lists = Lists.addGame(lists, "backlog", "game-a", "2026-11-17T00:03:00.000Z");
  assert.deepEqual(lists[0].games, ["game-a", "game-b"]);
  lists = Lists.moveGame(lists, "backlog", "game-b", -1, "2026-11-17T00:04:00.000Z");
  assert.deepEqual(lists[0].games, ["game-b", "game-a"]);
  lists = Lists.removeGame(lists, "backlog", "game-b", "2026-11-17T00:05:00.000Z");
  assert.deepEqual(lists[0].games, ["game-a"]);
});

test("export/import round-trip keeps list names and ordered game IDs", () => {
  const source = Lists.createList([], "PS1 deep cuts", "ps1-cuts", "2026-11-17T00:00:00.000Z");
  const withGames = Lists.addGame(Lists.addGame(source, "ps1-cuts", "SLUS-00001"), "ps1-cuts", "SLES-00002");
  const restored = Lists.importLists(Lists.exportLists(withGames));
  assert.equal(restored.length, 1);
  assert.equal(restored[0].name, "PS1 deep cuts");
  assert.deepEqual(restored[0].games, ["SLUS-00001", "SLES-00002"]);
});

test("import rejects unsupported payloads and malformed JSON", () => {
  assert.throws(() => Lists.importLists("{nope"), /valid JSON/i);
  assert.throws(() => Lists.importLists({product: "OTHER", schemaVersion: "1.0.0", lists: []}), /supported/i);
});

test("normalization drops malformed entries and duplicate game IDs", () => {
  const lists = Lists.normalizeLists([
    null,
    {id: "one", name: "Valid", games: ["A", "A", "", 4, "B"]},
    {id: "two", name: "  ", games: ["C"]}
  ]);
  assert.equal(lists.length, 1);
  assert.deepEqual(lists[0].games, ["A", "B"]);
});

test("import normalizes duplicate and unsafe list IDs without merging lists", () => {
  const restored = Lists.importLists({
    product: "VITA NEXUS",
    schemaVersion: "1.0.0",
    lists: [
      {id: "same-id", name: "First", games: ["game-a"]},
      {id: "same-id", name: "Second", games: ["game-b"]},
      {id: "../unsafe", name: "Third", games: ["game-c"]}
    ]
  });
  assert.equal(new Set(restored.map((list) => list.id)).size, 3);
  assert.deepEqual(restored.map((list) => list.games), [["game-a"], ["game-b"], ["game-c"]]);
  assert.equal(restored[1].id, "imported-1");
  assert.equal(restored[2].id, "imported-2");
});

test("creating a list rejects duplicate or malformed IDs", () => {
  const initial = Lists.createList([], "First", "stable-id", "2026-11-17T00:00:00.000Z");
  assert.throws(() => Lists.createList(initial, "Duplicate", "stable-id"), /already exists/i);
  assert.throws(() => Lists.createList([], "Malformed", "../escape"), /invalid/i);
});
