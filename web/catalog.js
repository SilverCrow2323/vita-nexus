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
  loadError: false,
  userLists: [],
  activeListId: ""
};

function element(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined && text !== null) node.textContent = String(text);
  return node;
}

function formatBytes(value) {
  if (!Number.isFinite(value) || value <= 0) return "SIZE NOT LISTED";
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
  const homebrewText = game.homebrew ? [game.homebrew.author, game.homebrew.version, game.homebrew.categoryCode].join(" ") : "";
  const override = state.overrides[game.gameId] || {};
  const editorialText = [...(game.genres || []), ...(game.themes || []), ...(game.features || [])].join(" ");
  const artworkText = Object.values(override.artwork || {}).flat()
    .filter((asset) => asset && typeof asset === "object")
    .map((asset) => [asset.alt, asset.caption, asset.source, asset.credit, asset.license].join(" ")).join(" ");
  const ageText = firstNonEmptyArray(game.ageRatings, override.ageRatings)
    .map((rating) => [rating.system, rating.value, rating.territory, ...(rating.descriptors || [])].join(" ")).join(" ");
  const scoreText = firstNonEmptyArray(game.externalScores, override.externalScores)
    .map((score) => [score.provider, score.value, score.scale].join(" ")).join(" ");
  const editorial = override.editorial || {};
  const review = editorial.review || {};
  const reviewText = [editorial.rubric, editorial.standfirst, review.headline, review.body, review.verdict,
    ...(editorial.highlights || []), ...(editorial.caveats || []), ...(editorial.curiosities || [])].filter(Boolean).join(" ");
  return [game.title, game.sortTitle, game.summary, game.developer, homebrewText, editorialText, ageText, scoreText, reviewText, artworkText, releaseText].join(" ").toLocaleLowerCase();
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
  const override = state.overrides[game.gameId] || {};
  const artwork = mergeGameArtwork(game, override);
  const coverAsset = isDisplayableArtworkAsset(artwork.cover) ? artwork.cover : null;
  const cover = element("div", "game-cover");
  const homebrewIcon = game.homebrew && isSafeImageUrl(game.homebrew.iconUrl) ? game.homebrew.iconUrl : "";
  const imageUrl = (coverAsset && coverAsset.url) || homebrewIcon;

  if (imageUrl) {
    const image = element("img", homebrewIcon && !coverAsset ? "cover-image cover-image-icon" : "cover-image");
    image.src = imageUrl;
    image.alt = (coverAsset && coverAsset.alt) || (game.title + (homebrewIcon ? " homebrew icon" : " cover artwork"));
    image.loading = "lazy";
    image.decoding = "async";
    image.addEventListener("error", () => {
      image.remove();
      cover.classList.add("cover-fallback");
      cover.append(element("span", "cover-initial", game.title.slice(0, 1).toUpperCase()));
    }, {once: true});
    cover.append(image);
    if (isDisplayableArtworkAsset(artwork.logo)) {
      const logo = element("img", "game-cover-logo");
      logo.src = artwork.logo.url;
      logo.alt = artwork.logo.alt;
      logo.loading = "lazy";
      logo.decoding = "async";
      cover.append(logo);
    }
  } else {
    cover.classList.add("cover-fallback");
    cover.append(element("span", "cover-initial", game.title.slice(0, 1).toUpperCase()));
    cover.append(element("span", "cover-caption", "ARTWORK NOT ADDED"));
  }

  const info = element("div", "game-card-info");
  const meta = element("div", "game-card-meta");
  platforms.forEach((platform) => meta.append(makePlatformBadge(platform)));
  if (game.homebrew) meta.append(element("span", "target-badge", "FOR " + (game.releases?.[0]?.targetPlatform === "psp" ? "PSP" : "PS VITA")));
  info.append(meta);
  info.append(element("h3", "game-card-title", game.title));
  const indicators = createGameIndicators(game, state.overrides[game.gameId] || {});
  if (indicators.childElementCount) info.append(indicators);

  const summary = game.summary || state.overrides[game.gameId]?.summary;
  info.append(element("p", "game-card-summary", summary || "No blurb yet. Check the editions and IDs below."));

  const footer = element("div", "game-card-footer");
  footer.append(element("span", "", releases.length + (releases.length === 1 ? " EDITION" : " EDITIONS")));
  footer.append(element("span", "", uniqueRegions.length + (uniqueRegions.length === 1 ? " REGION" : " REGIONS")));
  info.append(footer);

  const button = element("button", "game-card-action", "Open game ↗");
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
    ? "Catalog failed to load."
    : games.length.toLocaleString() + " game" + (games.length === 1 ? "" : "s") + " found";
  document.querySelector("#source-label").textContent = "SOURCE: " + (state.catalog?.sources?.length || 0) + " INDEX FEEDS";

  const showEmpty = games.length === 0;
  emptyPanel.hidden = !showEmpty;
  grid.hidden = showEmpty;
  loadMoreButton.hidden = games.length <= state.visible;
  if (showEmpty) {
    if (state.loadError) {
      emptyTitle.textContent = "Catalog unavailable.";
      emptyMessage.textContent = "Couldn’t load the catalog. Try again or check the sync workflow.";
    } else if (total === 0) {
      emptyTitle.textContent = "The first sync hasn’t run yet.";
      emptyMessage.textContent = "The page is ready, but the first NoPayStation import hasn’t run. Run the catalog sync on GitHub to fill the index.";
    } else {
      emptyTitle.textContent = "Nothing found. Try another search.";
      emptyMessage.textContent = "Change the title, ID, region or platform filter.";
    }
  }
}

