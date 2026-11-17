# ✦ Cross-platform recommendation engine

> **The right recommendation can come from the wrong generation.**

VITA NEXUS treats PS Vita, PSP and PS1 as one discovery universe. A game page can recommend another title from any of those platforms—not just the platform currently being viewed.

## Recommendation inputs

The engine uses structured, attributable metadata:

| Signal | Why it matters |
|---|---|
| Genres and subgenres | Find a shared gameplay family |
| Themes and setting | Match the kind of world or story |
| Gameplay features | Match mechanics such as turn-based combat, exploration or co-op |
| Explicit game relations | Preserve editorial knowledge, series links and spiritual successors |
| Target platform | Encourage discovery outside the current generation |

Profile preferences and play history will be added to the native engine once the local profile and session stores are implemented. The web catalog currently uses public catalog metadata only.

## Ranking strategy

The initial browser engine is deterministic and runs locally:

1. Resolve explicit relations by canonical `gameId`.
2. Rank editorial links by confidence, with a small diversity preference for a different platform.
3. Compare normalized genre, theme and feature tags for other games.
4. Give stronger weight to shared genres and mechanics than to shared themes.
5. Apply a cross-platform diversity bonus when the candidate belongs to a different native platform.
6. Limit repeated candidates from the same platform and return at most six suggestions.

The current metadata-match weights are:

| Match | Weight |
|---|---:|
| Shared genre | +4 |
| Shared gameplay feature | +3 |
| Shared theme | +2 |
| Different native platform | +2 |

These are initial ranking weights, not review scores or probability claims. They will be tuned against curated test cases and real player feedback.

## Recommendation labels

- **SIMILAR EXPERIENCE** — shared gameplay or mechanics.
- **SAME VIBE** — similar themes, atmosphere or setting.
- **SAME SERIES** — explicit series relationship.
- **SPIRITUAL SUCCESSOR** — curated successor relationship.
- **METADATA MATCH** — calculated from shared genre, theme or feature metadata.

Every recommendation should include a short explanation. The UI must not invent similarity reasons.

## Honest empty states

Many NPS rows contain release metadata but no verified genres, themes, features or editorial relationships. In those cases the game page shows an explanation that recommendations will appear as verified metadata is added.

It is better to show an honest empty state than to manufacture similarities based on title-name coincidence.

## Data contract

Curated links use the `relations` array on a canonical game record:

~~~json
{
  "gameId": "example-game",
  "relationType": "similar-gameplay",
  "reason": "Both games focus on tactical, turn-based encounters.",
  "confidence": 0.88
}
~~~

Valid relation types are:

- `similar-gameplay`
- `same-vibe`
- `same-series`
- `spiritual-successor`
- `editor-pick`

The linked `gameId` must exist in the same catalog snapshot. Relations are cross-platform by default: no rule forces the linked game to share the source platform.

## Validation checklist

- [x] Runs in the browser without a remote AI service.
- [x] Resolves explicit canonical game relations.
- [x] Supports cross-platform candidate selection.
- [x] Shows the platform badge and a human-readable reason.
- [x] Does not fabricate recommendations when metadata is missing.
- [ ] Add curated seed relations for the first reviewed game set.
- [ ] Add profile-aware native scoring using preferences and play history.
- [ ] Add automated ranking-quality fixtures for known cross-platform pairs.
