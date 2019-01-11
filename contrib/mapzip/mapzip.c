#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <zlib.h>

#define COPY_BUF_SIZE 8192
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

void error (const char *fmt, ...)
{
    va_list ap;
    int e = errno;

    va_start(ap, fmt);
    fprintf(stderr, "mapzip: ");
    vfprintf(stderr, fmt, ap);
    if (e != 0) fprintf(stderr, " (%s)", strerror(e));
    fprintf(stderr,"\n");
    va_end(ap);
}


int concat (char *name, char *files[], int fcount) {

    int i;
    size_t rlen;
    long int size;
    FILE *in;
    gzFile out;
    char xpd_name[256], buf[COPY_BUF_SIZE];
    struct stat info;

    printf("creating xpilot data file %s.xpd\n", name);
    sprintf(xpd_name, "%s.xpd", name);
    if ((out = gzopen(xpd_name, "wb")) == NULL) {
        error("failed to open %s for writing", xpd_name);
        return 0;
    }
    
    if (!gzprintf(out,"XPD %d\n", fcount)) {
        error("failed to write header to %s", xpd_name);
        gzclose(out);
        return 0;
    }

    for (i = 0; i < fcount; i++) {

        if (stat(files[i], &info) == -1) {
            error("failed to get stat for %s", files[i]);
            gzclose(out);
            return 0;
        }

        if ((in = fopen(files[i], "r")) == NULL) {
            error("failed to open %s for reading", files[i]);
            gzclose(out);
            return 0;
        }

        size = (long int)info.st_size;
        printf("storing %s (%ld)\n", files[i], size, xpd_name);
        if (!gzprintf(out, "%s %ld\n", files[i], size)) {
            error("failed to write file info");
            gzclose(out);
            return 0;
        }

        while (size > 0) {

            rlen = fread(buf, 1, COPY_BUF_SIZE, in);
            if (!rlen) {
                if (ferror(in)) {
                    error("error when reading %s", files[i]);
                    fclose(in);
                    gzclose(out);
                    return 0;

                } else if (feof(in)) {
                    error("unexpected end of file %s", files[i]);
                    fclose(in);
                    gzclose(out);
                    return 0;
                }
            }

            if (!gzwrite(out, buf, rlen)) {
                error("failed to write to %s", xpd_name);
                fclose(in);
                gzclose(out);
                return 0;
            }

            size -= rlen;
        }

        fclose(in);
    }

    if (gzclose(out)) {
        error("failed to close file %s properly", xpd_name);
        return 0;
    }

    return 1;
}


int split (char *name) {

    gzFile in;
    FILE *out;
    size_t rlen, wlen;
    char xpd_name[256], buf[COPY_BUF_SIZE], fname[256];
    long int size;
    int count, i;


    if (mkdir(name, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
        error("failed to create directory %s", name);
        return 0;
    }

    sprintf(xpd_name, "%s.xpd", name);
    if ((in = gzopen(xpd_name, "rb")) == NULL) {
        error("failed to open %s for reading", xpd_name);
        return 0;
    }

    if (gzgets(in, buf, COPY_BUF_SIZE) == Z_NULL) {
        error("failed to read header from %s", xpd_name);
        return 0;
    }

    if (sscanf(buf, "XPD %d\n", &count) != 1) {
        error("invalid header in %s", xpd_name);
        return 0;
    }    

    for (i = 0; i < count; i++) {

        if (gzgets(in, buf, COPY_BUF_SIZE) == Z_NULL) {
            error("failed to read file info from %s", xpd_name);
            return 0;
        }

        sprintf(fname, "%s/", name);

        if (sscanf(buf, "%s\n%ld\n", fname + strlen(name) + 1, &size) != 2) {
            error("failed to read file info from %s", xpd_name);
            gzclose(in);
            return 0;
        }

        printf("extracting %s (%ld)\n", fname, size);

        if ((out = fopen(fname, "w")) == NULL) {
            error("failed to open %s for writing", buf);
            gzclose(in);
            return 0;
        }

        while (size > 0) {
            rlen = gzread(in, buf, MIN(COPY_BUF_SIZE, size));
            if (rlen == -1) {
                error("error when reading %s", xpd_name);
                gzclose(in);
                fclose(out);
                return 0;
            }
            if (rlen == 0) {
                error("unexpected end of file %s", xpd_name);
                gzclose(in);
                fclose(out);
                return 0;
            }

            wlen = fwrite(buf, 1, rlen, out);
            if (wlen != rlen) {
                error("failed to write to %s", fname);
                gzclose(in);
                fclose(out);
                return 0;
            }
            
            size -= rlen;
        }
        
        fclose(out);
    }

    gzclose(in);
    return 1;
}

void usage (void)
{
    fprintf(stderr, "usage: mapzip [-c name input-files] [-x name]\n");
}

int main (int argc, char *argv[])
{
    if (argc < 3) {
        usage();
        return 1;
    }

    if (strcmp("-c", argv[1]) == 0) {
        if (argc < 4) {
            usage();
            return 1;
        }
        if (!concat(argv[2], argv + 3, argc - 3)) {
            fprintf(stderr,"concatenating files into %s failed\n", argv[2]);
            return 1;
        }
        return 1;
    } 

    if (strcmp("-x", argv[1]) == 0) {
        if (!split(argv[2])) {
            fprintf(stderr,"splitting %s failed\n", argv[2]);
            return 1 ;
        }
    }

    return 0;
}