function addReleaseRow(release) {
  const row = element("div", "release-row");
  const main = element("div", "release-main");
  const targetLabel = release.targetPlatform === "psp" ? "PSP" : "PS VITA";
  const editionLabel = release.platform === "homebrew"
    ? "Homebrew for " + targetLabel
    : ({EU: "Europe", US: "North America", JP: "Japan", AS: "Asia", INT: "International", KR: "Korea", CN: "China"}[release.region] || release.region) + " edition";
  main.append(element("strong", "", editionLabel));
  if (release.titleId) main.append(element("span", "", release.titleId));
  if (release.sourceItemId) main.append(element("small", "", "SOURCE ID · " + release.sourceItemId));
  if (release.contentId) main.append(element("small", "", release.contentId));
  const details = element("div", "release-details");
  if (release.packageSizeBytes) details.append(element("span", "", formatBytes(release.packageSizeBytes)));
  if (release.requiredFirmware) details.append(element("span", "", "FW " + release.requiredFirmware));
  row.append(main, details);
  return row;
}

function firstNonEmptyArray(primary, fallback) {
  if (Array.isArray(primary) && primary.length) return primary;
  if (Array.isArray(fallback) && fallback.length) return fallback;
  return Array.isArray(primary) ? primary : Array.isArray(fallback) ? fallback : [];
}

function preferredAgeRating(game, override = {}) {
  const ratings = firstNonEmptyArray(game.ageRatings, override.ageRatings);
  return ratings.find((rating) => rating && rating.system === "PEGI")
    || ratings.find(Boolean)
    || null;
}

function preferredExternalScore(game, override = {}) {
  const scores = firstNonEmptyArray(game.externalScores, override.externalScores).filter(Boolean);
  if (!scores.length) return null;
  const priority = ["opencritic", "metacritic"];
  scores.sort((a, b) => {
    const indexA = priority.indexOf(String(a.provider || "").toLocaleLowerCase());
    const indexB = priority.indexOf(String(b.provider || "").toLocaleLowerCase());
    return (indexA < 0 ? priority.length : indexA) - (indexB < 0 ? priority.length : indexB);
  });
  return scores[0];
}

function formatScoreValue(score, requireScale = false) {
  if (!score || score.value === undefined || score.value === null) return "";
  const value = Number(score.value);
  const scale = Number(score.scale);
  if (!Number.isFinite(value) || (requireScale && (!Number.isFinite(scale) || scale <= 0))) return "";
  const format = (number) => Number.isInteger(number) ? String(number) : number.toFixed(1);
  return Number.isFinite(scale) && scale > 0 ? format(value) + "/" + format(scale) : format(value);
}

const APPROVED_ARTWORK_RIGHTS = new Set(["verified-license", "permission-granted", "public-domain"]);

function isSafeImageUrl(value) {
  return typeof value === "string" && value.startsWith("https://");
}

function isDisplayableArtworkAsset(asset) {
  if (!asset || !isSafeImageUrl(asset.url) || typeof asset.alt !== "string" || !asset.alt.trim()) return false;
  return !asset.rightsStatus || APPROVED_ARTWORK_RIGHTS.has(asset.rightsStatus);
}

function mergeGameArtwork(game, override = {}) {
  const upstream = game.artwork || {};
  const curated = override.artwork || {};
  const merged = {...upstream, ...curated};
  ["cover", "hero", "logo", "icon"].forEach((role) => {
    if (!curated[role]) merged[role] = upstream[role];
  });
  const screenshots = [...(curated.screenshots || []), ...(upstream.screenshots || [])];
  const seen = new Set();
  merged.screenshots = screenshots.filter((asset) => {
    if (!asset || !asset.url || seen.has(asset.url)) return false;
    seen.add(asset.url);
    return true;
  });
  return merged;
}

