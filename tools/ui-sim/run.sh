#!/usr/bin/env bash
# Build and drive the host UI on macOS/Linux through Docker Desktop/Engine.
set -euo pipefail
repo_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
scenario_path=${1:-"$repo_dir/tools/ui-sim/smoke.json"}
scenario_path=$(python3 -c 'from pathlib import Path; import sys; print(Path(sys.argv[1]).resolve(strict=True))' "$scenario_path")
mkdir -p "$repo_dir/.cache"
run_dir=$(mktemp -d "$repo_dir/.cache/ui-sim-run-XXXXXXXX")
image_name=${AURORA_UI_SIM_IMAGE:-aurora-ui-sim:local}
docker build -t "$image_name" -f "$repo_dir/tools/ui-sim/Dockerfile" "$repo_dir/tools/ui-sim" > "$run_dir/image-build.log" 2>&1 || { cat "$run_dir/image-build.log"; exit 1; }
image_arch=$(docker image inspect "$image_name" --format '{{.Architecture}}')
build_volume="aurora-ui-sim-noble-$image_arch"
docker run --rm --volume "$repo_dir:/source:ro" --volume "$build_volume:/build" "$image_name" \
    bash -lc 'cmake -S /source -B /build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=OFF -DBUILD_UI_SIM=ON && cmake --build /build --target aurora-ui-sim -j6' \
    > "$run_dir/compile.log" 2>&1 || { cat "$run_dir/compile.log"; exit 1; }
mkdir "$run_dir/artifacts"
printf 'UI simulator artifacts: %s\n' "$run_dir/artifacts"
# Runtime has no network access, including to paired PCs. No TV or credentials are mounted.
docker run --rm --network none --volume "$repo_dir:/source:ro" --volume "$build_volume:/build:ro" \
    --env "AURORA_UI_SIM_SIZE=${AURORA_UI_SIM_SIZE:-1920x1080}" \
    --volume "$run_dir/artifacts:/artifacts" --volume "$scenario_path:/scenario.json:ro" \
    "$image_name" python3 /source/tools/ui-sim/run.py --binary /build/tools/ui-sim/aurora-ui-sim \
    --scenario /scenario.json --output /artifacts
