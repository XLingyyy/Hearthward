# TASK-064 actual widget render review

- Root ran `Hearthward.Map064` with `-Map064Render`; the actual text-boundary regression failed before the production fix (`map064-text-red`, five assertions), then passed with the fix (`map064-text-green`, 1/1).
- Inspected both 1696×954 actual Slate widget PNGs at 100% text scale. The three-line authored sidebar description and full three-line main05 objective remain visible; legends and footer controls no longer overlap. Normal initial exploration, tracked known location, objective text and fog checks passed.
- Production changes measure the dynamic objective with the actual display typeface/wrap rules and fit it above the map bottom; the authored sidebar description receives 120px and six legend rows move down 64px. No text shortening, global font reduction or map projection change.
- Before/after images and layouts remain in `Saved/Task064/map-text-overlap-red` and `Saved/Task064/map-text-overlap-green`. This is widget render evidence; normal physical input, 125%/150% text scales and the remaining-garrison hint require their own TASK-069 verification.
