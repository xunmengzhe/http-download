#include "http-download.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static void usage(void)
{
    printf("Http file download usage:\n");
    printf("  -u url: Download file url.\n");
    printf("  -f filename: File local save name.\n");
    printf("  -j number: Work thread number.\n");
    printf("  -v num: Http version. 0:none; 1:http-1.0; 2:http-1.1; 3:http-2.0.\n");
    printf("  -m checksum-filename: Use MD5 checkout download file.\n");
    printf("  -h: Display this help and exit.\n");
}

int main(int argc, char **argv)
{
    int ch;
    std::string url;
    std::string fileName;
    int jobNum = 1;
    int version = HTTP_VERSION_NONE;
    int checkopt = 0; /*0:none; 1:md5*/
    std::string checkFileName;
    while ((ch = getopt(argc, argv, "u:f:h:v::j:m:")) != -1)
    {
        switch (ch)
        {
        case 'u':
            url = optarg;
            break;
        case 'f':
            fileName = optarg;
            break;
        case 'j':
            jobNum = atoi(optarg);
            if (jobNum <= 0)
                jobNum = 1;
            break;
        case 'v':
            version = atoi(optarg);
            if (!HTTP_VERSION_VALID(version))
                version = HTTP_VERSION_1_1;
            break;
        case 'm':
            checkopt = 1;
            checkFileName = optarg;
            break;
        case 'h':
            usage();
            exit(0);
        default:
            printf("Unknown option: %c\n", (char)optopt);
            usage();
            exit(1);
        }
    }
    if (url.empty())
    {
        printf("Please given URL directly.");
        usage();
        exit(1);
    }
    if (fileName.empty())
    {
        printf("Please given local save filename directly.");
        usage();
        exit(1);
    }
    CHttpDownload http(url, fileName, jobNum, version);
    if (0 != checkopt)
        http.SetCheckParam(checkopt, checkFileName);
    return http.Start();
}
