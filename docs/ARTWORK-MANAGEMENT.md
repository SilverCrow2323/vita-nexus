# 🖼️ Artwork management — the visual archive

Game pages should read like a printed review spread, not a folder full of random thumbnails. This guide defines how VITA NEXUS stores and displays cover art, hero images, logos, icons and screenshots.

## Where artwork lives

Curated visual assets belong in `web/data/editorial-overrides.json`, under the exact catalog `gameId`. They are separate from importer-owned catalog data, so a scheduled NPS/Homebrew refresh cannot erase human-reviewed artwork.

Supported roles:

| Role | Use in the magazine page |
|---|---|
| `cover` | Catalog card and fallback detail hero |
| `hero` | Wide feature image at the top of a game sheet |
| `logo` | Transparent title logo over the hero / cover |
| `icon` | Small app or homebrew identity asset |
| `screenshots` | Up to 12 gallery frames, displayed in a responsive 3-column grid |

The renderer merges each curated role over its upstream equivalent. A missing curated image never blanks an upstream cover or icon. Duplicate screenshot URLs are removed, and the gallery uses lazy-loaded images.

## Add a reviewed asset

Use the exact `gameId` from `web/data/catalog.json`:

```json
{
  "games": {
    "replace-with-exact-game-id": {
      "artwork": {
        "cover": {
          "url": "https://cdn.example.org/game-cover.jpg",
          "alt": "Front cover artwork for the European release",
          "caption": "European retail cover",
          "source": "Publisher press kit",
          "sourceUrl": "https://publisher.example.org/press",
          "credit": "Publisher / Studio",
          "license": "Permission granted by the rights holder",
          "rightsStatus": "permission-granted",
          "rightsReviewedAt": "2026-11-18T03:00:00+01:00",
          "width": 800,
          "height": 1200
        },
        "screenshots": [
          {
            "url": "https://cdn.example.org/gameplay-01.jpg",
            "alt": "Gameplay showing the player crossing a neon-lit street",
            "caption": "Night district — in-game capture",
            "source": "Official press kit",
            "sourceUrl": "https://publisher.example.org/press",
            "credit": "Publisher / Studio",
            "license": "Permission granted by the rights holder",
            "rightsStatus": "permission-granted",
            "rightsReviewedAt": "2026-11-18T03:00:00+01:00",
            "width": 1920,
            "height": 1080
          }
        ]
      }
    }
  }
}
```

This is a structural example, **not an actual game asset or permission record**. Replace every placeholder with verified information.

## Rights status

Every curated asset needs a rights decision:

| Status | Published in the gallery? | Meaning |
|---|---:|---|
| `verified-license` | ✅ | A license was checked and permits this use |
| `permission-granted` | ✅ | The rights holder granted permission for this use |
| `public-domain` | ✅ | Public-domain status was checked |
| `pending-review` | ❌ | Do not display until cleared |
| `rejected` | ❌ | Do not publish |

The status is not a license by itself. The `license` field must name the actual license or permission basis, and `rightsReviewedAt` records when it was checked. Preserve evidence of permission internally; do not expose private correspondence or personal details in the public JSON. A source URL is useful provenance, but a page being publicly accessible does not automatically grant permission to reuse its images.

All curated image and source URLs must use HTTPS. Provide meaningful alt text, an accurate credit and a source. Add dimensions where known. Do not upload scans of magazines, press packs or game manuals unless their reuse terms allow it. Prefer publisher-approved media, rights-cleared screenshots, or captures made for the project.

## Legacy upstream screenshots

Some homebrew source entries already provide screenshot URLs. They remain available for compatibility and are attributed to the upstream catalog. When the upstream data does not publish a license statement, the gallery labels this clearly as **RIGHTS NOT DECLARED BY UPSTREAM INDEX**. That is not a claim of permission; replace or supplement these assets with cleared material when possible.

## Validation and checks

The editorial override schema is `catalog/schema/editorial-overrides.schema.json`. The existing validator now checks:
- Every image is HTTPS and has alt text, source, credit and a license/usage statement.
- Every curated asset has a supported rights status and a timezone-aware review date.
- Optional source/evidence links use HTTPS.
- Width and height are sane positive integers; galleries are limited to 12 frames.
- Duplicate screenshot URLs and unknown asset fields are rejected.

Run the checks locally:

```sh
python -m json.tool catalog/schema/editorial-overrides.schema.json > /dev/null
python -m json.tool web/data/editorial-overrides.json > /dev/null
python tools/catalog-builder/validate_editorial_overrides.py web/data/editorial-overrides.json
node --check web/catalog.js
node --test tests/*.test.cjs
```

The quality workflow and scheduled catalog refresh both run the metadata validator. Assets marked `pending-review` or `rejected` remain stored for editorial workflow but are not rendered publicly.
