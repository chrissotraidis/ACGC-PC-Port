# Bellpad Apple integration branch

This branch maintains the effective sources used by [Bellpad](https://github.com/chrissotraidis/bellpad). It preserves the history of [birabittoh/ACGC-PC-Port](https://github.com/birabittoh/ACGC-PC-Port) at `915fb86ba9a6c2144dabda9143d93af7a3f92be7`. No upstream upgrade is included.

The initial 50 source commits import Bellpad's existing patches byte-for-byte; the app's `docs/source-maintenance/migration.json` records the patch-to-commit mapping. Fixes now belong in ordinary source commits on `bellpad/apple`. App pins are immutable; changing this branch or the fork default does not update other apps.

Upstream LICENSE and per-file notices remain in force for upstream material. Original Bellpad changes retain the app repository's existing licensing notice: no outbound license is granted unless a file or contribution states otherwise. This document does not relicense upstream or original Bellpad material, nor grant rights to game assets or third-party reconstructed material. No retail images, extracted data, saves or signing material are included by this migration.

Compare with `git diff 915fb86ba9a6c2144dabda9143d93af7a3f92be7..HEAD`. App-specific reports belong in Bellpad with its source pin and logs. Do not send reports upstream without establishing that an unmodified upstream build reproduces them.
