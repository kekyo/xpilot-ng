#!/bin/sh

set -eu

object_file=./test_sdlpaint_stage-sdlpaint.o
symbols_file=./run-sdlpaint-matrix-boundary.symbols

cleanup()
{
    rm -f -- "$symbols_file"
}

trap cleanup EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

if test ! -f "$object_file"; then
    echo "SDL paint test object is missing: $object_file" >&2
    exit 1
fi
if ! nm -u "$object_file" > "$symbols_file"; then
    echo "Could not inspect SDL paint symbols: $object_file" >&2
    exit 1
fi

for symbol in $(awk '{ print $NF }' "$symbols_file"); do
    case "$symbol" in
        _gl*)
            symbol=${symbol#_}
            ;;
    esac
    case "$symbol" in
        glMatrixMode|glPushMatrix|glPopMatrix|glLoadIdentity|glTranslatef|glScalef|gluOrtho2D)
            ;;
        gl*)
            echo "SDL paint bypasses the matrix-only OpenGL boundary: $symbol" >&2
            exit 1
            ;;
    esac
done
