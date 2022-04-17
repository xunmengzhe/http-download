#include "CDownload.h"
#include <errno.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>

class CFileReader {
private:
    FILE *m_fp;
    char *m_filename;
public:
    CFileReader(const char *filename);
    ~CFileReader();
    ssize_t Read(char *buf, size_t maxsize);
};

CFileReader::CFileReader(const char *filename)
        : m_fp(NULL)
{
    m_fp = fopen(filename, "rb");
    if (m_fp == NULL) {
        printf("fopen() for file:%s faile. errstr:%s\n",
                filename, strerror(errno));
        return;
    }
    m_filename = strdup(filename);
}

CFileReader::~CFileReader()
{
    if (m_filename != NULL) {
        free(m_filename);
        m_filename = NULL;
    }
    if (m_fp != NULL) {
        fclose(m_fp);
        m_fp = NULL;
    }
}

ssize_t
CFileReader::Read(char *buf, size_t maxsize)
{
    size_t rsize = fread(buf, 1, maxsize, m_fp);
    if (rsize == -1) {
        printf("read() from file:%s failed. errstr:%s\n",
                m_filename, strerror(errno));
        return -1;
    }
    return rsize;
}

class CFileWriter {
private:
    FILE *m_fp;
    char *m_filename;
public:
    CFileWriter(const char *filename);
    ~CFileWriter();
    size_t Write(char *buf, size_t size);
    ssize_t AppendWith(const char *filename);
};

CFileWriter::CFileWriter(const char *filename)
{
    m_fp = fopen(filename, "w+");
    if (m_fp == NULL) {
        printf("fopen() for file:%s faile. errstr:%s\n",
                filename, strerror(errno));
        return;
    }
    m_filename = strdup(filename);
}

CFileWriter::~CFileWriter()
{
    if (m_filename != NULL) {
        free(m_filename);
        m_filename = NULL;
    }
    if (m_fp != NULL) {
        fclose(m_fp);
        m_fp = NULL;
    }
    //unlink(m_filename);
}

size_t
CFileWriter::Write(char *buf, size_t size)
{
    size_t wlen, len;
    wlen = 0;
    while (wlen < size) {
        len = fwrite(buf+wlen, 1, size-wlen, m_fp);
        wlen += len;
    }
#if 0
    printf("m_fp:%p, thdid:%p, wlen:%lu, size:%lu, buf:%p\n",
            m_fp, pthread_self(), wlen, size, buf);
#endif
    return wlen;
}

ssize_t
CFileWriter::AppendWith(const char *filename)
{
    char buf[65536];
    ssize_t asize = 0;
    CFileReader reader(filename);
    while (1) {
        ssize_t rsize = reader.Read(buf, sizeof(buf)-1);
        if (rsize == -1) {
            printf("read() from file:%s append to file:%s failed.\n",
                    filename, m_filename);
            return -1;
        }
        if (rsize == 0) {
            printf("append file:%s to file:%s succeed. asize:%lu\n",
                    filename, m_filename, asize);
            return asize;
        }
        asize += Write(buf, rsize);
    }
    printf("append file:%s to file:%s succeed. asize:%lu\n",
            filename, m_filename, asize);
    return asize;
}

class CDownloadWorker {
private:
    int m_outfd;
    char *m_url;
    char m_range[128];
    size_t m_totalSize;
    size_t m_curSize;
    CURL *m_curl;
    CFileWriter *m_writer;
public:
    CDownloadWorker(const char *url, CFileWriter *writer,
            const size_t start, const size_t end);
    ~CDownloadWorker();
    int Start();
    size_t Write(char *buf, size_t size, size_t cnt);
};

static size_t
write_cb(char *buf, size_t size, size_t cnt, void *arg)
{
    CDownloadWorker *worker = (CDownloadWorker *)arg;
    return worker->Write(buf, size, cnt);
}

CDownloadWorker::CDownloadWorker(const char *url, CFileWriter *writer,
        const size_t start, const size_t end)
        : m_outfd(-1), m_totalSize(end-start+1),
        m_curSize(0), m_curl(NULL), m_writer(writer)
{
    snprintf(m_range, sizeof(m_range), "%ld-%ld", start, end);
    m_url = strdup(url);
}

CDownloadWorker::~CDownloadWorker()
{
    if (m_curl != NULL) {
        curl_easy_cleanup(m_curl);
        m_curl = NULL;
    }
    if (m_outfd != -1) {
        close(m_outfd);
        m_outfd = -1;
    }
    if (m_url != NULL) {
        free(m_url);
        m_url = NULL;
    }
}

