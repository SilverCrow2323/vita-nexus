const grid = document.querySelector("#catalog-grid");
const emptyPanel = document.querySelector("#catalog-empty");
const emptyTitle = document.querySelector("#empty-title");
const emptyMessage = document.querySelector("#empty-message");
const searchInput = document.querySelector("#catalog-search");
const sortSelect = document.querySelector("#catalog-sort");
const resultsLabel = document.querySelector("#results-label");
const loadMoreButton = document.querySelector("#load-more");
const dialog = document.querySelector("#game-dialog");
const dialogContent = document.querySelector("#dialog-content");

const PAGE_SIZE = 48;
const PLATFORM_LABELS = {
  psvita: "PS VITA",
  psp: "PSP",
  ps1: "PS1",
  homebrew: "HOMEBREW"
};
const REGION_ORDER = ["EU", "US", "JP", "AS", "INT", "KR", "CN", "UNKNOWN"];

const state = {
  catalog: null,
  overrides: {},
  query: "",
  platform: "all",
  sort: "title",
  visible: PAGE_SIZE,
  loadError: false
};

function element(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined && text !== null) node.textContent = String(text);
  return node;
}

function formatBytes(value) {
  if (!Number.isFinite(value) || value <= 0) return "SIZE UNKNOWN";
  const units = ["B", "KB", "MB", "GB"];
  let size = value;
  let unit = 0;
  while (size >= 1024 && unit < units.length - 1) {
    size /= 1024;
    unit += 1;
  }
  return size.toFixed(unit === 0 ? 0 : 1) + " " + units[unit];
}

function releaseDate(game) {
  const dates = (game.releases || [])
    .map((release) => Date.parse(release.sourceUpdatedAt || ""))
    .filter(Number.isFinite);
  return dates.length ? Math.max(...dates) : 0;
}

function gamePlatforms(game) {
  const fromReleases = (game.releases || []).map((release) => release.platform);
  return [...new Set([...(game.platforms || []), ...fromReleases])];
}

function gameSearchText(game) {
  const releaseText = (game.releases || [])
    .map((release) => [release.titleId, release.contentId, release.region].join(" "))
    .join(" ");
  return [game.title, game.sortTitle, game.summary, releaseText].join(" ").toLocaleLowerCase();
}

function filteredGames() {
  const games = (state.catalog && state.catalog.games) || [];
  const query = state.query.trim().toLocaleLowerCase();
  const matches = games.filter((game) => {
    const platformMatch = state.platform === "all" || gamePlatforms(game).includes(state.platform);
    const queryMatch = !query || gameSearchText(game).includes(query);
    return platformMatch && queryMatch;
  });

  matches.sort((a, b) => {
    if (state.sort === "size") {
      const aSize = Math.max(0, ...(a.releases || []).map((release) => release.packageSizeBytes || 0));
      const bSize = Math.max(0, ...(b.releases || []).map((release) => release.packageSizeBytes || 0));
      return bSize - aSize || a.title.localeCompare(b.title);
    }
    if (state.sort === "updated") return releaseDate(b) - releaseDate(a) || a.title.localeCompare(b.title);
    return (a.sortTitle || a.title).localeCompare(b.sortTitle || b.title, undefined, {sensitivity: "base"});
  });
  return matches;
}

function makePlatformBadge(platform) {
  return element("span", "platform-badge badge-" + platform, PLATFORM_LABELS[platform] || platform.toUpperCase());
}