function createGameIndicators(game, override = {}) {
  const indicators = element("div", "game-card-indicators");
  const ageRating = preferredAgeRating(game, override);
  if (ageRating) {
    const badge = element("span", "game-card-indicator age", ageRating.system + " " + ageRating.value);
    badge.setAttribute("aria-label", ageRating.system + " age rating: " + ageRating.value
      + (ageRating.territory ? " · " + ageRating.territory : ""));
    indicators.append(badge);
  }

  const externalScore = preferredExternalScore(game, override);
  const editorialScore = override.editorial && override.editorial.review && override.editorial.review.score;
  const chosenScore = externalScore || editorialScore;
  const formattedScore = formatScoreValue(chosenScore, Boolean(editorialScore && !externalScore));
  if (formattedScore) {
    const label = externalScore ? String(externalScore.provider || "REVIEW").toUpperCase() : "NEXUS";
    const badge = element("span", "game-card-indicator score", label + " " + formattedScore);
    badge.setAttribute("aria-label", (externalScore ? "External score from " + externalScore.provider : "VITA NEXUS editorial score")
      + ": " + formattedScore);
    indicators.append(badge);
  }
  return indicators;
}

function normalizedTags(values) {
  return new Set((values || []).map((value) => String(value).trim().toLocaleLowerCase()).filter(Boolean));
}

function recommendationPlatforms(game) {
  const releases = game.releases || [];
  const platforms = new Set(gamePlatforms(game));
  releases.forEach((release) => {
    if (release.targetPlatform) platforms.add(release.targetPlatform);
  });
  return platforms;
}

function getRecommendations(game, override) {
  const allGames = (state.catalog && state.catalog.games) || [];
  const relationRecords = firstNonEmptyArray(game.relations, override.relations);
  const byId = new Map(allGames.map((item) => [item.gameId, item]));
  const nativePlatforms = new Set(["psvita", "psp", "ps1"]);
  const sourcePlatforms = new Set([...recommendationPlatforms(game)].filter((platform) => nativePlatforms.has(platform)));
  const isCrossPlatform = (candidate) => [...recommendationPlatforms(candidate)].some(
    (platform) => nativePlatforms.has(platform) && !sourcePlatforms.has(platform)
  );
  const selected = [];
  const selectedIds = new Set([game.gameId]);

  relationRecords
    .slice()
    .sort((a, b) => {
      const gameA = byId.get(a.gameId);
      const gameB = byId.get(b.gameId);
      const crossBonusA = gameA && isCrossPlatform(gameA) ? 0.08 : 0;
      const crossBonusB = gameB && isCrossPlatform(gameB) ? 0.08 : 0;
      return ((b.confidence || 0) + crossBonusB) - ((a.confidence || 0) + crossBonusA);
    })
    .forEach((relation) => {
      const candidate = byId.get(relation.gameId);
      if (!candidate || selectedIds.has(candidate.gameId)) return;
      selectedIds.add(candidate.gameId);
      selected.push({
        game: candidate,
        reason: relation.reason || "Curated by the VITA NEXUS editorial catalog.",
        label: relation.relationType === "same-vibe" ? "SAME VIBE"
          : relation.relationType === "same-series" ? "SAME SERIES"
          : relation.relationType === "spiritual-successor" ? "SPIRITUAL SUCCESSOR"
          : "SIMILAR EXPERIENCE",
        confidence: relation.confidence || 0
      });
    });

  const sourceGenres = normalizedTags(firstNonEmptyArray(game.genres, override.genres));
  const sourceThemes = normalizedTags(firstNonEmptyArray(game.themes, override.themes));
  const sourceFeatures = normalizedTags(firstNonEmptyArray(game.features, override.features));
  const ranked = [];

  if (sourceGenres.size || sourceThemes.size || sourceFeatures.size) {
    allGames.forEach((candidate) => {
      if (selectedIds.has(candidate.gameId)) return;
      const candidateOverride = state.overrides[candidate.gameId] || {};
      const genres = normalizedTags(firstNonEmptyArray(candidate.genres, candidateOverride.genres));
      const themes = normalizedTags(firstNonEmptyArray(candidate.themes, candidateOverride.themes));
      const features = normalizedTags(firstNonEmptyArray(candidate.features, candidateOverride.features));
      const sharedGenres = [...genres].filter((tag) => sourceGenres.has(tag));
      const sharedThemes = [...themes].filter((tag) => sourceThemes.has(tag));
      const sharedFeatures = [...features].filter((tag) => sourceFeatures.has(tag));
      const score = sharedGenres.length * 4 + sharedThemes.length * 2 + sharedFeatures.length * 3;
      if (!score) return;
      const reasons = [];
      if (sharedGenres.length) reasons.push("Shared genres: " + sharedGenres.slice(0, 2).join(", "));
      if (sharedThemes.length) reasons.push("Similar themes: " + sharedThemes.slice(0, 2).join(", "));
      if (sharedFeatures.length) reasons.push("Shared features: " + sharedFeatures.slice(0, 2).join(", "));
      const crossPlatform = isCrossPlatform(candidate);
      ranked.push({
        game: candidate,
        reason: reasons.join(" · "),
        score: score + (crossPlatform ? 2 : 0),
        crossPlatform
      });
    });

    ranked.sort((a, b) => b.score - a.score || a.game.title.localeCompare(b.game.title));
    const platformCounts = new Map();
    ranked.forEach((item) => {
      if (selected.length >= 6) return;
      if (selectedIds.has(item.game.gameId)) return;
      const primaryPlatform = [...recommendationPlatforms(item.game)][0] || "unknown";
      const count = platformCounts.get(primaryPlatform) || 0;
      if (count >= 2) return;
      platformCounts.set(primaryPlatform, count + 1);
      selectedIds.add(item.game.gameId);
      selected.push({
        game: item.game,
        reason: item.reason,
        label: "METADATA MATCH",
        confidence: Math.min(1, item.score / 12)
      });
    });
  }

  return selected.slice(0, 6);
}

