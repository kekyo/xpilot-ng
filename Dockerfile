ARG DEBIAN_IMAGE=docker.io/library/debian:trixie-slim@sha256:d7e12182ce18b85b93007c1dedf31f2d29e01ccf3182cc4017c709b6259bc132

FROM ${DEBIAN_IMAGE} AS builder

ARG XPILOT_BUILD_JOBS=2
ARG XPILOT_VERSION=development

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        build-essential \
        libexpat1-dev \
        pkgconf \
        zlib1g-dev; \
    rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . .

RUN set -eux; \
    test -x ./configure; \
    mkdir -p /build /out/usr/games \
        /out/usr/share/games/xpilot-infinity \
        /out/usr/share/doc/xpilot-infinity-server; \
    cd /build; \
    /workspace/configure \
        --prefix=/usr \
        --bindir=/usr/games \
        --datadir=/usr/share/games \
        --disable-sdl-client \
        --disable-x11-client \
        --disable-replay \
        --disable-xp-mapedit \
        --disable-sound; \
    make -C src/common -j"$XPILOT_BUILD_JOBS" \
        "XPILOT_VERSION=$XPILOT_VERSION"; \
    make -C src/server -j"$XPILOT_BUILD_JOBS" \
        "XPILOT_VERSION=$XPILOT_VERSION"; \
    install -m 0755 src/server/xpilot-infinity-server \
        /out/usr/games/xpilot-infinity-server; \
    strip --strip-unneeded --remove-section=.comment --remove-section=.note \
        /out/usr/games/xpilot-infinity-server; \
    install -m 0644 \
        /workspace/lib/defaults.txt \
        /workspace/lib/password.txt \
        /workspace/lib/robots.txt \
        /workspace/lib/shipshapes.txt \
        /out/usr/share/games/xpilot-infinity/; \
    make -C lib/maps install-data DESTDIR=/out; \
    install -m 0644 /workspace/COPYING \
        /out/usr/share/doc/xpilot-infinity-server/COPYING

FROM ${DEBIAN_IMAGE} AS runtime

ARG DEBIAN_IMAGE
ARG XPILOT_REVISION=unknown
ARG XPILOT_SOURCE=https://github.com/kekyo/xpilot-infinity
ARG XPILOT_VERSION=development

LABEL org.opencontainers.image.title="XPilot Infinity Server" \
      org.opencontainers.image.description="Dedicated XPilot Infinity game server" \
      org.opencontainers.image.source="$XPILOT_SOURCE" \
      org.opencontainers.image.documentation="$XPILOT_SOURCE#readme" \
      org.opencontainers.image.version="$XPILOT_VERSION" \
      org.opencontainers.image.revision="$XPILOT_REVISION" \
      org.opencontainers.image.licenses="GPL-2.0-or-later" \
      org.opencontainers.image.base.name="$DEBIAN_IMAGE"

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends libexpat1 zlib1g; \
    rm -rf /var/lib/apt/lists/*; \
    groupadd --gid 10001 xpilot; \
    useradd --uid 10001 --gid 10001 \
        --home-dir /var/lib/xpilot-infinity-server \
        --no-create-home --shell /usr/sbin/nologin xpilot; \
    install -d -o 10001 -g 10001 -m 0750 \
        /var/lib/xpilot-infinity-server

COPY --from=builder /out/ /

WORKDIR /var/lib/xpilot-infinity-server
USER 10001:10001

EXPOSE 15345/tcp
STOPSIGNAL SIGTERM

ENTRYPOINT ["/usr/games/xpilot-infinity-server"]
CMD ["-noQuit", "+reportMeta", "-map", "ndh.xp2", "-tcp"]
