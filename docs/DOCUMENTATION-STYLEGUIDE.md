# 🎨 Documentation style guide

Documentation is part of the VITA NEXUS experience. It should feel like a well-designed field guide to a gaming universe—not a wall of anonymous text.

## Editorial voice

- Write in clear, direct English.
- Be friendly, confident and occasionally playful.
- Prefer useful specifics over marketing fluff.
- Explain technical limits honestly.
- Keep paragraphs short and headings descriptive.
- Define acronyms the first time they appear.

## Visual rhythm

| Element | Best use |
|---|---|
| Hero banner | Open major guides and release notes |
| Badges | Project status, platform, maturity or risk |
| Callout blocks | Critical notes, caveats and useful shortcuts |
| Tables | Comparisons, compatibility and structured facts |
| Mermaid diagrams | Architecture, lifecycle and data flow |
| Screenshots | UI walkthroughs and before/after examples |
| Cards and grids | Feature overviews and navigation |
| Checklists | Setup, validation and release readiness |
| Code blocks | Exact commands, paths, schemas and config |
| Changelogs | Explain what changed and why |

Do not use every element on every page. Give each page its own rhythm.

## Recommended page structure

1. **Visual opening:** short title, one-sentence purpose and optional banner.
2. **At-a-glance:** status, platform, prerequisites or key facts.
3. **Main story:** grouped sections with tables, diagrams or examples.
4. **Practical details:** commands, paths, configuration or workflow.
5. **Caveats:** known limitations and unverified behavior.
6. **Next step:** links to the most relevant related guide.

## Status vocabulary

- 🟢 **Implemented** — present and tested.
- 🟡 **In progress** — actively being developed.
- 🔵 **Planned** — accepted scope, not implemented.
- 🟠 **Needs validation** — awaiting hardware or source verification.
- ⚪ **Not started** — no implementation yet.
- 🔴 **Blocked** — cannot proceed without a dependency or decision.

Never label a planned feature as implemented because it appears in a mockup.

## Screenshots and artwork

- Use original project screenshots for actual implementation.
- Label concept art and mockups as concepts.
- Keep screenshots crisp and crop them consistently.
- Do not use copyrighted game artwork as project branding.
- Record source and usage rights for third-party artwork.
- Provide alt text for every meaningful image.

## Technical writing rules

- Show exact paths and commands.
- State tested hardware, firmware and software versions.
- Distinguish observed behavior from assumptions.
- Add a reproducible validation procedure for compatibility claims.
- Keep architecture diagrams synchronized with code.
- Link upstream documentation rather than copying entire manuals.

## README design

The repository README should be a visual front door: project banner, one-paragraph pitch, product pillars, architecture sketch, status table and a short documentation map. Detailed implementation notes belong in **docs/**.

## Release notes

Every release note should include a compact release banner and version, highlights grouped by user impact, screenshots or GIFs when they show real behavior, fixes and known issues, a compatibility matrix, upgrade notes, checksums and artifact links where applicable.

## Brand consistency

Use the VITA NEXUS identity: deep midnight surfaces, electric violet, cyan accents, crisp white typography and restrained motion. The same visual language should connect the native UI, website, README and documentation.