function createRecommendationCard(item) {
  const game = item.game;
  const card = element("button", "recommendation-card");
  card.type = "button";
  const artwork = mergeGameArtwork(game, state.overrides[game.gameId] || {});
  const coverAsset = isDisplayableArtworkAsset(artwork.cover) ? artwork.cover : null;
  const homebrewIcon = game.homebrew && isSafeImageUrl(game.homebrew.iconUrl) ? game.homebrew.iconUrl : "";
  const imageUrl = (coverAsset && coverAsset.url) || homebrewIcon;
  const visual = element("div", "recommendation-visual");
  if (imageUrl) {
    const image = element("img", game.homebrew && !(artwork.cover && artwork.cover.url) ? "recommendation-icon" : "");
    image.src = imageUrl;
    image.alt = (coverAsset && coverAsset.alt) || (game.title + " artwork");
    image.loading = "lazy";
    visual.append(image);
  } else {
    visual.append(element("span", "", game.title.slice(0, 1).toUpperCase()));
  }

  const content = element("div", "recommendation-copy");
  content.append(element("span", "recommendation-type", item.label));
  content.append(element("strong", "", game.title));
  const badges = element("div", "recommendation-platforms");
  gamePlatforms(game).forEach((platform) => badges.append(makePlatformBadge(platform)));
  content.append(badges);
  content.append(element("p", "", item.reason));
  card.append(visual, content);
  card.addEventListener("click", () => openGame(game));
  return card;
}

function renderEditorialFacts(game, override, body) {
  const tags = [
    ...firstNonEmptyArray(game.genres, override.genres).map((value) => ({kind:"GENRE",value})),
    ...firstNonEmptyArray(game.themes, override.themes).map((value) => ({kind:"THEME",value})),
    ...firstNonEmptyArray(game.features, override.features).map((value) => ({kind:"FEATURE",value}))
  ];
  if (tags.length) {
    const section = element("section", "editorial-facts");
    section.append(element("h3", "", "What’s in the game"));
    const list = element("div", "editorial-tags");
    tags.slice(0, 24).forEach((tag) => {
      list.append(element("span", "editorial-tag", tag.value));
    });
    section.append(list);
    body.append(section);
  }

  const ageRatings = firstNonEmptyArray(game.ageRatings, override.ageRatings);
  if (ageRatings.length) {
    const section = element("section", "age-rating-section");
    section.append(element("h3", "", "Age rating"));
    ageRatings.forEach((rating) => {
      const row = element("div", "age-rating-row");
      row.append(element("strong", "", rating.system + " · " + rating.value));
      row.append(element("span", "", rating.territory || "Territory not specified"));
      if (rating.descriptors && rating.descriptors.length) {
        row.append(element("small", "", rating.descriptors.join(" · ")));
      }
      section.append(row);
    });
    body.append(section);
  }

  const externalScores = firstNonEmptyArray(game.externalScores, override.externalScores);
  if (externalScores.length) {
    const section = element("section", "external-scores-section");
    section.append(element("h3", "", "Review scores"));
    const list = element("div", "external-scores-list");
    externalScores.forEach((score) => {
      const row = element("div", "external-score-row");
      row.append(element("strong", "", score.provider));
      row.append(element("span", "external-score-value", String(score.value) + (score.scale ? " / " + score.scale : "")));
      if (score.voteCount !== undefined && score.voteCount !== null) {
        row.append(element("small", "", Number(score.voteCount).toLocaleString() + " votes"));
      }
      list.append(row);
    });
    section.append(list);
    body.append(section);
  }
}

function addEditorialList(parent, heading, values, className) {
  if (!Array.isArray(values) || !values.length) return;
  const panel = element("section", className);
  panel.append(element("h4", "", heading));
  const list = element("ul", "");
  values.forEach((value) => list.append(element("li", "", value)));
  panel.append(list);
  parent.append(panel);
}

