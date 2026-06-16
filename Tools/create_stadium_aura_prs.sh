#!/usr/bin/env bash
set -euo pipefail
REPO="svg561/stadium-aura"
cd "$(dirname "$0")/.."

if ! gh auth status -h github.com &>/dev/null; then
  echo "ERROR: gh not authenticated. Run: gh auth login --web"
  echo "   or: export GH_TOKEN=<token with repo scope>"
  exit 1
fi

pr_body() {
  local summary="$1"
  cat <<EOF
## Summary
${summary}

## Files changed
See the **Compare** tab on this PR for the full file list.

## Risk
Review diff size and DSP/UI touch points; stacked base means merge order matters.

## Testing checklist
- [ ] Clean checkout of head branch builds (CMake/Xcode as documented in PR1)
- [ ] Standalone/plugin loads without crash
- [ ] Smoke-test audio path affected by this PR
- [ ] No regressions in prior stack PRs once merged in order

## Notes
Commit \`3caec08\` is excluded from this stack. No build artifacts or junk files included.
EOF
}

create_if_missing() {
  local head="$1" base="$2" title="$3" summary="$4" draft="${5:-false}"
  if gh pr list --repo "$REPO" --head "$head" --json number,url --jq '.[0].url' 2>/dev/null | grep -q github; then
    echo "EXISTS: $head -> $(gh pr list --repo "$REPO" --head "$head" --json url --jq '.[0].url')"
    return 0
  fi
  local args=(pr create --repo "$REPO" --head "$head" --base "$base" --title "$title" --body "$(pr_body "$summary")")
  [[ "$draft" == "true" ]] && args+=(--draft)
  gh "${args[@]}"
}

# PR1
create_if_missing "pr/01-juce-build-docs" "main" \
  "PR1: JUCE build and docs" \
  "Foundation: JUCE submodule/build wiring and documentation for Stadium Aura."

# PR2
create_if_missing "pr/02-standalone-audio-io" "pr/01-juce-build-docs" \
  "PR2: Standalone audio I/O" \
  "Standalone app audio device I/O on top of the JUCE build foundation."

# PR3 (draft)
create_if_missing "pr/03-dsp-core-eq-chain" "pr/01-juce-build-docs" \
  "PR3: DSP core EQ chain" \
  "Core EQ/DSP chain; stacks on PR1 (parallel to PR2)." "true"

# PR4 (draft)
create_if_missing "pr/04-compressor-engine" "pr/03-dsp-core-eq-chain" \
  "PR4: Compressor engine" \
  "Compressor DSP engine stacked on the core EQ chain." "true"

# PR5+6
create_if_missing "pr/05-expanded-eq-and-layout" "pr/03-dsp-core-eq-chain" \
  "PR5+6: Expanded EQ and layout" \
  "Expanded EQ behavior and related layout work on the DSP core branch."

# PR7+8 (draft)
create_if_missing "pr/07-aura-big" "pr/03-dsp-core-eq-chain" \
  "PR7+8: Aura big UI/feature slice" \
  "Large Aura UI/feature slice; review as draft before ready." "true"

# PR9
create_if_missing "pr/09-compressor-ui" "pr/04-compressor-engine" \
  "PR9: Compressor UI" \
  "Compressor user interface wired to the compressor engine (PR4)."

# PR10
create_if_missing "pr/10-repo-hygiene" "pr/01-juce-build-docs" \
  "PR10: Repo hygiene" \
  "Repository hygiene cleanup stacked on PR1."

# PR11
create_if_missing "pr/11-mic-character" "pr/03-dsp-core-eq-chain" \
  "PR11: Mic character" \
  "Mic character processing on the DSP core EQ chain."

# PR12
create_if_missing "pr/12-vocal-chain-presets" "pr/11-mic-character" \
  "PR12: Vocal chain presets" \
  "Vocal chain factory presets stacked on mic character (PR11)."
