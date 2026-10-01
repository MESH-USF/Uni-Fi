# Repository maintenance

## Git layout in this workspace

The managed workspace provides an empty root `.git` directory as a read-only
mount. That prevents `git init` from creating a normal repository there. Uni-Fi
therefore stores the repository database in the ignored `.git-data/` directory
and uses `scripts/gitw` to point Git at the normal project work tree.

```bash
./scripts/gitw status
./scripts/gitw diff
./scripts/gitw add path/to/file
./scripts/gitw commit -m "Describe the change"
```

`.git-data/` contains actual Git objects, refs, config, and commit history. It
is not a source-code substitute or a generated snapshot.

## Convert to a conventional checkout

Only do this after copying the project outside the managed environment, where
the placeholder `.git` mount is absent or writable:

```bash
rmdir .git
mv .git-data .git
git config --unset core.worktree
git status
```

The stored repository uses `core.worktree=..`; unsetting it lets a conventional
root `.git` directory infer its work tree normally. Do not delete `.git-data`
before confirming the conversion and commit history.

## Contribution practice

- Keep commits focused and action-oriented.
- Append meaningful work and validation results to `docs/DEVELOPMENT_LOG.md`.
- Add or supersede an ADR when changing security, radio behavior, retention,
  emergency semantics, hardware feedback, or the radio/phone boundary.
- Never commit `firmware/examples/companion_radio/UniFiProvisioning.h`, keys,
  signing material, device inventories containing secrets, or generated builds.