function renderEditorialReview(game, override, body) {
  const editorial = override.editorial || {};
  const review = editorial.review || {};
  const credit = editorial.credit || {};
  const highlights = editorial.highlights || [];
  const caveats = editorial.caveats || [];
  const curiosities = editorial.curiosities || [];
  const hasReview = Boolean(review.headline || review.body || review.verdict || review.score);
  if (!hasReview && !editorial.standfirst && !highlights.length && !caveats.length && !curiosities.length) return;

  const section = element("section", "editorial-review");
  const masthead = element("div", "editorial-review-masthead");
  masthead.append(element("span", "editorial-review-rubric", editorial.rubric || "NEXUS FIELD REPORT"));
  masthead.append(element("span", "editorial-review-folio", "GAME FILE / " + game.gameId.slice(0, 12).toUpperCase()));
  section.append(masthead);

  if (editorial.standfirst) section.append(element("p", "editorial-review-standfirst", editorial.standfirst));

  if (hasReview) {
    const main = element("div", "editorial-review-main");
    const copy = element("div", "editorial-review-copy");
    if (review.headline) copy.append(element("h3", "", review.headline));
    if (review.body) copy.append(element("p", "", review.body));
    main.append(copy);

    if (review.score) {
      const formatted = formatScoreValue(review.score, true);
      if (formatted) {
        const stamp = element("aside", "editorial-score-stamp");
        stamp.append(element("span", "", "NEXUS SCORE"));
        stamp.append(element("strong", "", String(review.score.value)));
        stamp.append(element("small", "", "/ " + String(review.score.scale)));
        if (review.score.methodology) stamp.append(element("p", "", review.score.methodology));
        main.append(stamp);
      }
    }
    section.append(main);
    if (review.verdict) {
      const verdict = element("div", "editorial-verdict");
      verdict.append(element("span", "", "THE VERDICT"));
      verdict.append(element("p", "", review.verdict));
      section.append(verdict);
    }
  }

  if (highlights.length || caveats.length) {
    const notes = element("div", "editorial-review-notes");
    addEditorialList(notes, "THE GOOD STUFF", highlights, "editorial-review-list strengths");
    addEditorialList(notes, "ROUGH EDGES", caveats, "editorial-review-list caveats");
    section.append(notes);
  }

  addEditorialList(section, "BACK-PAGE CURIOSITIES", curiosities, "editorial-curiosities");

  if (credit.byline || credit.reviewedAt || (credit.sources && credit.sources.length)) {
    const footer = element("footer", "editorial-review-credit");
    const signature = element("span", "", credit.byline ? "BY " + credit.byline : "VITA NEXUS EDITORIAL");
    footer.append(signature);
    if (credit.reviewedAt) footer.append(element("time", "", "REVIEWED " + credit.reviewedAt.slice(0, 10)));
    if (credit.sources && credit.sources.length) {
      const sourceLinks = element("div", "editorial-source-links");
      credit.sources.forEach((source) => {
        if (!source || typeof source.url !== "string" || !source.url.startsWith("https://")) return;
        const link = element("a", "", source.title || "SOURCE");
        link.href = source.url;
        link.target = "_blank";
        link.rel = "noopener noreferrer";
        sourceLinks.append(link);
      });
      if (sourceLinks.childElementCount) footer.append(sourceLinks);
    }
    section.append(footer);
  }

  body.append(section);
}

function artworkCreditLine(asset, role) {
  const line = element("p", "artwork-credit-line");
  line.append(element("strong", "", role.toUpperCase()));
  if (asset.credit) line.append(element("span", "", "CREDIT · " + asset.credit));
  if (asset.license) line.append(element("span", "", "RIGHTS · " + asset.license));
  if (asset.rightsStatus) line.append(element("span", "artwork-rights-status", asset.rightsStatus.replace(/-/g, " ").toUpperCase()));
  if (asset.sourceUrl && isSafeImageUrl(asset.sourceUrl)) {
    const source = element("a", "", "SOURCE ↗");
    source.href = asset.sourceUrl;
    source.target = "_blank";
    source.rel = "noopener noreferrer";
    line.append(source);
  } else if (asset.source) {
    line.append(element("span", "", "SOURCE · " + asset.source));
  }
  return line;
}

function renderArtworkCredits(artwork, parent) {
  const roles = [
    ["cover", "COVER ART"],
    ["hero", "FEATURE ART"],
    ["logo", "LOGO"],
    ["icon", "ICON"]
  ].filter(([role]) => isDisplayableArtworkAsset(artwork[role]));
  if (!roles.length) return;
  const section = element("section", "artwork-credit-panel");
  section.append(element("h3", "", "ARTWORK CREDITS & RIGHTS"));
  roles.forEach(([role, label]) => section.append(artworkCreditLine(artwork[role], label)));
  parent.append(section);
}

