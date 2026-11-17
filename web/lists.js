(function (root) {
  "use strict";

  const MAX_LISTS = 100;
  const MAX_ENTRIES = 500;
  const MAX_NAME = 80;
  const STORAGE_KEY = "vita-nexus.user-lists.v1";

  function cleanName(value) {
    return typeof value === "string" ? value.trim().slice(0, MAX_NAME) : "";
  }

  function normalizeLists(value) {
    if (!Array.isArray(value)) return [];
    return value.slice(0, MAX_LISTS).map((item, index) => {
      if (!item || typeof item !== "object") return null;
      const name = cleanName(item.name);
      if (!name) return null;
      const games = Array.isArray(item.games)
        ? [...new Set(item.games.filter((id) => typeof id === "string" && id.length > 0 && id.length <= 160))].slice(0, MAX_ENTRIES)
        : [];
      return {
        id: typeof item.id === "string" && item.id ? item.id.slice(0, 120) : "imported-" + index,
        name,
        description: typeof item.description === "string" ? item.description.slice(0, 280) : "",
        createdAt: typeof item.createdAt === "string" ? item.createdAt : "",
        updatedAt: typeof item.updatedAt === "string" ? item.updatedAt : "",
        games
      };
    }).filter(Boolean);
  }

  function createList(lists, name, id, timestamp) {
    const clean = cleanName(name);
    if (!clean) throw new Error("Give the list a name first.");
    if (lists.length >= MAX_LISTS) throw new Error("You have reached the 100-list limit.");
    const now = timestamp || new Date().toISOString();
    return [...lists, {id: id || "list-" + Date.now().toString(36), name: clean, description: "", createdAt: now, updatedAt: now, games: []}];
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
