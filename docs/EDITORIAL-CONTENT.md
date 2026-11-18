# 📰 Editorial content & game pages

VITA NEXUS separates **catalog facts** from **editorial judgement**. Upstream indexes remain the source of identifiers and edition metadata; curated copy lives in `web/data/editorial-overrides.json` so a catalog refresh cannot wipe it.

## The page anatomy

Every game sheet can combine three kinds of information:

| Page element | Data source | What it means |
|---|---|---|
| Platform, edition, region and identifiers | Shared catalog | Verifiable release metadata |
| PEGI / ESRB and external review scores | Catalog or curated override | Third-party data with its own source |
| NEXUS review, verdict, highlights and curiosities | Editorial override | Original VITA NEXUS commentary |

A missing field stays absent. There are no invented ratings, placeholder review scores or synthetic cover credits.

## Add a curated review

Add an entry under `games`, keyed by the exact catalog `gameId`:

```json
{
  "games": {
    "replace-with-exact-game-id": {
      "editorial": {
        "rubric": "NEXUS FIELD REVIEW",
        "standfirst": "One-line editorial introduction.",
        "review": {
          "headline": "A headline with a point of view",
          "body": "The review explains the experience, its strengths and its limits.",
          "verdict": "A clear recommendation, not a marketing slogan.",
          "score": {
            "value": 84,
            "scale": 100,
            "methodology": "A short note explaining how the score was judged."
          }
        },
        "highlights": ["A specific strength"],
        "caveats": ["A meaningful limitation"],
        "curiosities": ["A sourced detail worth a magazine sidebar."],
        "credit": {
          "byline": "VITA NEXUS Editorial Desk",
          "reviewedAt": "2026-11-18T00:00:00Z",
          "sources": [
            { "title": "Source title", "url": "https://source.example/item" }
          ]
        }
      }
    }
  }
}
```

This is a structural example, **not a review of a real game**. Replace the game ID, text, date and source with checked material before publishing.

## Editorial rules

- Keep platform, release and product identifiers out of opinion copy; they already have dedicated factual fields.
- Use an exact `gameId`. Never create a duplicate game record just to attach a review.
- A NEXUS score is an editorial assessment, not an average of user votes or a third-party score. Its scale and method must be explicit.
- PEGI, ESRB and other external scores must retain the provider/system, value and source. Do not convert one rating system into another.
- Every editorial block carries a byline and review date. Every listed source must use HTTPS.
- Artwork must include source, credit, license/usage terms and a reviewed rights status. Curated asset rules and gallery behavior are documented in [Artwork management](ARTWORK-MANAGEMENT.md). Do not mirror a random image from search results.
- Keep copy compact: 900 characters for review body, four highlights, three caveats and four sidebar curiosities.
- Use plain language. Say what works, what does not, and who is likely to enjoy it.
- If the evidence is missing or disputed, leave the field empty and mark the uncertainty in the copy only when useful.

## Validation

The editorial data has its own schema at `catalog/schema/editorial-overrides.schema.json` and is checked by:

```sh
python -m json.tool catalog/schema/editorial-overrides.schema.json
python tools/catalog-builder/validate_editorial_overrides.py web/data/editorial-overrides.json
```

The quality workflow runs both checks. The browser renders editorial blocks only when corresponding content exists; ordinary catalog pages remain complete without them.

## Magazine layout

The web game sheet is composed like a compact review spread: headline and standfirst, a score stamp when one exists, verdict panel, strengths/limitations columns, a short back-page curiosity rail and a byline/source footer. The score is never given a dominant visual treatment when no editorial score is published.

This is an original VITA NEXUS layout language inspired by print gaming magazines, not a reproduction of a specific publication's scans or artwork.