int
CDownloadWorker::Start()
{
    m_curl = curl_easy_init();
    if (m_curl == NULL) {
        printf("curl_easy_init() failed.\n");
        return -1;
    }
    CURLcode res = curl_easy_setopt(m_curl, CURLOPT_URL, m_url);
    if (res != CURLE_OK) {
        printf("curl_easy_setopt() for CURLOPT_URL failed."
                " url:%s, range:%s, errstr:%s\n",
                m_url, m_range, curl_easy_strerror(res));
        return -1;
    }
    res = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, write_cb);
    if (res != CURLE_OK) {
        printf("curl_easy_setopt() for CURLOPT_WRITEFUNCTION failed."
                " url:%s, range:%s, errstr:%s\n",
                m_url, m_range, curl_easy_strerror(res));
        return -1;
    }
    res = curl_easy_setopt (m_curl, CURLOPT_WRITEDATA, (void *)this);
    if (res != CURLE_OK) {
        printf("curl_easy_setopt() for CURLOPT_WRITEDATA failed."
                " url:%s, range:%s, errstr:%s\n",
                m_url, m_range, curl_easy_strerror(res));
        return -1;
    }
    res = curl_easy_setopt (m_curl, CURLOPT_RANGE, m_range);
    if (res != CURLE_OK) {
        printf("curl_easy_setopt() for CURLOPT_RANGE failed."
                " url:%s, range:%s, errstr:%s\n",
                m_url, m_range, curl_easy_strerror(res));
        return -1;
    }
    res = curl_easy_perform(m_curl);
    if (res != CURLE_OK) {
        printf("curl_easy_perform() failed."
                " url:%s, range:%s, errstr:%s\n",
                m_url, m_range, curl_easy_strerror(res));
        return -1;
    }
    printf("url:%s, range:%s, totalSize:%lu, curSize:%lu succeed.\n",
            m_url, m_range, m_totalSize, m_curSize);
    return 0;
}

size_t
CDownloadWorker::Write(char *buf, size_t size, size_t cnt)
{
    size_t wlen = size * cnt;
    size_t len = 0;
    if ((m_curSize + wlen) > m_totalSize) {
        wlen = m_totalSize - m_curSize;
    }
    len = m_writer->Write(buf, wlen);
    m_curSize += len;
    return len;
}

typedef struct thdinfo {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int waiting;
    int thdcnt;
}thdinfo_t;

static void
thdinfo_init(thdinfo_t *info, int thdcnt)
{
    pthread_mutex_init(&info->mutex, NULL);
    pthread_cond_init(&info->cond, NULL);
    info->thdcnt = thdcnt;
    info->waiting = 0;
}

static void
thdinfo_final(thdinfo_t *info)
{
    pthread_mutex_lock(&info->mutex);
    while (info->thdcnt > 0) {
        info->waiting = 1;
        pthread_cond_wait(&info->cond, &info->mutex);
        info->waiting = 0;
    }
    pthread_mutex_unlock(&info->mutex);
    pthread_mutex_destroy(&info->mutex);
    pthread_cond_destroy(&info->cond);
}

typedef struct thdarg {
    pthread_t thdid;
    int index;
    const char *url;
    const char *filename;
    size_t start;
    size_t end;
    thdinfo_t *info;
} thdarg_t;

static void *
thread_cb(void *arg)
{
    thdarg_t *targ = (thdarg_t *)arg;
    char filename[PATH_MAX] = {0};
    sprintf(filename, ".%s.%d", targ->filename, targ->index);
    CFileWriter writer(filename);
    CDownloadWorker worker(targ->url, &writer, targ->start, targ->end);
    int ret = worker.Start();
    if (ret != 0) {
        printf("%04d. Thread:%p worker failed."
                " url:%s, filename:%s, start:%lu, end:%lu\n",
                targ->index, &targ->thdid, targ->url,
                filename, targ->start, targ->end);
#if 0
    } else {
        printf("%04d. Thread:%p worker succeed."
                " url:%s, filename:%s, start:%lu, end:%lu\n",
                targ->index, &targ->thdid, targ->url,
                targ->filename, targ->start, targ->end);
#endif
    }
    pthread_mutex_lock(&targ->info->mutex);
    --targ->info->thdcnt;
    if (targ->info->thdcnt == 0 && targ->info->waiting) {
        pthread_cond_signal(&targ->info->cond);
    }
#if 0
    printf("%04d. Thread:%p. targ->info->thdcnt:%d, targ->info->waiting:%d\n",
            targ->index, &targ->thdid, targ->info->thdcnt, targ->info->waiting);
#endif
    pthread_mutex_unlock(&targ->info->mutex);
    //printf("%04d, Thread:%p exit.\n", targ->index, &targ->thdid);
    return arg;
}

CDownload::CDownload(const char *url, const char *outfile, const int thdCnt)
        : m_thdCnt(thdCnt), m_thds(NULL)
{
    m_url = strdup(url);
    m_filename = strdup(outfile);
}

CDownload::~CDownload()
{
    if (m_thds != NULL) {
        int index;
        thdarg_t *thds = (thdarg_t *)m_thds;
        for (index = 0; index < m_thdCnt && thds[index].thdid != 0; ++index) {
            pthread_join(thds[index].thdid, NULL);
            thds[index].thdid = 0;
        }
        delete [](thdarg_t *)m_thds;
        m_thds = NULL;
    }
    if (m_url != NULL) {
        free(m_url);
        m_url = NULL;
    }
    if (m_filename != NULL) {
        free(m_filename);
        m_filename = NULL;
    }
}

