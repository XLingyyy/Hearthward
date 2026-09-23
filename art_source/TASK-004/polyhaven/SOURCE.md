# Poly Haven source assets

This directory contains the original Poly Haven source files copied from the local `Resource/polyhaven` collection for TASK-004 asset preparation.

## License

Poly Haven states that all of its assets are released under CC0. Commercial use, modification, and redistribution are permitted.

- License: https://polyhaven.com/license
- Source library: https://polyhaven.com/
- Import and optimization assessment: [`../../../resourceSummary.md`](../../../resourceSummary.md)

## Included assets

| Asset | Source page |
|---|---|
| Grass Medium 01 | https://polyhaven.com/a/grass_medium_01 |
| Shrub 01 | https://polyhaven.com/a/shrub_01 |
| Tree Stump 01 | https://polyhaven.com/a/tree_stump_01 |
| Jacaranda Tree | https://polyhaven.com/a/jacaranda_tree |
| Island Tree 02 | https://polyhaven.com/a/island_tree_02 |
| Fir Tree 01 | https://polyhaven.com/a/fir_tree_01 |
| Pine Tree 01 | https://polyhaven.com/a/pine_tree_01 |
| Grass Ground | https://polyhaven.com/a/grass_ground |
| Dirt | https://polyhaven.com/a/dirt |
| Rocky Terrain | https://polyhaven.com/a/rocky_terrain |

The collection now contains 91 source files totaling 3,566,021,342 bytes. This batch adds 38 Fir Tree 01 and Pine Tree 01 files (2,266,006,664 bytes) to the 53 files already in the repository. The 22 previously present tree files and all 38 new tree files were compared byte-for-byte by SHA-256 with the local source collection; no mismatches were found.

## Repository handling

- Binary model and texture files are tracked with Git LFS according to the repository `.gitattributes` rules.
- These are production-source candidates, not imported Unreal Engine assets.
- No FBX/glTF conversion, texture conversion, LOD generation, collision authoring, or UE validation has been performed.
- The accompanying Sketchfab collection is intentionally excluded because its original page, author, and exact redistribution license have not yet been recovered.
