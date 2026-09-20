# Sketchfab source assets

This directory contains the original files copied from the local `Resource/sketchfab` collection for TASK-004 asset preparation.

## License status

On 2026-09-20, the project owner explicitly confirmed that all remaining Sketchfab files in this collection have permission to be included in the Hearthward shared repository. The files are committed on that authorization.

The local collection does not contain the original Sketchfab page URLs/UIDs, author names, download dates, or per-asset license documents. Those provenance fields remain to be recovered and recorded. This file therefore records owner authorization, not independent license verification or a claim that every asset uses the same Sketchfab license type.

- Sketchfab license reference: https://sketchfab.com/licenses
- Import and optimization assessment: [`../../../resourceSummary.md`](../../../resourceSummary.md)

## Included groups

- Grass source archive and external texture maps.
- Large rock source archive and external texture map.
- Medium rock source archive and external diffuse/normal maps.
- Small rock source archive and external diffuse/AO maps.

The copied collection contains 24 source files totaling 242,345,494 bytes. A byte-for-byte SHA-256 comparison against the local source collection reported zero mismatches before commit.

## Repository handling

- ZIP, JPEG, and JPG files are tracked with Git LFS according to the repository `.gitattributes` rules.
- The archives are preserved as downloaded; their contents have not been extracted into the repository.
- These are production-source candidates, not imported Unreal Engine assets.
- No FBX/OBJ conversion, texture conversion, LOD generation, collision authoring, or UE validation has been performed.