function createGameCard(game) {
  const card = element("article", "game-card");
  const platforms = gamePlatforms(game);
  const releases = game.releases || [];
  const uniqueRegions = [...new Set(releases.map((release) => release.region))].sort(
    (a, b) => REGION_ORDER.indexOf(a) - REGION_ORDER.indexOf(b)
  );
  const artwork = game.artwork || state.overrides[game.gameId]?.artwork || {};
  const cover = element("div", "game-cover");
  const imageUrl = artwork.cover && artwork.cover.url;

  if (imageUrl) {
    const image = element("img", "cover-image");
    image.src = imageUrl;
    image.alt = artwork.cover.alt || (game.title + " cover artwork");
    image.loading = "lazy";
    image.decoding = "async";
    image.addEventListener("error", () => {
      image.remove();
      cover.classList.add("cover-fallback");
      cover.append(element("span", "cover-initial", game.title.slice(0, 1).toUpperCase()));
    }, {once: true});
    cover.append(image);
  } else {
    cover.classList.add("cover-fallback");
    cover.append(element("span", "cover-initial", game.title.slice(0, 1).toUpperCase()));
    cover.append(element("span", "cover-caption", "ARTWORK NOT ADDED"));
  }

  const info = element("div", "game-card-info");
  const meta = element("div", "game-card-meta");
  platforms.forEach((platform) => meta.append(makePlatformBadge(platform)));
  info.append(meta);
  info.append(element("h3", "game-card-title", game.title));

  const summary = game.summary || state.overrides[game.gameId]?.summary;
  info.append(element("p", "game-card-summary", summary || "Editorial description not added yet. Release metadata is available below."));

  const footer = element("div", "game-card-footer");
  footer.append(element("span", "", releases.length + (releases.length === 1 ? " EDITION" : " EDITIONS")));
  footer.append(element("span", "", uniqueRegions.length + (uniqueRegions.length === 1 ? " REGION" : " REGIONS")));
  info.append(footer);

  const button = element("button", "game-card-action", "Explore game ↗");
  button.type = "button";
  button.addEventListener("click", () => openGame(game));
  info.append(button);

  card.append(cover, info);
  return card;
}

function render() {
  const games = filteredGames();
  const visible = games.slice(0, state.visible);
  grid.replaceChildren(...visible.map(createGameCard));
  grid.setAttribute("aria-busy", "false");

  const total = (state.catalog && state.catalog.totals && state.catalog.totals.games) || 0;
  const releaseTotal = (state.catalog && state.catalog.totals && state.catalog.totals.releases) || 0;
  document.querySelector("#games-count").textContent = total.toLocaleString();
  document.querySelector("#releases-count").textContent = releaseTotal.toLocaleString();
  const generatedAt = state.catalog && state.catalog.generatedAt;
  document.querySelector("#updated-at").textContent = generatedAt
    ? new Date(generatedAt).toLocaleDateString(undefined, {year: "numeric", month: "short", day: "numeric"})
    : "NOT SYNCED";

  resultsLabel.textContent = state.loadError
    ? "Catalog data could not be loaded."
    : games.length.toLocaleString() + " game" + (games.length === 1 ? "" : "s") + " found";
  document.querySelector("#source-label").textContent = "SOURCE: " + (state.catalog?.sources?.length || 0) + " INDEX FEEDS";

  const showEmpty = games.length === 0;
  emptyPanel.hidden = !showEmpty;
  grid.hidden = showEmpty;
  loadMoreButton.hidden = games.length <= state.visible;
  if (showEmpty) {
    if (state.loadError) {
      emptyTitle.textContent = "Catalog unavailable.";
      emptyMessage.textContent = "The catalog snapshot could not be loaded. Try again later or inspect the catalog sync workflow.";
    } else if (total === 0) {
      emptyTitle.textContent = "The first catalog sync is pending.";
      emptyMessage.textContent = "The browser is ready, but the first NoPayStation metadata import has not run yet. Start the catalog sync workflow on GitHub to populate the public index.";
    } else {
      emptyTitle.textContent = "No games found.";
      emptyMessage.textContent = "Try another title, identifier, region or platform filter.";
    }
  }
}

function addReleaseRow(release) {
  const row = element("div", "release-row");
  const main = element("div", "release-main");
  main.append(element("strong", "", release.region + " edition"));
  main.append(element("span", "", release.titleId));
  if (release.contentId) main.append(element("small", "", release.contentId));
  const details = element("div", "release-details");
  details.append(element("span", "", formatBytes(release.packageSizeBytes)));
  if (release.requiredFirmware) details.append(element("span", "", "FW " + release.requiredFirmware));
  row.append(main, details);
  return row;
}

