#ifndef _H_CDOWNLOAD_H_
#define _H_CDOWNLOAD_H_

#include <stdio.h>

class CDownload {
private:
    char *m_url;
    char *m_filename;
    int m_thdCnt;
    void *m_thds;
public:
    CDownload(const char *url, const char *outfile, const int thdCnt);
    ~CDownload();
    int Start();
private:
    ssize_t GetSize();
    int MergeFiles();
};

#endif /* _H_CDOWNLOAD_H_ */
