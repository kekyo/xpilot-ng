# libtess2 upstream provenance

Upstream repository: https://github.com/memononen/libtess2

Pinned upstream commit:
`8dbd6483e920311a58c9af10a10beb278efebc36`

Commit page:
https://github.com/memononen/libtess2/commit/8dbd6483e920311a58c9af10a10beb278efebc36

Source archive:
https://codeload.github.com/memononen/libtess2/tar.gz/8dbd6483e920311a58c9af10a10beb278efebc36

SHA-256 of the downloaded gzip archive:
`083f507dd3b27eba01fa540b9efe3bbeed36d5bd4ea0e8869897d21f52c5f98b`

SHA-256 of the uncompressed tar stream:
`c98957d98aeadb5357a84ed4f3821b69b7a1301a334a7efae495b9b5eb10ed18`

The archive hashes can be reproduced with:

```sh
curl --fail --location \
  https://codeload.github.com/memononen/libtess2/tar.gz/8dbd6483e920311a58c9af10a10beb278efebc36 \
  --output libtess2-8dbd6483e920311a58c9af10a10beb278efebc36.tar.gz
sha256sum libtess2-8dbd6483e920311a58c9af10a10beb278efebc36.tar.gz
gzip -dc libtess2-8dbd6483e920311a58c9af10a10beb278efebc36.tar.gz \
  | sha256sum
```

The following upstream files are vendored:

- `Include/tesselator.h`
- `Source/bucketalloc.c`
- `Source/bucketalloc.h`
- `Source/dict.c`
- `Source/dict.h`
- `Source/geom.c`
- `Source/geom.h`
- `Source/mesh.c`
- `Source/mesh.h`
- `Source/priorityq.c`
- `Source/priorityq.h`
- `Source/sweep.c`
- `Source/sweep.h`
- `Source/tess.c`
- `Source/tess.h`
- `LICENSE.txt`
- `README.md`

This is the complete source and header set of the upstream `libtess2` Bazel
library target at the pinned commit, plus its license and README. Every listed
file is copied byte-for-byte from the upstream archive and is unmodified.
`UPSTREAM.md` is local provenance documentation and is not an upstream file.

libtess2 is distributed under the SGI Free Software License B, Version 2.0.
The complete upstream license notice is retained in `LICENSE.txt`.
