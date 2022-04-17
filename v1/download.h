#ifndef __DOWNLOAD_H__
#define __DOWNLOAD_H__

#include <string>
#include <pthread.h>
#include "curl.h"

class CDownload;

typedef struct download_jobs_st{
    CURL *curl;
    pthread_t thdid;
    int fd;
    long start; /*存储于本地文件的偏移量开始*/
    long end;   /*存储于本地文件的偏移量结尾*/
    long offset;/*相较于start的偏移*/
    CDownload *download;
}download_jobs_t;

class CDownload{
public:
    pthread_mutex_t m_mutex;
    int m_workJobNum;
protected:
    std::string m_url;
    long m_size;
    std::string m_fileName;
    int m_fd;
    int m_jobNum;
    download_jobs_t *m_jobs;
    int m_checkopt;
    std::string m_checksumFileName;
    
public:
    CDownload(std::string url, 
              std::string fileName,
              const int jobNum);
    ~CDownload();
    int SetCheckParam(const int opt,
                      std::string checksumFileName);
    int Start();
    long GetSize();

private:
    int ChecksumVerify();
};

#endif /*__DOWNLOAD_H__*/
