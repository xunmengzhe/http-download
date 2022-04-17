#include "CDownload.h"
#include <stdio.h>
#include <stdlib.h>

static void
usage(const char *progname)
{
    printf("%s url filename thdcnt\n", progname);
}

int
main(int argc, char **argv)
{
    if (argc < 4) {
        printf("Invalid param.\n");
        usage(argv[0]);
        return 0;
    }
    char *url = argv[1];
    char *filename = argv[2];
    int thdcnt = atoi(argv[3]);
    CDownload dl(url, filename, thdcnt);
    //CDownload dl("https://www.gnu.org/software/make/manual/make.pdf", "out.pdf", 8);
    //CDownload dl("https://www.rfc-editor.org/rfc/pdfrfc/rfc1034.txt.pdf", "out.pdf", 3);
    dl.Start();
    return 0;
}
