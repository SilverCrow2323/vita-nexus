# 🌐 Web companion

The website is a first-class VITA NEXUS surface, not a second native app.

## Hosting choice

The initial web companion is published with **GitHub Pages** from the **web/** directory using GitHub Actions.

Expected project URL:

**https://silvercrow2323.github.io/vita-nexus/**

The address becomes usable only after Pages is enabled and the first deployment succeeds.

## What Pages can host

GitHub Pages serves static HTML, CSS, JavaScript and build output. That is ideal for a polished project landing page, documentation and release notes, a read-only catalog backed by versioned JSON, browser-side search and filters, game detail pages, and static public ranking snapshots.

It is not an application server. Do not place API secrets, private user data or credentials in the published site.

## What needs a separate backend

| Feature | Static Pages alone? | Future approach |
|---|---:|---|
| Browse catalog JSON | ✅ | Static data files |
| Search and filter | ✅ | Browser-side index |
| Read public rankings | ✅ | Published JSON snapshots |
| Save a list in the current browser | ✅ | Local browser storage / export |
| Public accounts and profiles | ❌ | Authenticated backend |
| Submit and vote on public lists | ❌ | API, validation and moderation |
| Cloud sync with a Vita | ❌ | Opt-in sync service |
| Read Vita memory-card contents | ❌ | Native app only |

The native application remains fully useful offline. A web sign-in must never be required to use local profiles or playtime tracking.

## Initial deployment

The workflow is stored at **.github/workflows/pages.yml**. It publishes only the contents of **web/** when that directory or the workflow changes on **main**.

To activate it:

1. Open the repository on GitHub.
2. Go to **Settings → Pages**.
3. Under **Build and deployment**, choose **GitHub Actions** as the source.
4. Push to **main** or run the workflow manually from **Actions**.
5. Open the deployment URL shown by the workflow.

## Web experience roadmap

### Phase 1 — Project portal
A responsive landing page, clear project status, feature overview and links to documentation.

### Phase 2 — Catalog browser
A shared, versioned game/release dataset with platform filters, search and game detail routes.

### Phase 3 — Rankings and lists
Publish static global charts and allow browser-local custom lists with JSON export/import.

### Phase 4 — Community services
Only after a backend is selected: accounts, public submissions, voting, moderation and optional cross-device sync.

## Performance and accessibility

- Mobile-first responsive layouts.
- Keyboard and touch navigation.
- Semantic HTML and visible focus.
- Reduced-motion support.
- Lazy-loaded artwork with explicit dimensions.
- Search/filter should remain useful without external scripts or third-party trackers.
- Keep the initial payload small; publish artwork separately from metadata where practical.
