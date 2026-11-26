(function (root) {
  "use strict";

  const MAX_LISTS = 100;
  const MAX_ENTRIES = 500;
  const MAX_NAME = 80;
  const STORAGE_KEY = "vita-nexus.user-lists.v1";
  const ID_RE = /^[A-Za-z0-9][A-Za-z0-9._:-]{0,119}$/;

  function cleanName(value) {
    return typeof value === "string" ? value.trim().slice(0, MAX_NAME) : "";
  }

  function importedId(value, index, used) {
    const candidate = typeof value === "string" ? value.trim() : "";
    if (ID_RE.test(candidate) && !used.has(candidate)) return candidate;
    let fallback = "imported-" + index;
    let suffix = 1;
    while (used.has(fallback)) fallback = "imported-" + index + "-" + suffix++;
    return fallback;
  }

  function normalizeLists(value) {
    if (!Array.isArray(value)) return [];
    const usedIds = new Set();
    return value.slice(0, MAX_LISTS).map((item, index) => {
      if (!item || typeof item !== "object" || Array.isArray(item)) return null;
      const name = cleanName(item.name);
      if (!name) return null;
      const id = importedId(item.id, index, usedIds);
      usedIds.add(id);
      const games = Array.isArray(item.games)
        ? [...new Set(item.games.filter((gameId) => typeof gameId === "string" && gameId.length > 0 && gameId.length <= 160))].slice(0, MAX_ENTRIES)
        : [];
      return {
        id,
        name,
        description: typeof item.description === "string" ? item.description.slice(0, 280) : "",
        createdAt: typeof item.createdAt === "string" ? item.createdAt.slice(0, 40) : "",
        updatedAt: typeof item.updatedAt === "string" ? item.updatedAt.slice(0, 40) : "",
        games
      };
    }).filter(Boolean);
  }

  function createList(lists, name, id, timestamp) {
    const clean = cleanName(name);
    if (!clean) throw new Error("Give the list a name first.");
    if (lists.length >= MAX_LISTS) throw new Error("You have reached the 100-list limit.");
    const usedIds = new Set(lists.map((item) => item && item.id).filter((value) => typeof value === "string"));
    let nextId = id || "list-" + Date.now().toString(36);
    if (typeof nextId !== "string" || !ID_RE.test(nextId)) throw new Error("The list ID is invalid.");
    if (usedIds.has(nextId)) throw new Error("A list with this ID already exists.");
    const now = timestamp || new Date().toISOString();
    return [...lists, {id: nextId, name: clean, description: "", createdAt: now, updatedAt: now, games: []}];
  }

  function addGame(lists, listId, gameId, timestamp) {
    if (typeof gameId !== "string" || !gameId) throw new Error("This game has no catalog ID.");
    return lists.map((list) => list.id !== listId || list.games.includes(gameId)
      ? list
      : {...list, games: [...list.games, gameId].slice(0, MAX_ENTRIES), updatedAt: timestamp || new Date().toISOString()});
  }

  function removeGame(lists, listId, gameId, timestamp) {
    return lists.map((list) => list.id !== listId
      ? list
      : {...list, games: list.games.filter((id) => id !== gameId), updatedAt: timestamp || new Date().toISOString()});
  }

  function moveGame(lists, listId, gameId, offset, timestamp) {
    return lists.map((list) => {
      if (list.id !== listId) return list;
      const from = list.games.indexOf(gameId);
      const to = from + offset;
      if (from < 0 || to < 0 || to >= list.games.length) return list;
      const games = [...list.games];
      [games[from], games[to]] = [games[to], games[from]];
      return {...list, games, updatedAt: timestamp || new Date().toISOString()};
    });
  }

  function exportLists(lists) {
    return JSON.stringify({schemaVersion: "1.0.0", product: "VITA NEXUS", exportedAt: new Date().toISOString(), lists: normalizeLists(lists)}, null, 2);
  }

  function importLists(raw) {
    let parsed;
    try { parsed = typeof raw === "string" ? JSON.parse(raw) : raw; }
    catch (_) { throw new Error("That file is not valid JSON."); }
    if (!parsed || parsed.product !== "VITA NEXUS" || parsed.schemaVersion !== "1.0.0" || !Array.isArray(parsed.lists)) {
      throw new Error("This is not a supported VITA NEXUS list export.");
    }
    if (parsed.lists.length > MAX_LISTS) throw new Error("The import exceeds the 100-list limit.");
    const lists = normalizeLists(parsed.lists);
    if (!lists.length && parsed.lists.length) throw new Error("No valid lists were found in that file.");
    return lists;
  }

  root.VitaNexusLists = {STORAGE_KEY, normalizeLists, createList, addGame, removeGame, moveGame, exportLists, importLists};
})(globalThis);
