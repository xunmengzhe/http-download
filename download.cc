#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>

#include "curl.h"
#include "download.h"
#include "debug.h"
#include "md5.h"

static size_t download_write_cb(char *ptr, 
                                size_t size, 
                                size_t nmemb, 
                                void *userdata)
{
    download_jobs_t *job = (download_jobs_t *)userdata;
    size_t len = 0;
    size_t wlen = 0;
    //printf("thdid:%d, size:%lu, nmemb:%lu.\n", pthread_self(), size, nmemb);
    pthread_mutex_lock(&job->download->m_mutex);
    lseek(job->fd, job->start + job->offset, SEEK_SET);
    if (((job->start + job->offset) + size * nmemb) < job->end)
        wlen = size * nmemb;
    else
        wlen = job->end - job->start - job->offset + 1;
    while (len < wlen)
    {
        len += write(job->fd, ptr + len, wlen - len);
        fsync(job->fd);
    }
    job->offset += len;
    pthread_mutex_unlock(&job->download->m_mutex);
    return len;
}

static void *download_thd_func(void *arg)
{
    download_jobs_t *job = (download_jobs_t *)arg;
    printf("thdid:%ld, start:%ld, end:%ld, offset:%ld.\n",
        job->thdid, job->start, job->end, job->offset);
    curl_easy_perform(job->curl);
    pthread_mutex_lock(&job->download->m_mutex);
    --job->download->m_workJobNum;
    pthread_mutex_unlock(&job->download->m_mutex);
    return arg;
}

CDownload::CDownload(std::string url, 
                     std::string fileName,
                     const int jobNum = 1)
           : m_url(url), m_fileName(fileName)
{
    size_t pos = fileName.find_last_of('/');
    int index;
    CURL *curl;
    char range[128];
    long fragmentSize;
    if (std::string::npos != pos)
    {
        std::string filePath = fileName.substr(0, pos);
        if (0 != access(filePath.c_str(), F_OK))
            mkdir(filePath.c_str(), 0666);
    }
    if (0 == access(fileName.c_str(), W_OK))
        m_fd = open(fileName.c_str(), O_WRONLY | O_TRUNC);
    else
        m_fd = open(fileName.c_str(), 
                O_WRONLY | O_CREAT, 
                S_IRUSR | S_IWUSR | S_IWUSR | S_IWGRP | S_IROTH | S_IWOTH);

    pthread_mutex_init(&m_mutex, NULL);
    m_checkopt = 0;
    m_size = GetSize();
    m_workJobNum = 0;
    m_jobNum= jobNum > 0 ? jobNum : 1;
    m_jobs = new download_jobs_t[m_jobNum];
    fragmentSize = (m_size + m_jobNum - 1) / m_jobNum;
    printf("m_size:%ld, m_jobNum:%d, fragmentSize:%ld.\n", 
        m_size, m_jobNum, fragmentSize);
    for (index = 0; index < m_jobNum; ++index)
    {
        m_jobs[index].download = this;
        m_jobs[index].fd = m_fd;
        m_jobs[index].start = index * fragmentSize;
        m_jobs[index].end = (index + 1) * fragmentSize - 1;
        m_jobs[index].end = m_jobs[index].end >= m_size ? m_size-1 : m_jobs[index].end;
        m_jobs[index].offset = 0;
        bzero(range, sizeof(range));
        snprintf(range, sizeof(range), "%ld-%ld", m_jobs[index].start, m_jobs[index].end);
        curl = curl_easy_init();
        m_jobs[index].curl = curl;
        curl_easy_setopt(curl, CURLOPT_URL, m_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, download_write_cb);
        curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *)&m_jobs[index]);
        curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1L);  
        curl_easy_setopt (curl, CURLOPT_LOW_SPEED_LIMIT, 1L);  
        curl_easy_setopt (curl, CURLOPT_LOW_SPEED_TIME, 5L);  
        curl_easy_setopt (curl, CURLOPT_RANGE, range);
        printf("range:%s.\n", range);
    }
}

CDownload::~CDownload()
{
    int index;
    if (NULL == m_jobs)
        return;
    pthread_mutex_lock(&m_mutex);
    for (index = 0; index < m_jobNum; ++index)
    {
        if (NULL != m_jobs[index].curl)
        {
            curl_easy_cleanup(m_jobs[index].curl);
            m_jobs[index].curl = NULL;
        }
    }
    if (m_fd >= 0)
    {
        close(m_fd);
        m_fd = -1;
    }
    pthread_mutex_unlock(&m_mutex);
    pthread_mutex_destroy(&m_mutex);
}

