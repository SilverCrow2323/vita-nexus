# 🧷 VITA NEXUS — Visual + writing rules

This is the house style for the website, native UI, README and project docs. Keep it recognisable. Keep it readable. Don't turn it into a corporate dashboard.

## The look: underground cyberpunk manga

Think bootleg manga zine, late-night arcade flyer and photocopied gig poster—printed in loud colour.

- **Ink:** near-black, dirty charcoal and warm paper-white.
- **Punch colours:** electric magenta, acid yellow, cyan and ultraviolet.
- **Texture:** halftone dots, rough grain, scan lines, hand-drawn marks, sticker shapes and hard shadows.
- **Layout:** uneven grids, poster-like hero panels, sharp corners, angled labels and bold section dividers.
- **Type:** condensed display faces for headlines, clean sans-serif for body copy and mono for IDs, stats and technical labels.
- **Motion:** quick and purposeful. Small reveals, sharp hover shifts and restrained glitch details. Never make reading or navigation harder.

Avoid glossy glass cards, soft pastel gradients, generic startup illustrations and over-rounded components. Do not bury important information under texture.

## The voice: say it straight

Write like a person who knows the hardware and respects the reader's time.

- Use short sentences and everyday English.
- Put the useful part first.
- Name the action: **Search games**, **Open details**, **Check compatibility**.
- Explain limitations without hiding behind jargon.
- A little attitude is welcome. Sarcasm is fine; misinformation isn't.
- Cut phrases like "unlock the power of", "seamlessly integrated", "next-generation experience" and other marketing fog.

### Copy examples

| Skip this | Write this |
|---|---|
| Explore the entire catalog | Find a game |
| Your PlayStation universe, connected | PS Vita, PSP, PS1 and homebrew. One index. |
| Discover a personalised gaming experience | Find games with the same vibe |
| Metadata unavailable | No data yet |
| Feature currently planned for a future release | Not built yet. It's on the roadmap. |

## Page rhythm

Give each page a strong opening and a clear path through the content. Use a mix of:
- Big headline + one-line explanation
- At-a-glance stats and status labels
- Cards, tables and comparison grids
- Diagrams when they explain real architecture
- Screenshots for implemented UI
- Callouts for caveats, safety and compatibility notes
- Checklists for setup and release validation

Don't stack the same component endlessly. Every block should earn its space.

## Status vocabulary

- 🟢 **Live / implemented** — available in the repo or site; say whether it has been tested.
- 🟡 **In progress** — work has started, but it is not finished.
- 🔵 **Planned** — accepted scope, not implemented.
- 🟠 **Needs testing** — waiting for hardware, source or reproducible verification.
- ⚪ **Not started** — nobody has built it yet.
- 🔴 **Blocked** — cannot proceed until a dependency is resolved.

A mockup is not a build. A successful website deploy does not prove the native app works.

## Artwork + metadata

- Use original VITA NEXUS artwork for project branding.
- Game cover art belongs to its respective owners; preserve attribution and source URLs.
- Mark placeholder artwork clearly.
- Never invent age ratings, review scores, compatibility or release details.
- Keep the source and territory clear for external scores and age classifications.
- Add useful alt text to every meaningful image.

## Accessibility + implementation

- Keep contrast high and body copy readable.
- Preserve keyboard navigation, visible focus and semantic headings.
- Respect `prefers-reduced-motion`.
- Keep labels clear on small screens.
- Never use texture, animation or colour as the only way to communicate status.
- Test the actual page at desktop and mobile widths.
