#include "http-download.h"
#include "curl.h"
#include "debug.h"

CHttpDownload::CHttpDownload(std::string url,
                             std::string fileName,
                             const int jobNum = 1,
                             const int version = HTTP_VERSION_1_1)
             :CDownload(url, fileName, jobNum)
{
    m_version = HTTP_VERSION_VALID(version) ? version : HTTP_VERSION_NONE;
}

CHttpDownload::~CHttpDownload()
{}

long CHttpDownload::HttpCurlVersion()
{
    switch (m_version)
    {
    case HTTP_VERSION_NONE:
        return CURL_HTTP_VERSION_NONE;
    case HTTP_VERSION_1_0:
        return CURL_HTTP_VERSION_1_0;
    case HTTP_VERSION_1_1:
        return CURL_HTTP_VERSION_1_1;
    case HTTP_VERSION_2_0:
        return CURL_HTTP_VERSION_2_0;
    default:
        return CURL_HTTP_VERSION_NONE;
    }
}

int CHttpDownload::Start()
{
    int index;
    for (index = 0; index < m_jobNum; ++index)
    {
        curl_easy_setopt(m_jobs[index].curl, CURLOPT_HTTP_VERSION, HttpCurlVersion());
    }
    return CDownload::Start();
}