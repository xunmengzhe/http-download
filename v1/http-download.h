#ifndef __HTTP_DOWNLOAD_H__
#define __HTTP_DOWNLOAD_H__

#include "download.h"

enum {
    HTTP_VERSION_NONE   = 0,
    HTTP_VERSION_1_0    = 1,/*HTTP-1.0*/
    HTTP_VERSION_1_1    = 2,/*HTTP-1.1*/
    HTTP_VERSION_2_0    = 3,/*HTTP-2.0*/
};
#define HTTP_VERSION_VALID(ver) ((ver) >= HTTP_VERSION_NONE && (ver) <= HTTP_VERSION_2_0)

class CHttpDownload : public CDownload{
private:
    int m_version;
public:
    CHttpDownload(std::string url, 
                  std::string fileName,
                  const int jobNum,
                  const int version);
    ~CHttpDownload();
    int Start();
private:
    long HttpCurlVersion();
};

#endif /*__HTTP_DOWNLOAD_H__*/