long CDownload::GetSize()
{
    double size = 0;
    CURLcode res;
    CURL *curl = curl_easy_init();
    const char *url = m_url.c_str();
    if (NULL == curl)
    {
        DB_ERR("curl_easy_init() fail.");
        goto err;
    }
    res = curl_easy_setopt(curl, CURLOPT_URL, url);
    if (CURLE_OK != res)
    {
        DB_ERR("curl_easy_setopt() fail for option 'CURLOPT_URL'.");
        goto err;
    }
    /*Pass in onoff set to 1 to tell the library to include the header 
     *in the body output for requests with this handle. */
    res = curl_easy_setopt(curl, CURLOPT_HEADER, 1);
    if (CURLE_OK != res)
    {
        DB_ERR("curl_easy_setopt() fail for option 'CURLOPT_HEADER'.");
        goto err;
    }
    /*A long parameter set to 1 tells libcurl to not include the body-part 
     *in the output when doing what would otherwise be a download*/
    res = curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
    if (CURLE_OK != res)
    {
        DB_ERR("curl_easy_setopt() fail for option 'CURLOPT_NOBODY'.");
        goto err;
    }
    res = curl_easy_perform(curl);
    if (CURLE_OK != res)
    {
        DB_ERR("curl_easy_perform() fail.");
        goto err;
    }
    /*Pass a pointer to a curl_off_t to receive the content-length of the download. 
     *This is the value read from the Content-Length: field. 
     *Stores -1 if the size isn't known*/
    res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &size);
    if (CURLE_OK != res)
    {
        DB_ERR("curl_easy_getinfo() fail.");
        goto err;
    }
    DB_INF("fileSize:%.0f", size);
    curl_easy_cleanup(curl);
    return size;
err:
    if (NULL != curl)
    {
        curl_easy_cleanup(curl);
    }
    return -1;
}

int CDownload::SetCheckParam(const int opt,
                             std::string fileName)
{
    m_checkopt = opt;
    m_checksumFileName = fileName;
    return 0;
}

int CDownload::ChecksumVerify()
{
    if (0 != m_checkopt && !m_checksumFileName.empty())
    {
        if (1 == m_checkopt)
        {
            char md5Value[16] = {0};
            if (0 == MD5File(m_fileName.c_str(), md5Value))
            {
                char md5ValueStr1[33] = {0};
                char md5ValueStr2[33] = {0};
                char *p = md5ValueStr1;
                int ch;
                int index;
                int fd;
                for (index = 0; index < 16; ++index, p += 2)
                {
                    ch = md5Value[index];
                    ch &= 0x000000ff;
                    sprintf(p, "%02x", ch);
                }
                if (0 != access(m_checksumFileName.c_str(), R_OK))
                {
                    printf("Checksum file '%s' is not exist OR don't have read permissions.\n", 
                        m_checksumFileName.c_str());
                    return -1;
                }
                fd = open(m_checksumFileName.c_str(), O_RDONLY);
                ASSERT(fd >= 0);
                read(fd, md5ValueStr2, 32);
                printf("md5Value1[%s]\nmd5Value2[%s]\n", md5ValueStr1, md5ValueStr2);
                if (0 == memcmp(md5ValueStr1, md5ValueStr2, 32))
                {
                    printf("File verify check successfully.\n");
                    close(fd);
                    return 0;
                }
                else
                {
                    printf("File verify check failure.\n");
                    close(fd);
                    return -1;
                }
            }
            else
            {
                printf("File verify check failure.\n");
                return -1;
            }
        }
    }
    return 0;
}

int CDownload::Start()
{
    int index;
    printf("---------- Starting download [url:%s, fileSize:%ld, jobNum:%d] ...... ----------\n", 
        m_url.c_str(), m_size, m_jobNum);
    for (index = 0; index < m_jobNum; ++index)
    {
        pthread_create(&(m_jobs[index].thdid), NULL, download_thd_func, &m_jobs[index]);
        pthread_mutex_lock(&m_mutex);
        ++m_workJobNum;
        pthread_mutex_unlock(&m_mutex);
    }
    pthread_mutex_lock(&m_mutex);
    while (m_workJobNum > 0)
    {
        pthread_mutex_unlock(&m_mutex);
        usleep(10000);
        pthread_mutex_lock(&m_mutex);
    }
    pthread_mutex_unlock(&m_mutex);
    ChecksumVerify();
    return 0;
}