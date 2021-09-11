#include "network/HttpClient.h"

#include <glog/logging.h>

#include <mutex>
#include <thread>
#include "common.h"

namespace whale {
namespace vision {

static std::mutex mt_;

        typedef struct {
            char*       memory;
            size_t      size;
        } MemoryStruct;

class HttpClient {
 public:
    HttpClient();
    ~HttpClient();
    // true means success
    bool Post(const char *url, const std::string &value, std::string &re);
    bool Get(const char *url, const std::string &value, std::string &re);

 private:
    static size_t OnWriteData(void *buffer, size_t size, size_t nmemb,
                              void *lpVoid) {
        std::string *str = dynamic_cast<std::string *>((std::string *)lpVoid);
        if (NULL == str || NULL == buffer) {
            return -1;
        }

        char *pData = (char *)buffer;
        str->append(pData, size * nmemb);
        return nmemb;
    };

    // curl调试
    static int OnDebug(CURL *, curl_infotype itype, char *pData, size_t size,
                       void *) {
        if (itype == CURLINFO_TEXT) {
            LOG(INFO)
                << "[TEXT]"
                << pData;  // logOutput(string(pData));//printf("[TEXT]%s\n",
                           // pData);
        } else if (itype == CURLINFO_HEADER_IN) {
            LOG(INFO) << "[HEADER_IN]"
                      << pData;  // printf("[HEADER_IN]%s\n", pData);
        } else if (itype == CURLINFO_HEADER_OUT) {
            LOG(INFO) << "[HEADER_OUT]"
                      << pData;  // printf("[HEADER_OUT]%s\n", pData);
        } else if (itype == CURLINFO_DATA_IN) {
            LOG(INFO) << "[DATA_IN]"
                      << pData;  // printf("[DATA_IN]%s\n", pData);
        } else if (itype == CURLINFO_DATA_OUT) {
            LOG(INFO) << "[DATA_OUT]"
                      << pData;  // printf("[DATA_OUT]%s\n", pData);
        }
        return 0;
    };

 private:
    CURL *m_curl;
};

HttpClient::HttpClient() {
    std::lock_guard<std::mutex> ld(mt_);
    curl_global_init(CURL_GLOBAL_WIN32);
};

bool HttpClient::Post(const char *url, const std::string &value,
                      std::string &re) {
    m_curl = curl_easy_init();  // init
    if (!m_curl) return false;
#ifdef __BUILD_GTESTS__
    __http__last_data__ = value;
#endif

    //用于调试的设置
    if (false) {
        curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }

    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(m_curl, CURLOPT_URL, url);
    curl_easy_setopt(m_curl, CURLOPT_POST, 1);  //设置为非0表示本次操作为POST
    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, value.c_str());
    curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, NULL);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&re);
    curl_easy_setopt(m_curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 3000);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 300);

    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);  //支持服务器跳转
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE,
                     1L);  // enable TCP keep-alive for this transfer
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPIDLE,
                     120L);  // keep-alive idle time to 120 seconds
    curl_easy_setopt(
        m_curl, CURLOPT_TCP_KEEPINTVL,
        60L);  // interval time between keep-alive probes: 60 seconds

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers,
                                "Content-Type:application/json;charset=UTF-8");
    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);

    auto res = curl_easy_perform(m_curl);
    curl_slist_free_all(headers);  //清理headers,防止内存泄漏

    if (re.find("404") != std::string::npos)
        LOG(ERROR) << "ERROR: CANNOT USE HTTP POST !!!";
    LOG(INFO) << "NOTE: POST URL is: " << url << ", value: " << value
              << ", response: " << re << ", res:" << res;

    return (res == CURLE_OK) ? true : false;
};

bool HttpClient::Get(const char *url, const std::string &value,
                     std::string &re) {
    m_curl = curl_easy_init();  // init
    if (!m_curl) return false;
#ifdef __BUILD_GTESTS__
    __http__last_data__ = value;
#endif

    //用于调试的设置
    if (false) {
        curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 1);
        curl_easy_setopt(m_curl, CURLOPT_DEBUGFUNCTION, OnDebug);
    }

    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0);
    curl_easy_setopt(m_curl, CURLOPT_URL, url);
    curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, 3000);
    curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, 300);
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, OnWriteData);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, (void *)&re);

    curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);  //支持服务器跳转
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPALIVE,
                     1L);  // enable TCP keep-alive for this transfer
    curl_easy_setopt(m_curl, CURLOPT_TCP_KEEPIDLE,
                     120L);  // keep-alive idle time to 120 seconds
    curl_easy_setopt(
        m_curl, CURLOPT_TCP_KEEPINTVL,
        60L);  // interval time between keep-alive probes: 60 seconds

    auto res = curl_easy_perform(m_curl);

    if (re.find("404") != std::string::npos)
        LOG(ERROR) << "ERROR: CANNOT USE HTTP GET !!!";
    LOG(INFO) << "NOTE: GET URL is: " << url << ", value: " << value
              << ", response: " << re << ", res:" << res;

    return (res == CURLE_OK) ? true : false;
};

//        bool HttpClient::Get(const char *url, const std::string &value,
//                             std::string &response) {
//
//
////            MemoryStruct chunk;
////            memset(&chunk,0x00, sizeof(MemoryStruct));
////            chunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
////            chunk.size = 1;    /* no data at this point */
//
//            CURL *curl = curl_easy_init();
//            if (nullptr == curl) {
//                printf("get data init curl failed !!!\n");
////                free(chunk.memory);
//                return -1;
//            }
//
//            curl_easy_setopt(curl, CURLOPT_URL, url);
//            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
//            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
//            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);
//            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
//            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30);
//            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
//            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
//
//            CURLcode curlCode = curl_easy_perform(curl);
//            if (CURLE_OK != curlCode) {
//                printf("EasyHttpCurl::EasyHttpCurl_Heartbeat_GetRequest: curl_easy_perform() url:%s, failed: %s\n",url, curl_easy_strerror(curlCode));
//                curl_easy_cleanup(curl);
////                free(chunk.memory);
//                return false;
//            }
//
////            response = chunk.memory;
//            curl_easy_cleanup(curl);
////            free(chunk.memory);
//
//            LOG(INFO) << "NOTE: GET URL is: " << url << ", value: " << value
//                      << ", response: " << response << ", curlCode:" << curlCode;
//
//            return (curlCode == CURLE_OK) ? true : false;
//        }


HttpClient::~HttpClient() {
    std::lock_guard<std::mutex> ld(mt_);
    curl_easy_cleanup(m_curl);
    curl_global_cleanup();
};

bool IOTHeartBeat(const std::string &device_sn) {
    return false;
};

bool SendToChunnel() {
return false;
};

bool HttpPost(const char *url, const std::string &value, std::string &re) {
    HttpClient client;
    return client.Post(url, value, re);
};

bool HttpGet(const char *url, const std::string &value, std::string &re) {
    HttpClient client;
    return client.Get(url, value, re);
};

}  // namespace vision
}  // namespace whale