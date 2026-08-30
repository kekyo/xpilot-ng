#!/bin/sh

set -eu

project_root=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
container_builder=${XPILOT_CONTAINER_BUILDER:-$project_root/build_container_image.sh}
container_engine=${CONTAINER_ENGINE:-podman}
metadata_resolver=${XPILOT_BUILD_METADATA_RESOLVER:-$project_root/config/resolve-build-metadata.sh}
resolved_metadata=$("$metadata_resolver")
version=$(printf '%s\n' "$resolved_metadata" | sed -n '1p')
revision=$(printf '%s\n' "$resolved_metadata" | sed -n '2p')
local_manifest=localhost/xpilot-infinity-server:"$version"-multi
remote_image=docker.io/kekyo/xpilot-infinity-server:"$version"

"$container_builder" \
  --version "$version" \
  --revision "$revision" \
  --platform linux/amd64,linux/arm64 \
  --tag "$local_manifest"

"$container_engine" manifest push --all \
  "$local_manifest" \
  "docker://$remote_image"

"$container_engine" manifest push --all \
  "$local_manifest" \
  docker://docker.io/kekyo/xpilot-infinity-server:latest