function renderArtworkGallery(game, artwork, body) {
  const candidates = Array.isArray(artwork.screenshots) ? artwork.screenshots.slice() : [];
  const upstreamShots = game.homebrew && Array.isArray(game.homebrew.screenshotUrls)
    ? game.homebrew.screenshotUrls : [];
  upstreamShots.forEach((url, index) => {
    if (!isSafeImageUrl(url)) return;
    candidates.push({
      url,
      alt: game.title + " homebrew screenshot " + (index + 1),
      caption: "UPSTREAM HOMEBREW CATALOG SCREENSHOT",
      source: "VitaHomebrewDB",
      credit: "VitaHomebrewDB entry",
      license: "Usage terms not supplied by the source index"
    });
  });

  const seen = new Set();
  const screenshots = candidates.filter((asset) => {
    if (!isDisplayableArtworkAsset(asset) || seen.has(asset.url)) return false;
    seen.add(asset.url);
    return true;
  }).slice(0, 12);
  if (!screenshots.length) return;

  const section = element("section", "artwork-gallery-section");
  const heading = element("div", "artwork-gallery-heading");
  heading.append(element("span", "artwork-gallery-kicker", "VISUAL ARCHIVE / " + String(screenshots.length).padStart(2, "0") + " FRAMES"));
  heading.append(element("h3", "", "Screenshots & gallery"));
  section.append(heading);
  const grid = element("div", "artwork-gallery-grid");

  screenshots.forEach((asset, index) => {
    const figure = element("figure", "artwork-gallery-figure");
    const link = element("a", "artwork-gallery-link");
    link.href = asset.url;
    link.target = "_blank";
    link.rel = "noopener noreferrer";
    link.setAttribute("aria-label", "Open full-size image: " + asset.alt);
    const image = element("img", "artwork-gallery-image");
    image.src = asset.url;
    image.alt = asset.alt;
    image.loading = "lazy";
    image.decoding = "async";
    if (Number.isInteger(asset.width) && Number.isInteger(asset.height)) {
      image.setAttribute("width", String(asset.width));
      image.setAttribute("height", String(asset.height));
    }
    image.addEventListener("error", () => {
      figure.classList.add("artwork-image-unavailable");
      link.remove();
      figure.append(element("span", "", "IMAGE UNAVAILABLE"));
    }, {once: true});
    link.append(image);
    figure.append(link);

    const caption = element("figcaption", "artwork-gallery-caption");
    caption.append(element("strong", "", asset.caption || asset.alt));
    if (asset.credit) caption.append(element("span", "", "CREDIT · " + asset.credit));
    if (asset.license) caption.append(element("span", "", "RIGHTS · " + asset.license));
    if (!asset.rightsStatus) caption.append(element("span", "artwork-rights-warning", "RIGHTS NOT DECLARED BY UPSTREAM INDEX"));
    if (asset.sourceUrl && isSafeImageUrl(asset.sourceUrl)) {
      const source = element("a", "", "SOURCE ↗");
      source.href = asset.sourceUrl;
      source.target = "_blank";
      source.rel = "noopener noreferrer";
      caption.append(source);
    } else if (asset.source) {
      caption.append(element("span", "", "SOURCE · " + asset.source));
    }
    figure.append(caption);
    grid.append(figure);
  });
  section.append(grid);
  const notice = element("p", "artwork-gallery-note",
    "Curated images are shown only after their rights status is approved. Legacy upstream screenshots remain attributed to their source; the source index does not provide a license statement.");
  section.append(notice);
  body.append(section);
}