function openGame(game) {
  const override = state.overrides[game.gameId] || {};
  const artwork = game.artwork || override.artwork || {};
  dialogContent.replaceChildren();

  const hero = element("div", "dialog-hero");
  if (artwork.hero && artwork.hero.url) {
    const heroImage = element("img", "dialog-hero-image");
    heroImage.src = artwork.hero.url;
    heroImage.alt = artwork.hero.alt || (game.title + " artwork");
    hero.append(heroImage);
  } else {
    hero.classList.add("dialog-hero-fallback");
    hero.append(element("span", "", game.title.slice(0, 1).toUpperCase()));
  }
  dialogContent.append(hero);

  const body = element("div", "dialog-body");
  const platformLine = element("div", "dialog-platforms");
  gamePlatforms(game).forEach((platform) => platformLine.append(makePlatformBadge(platform)));
  body.append(platformLine);
  body.append(element("h2", "dialog-title", game.title));
  body.append(element("p", "dialog-summary", game.description || override.description || game.summary || override.summary || "No editorial description has been added yet."));

  const facts = element("div", "dialog-facts");
  if (game.developer || override.developer) facts.append(element("span", "", "DEVELOPER · " + (game.developer || override.developer)));
  if (game.publisher || override.publisher) facts.append(element("span", "", "PUBLISHER · " + (game.publisher || override.publisher)));
  if (game.releaseYear || override.releaseYear) facts.append(element("span", "", "YEAR · " + (game.releaseYear || override.releaseYear)));
  body.append(facts);

  const releasesHeading = element("div", "release-heading");
  releasesHeading.append(element("h3", "", "Available editions"));
  releasesHeading.append(element("span", "", (game.releases || []).length + " RELEASES"));
  body.append(releasesHeading);

  const releases = [...(game.releases || [])].sort((a, b) => {
    const platformOrder = ["psvita", "psp", "ps1", "homebrew"];
    return platformOrder.indexOf(a.platform) - platformOrder.indexOf(b.platform)
      || REGION_ORDER.indexOf(a.region) - REGION_ORDER.indexOf(b.region);
  });
  const releaseList = element("div", "release-list");
  releases.forEach((release) => {
    const section = element("section", "release-section");
    const label = element("div", "release-platform");
    label.append(makePlatformBadge(release.platform));
    section.append(label, addReleaseRow(release));
    releaseList.append(section);
  });
  body.append(releaseList);

  const note = element("p", "dialog-source-note", "Release identifiers and descriptive fields are sourced from public metadata indexes. Package URLs and license material are not part of VITA NEXUS.");
  body.append(note);
  dialogContent.append(body);
  if (typeof dialog.showModal === "function") dialog.showModal();
  else dialog.setAttribute("open", "");
}

async function loadCatalog() {
  grid.setAttribute("aria-busy", "true");
  try {
    const [catalogResponse, overrideResponse] = await Promise.all([
      fetch("data/catalog.json", {cache: "no-store"}),
      fetch("data/editorial-overrides.json", {cache: "no-store"})
    ]);
    if (!catalogResponse.ok) throw new Error("Catalog request failed: " + catalogResponse.status);
    state.catalog = await catalogResponse.json();
    if (overrideResponse.ok) {
      const overrideData = await overrideResponse.json();
      state.overrides = overrideData.games || {};
    }
  } catch (error) {
    console.error(error);
    state.loadError = true;
  }
  render();
}

searchInput.addEventListener("input", () => {
  state.query = searchInput.value;
  state.visible = PAGE_SIZE;
  render();
});

sortSelect.addEventListener("change", () => {
  state.sort = sortSelect.value;
  state.visible = PAGE_SIZE;
  render();
});

document.querySelectorAll("[data-platform]").forEach((button) => {
  button.addEventListener("click", () => {
    state.platform = button.dataset.platform;
    state.visible = PAGE_SIZE;
    document.querySelectorAll("[data-platform]").forEach((item) => {
      const active = item === button;
      item.classList.toggle("active", active);
      item.setAttribute("aria-pressed", String(active));
    });
    render();
  });
});

loadMoreButton.addEventListener("click", () => {
  state.visible += PAGE_SIZE;
  render();
});

document.addEventListener("keydown", (event) => {
  if ((event.metaKey || event.ctrlKey) && event.key.toLowerCase() === "k") {
    event.preventDefault();
    searchInput.focus();
  }
  if (event.key === "Escape" && dialog.open) dialog.close();
});

const menuButton = document.querySelector(".menu-toggle");
const navigation = document.querySelector(".main-nav");
if (menuButton && navigation) {
  menuButton.addEventListener("click", () => {
    const isOpen = navigation.classList.toggle("open");
    menuButton.setAttribute("aria-expanded", String(isOpen));
    menuButton.setAttribute("aria-label", isOpen ? "Close navigation" : "Open navigation");
    menuButton.textContent = isOpen ? "×" : "☰";
  });
}

loadCatalog();