#define HTTP_CONTENT_LENGTH         "Content-Length:"
#define HTTP_CONTENT_LENGTH_SIZE    15
static size_t
header_callback(char *buf, size_t size,
                size_t cnt, void *arg)
{
    //printf("Header <%s>\n", buf);
    if (strncasecmp(HTTP_CONTENT_LENGTH, buf,
                HTTP_CONTENT_LENGTH_SIZE) == 0) {
        ssize_t *length = (ssize_t *)arg;
        *length = strtoull(buf+HTTP_CONTENT_LENGTH_SIZE, NULL, 0);
        printf("length:%lu\n", *length);
        //return 0;
    }
    return cnt * size;
}

ssize_t
CDownload::GetSize()
{
    ssize_t size = 0;
    CURL *curl = curl_easy_init();
    if (curl) {
        CURLcode res = curl_easy_setopt(curl, CURLOPT_URL, m_url);
        if (res != CURLE_OK) {
            printf("curl_easy_setopt() for CURLOPT_URL"
                    " failed. url:%s errstr:%s\n",
                    m_url, curl_easy_strerror(res));
            goto out;
        }
        res = curl_easy_setopt(curl, CURLOPT_HEADER, 1);
        if (res != CURLE_OK) {
            printf("curl_easy_setopt() for CURLOPT_HEADER"
                    " failed. url:%s errstr:%s\n",
                    m_url, curl_easy_strerror(res));
            goto out;
        }
        res = curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        if (res != CURLE_OK) {
            printf("curl_easy_setopt() for CURLOPT_NOBODY"
                    " failed. url:%s errstr:%s\n",
                    m_url, curl_easy_strerror(res));
            goto out;
        }
        res = curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        if (res != CURLE_OK) {
            printf("curl_easy_setopt() for CURLOPT_HEADERFUNCTION"
                    " failed. url:%s errstr:%s\n",
                    m_url, curl_easy_strerror(res));
            goto out;
        }
        res = curl_easy_setopt(curl, CURLOPT_HEADERDATA, &size);
        if (res != CURLE_OK) {
            printf("curl_easy_setopt() for CURLOPT_HEADERDATA"
                    " failed. url:%s errstr:%s\n",
                    m_url, curl_easy_strerror(res));
            goto out;
        }
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            printf("curl_easy_perform() failed. url:%s, errstr:%s\n",
                    m_url, curl_easy_strerror(res));
            goto out;
        }
#if 0
        res = curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &size);
        if (res != CURLE_OK) {
            printf("curl_easy_getinfo() for CURLINFO_CONTENT_LENGTH_DOWNLOAD_T"
                    " failed. url:%s errstr:%s\n",
                    m_url, curl_easy_strerror(res));
            goto out;
        }
#endif
        printf("size:%lu\n", size);
    } else {
        printf("curl_easy_init() failed.\n");
    }
out:
    if (curl != NULL) {
        curl_easy_cleanup(curl);
    }
    return size;
}

int
CDownload::Start()
{
    printf("Start download ......\n");
    thdarg_t *thds = new thdarg_t[m_thdCnt];
    if (thds == NULL) {
        printf("New object array for thdarg_t failed.\n");
        return -1;
    }
    m_thds = (void *)thds;
    size_t totalSize = GetSize();
    size_t peerSize = totalSize / m_thdCnt;
    thdinfo_t info;
    thdinfo_init(&info, m_thdCnt);
    int index;
    for (index = 0; index < m_thdCnt; ++index) {
        thds[index].thdid = 0;
        thds[index].index = index;
        thds[index].url = m_url;
        thds[index].filename = m_filename;
        thds[index].info = &info;
        thds[index].start = peerSize * index;
        if ((index+1) == m_thdCnt) {
            thds[index].end = totalSize;
        } else {
            thds[index].end = thds[index].start + peerSize - 1;
        }
    }
    for (index = 0; index < (m_thdCnt-1); ++index) {
        thdarg_t *thd = &thds[index];
        int ret = pthread_create(&thd->thdid, NULL, thread_cb, thd);
        if (ret != 0) {
            printf("pthread_create() failed for thread:%d, errstr:%s\n",
                    thd->index, strerror(errno));
            return -1;
        }
    }
    thds[index].thdid = pthread_self();
    thread_cb(&thds[index]);
    thdinfo_final(&info);
    return MergeFiles();
}

int
CDownload::MergeFiles()
{
    CFileWriter dst(m_filename);
    int index;
    for (index = 0; index < m_thdCnt; ++index) {
        char filename[1024] = {0};
        sprintf(filename, ".%s.%d", m_filename, index);
        ssize_t asize = dst.AppendWith(filename);
        if (asize < 0) {
            printf("Append file:%s to file:%s failed.\n",
                    filename, m_filename);
            return -1;
        }
        unlink(filename);
    }
    return 0;
}