function openGame(game) {
  const override = state.overrides[game.gameId] || {};
  const artwork = mergeGameArtwork(game, override);
  dialogContent.replaceChildren();

  const hero = element("div", "dialog-hero");
  const heroAsset = isDisplayableArtworkAsset(artwork.hero) ? artwork.hero
    : isDisplayableArtworkAsset(artwork.cover) ? artwork.cover : null;
  if (heroAsset) {
    const heroImage = element("img", "dialog-hero-image");
    heroImage.src = heroAsset.url;
    heroImage.alt = heroAsset.alt;
    heroImage.loading = "eager";
    heroImage.decoding = "async";
    hero.append(heroImage);
  } else if (game.homebrew && isSafeImageUrl(game.homebrew.iconUrl)) {
    hero.classList.add("dialog-hero-homebrew");
    const iconImage = element("img", "dialog-hero-homebrew-icon");
    iconImage.src = game.homebrew.iconUrl;
    iconImage.alt = game.title + " homebrew icon";
    iconImage.loading = "lazy";
    hero.append(iconImage);
  } else {
    hero.classList.add("dialog-hero-fallback");
    hero.append(element("span", "", game.title.slice(0, 1).toUpperCase()));
  }
  if (isDisplayableArtworkAsset(artwork.logo)) {
    const logoImage = element("img", "dialog-hero-logo");
    logoImage.src = artwork.logo.url;
    logoImage.alt = artwork.logo.alt;
    logoImage.loading = "lazy";
    logoImage.decoding = "async";
    hero.append(logoImage);
  }
  dialogContent.append(hero);

  const body = element("div", "dialog-body");
  const platformLine = element("div", "dialog-platforms");
  gamePlatforms(game).forEach((platform) => platformLine.append(makePlatformBadge(platform)));
  body.append(platformLine);
  const listAction = element("button", "button button-primary dialog-list-action", "ADD TO LIST +");
  listAction.type = "button";
  listAction.addEventListener("click", () => addGameToActiveList(game));
  body.append(listAction);
  body.append(element("h2", "dialog-title", game.title));
  body.append(element("p", "dialog-summary", game.description || override.description || game.summary || override.summary || "No description yet. The release details are still worth a look."));
  renderArtworkCredits(artwork, body);
  renderArtworkGallery(game, artwork, body);

  const facts = element("div", "dialog-facts");
  if (game.developer || override.developer) facts.append(element("span", "", "DEVELOPER · " + (game.developer || override.developer)));
  if (game.publisher || override.publisher) facts.append(element("span", "", "PUBLISHER · " + (game.publisher || override.publisher)));
  if (game.releaseYear || override.releaseYear) facts.append(element("span", "", "YEAR · " + (game.releaseYear || override.releaseYear)));
  if (game.homebrew && game.homebrew.author) facts.append(element("span", "", "BY · " + game.homebrew.author));
  if (game.homebrew && game.homebrew.version) facts.append(element("span", "", "VERSION · " + game.homebrew.version));
  body.append(facts);
  renderEditorialFacts(game, override, body);
  renderEditorialReview(game, override, body);

  const releasesHeading = element("div", "release-heading");
  releasesHeading.append(element("h3", "", "Versions in the stack"));
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

  if (game.homebrew) {
    const metadata = element("section", "homebrew-details");
    metadata.append(element("h3", "", "Homebrew notes"));
    if (game.homebrew.requirements) metadata.append(element("p", "homebrew-requirements", "NEEDS · " + game.homebrew.requirements));
    if (game.homebrew.changelog) {
      const change = element("details", "homebrew-changelog");
      change.append(element("summary", "", "Release notes"));
      change.append(element("p", "", game.homebrew.changelog));
      metadata.append(change);
    }
    const links = element("div", "homebrew-links");
    const addExternalLink = (label, url) => {
      if (typeof url !== "string" || !url.startsWith("https://")) return;
      const anchor = element("a", "", label + " ↗");
      anchor.href = url;
      anchor.target = "_blank";
      anchor.rel = "noopener noreferrer";
      links.append(anchor);
    };
    addExternalLink("Source code", game.homebrew.sourceProjectUrl);
    addExternalLink("Release page", game.homebrew.releasePageUrl);
    addExternalLink("Original catalog", game.homebrew.sourceCatalogUrl);
    if (links.childElementCount) metadata.append(links);
    body.append(metadata);
  }

  const recommendations = getRecommendations(game, override);
  const recommendationSection = element("section", "similar-games-section");
  recommendationSection.append(element("h3", "", "MORE LIKE THIS"));
  recommendationSection.append(element("p", "similar-games-intro", "Same energy. Different platforms."));
  if (recommendations.length) {
    const recommendationGrid = element("div", "recommendation-grid");
    recommendations.forEach((item) => recommendationGrid.append(createRecommendationCard(item)));
    recommendationSection.append(recommendationGrid);
  } else {
    recommendationSection.append(element("p", "similar-games-empty", "No matches yet. Similar picks show up when we have enough genre, theme and gameplay data."));
  }
  body.append(recommendationSection);

  const note = element("p", "dialog-source-note", "Release identifiers and descriptive fields are sourced from public metadata indexes. Package URLs, payloads and license material are not part of VITA NEXUS. Homebrew artwork remains hosted by its original catalog source and belongs to its respective creators.");
  body.append(note);
  dialogContent.append(body);
  if (typeof dialog.showModal === "function") {
    if (!dialog.open) dialog.showModal();
  } else {
    dialog.setAttribute("open", "");
  }
}


const listCreateForm = document.querySelector("#list-create-form");
const listNameInput = document.querySelector("#list-name");
const listSelect = document.querySelector("#list-select");
const listEntries = document.querySelector("#list-entries");
const listStatus = document.querySelector("#lists-status");
const listExportButton = document.querySelector("#list-export");
const listImportInput = document.querySelector("#list-import");

function saveUserLists() {
  try {
    localStorage.setItem(VitaNexusLists.STORAGE_KEY, JSON.stringify(state.userLists));
    return true;
  } catch (error) {
    console.error("Could not save personal lists", error);
    listStatus.textContent = "Browser storage is unavailable or full. Export your lists to keep a copy.";
    return false;
  }
}

function loadUserLists() {
  try {
    const stored = localStorage.getItem(VitaNexusLists.STORAGE_KEY);
    state.userLists = stored ? VitaNexusLists.normalizeLists(JSON.parse(stored)) : [];
  } catch (error) {
    console.error("Could not read personal lists", error);
    state.userLists = [];
    listStatus.textContent = "Saved lists could not be read. Import a backup or check browser storage.";
  }
  state.activeListId = state.userLists[0]?.id || "";
}

