# AGENTS.md for mapper

## Purpose
This repository contains the main OpenOrienteering Mapper application source code.

## Important development rule
Edits are usually made here first, but Android APK builds are not produced directly from this working tree.

The actual Android build source used by superbuild is:

```text
~/src/superbuild/build-android/source/openorienteering-mapper-git-master

Therefore, changes made in ~/src/mapper do not automatically appear in the built APK.

Before building Android, sync this repository into the superbuild source tree.

Android build sync rule

Use this before Android builds:

rsync -a --delete --exclude '.git' \
  ~/src/mapper/ \
  ~/src/superbuild/build-android/source/openorienteering-mapper-git-master/
Branch workflow

Typical flow:

Create or checkout a branch in ~/src/mapper
Implement changes here
Sync into the superbuild source tree
Run Android build via ~/src/oom-build-tools/oom-run.sh
Test the signed APK from GitHub Actions artifacts or a manually signed local APK
What to avoid
Do not assume that changing files here is enough to affect Android builds
Do not change unrelated desktop behavior unless required
Do not refactor broadly unless explicitly requested
Do not modify build scripts in this repo when the issue is only in runtime/UI code
When implementing Android-facing features
Verify whether the feature is actually reachable from Android UI
Do not assume that adding settings keys alone makes the feature visible
If adding a settings page action, confirm the Android app has a path to open it
Helpful checks

Search for feature strings:

git grep "Configure mobile toolbars" || true
git grep "mobile_top_actions" || true
git grep "mobile_bottom_actions" || true

Check current branch:

git branch --show-current
Notes for agents

The user often tests changes by building Android APKs and installing them on a real device.
A feature is not considered complete until it is visible or usable in the APK.
