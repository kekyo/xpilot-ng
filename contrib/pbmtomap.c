/*
 * Written by Greg Renda (greg@ncd.com)
 * 99/10/09 revised by Ben Armstrong (synrg@sanctuary.nslug.ns.ca)
 * - supported bit-packed pbm variant
 * - fixed handling of whitespace in pixel data
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* The man-page for pbm says no line can exceed 70 chars (except, of
   course, the pixel-data in bit-packed mode, but we don't buffer
   that through 'buf' */
#define SIZE 70
#define WHITESPACE " \t\r\n"

static void
fatalError(message, arg1, arg2)
char           *message, *arg1, *arg2;
{
    fprintf(stderr, message, arg1, arg2);
    fprintf(stderr, ".\n");
    exit(1);
}

#define NEXT_TOKEN()							       \
    state++;								       \
    if (!(p = strtok(NULL, WHITESPACE)))                                       \
        break;

char            buf[SIZE], *p;
int             state = 0, width, height, count = 0;

void process_bit_packed()
{
    int c,mask;

    putchar('\n');
    while ((c = fgetc(stdin)) != EOF)
    {
        for (mask = 128 ; mask > 0 ; mask = mask >> 1)
        {
            if ((c & mask) == mask)
                putchar('x');
            else
                putchar(' ');
            count++;
            if (!(count % width))
            {
		/* end of row, skip over remaining bits
                   to next char */
                putchar('\n');
                break;
            }
        }
    }
}

void process_ascii()
{
    while (*p)
    {
        /* Break output into 'width' wide rows, which is not
           necessarily at the end of a line of input */
	if (!(count % width))
            putchar('\n');
        if (*p == '1')
        {
	    putchar('x');
	    count++;
        }
        else if (*p == '0')
        {
	    putchar(' ');
	    count++;
        }
        p++;
    }
}

int
main()
{
    char *n;
    int bit_packed;

    while (fgets(buf, SIZE, stdin))
    {
        /* Comments can only start at beginning of line.  I'm not sure
           if this is an accurate interpretation of the pbm spec. */
	p = strchr(buf, '#');
	if (p)
	    *p = 0;

        /* Blank lines are ignored */
	if (!(p = strtok(buf, WHITESPACE)))
            continue;

        /* Each case is responsible for getting the next token before
           iterating thru the loop again, except in bit-packed mode,
           there can only be one whitespace char between height and the
           pixel data.  When all tokens are exhausted, we fall through
           to the outer loop to get another line, except in bit-packed
           mode which processes all remaining input and does not return. */
	while (p)
        {
	    switch (state)
	    {
	    case 0:
		bit_packed = !strcmp(p, "P4");
		if (strcmp(p, "P1") && !bit_packed)
		    fatalError("Unrecognized input format");
		NEXT_TOKEN();
	    case 1:
		width = atoi(p);
		NEXT_TOKEN();
	    case 2:
		height = atoi(p);
		printf("%dx%d\n", width, height);
		printf("0\n");
		printf("This map hasn't got a name, please give it one\n");
		printf("%s", (n = getenv("USER")) ? n : "Unknown");
		count = 0;
		if (bit_packed)
                    state++;
                else
                {
		    NEXT_TOKEN();
                }
	    default:
                if (bit_packed)
                {
                    /* In bit-packed mode, there are no line-breaks,
                       so all remaining chars in the input stream
                       need to be read by process_bit_packed() and
                       then we exit. */
                    process_bit_packed();
                    exit(0);
                }
                else
                {
		    /* ASCII mode processes a single token and then
                       iterates through the token loop again */
                    process_ascii();
                    NEXT_TOKEN();
                }
            }
        }
    }

    /* Some people seem to be unwilling to learn that
     * every program must return a defined value.
     */
    return 0;
}