function renderUserLists() {
  const currentId = state.activeListId;
  listSelect.replaceChildren();
  if (!state.userLists.length) {
    const option = element("option", "", "Create a list to get started");
    option.value = "";
    listSelect.append(option);
    listSelect.disabled = true;
  } else {
    state.userLists.forEach((list) => {
      const option = element("option", "", list.name + " (" + list.games.length + ")");
      option.value = list.id;
      listSelect.append(option);
    });
    listSelect.disabled = false;
    if (state.userLists.some((list) => list.id === currentId)) state.activeListId = currentId;
    else state.activeListId = state.userLists[0].id;
    listSelect.value = state.activeListId;
  }
  const active = state.userLists.find((list) => list.id === state.activeListId);
  listExportButton.disabled = !active;
  listEntries.replaceChildren();
  if (!active) {
    listEntries.append(element("li", "list-empty", "No list yet. Create one, then add games from any catalog card."));
    return;
  }
  if (!active.games.length) {
    listEntries.append(element("li", "list-empty", "This list is empty. Open a game and hit ADD TO LIST."));
    return;
  }
  active.games.forEach((gameId, index) => {
    const game = state.catalog?.games?.find((item) => item.gameId === gameId);
    const row = element("li", "list-entry");
    const title = element("span", "list-entry-title", game ? game.title : "Unknown catalog entry · " + gameId);
    const position = element("span", "list-entry-position", String(index + 1).padStart(2, "0"));
    const actions = element("div", "list-entry-actions");
    const moveUp = element("button", "", "↑");
    moveUp.type = "button";
    moveUp.disabled = index === 0;
    moveUp.setAttribute("aria-label", "Move " + (game?.title || gameId) + " up");
    moveUp.addEventListener("click", () => updateUserLists(VitaNexusLists.moveGame(state.userLists, active.id, gameId, -1)));
    const moveDown = element("button", "", "↓");
    moveDown.type = "button";
    moveDown.disabled = index === active.games.length - 1;
    moveDown.setAttribute("aria-label", "Move " + (game?.title || gameId) + " down");
    moveDown.addEventListener("click", () => updateUserLists(VitaNexusLists.moveGame(state.userLists, active.id, gameId, 1)));
    const remove = element("button", "", "×");
    remove.type = "button";
    remove.setAttribute("aria-label", "Remove " + (game?.title || gameId));
    remove.addEventListener("click", () => updateUserLists(VitaNexusLists.removeGame(state.userLists, active.id, gameId)));
    actions.append(moveUp, moveDown, remove);
    row.append(position, title, actions);
    listEntries.append(row);
  });
}

function updateUserLists(lists, message) {
  state.userLists = lists;
  saveUserLists();
  renderUserLists();
  if (message) listStatus.textContent = message;
}

function addGameToActiveList(game) {
  if (!state.activeListId) {
    listStatus.textContent = "Create or select a list first.";
    return;
  }
  const active = state.userLists.find((list) => list.id === state.activeListId);
  if (active?.games.includes(game.gameId)) {
    listStatus.textContent = game.title + " is already in " + active.name + ".";
    return;
  }
  updateUserLists(VitaNexusLists.addGame(state.userLists, state.activeListId, game.gameId), game.title + " added to " + (active?.name || "your list") + ".");
}

listCreateForm.addEventListener("submit", (event) => {
  event.preventDefault();
  try {
    const name = listNameInput.value;
    const list = VitaNexusLists.createList(state.userLists, name);
    state.activeListId = list[list.length - 1].id;
    updateUserLists(list, "List created. Now go fill it with good stuff.");
    listNameInput.value = "";
  } catch (error) {
    listStatus.textContent = error.message;
  }
});

listSelect.addEventListener("change", () => {
  state.activeListId = listSelect.value;
  renderUserLists();
});

listExportButton.addEventListener("click", () => {
  const active = state.userLists.find((list) => list.id === state.activeListId);
  if (!active) return;
  const payload = VitaNexusLists.exportLists([active]);
  const blob = new Blob([payload], {type: "application/json"});
  const url = URL.createObjectURL(blob);
  const anchor = element("a", "", "");
  anchor.href = url;
  anchor.download = "vita-nexus-" + active.name.toLowerCase().replace(/[^a-z0-9]+/g, "-").replace(/^-|-$/g, "") + ".json";
  anchor.click();
  URL.revokeObjectURL(url);
  listStatus.textContent = "Exported " + active.name + ". Keep that file somewhere safe.";
});

listImportInput.addEventListener("change", async () => {
  const file = listImportInput.files && listImportInput.files[0];
  if (!file) return;
  try {
    const imported = VitaNexusLists.importLists(await file.text());
    const existingIds = new Set(state.userLists.map((list) => list.id));
    const additions = imported.map((list, index) => ({
      ...list,
      id: existingIds.has(list.id) ? list.id + "-imported-" + Date.now().toString(36) + "-" + index : list.id
    }));
    if (state.userLists.length + additions.length > 100) throw new Error("Import would exceed the 100-list limit.");
    updateUserLists([...state.userLists, ...additions], "Imported " + additions.length + " list(s). Nothing already saved was overwritten.");
    if (additions.length) {
      state.activeListId = additions[0].id;
      renderUserLists();
    }
  } catch (error) {
    listStatus.textContent = error.message;
  } finally {
    listImportInput.value = "";
  }
});

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

loadUserLists();
loadCatalog().then(() => renderUserLists());
