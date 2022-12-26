#define BAMBU_DYNAMIC

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <string.h>
#include <fcntl.h>
#if defined(_MSC_VER) || defined(_WIN32)
#include <Windows.h>
#include <WinInet.h>
#include <Shlobj.h> 
#include <shlwapi.h>
#include <io.h>
#include "getopt.h"
#else
#include <getopt.h>
#include <ctype.h>
#include <dlfcn.h>
#include <unistd.h>
#include <curl/curl.h>
#define _snprintf snprintf
#define _stricmp strcmp
#endif
#include "cJSON.h"
#include "BambuTunnel.h"

#define BAMBUE_START_STREAM_RETRY_COUNT (40)

/* ret 0 means stop, 1 means continue */
typedef int (*enum_dev_lst_cb)(void *ctx, cJSON *device_info_item);

static int get_bambu_studio_user_info(char **user_id, char **dev_id, char **token, char **region)
{
    int ret = 0;
#if defined(_MSC_VER) || defined(_WIN32)
    char app_data_path[MAX_PATH + 1] = {0};
    char bambu_cfg_path[MAX_PATH + 1] = {0};
#else
    char bambu_cfg_path[256 + 1] = {0};
#endif
    FILE *cfg_file = NULL;
    char *cfg_data = NULL;
    cJSON *cfg_json = NULL;
    cJSON *last_monitor_machine_item = NULL;
    cJSON *user_item = NULL;
    cJSON *user_id_item = NULL;
    cJSON *token_item = NULL;
    cJSON *country_code_item = NULL;
    char *user_id_str = NULL;
    char *dev_id_str = NULL;
    char *token_str = NULL;
    char *region_str = NULL;

    do
    {
#if defined(_MSC_VER) || defined(_WIN32)
        if (PathFileExistsA("BambuNetworkEngine.conf"))
        {
            _snprintf(bambu_cfg_path, sizeof(bambu_cfg_path), "BambuNetworkEngine.conf");
        }
        else if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_DEFAULT, app_data_path)))
        {
            _snprintf(bambu_cfg_path, sizeof(bambu_cfg_path), "%s\\BambuStudio\\BambuNetworkEngine.conf", app_data_path);
            if (!PathFileExistsA(bambu_cfg_path))
            {
                ret = -1;
                fprintf(stderr, "get %s failed\n", bambu_cfg_path);
                break;
            }
        }
        else
        {
            ret = -1;
            fprintf(stderr, "SHGetFolderPath failed 0x%x\n", GetLastError());
            break;
        }
#else
        if (access("BambuNetworkEngine.conf", F_OK) == 0) 
        {
            snprintf(bambu_cfg_path, sizeof(bambu_cfg_path), "BambuNetworkEngine.conf");
        }
        else 
        {
            ret = -1;
            fprintf(stderr, "BambuNetworkEngine.conf not exist\n");
            break;
        }
#endif

        cfg_file = fopen(bambu_cfg_path, "r");
        if (cfg_file == NULL)
        {
            fprintf(stderr, "fopen failed\n");
            break;
        }

        fseek(cfg_file, 0L, SEEK_END);
        size_t cfg_file_size = ftell(cfg_file);
        if (cfg_file_size == 0)
        {
            fprintf(stderr, "cfg_file is empty\n");
            break;            
        }
        fseek(cfg_file, 0L, SEEK_SET);
        cfg_data = (char *)malloc(cfg_file_size);
        if (cfg_data == NULL)
        {
            fprintf(stderr, "malloc cfg_data failed\n");
            break;            
        }

        fread(cfg_data, 1, cfg_file_size, cfg_file);
        cfg_json = cJSON_Parse(cfg_data);
        if (cfg_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "Parse cfg json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        last_monitor_machine_item = cJSON_GetObjectItem(cfg_json, "last_monitor_machine");
        if (last_monitor_machine_item == NULL 
            || !cJSON_IsString(last_monitor_machine_item))
        {
            ret = -1;
            fprintf(stderr, "failed get last_monitor_machine\n");
            break;
        }

        user_item = cJSON_GetObjectItem(cfg_json, "user");
        if (user_item == NULL)
        {
            ret = -1;
            fprintf(stderr, "failed get user_item\n");
            break;
        }

        user_id_item = cJSON_GetObjectItem(user_item, "user_id");
        if (user_id_item == NULL 
            || !cJSON_IsString(user_id_item))
        {
            ret = -1;
            fprintf(stderr, "failed get user_id\n");
            break;
        }

        token_item = cJSON_GetObjectItem(user_item, "token");
        if (token_item == NULL 
            || !cJSON_IsString(token_item))
        {
            ret = -1;
            fprintf(stderr, "failed get token\n");
            break;
        }

        country_code_item = cJSON_GetObjectItem(cfg_json, "country_code");
        if (country_code_item == NULL 
            || !cJSON_IsString(country_code_item))
        {
            ret = -1;
            fprintf(stderr, "failed get country_code\n");
            break;
        }

        size_t user_id_str_len = strlen(cJSON_GetStringValue(user_id_item));
        user_id_str = (char *)malloc(user_id_str_len + sizeof('\0'));
        if (user_id_str == NULL)
        {
            fprintf(stderr, "malloc user_id_str failed\n");
            break;            
        }
        strcpy(user_id_str, cJSON_GetStringValue(user_id_item));

        size_t dev_id_str_len = strlen(cJSON_GetStringValue(last_monitor_machine_item));
        dev_id_str = (char *)malloc(dev_id_str_len + sizeof('\0'));
        if (dev_id_str == NULL)
        {
            fprintf(stderr, "malloc dev_id_str failed\n");
            break;            
        }
        strcpy(dev_id_str, cJSON_GetStringValue(last_monitor_machine_item));

        size_t token_str_len = strlen(cJSON_GetStringValue(token_item));
        token_str = (char *)malloc(token_str_len + sizeof('\0'));
        if (token_str == NULL)
        {
            fprintf(stderr, "malloc token_str failed\n");
            break;            
        }
        strcpy(token_str, cJSON_GetStringValue(token_item));

        size_t region_str_len = strlen(cJSON_GetStringValue(country_code_item));
        region_str = (char *)malloc(region_str_len + sizeof('\0'));
        if (region_str == NULL)
        {
            fprintf(stderr, "malloc region_str failed\n");
            break;            
        }
        strcpy(region_str, cJSON_GetStringValue(country_code_item));
        size_t i;
        for (i = 0; i < region_str_len; i++)
        {
            region_str[i] = (char)tolower(region_str[i]);
        }

        ret = 0;
    } while(false);

    if (cfg_json != NULL)
    {
        cJSON_Delete(cfg_json);
        cfg_json = NULL;
    }

    if (cfg_data != NULL)
    {
        free(cfg_data);
        cfg_data = NULL;
    }

    if (cfg_file != NULL)
    {
        fclose(cfg_file);
        cfg_file = NULL;
    }

    if (ret != 0)
    {
        if (user_id_str != NULL)
        {
            free(user_id_str);
            user_id_str = NULL;
        }

        if (dev_id_str != NULL)
        {
            free(dev_id_str);
            dev_id_str = NULL;
        }

        if (token_str != NULL)
        {
            free(token_str);
            token_str = NULL;
        }

        if (region_str != NULL)
        {
            free(region_str);
            region_str = NULL;
        }
    }
    else
    {
        *user_id = user_id_str;
        *dev_id = dev_id_str;
        *token = token_str;
        *region = region_str;
    }

    return ret;
}

#if defined(_MSC_VER) || defined(_WIN32)
static int get_camera_url(char camera_url[256], const char *user_id, const char *dev_id, const char *token_str, const char *region)
{
    int ret = 0;
    static char site_cn[] = "api.bambulab.cn";
    static char site_global[] = "api.bambulab.com";
    static char site_param[] = "v1/iot-service/api/user/ttcode";
    const char* accept_type[] = {"application/json", NULL};
    static char header[4096] = {0};
    static char body[256] = {0};
    int is_global_site = 0;
    char rsp_json[1024] = {0};
    DWORD byte_read = 0;
    DWORD byte_write = 0;
    INTERNET_BUFFERSA inet_buf = {0};

    HINTERNET inet_hnd = NULL;
    HINTERNET connect_hnd = NULL;
    HINTERNET req_hnd = NULL;

    cJSON *iot_json = NULL;
    cJSON *message_item = NULL;
    cJSON *ttcode_item = NULL;
    cJSON *passwd_item = NULL;
    cJSON *authkey_item = NULL;

    do
    {
        if (0 == _stricmp(region,  "cn"))
        {
            is_global_site = 0;
        }
        else
        {
            is_global_site = 1;
        }

        _snprintf(header, sizeof(header), 
            "content-Type: application/json\n\r"
            "user-id: %s\n\r"
            "Authorization: Bearer %s\n\r",
            user_id, token_str);
        _snprintf(body, sizeof(body), 
            "{\"dev_id\": \"%s\"}", dev_id);

        inet_hnd = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (inet_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "InternetOpenW failed 0x%x\n", GetLastError());
            break;
        }

        connect_hnd = InternetConnectA(inet_hnd, is_global_site ? site_global : site_cn, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
        if (connect_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "InternetConnectW failed 0x%x\n", GetLastError());
            break;
        }


        req_hnd = HttpOpenRequestA(connect_hnd, "POST", site_param, "HTTP/1.1", NULL, accept_type, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_COOKIES, 0);
        if (req_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "HttpOpenRequestA failed 0x%x\n", GetLastError());
            break;
        }

        if (!HttpAddRequestHeadersA(req_hnd, header, -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE))
        {
            ret = -1;
            fprintf(stderr, "HttpAddRequestHeadersA failed 0x%x\n", GetLastError());
            break;
        }

        inet_buf.dwStructSize = sizeof(inet_buf);
        //inet_buf.lpvBuffer = &body[0];
        //inet_buf.dwBufferLength = strlen(body);
        //inet_buf.dwBufferTotal = inet_buf.dwBufferLength;
        inet_buf.dwBufferTotal = strlen(body);

        if (!HttpSendRequestExA(req_hnd, &inet_buf, NULL, 0, 0))
        {
            ret = -1;
            fprintf(stderr, "HttpSendRequestExA failed 0x%x\n", GetLastError());
            break;
        }

        if (!InternetWriteFile(req_hnd, body, strlen(body), &byte_write))
        {
            ret = -1;
            fprintf(stderr, "InternetWriteFile failed 0x%x\n", GetLastError());
            break;
        }

        HttpEndRequestA(req_hnd, NULL, 0, 0);

        if (!InternetReadFile(req_hnd, rsp_json, sizeof(rsp_json) - sizeof('\0'), &byte_read))
        {
            ret = -1;
            fprintf(stderr, "InternetReadFile failed 0x%x\n", GetLastError());
            break;
        }

        //fprintf(stderr, "%s\n", rsp_json);
        if (byte_read == 0)
        {
            DWORD err = 0;
            InternetGetLastResponseInfoA(&err, NULL, 0);
            ret = -1;
            fprintf(stderr, "InternetReadFile read empty %d\n", err);
            break;
        }

        iot_json = cJSON_Parse(rsp_json);
        if (iot_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "Parse json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        message_item = cJSON_GetObjectItem(iot_json, "message");
        if (message_item == NULL 
            || !cJSON_IsString(message_item)
            || 0 != strcmp(cJSON_GetStringValue(message_item), "success"))
        {
            ret = -1;
            fprintf(stderr, "iot req failed\n %s\n", rsp_json);
            break;
        }

        ttcode_item = cJSON_GetObjectItem(iot_json, "ttcode");
        if (ttcode_item == NULL
            || !cJSON_IsString(ttcode_item))
        {
            ret = -1;
            fprintf(stderr, "failed get ttcode\n %s\n", rsp_json);
            break;
        }

        passwd_item = cJSON_GetObjectItem(iot_json, "passwd");
        if (passwd_item == NULL
            || !cJSON_IsString(passwd_item))
        {
            ret = -1;
            fprintf(stderr, "failed get passwd\n %s\n", rsp_json);
            break;
        }

        authkey_item = cJSON_GetObjectItem(iot_json, "authkey");
        if (authkey_item == NULL
            || !cJSON_IsString(authkey_item))
        {
            ret = -1;
            fprintf(stderr, "failed get authkey\n %s\n", rsp_json);
            break;
        }

        _snprintf(camera_url, 256, "bambu:///%s?authkey=%s&passwd=%s&region=%s",
            cJSON_GetStringValue(ttcode_item),
            cJSON_GetStringValue(authkey_item),
            cJSON_GetStringValue(passwd_item),
            region
            );

        ret = 0;
    } while (false);

    if (iot_json != NULL)
    {
        cJSON_Delete(iot_json);
        iot_json = NULL;
    }

    if (req_hnd != NULL)
    {
        InternetCloseHandle(req_hnd);
        req_hnd = NULL;
    }

    if (connect_hnd != NULL)
    {
        InternetCloseHandle(connect_hnd);
        connect_hnd = NULL;
    }

    if (inet_hnd != NULL)
    {
        InternetCloseHandle(inet_hnd);
        inet_hnd = NULL;
    }

    return ret;
}

static int get_user_token(const char *user_name, const char *passwd, const char *region, char **token_str)
{
    int ret = 0;
    static char site_cn[] = "bambulab.cn";
    static char site_global[] = "bambulab.com";
    static char site_param[] = "/api/sign-in/form";
    const char* accept_type[] = {"application/json", NULL};
    static char header[4096] = {0};
    static char body[256] = {0};
    static char token_cookie[4096] = {0};
    int is_global_site = 0;
    DWORD byte_read = 0;
    DWORD byte_write = 0;
    INTERNET_BUFFERSA inet_buf = {0};

    HINTERNET inet_hnd = NULL;
    HINTERNET connect_hnd = NULL;
    HINTERNET req_hnd = NULL;
    do
    {
        if (0 == _stricmp(region,  "cn"))
        {
            is_global_site = 0;
        }
        else
        {
            is_global_site = 1;
        }

        _snprintf(header, sizeof(header), 
            "content-Type: application/json\n\r");
        _snprintf(body, sizeof(body), 
            "{\"account\":\"%s\",\"password\":\"%s\",\"apiError\":\"\"}", user_name, passwd);

        inet_hnd = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (inet_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "InternetOpenW failed 0x%x\n", GetLastError());
            break;
        }

        DWORD option = 3;//  INTERNET_SUPPRESS_COOKIE_PERSIST;
        InternetSetOption(inet_hnd, INTERNET_OPTION_SUPPRESS_BEHAVIOR, &option, sizeof(option));

        connect_hnd = InternetConnectA(inet_hnd, is_global_site ? site_global : site_cn, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
        if (connect_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "InternetConnectW failed 0x%x\n", GetLastError());
            break;
        }


        req_hnd = HttpOpenRequestA(connect_hnd, "POST", site_param, "HTTP/1.1", NULL, accept_type, INTERNET_FLAG_SECURE, 0);
        if (req_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "HttpOpenRequestA failed 0x%x\n", GetLastError());
            break;
        }

        if (!HttpAddRequestHeadersA(req_hnd, header, -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE))
        {
            ret = -1;
            fprintf(stderr, "HttpAddRequestHeadersA failed 0x%x\n", GetLastError());
            break;
        }

        inet_buf.dwStructSize = sizeof(inet_buf);
        //inet_buf.lpvBuffer = &body[0];
        //inet_buf.dwBufferLength = strlen(body);
        //inet_buf.dwBufferTotal = inet_buf.dwBufferLength;
        inet_buf.dwBufferTotal = strlen(body);

        if (!HttpSendRequestExA(req_hnd, &inet_buf, NULL, 0, 0))
        {
            ret = -1;
            fprintf(stderr, "HttpSendRequestExA failed 0x%x\n", GetLastError());
            break;
        }

        if (!InternetWriteFile(req_hnd, body, strlen(body), &byte_write))
        {
            ret = -1;
            fprintf(stderr, "InternetWriteFile failed 0x%x\n", GetLastError());
            break;
        }

        HttpEndRequestA(req_hnd, NULL, 0, 0);

        static char cookies_url[256] = {0};
        _snprintf(cookies_url, sizeof(cookies_url), "https://%s", is_global_site ? site_global : site_cn);

        DWORD buf_len = sizeof(token_cookie) - sizeof('\0');
        if (!InternetGetCookieExA(cookies_url, "token", token_cookie, &buf_len, INTERNET_COOKIE_HTTPONLY, NULL))
        {
            ret = -1;
            fprintf(stderr, "InternetGetCookieExA failed 0x%x\n", GetLastError());
            break;
        }

        if (token_str)
        {
            char *s = token_cookie;
            if (0 == strncmp(s, "token=", strlen("token=")))
            {
                s += strlen("token=");
            }
            size_t len = strlen(s) + sizeof('\0');
            char *token_temp = (char *)malloc(len);
            if (token_temp == NULL)
            {
                ret = -1;
                fprintf(stderr, "error malloc memory for token\n");
                break;
            }

            strcpy(token_temp, s);
            *token_str = token_temp;
        }

        ret = 0;
    } while (false);

    if (req_hnd != NULL)
    {
        InternetCloseHandle(req_hnd);
        req_hnd = NULL;
    }

    if (connect_hnd != NULL)
    {
        InternetCloseHandle(connect_hnd);
        connect_hnd = NULL;
    }

    if (inet_hnd != NULL)
    {
        InternetCloseHandle(inet_hnd);
        inet_hnd = NULL;
    }

    return ret;
}

static int get_user_id(const char *token, const char *region, char **user_id)
{
    int ret = 0;
    static char site_cn[] = "api.bambulab.cn";
    static char site_global[] = "api.bambulab.com";
    static char site_param[] = "v1/user-service/my/profile";
    const char* accept_type[] = {"application/json", NULL};
    static char header[4096] = {0};
    int is_global_site = 0;
    char rsp_json[1024] = {0};
    DWORD byte_read = 0;

    HINTERNET inet_hnd = NULL;
    HINTERNET connect_hnd = NULL;
    HINTERNET req_hnd = NULL;

    cJSON *ret_json = NULL;
    cJSON *uid_str_item = NULL;

    do
    {
        if (0 == _stricmp(region,  "cn"))
        {
            is_global_site = 0;
        }
        else
        {
            is_global_site = 1;
        }

        _snprintf(header, sizeof(header), 
            "content-Type: application/json\n\r"
            "Authorization: Bearer %s\n\r",
            token);

        inet_hnd = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (inet_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "InternetOpenW failed 0x%x\n", GetLastError());
            break;
        }

        connect_hnd = InternetConnectA(inet_hnd, is_global_site ? site_global : site_cn, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
        if (connect_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "InternetConnectW failed 0x%x\n", GetLastError());
            break;
        }


        req_hnd = HttpOpenRequestA(connect_hnd, "GET", site_param, "HTTP/1.1", NULL, accept_type, INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_COOKIES, 0);
        if (req_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "HttpOpenRequestA failed 0x%x\n", GetLastError());
            break;
        }

        if (!HttpAddRequestHeadersA(req_hnd, header, -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE))
        {
            ret = -1;
            fprintf(stderr, "HttpAddRequestHeadersA failed 0x%x\n", GetLastError());
            break;
        }

        if (!HttpSendRequestExA(req_hnd, NULL, NULL, 0, 0))
        {
            ret = -1;
            fprintf(stderr, "HttpSendRequestExA failed 0x%x\n", GetLastError());
            break;
        }

        HttpEndRequestA(req_hnd, NULL, 0, 0);

        if (!InternetReadFile(req_hnd, rsp_json, sizeof(rsp_json) - sizeof('\0'), &byte_read))
        {
            ret = -1;
            fprintf(stderr, "InternetReadFile failed 0x%x\n", GetLastError());
            break;
        }

        //fprintf(stderr, "%s\n", rsp_json);
        if (byte_read == 0)
        {
            DWORD err = 0;
            InternetGetLastResponseInfoA(&err, NULL, 0);
            ret = -1;
            fprintf(stderr, "InternetReadFile read empty %d\n", err);
            break;
        }

        ret_json = cJSON_Parse(rsp_json);
        if (ret_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "Parse json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        uid_str_item = cJSON_GetObjectItem(ret_json, "uidStr");
        if (uid_str_item == NULL 
            || !cJSON_IsString(uid_str_item))
        {
            ret = -1;
            fprintf(stderr, "failed get uidStr\n %s\n", rsp_json);
            break;
        }

        if (user_id != NULL)
        {
            size_t len = strlen(cJSON_GetStringValue(uid_str_item)) + sizeof('\0');
            char *str = (char *)malloc(len);
            strcpy(str, cJSON_GetStringValue(uid_str_item));
            *user_id = str;
        }

        ret = 0;
    } while (false);

    if (ret_json != NULL)
    {
        cJSON_Delete(ret_json);
        ret_json = NULL;
    }

    if (req_hnd != NULL)
    {
        InternetCloseHandle(req_hnd);
        req_hnd = NULL;
    }

    if (connect_hnd != NULL)
    {
        InternetCloseHandle(connect_hnd);
        connect_hnd = NULL;
    }

    if (inet_hnd != NULL)
    {
        InternetCloseHandle(inet_hnd);
        inet_hnd = NULL;
    }

    return ret;
}

static int enum_dev_lst(const char *token, const char *region, enum_dev_lst_cb callback, void *ctx)
{
    int ret = 0;
    static char site_cn[] = "api.bambulab.cn";
    static char site_global[] = "api.bambulab.com";
    static char site_param[] = "v1/iot-service/api/user/bind";
    const char* accept_type[] = {"application/json", NULL};
    static char header[4096] = {0};
    int is_global_site = 0;
    char rsp_json[1024] = {0};
    DWORD byte_read = 0;

    HINTERNET inet_hnd = NULL;
    HINTERNET connect_hnd = NULL;
    HINTERNET req_hnd = NULL;

    cJSON *ret_json = NULL;
    cJSON *message_item = NULL;
    cJSON *devices_item = NULL;
    cJSON *device_info_item = NULL;

    do
    {
        if (0 == _stricmp(region,  "cn"))
        {
            is_global_site = 0;
        }
        else
        {
            is_global_site = 1;
        }

        _snprintf(header, sizeof(header), 
            "content-Type: application/json\n\r"
            "Authorization: Bearer %s\n\r",
            token);

        inet_hnd = InternetOpenA("", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
        if (inet_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "InternetOpenW failed 0x%x\n", GetLastError());
            break;
        }

        connect_hnd = InternetConnectA(inet_hnd, is_global_site ? site_global : site_cn, INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, NULL);
        if (connect_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "InternetConnectW failed 0x%x\n", GetLastError());
            break;
        }


        req_hnd = HttpOpenRequestA(connect_hnd, "GET", site_param, "HTTP/1.1", NULL, accept_type, INTERNET_FLAG_SECURE |  INTERNET_FLAG_NO_COOKIES, 0);
        if (req_hnd == NULL)
        {
            ret = -1;
            fprintf(stderr, "HttpOpenRequestA failed 0x%x\n", GetLastError());
            break;
        }

        if (!HttpAddRequestHeadersA(req_hnd, header, -1, HTTP_ADDREQ_FLAG_ADD | HTTP_ADDREQ_FLAG_REPLACE))
        {
            ret = -1;
            fprintf(stderr, "HttpAddRequestHeadersA failed 0x%x\n", GetLastError());
            break;
        }

        if (!HttpSendRequestExA(req_hnd, NULL, NULL, 0, 0))
        {
            ret = -1;
            fprintf(stderr, "HttpSendRequestExA failed 0x%x\n", GetLastError());
            break;
        }

        HttpEndRequestA(req_hnd, NULL, 0, 0);

        if (!InternetReadFile(req_hnd, rsp_json, sizeof(rsp_json) - sizeof('\0'), &byte_read))
        {
            ret = -1;
            fprintf(stderr, "InternetReadFile failed 0x%x\n", GetLastError());
            break;
        }

        //fprintf(stderr, "%s\n", rsp_json);
        if (byte_read == 0)
        {
            DWORD err = 0;
            InternetGetLastResponseInfoA(&err, NULL, 0);
            ret = -1;
            fprintf(stderr, "InternetReadFile read empty %d\n", err);
            break;
        }

        ret_json = cJSON_Parse(rsp_json);
        if (ret_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "Parse json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        message_item = cJSON_GetObjectItem(ret_json, "message");
        if (message_item == NULL 
            || !cJSON_IsString(message_item)
            || 0 != strcmp(cJSON_GetStringValue(message_item), "success"))
        {
            ret = -1;
            fprintf(stderr, "iot req failed\n %s\n", rsp_json);
            break;
        }

        devices_item = cJSON_GetObjectItem(ret_json, "devices");
        if (devices_item == NULL 
            || !cJSON_IsArray(devices_item)
            || cJSON_GetArraySize(devices_item) <= 0)
        {
            ret = -1;
            fprintf(stderr, "failed get devices list\n %s\n", rsp_json);
            break;
        }

        size_t item_count = cJSON_GetArraySize(devices_item);
        size_t i = 0;
        for (i = 0; i < item_count; i++)
        {
            device_info_item = cJSON_GetArrayItem(devices_item, i);
            if (device_info_item == NULL 
                || !cJSON_IsObject(device_info_item))
            {
                ret = -1;
                fprintf(stderr, "failed get device info object idx: %u\n %s\n", (unsigned int)i, rsp_json);
                break;
            }

            if (!callback(ctx, device_info_item))
            {
                break;
            }
        }
    } while (false);

    if (req_hnd != NULL)
    {
        InternetCloseHandle(req_hnd);
        req_hnd = NULL;
    }

    if (connect_hnd != NULL)
    {
        InternetCloseHandle(connect_hnd);
        connect_hnd = NULL;
    }

    if (inet_hnd != NULL)
    {
        InternetCloseHandle(inet_hnd);
        inet_hnd = NULL;
    }

    if (ret_json != NULL)
    {
        cJSON_Delete(ret_json);
        ret_json = NULL;
    }

    return ret;
}

#else

struct bambu_curl_memory_data
{
    char *data;
    size_t pos;
    size_t total_size;
};

static size_t body_read_callback(char *buffer, size_t size, size_t nitems, void *stream)
{
    struct bambu_curl_memory_data *data = (struct bambu_curl_memory_data *)stream;
    size_t read_size = 0;

    if (data->pos + size * nitems >= data->total_size)
    {
        read_size = data->total_size - data->pos;
    }
    else if (data->pos >= data->total_size)
    {
        read_size = 0;
        return read_size;
    }
    else 
    {
        read_size = size * nitems;
    }

    memcpy(buffer, &data->data[data->pos], read_size);
    data->pos += read_size;

    return read_size;
}

size_t rsp_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    struct bambu_curl_memory_data *data = (struct bambu_curl_memory_data *)userdata;
    size_t write_size = 0;

    if (data->pos + size * nmemb >= data->total_size)
    {
        write_size = data->total_size - data->pos;
    }
    else if (data->pos >= data->total_size)
    {
        write_size = size * nmemb;
        return write_size;
    }
    else 
    {
        write_size = size * nmemb;
    }

    memcpy(&data->data[data->pos], ptr, write_size);
    data->pos += write_size;

    return write_size;
}

struct data {
  char trace_ascii; /* 1 or 0 */
};
 
static
void curl_dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
  size_t i;
  size_t c;
 
  unsigned int width = 0x10;
 
  if(nohex)
    /* without the hex output, we can fit more on screen */
    width = 0x40;
 
  fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n",
          text, (unsigned long)size, (unsigned long)size);
 
  for(i = 0; i<size; i += width) {
 
    fprintf(stream, "%4.4lx: ", (unsigned long)i);
 
    if(!nohex) {
      /* hex not disabled, show it */
      for(c = 0; c < width; c++)
        if(i + c < size)
          fprintf(stream, "%02x ", ptr[i + c]);
        else
          fputs("   ", stream);
    }
 
    for(c = 0; (c < width) && (i + c < size); c++) {
      /* check for 0D0A; if found, skip past and start a new line of output */
      if(nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D &&
         ptr[i + c + 1] == 0x0A) {
        i += (c + 2 - width);
        break;
      }
      fprintf(stream, "%c",
              (ptr[i + c] >= 0x20) && (ptr[i + c]<0x80)?ptr[i + c]:'.');
      /* check again for 0D0A, to avoid an extra \n if it's at width */
      if(nohex && (i + c + 2 < size) && ptr[i + c + 1] == 0x0D &&
         ptr[i + c + 2] == 0x0A) {
        i += (c + 3 - width);
        break;
      }
    }
    fputc('\n', stream); /* newline */
  }
  fflush(stream);
}
 
static
int curl_trace(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp)
{
  struct data *config = (struct data *)userp;
  const char *text;
  (void)handle; /* prevent compiler warning */
 
  switch(type) {
  case CURLINFO_TEXT:
    fprintf(stderr, "== Info: %s", data);
    /* FALLTHROUGH */
  default: /* in case a new one is introduced to shock us */
    return 0;
 
  case CURLINFO_HEADER_OUT:
    text = "=> Send header";
    break;
  case CURLINFO_DATA_OUT:
    text = "=> Send data";
    break;
  case CURLINFO_SSL_DATA_OUT:
    text = "=> Send SSL data";
    break;
  case CURLINFO_HEADER_IN:
    text = "<= Recv header";
    break;
  case CURLINFO_DATA_IN:
    text = "<= Recv data";
    break;
  case CURLINFO_SSL_DATA_IN:
    text = "<= Recv SSL data";
    break;
  }
 
  curl_dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
  return 0;
}

static int get_user_token(const char *user_name, const char *passwd, const char *region, char **token_str)
{
    int ret = 0;
    static char site_cn[] = "https://bambulab.cn/api/sign-in/form";
    static char site_global[] = "https://bambulab.com/api/sign-in/form";
    static char body[256] = {0};
    int is_global_site = 0;
    CURL *curl = NULL;
    CURLcode res;
    struct curl_slist *header_list = NULL;
    struct curl_slist *cookies = NULL;

    do
    {
        if (0 == _stricmp(region,  "cn"))
        {
            is_global_site = 0;
        }
        else
        {
            is_global_site = 1;
        }

        _snprintf(body, sizeof(body), 
            "{\"account\":\"%s\",\"password\":\"%s\",\"apiError\":\"\"}", user_name, passwd);

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if (curl == NULL)
        {
            ret = -1;
            fprintf(stderr, "curl_easy_init failed\n");
            break;
        }

        curl_easy_setopt(curl, CURLOPT_URL, is_global_site ? site_global : site_cn);

        /* enable the cookie engine */
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        header_list = curl_slist_append(header_list, 
            "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &body[0]);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            ret = -1;
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
            break;
        }

        res = curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &cookies);
        if (res)
        {
            ret = -1;
            fprintf(stderr, "curl get cookies failed: %s\n",
                curl_easy_strerror(res));
            break;
        }

        if (cookies == NULL)
        {
            ret = -1;
            fprintf(stderr, "cookies is empty\n");
            break;
        }

        struct curl_slist *each = cookies;
        const char *cookies_token_str = NULL;
        while (each) 
        {
            const char *s = NULL;
            const char *sep = "\t";
            size_t idx = 0;
            s = strstr(each->data, sep);
            if (s == NULL)
            {
                sep = " ";
                s = strstr(each->data, sep);
            }
            do
            {
                if (*(s + 1) == '\0')
                {
                    break;
                }

                if (*(s + 1) == sep[0])
                {
                    continue;
                }
                idx++;

                if (idx < 5)
                {
                    continue;
                }

                if (idx == 5)
                {
                    if (0 != strncmp(s + 1, "token", 5)) 
                    {
                        break;
                    }
                }
                else if (idx == 6)
                {
                    cookies_token_str = s + 1;
                    break;
                }
            } while((s = strstr(s + 1, sep)));

            if (cookies_token_str != NULL)
            {
                break;
            }
            each = each->next;
        }

        if (cookies_token_str == NULL)
        {
            fprintf(stderr, "error get token\n");
            ret = -1;
            break;
        }

        if (token_str)
        {
            size_t len = strlen(cookies_token_str) + sizeof('\0');
            char *token_temp = (char *)malloc(len);
            if (token_temp == NULL)
            {
                ret = -1;
                fprintf(stderr, "error malloc memory for token\n");
                break;
            }

            strcpy(token_temp, cookies_token_str);
            *token_str = token_temp;
        }

        ret = 0;
    } while (false);

    if (cookies != NULL)
    {
        curl_slist_free_all(cookies);
        cookies = NULL;
    }

    if (header_list != NULL)
    {
        curl_slist_free_all(header_list);
        header_list = NULL;
    }

    if (curl != NULL)
    {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    
    curl_global_cleanup();

    return ret;
}

static int get_user_id(const char *token, const char *region, char **user_id)
{
    int ret = 0;
    static char site_cn[] = "https://api.bambulab.cn/v1/user-service/my/profile";
    static char site_global[] = "https://api.bambulab.com/v1/user-service/my/profile";
    static char header_auth[4096] = {0};
    static char body[256] = {0};
    int is_global_site = 0;
    static char rsp_json[1024] = {0};
    CURL *curl = NULL;
    CURLcode res;
    struct curl_slist *header_list = NULL;

    cJSON *ret_json = NULL;
    cJSON *uid_str_item = NULL;

    do
    {
        if (0 == _stricmp(region,  "cn"))
        {
            is_global_site = 0;
        }
        else
        {
            is_global_site = 1;
        }

        _snprintf(header_auth, sizeof(header_auth), 
            "Authorization: Bearer %s",
            token);

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if (curl == NULL)
        {
            ret = -1;
            fprintf(stderr, "curl_easy_init failed\n");
            break;
        }

        curl_easy_setopt(curl, CURLOPT_URL, is_global_site ? site_global : site_cn);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        header_list = curl_slist_append(header_list, 
            "Content-Type: application/json");
        header_list = curl_slist_append(header_list, "accept: application/json");
        header_list = curl_slist_append(header_list, header_auth);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

        struct bambu_curl_memory_data rsp_data = {0};
        rsp_data.data = &rsp_json[0];
        rsp_data.total_size = sizeof(rsp_json) - sizeof('\0');
        rsp_data.pos = 0;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rsp_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&rsp_data);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            ret = -1;
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
            break;
        }

        ret_json = cJSON_Parse(rsp_json);
        if (ret_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "Parse json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        uid_str_item = cJSON_GetObjectItem(ret_json, "uidStr");
        if (uid_str_item == NULL 
            || !cJSON_IsString(uid_str_item))
        {
            ret = -1;
            fprintf(stderr, "failed get uidStr\n %s\n", rsp_json);
            break;
        }

        if (user_id != NULL)
        {
            size_t len = strlen(cJSON_GetStringValue(uid_str_item)) + sizeof('\0');
            char *str = (char *)malloc(len);
            strcpy(str, cJSON_GetStringValue(uid_str_item));
            *user_id = str;
        }

        ret = 0;
    } while (false);

    if (ret_json != NULL)
    {
        cJSON_Delete(ret_json);
        ret_json = NULL;
    }

    if (header_list != NULL)
    {
        curl_slist_free_all(header_list);
        header_list = NULL;
    }

    if (curl != NULL)
    {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    
    curl_global_cleanup();

    return ret;
}

static int enum_dev_lst(const char *token, const char *region, enum_dev_lst_cb callback, void *ctx)
{
    int ret = 0;
    static char site_cn[] = "https://api.bambulab.cn/v1/iot-service/api/user/bind";
    static char site_global[] = "https://api.bambulab.com/v1/iot-service/api/user/bind";
    static char header_auth[4096] = {0};
    static char body[256] = {0};
    int is_global_site = 0;
    static char rsp_json[1024] = {0};
    CURL *curl = NULL;
    CURLcode res;
    struct curl_slist *header_list = NULL;

    cJSON *ret_json = NULL;
    cJSON *message_item = NULL;
    cJSON *devices_item = NULL;
    cJSON *device_info_item = NULL;
    cJSON *dev_id_item = NULL;

    do
    {
        if (0 == _stricmp(region,  "cn"))
        {
            is_global_site = 0;
        }
        else
        {
            is_global_site = 1;
        }

        _snprintf(header_auth, sizeof(header_auth), 
            "Authorization: Bearer %s",
            token);

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if (curl == NULL)
        {
            ret = -1;
            fprintf(stderr, "curl_easy_init failed\n");
            break;
        }

        curl_easy_setopt(curl, CURLOPT_URL, is_global_site ? site_global : site_cn);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        header_list = curl_slist_append(header_list, 
            "Content-Type: application/json");
        header_list = curl_slist_append(header_list, "accept: application/json");
        header_list = curl_slist_append(header_list, header_auth);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

        struct bambu_curl_memory_data rsp_data = {0};
        rsp_data.data = &rsp_json[0];
        rsp_data.total_size = sizeof(rsp_json) - sizeof('\0');
        rsp_data.pos = 0;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rsp_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&rsp_data);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            ret = -1;
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
            break;
        }

        ret_json = cJSON_Parse(rsp_json);
        if (ret_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "Parse json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        message_item = cJSON_GetObjectItem(ret_json, "message");
        if (message_item == NULL 
            || !cJSON_IsString(message_item)
            || 0 != strcmp(cJSON_GetStringValue(message_item), "success"))
        {
            ret = -1;
            fprintf(stderr, "iot req failed\n %s\n", rsp_json);
            break;
        }

        devices_item = cJSON_GetObjectItem(ret_json, "devices");
        if (devices_item == NULL 
            || !cJSON_IsArray(devices_item)
            || cJSON_GetArraySize(devices_item) <= 0)
        {
            ret = -1;
            fprintf(stderr, "failed get devices list\n %s\n", rsp_json);
            break;
        }

        size_t item_count = cJSON_GetArraySize(devices_item);
        size_t i = 0;
        for (i = 0; i < item_count; i++)
        {
            device_info_item = cJSON_GetArrayItem(devices_item, i);
            if (device_info_item == NULL 
                || !cJSON_IsObject(device_info_item))
            {
                ret = -1;
                fprintf(stderr, "failed get device info object idx: %u\n %s\n", (unsigned int)i, rsp_json);
                break;
            }

            if (!callback(ctx, device_info_item))
            {
                break;
            }
        }
    } while (false);

    if (ret_json != NULL)
    {
        cJSON_Delete(ret_json);
        ret_json = NULL;
    }

    if (header_list != NULL)
    {
        curl_slist_free_all(header_list);
        header_list = NULL;
    }

    if (curl != NULL)
    {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    
    curl_global_cleanup();

    return ret;
}

static int get_camera_url(char camera_url[256], const char *user_id, const char *dev_id, const char *token_str, const char *region)
{
    int ret = 0;
    static char site_cn[] = "https://api.bambulab.cn/v1/iot-service/api/user/ttcode";
    static char site_global[] = "https://api.bambulab.com/v1/iot-service/api/user/ttcode";
    static char header_auth[4096] = {0};
    static char header_userid[256] = {0};
    static char body[256] = {0};
    int is_global_site = 0;
    static char rsp_json[1024] = {0};
    CURL *curl = NULL;
    CURLcode res;
    struct curl_slist *header_list = NULL;

    cJSON *iot_json = NULL;
    cJSON *message_item = NULL;
    cJSON *ttcode_item = NULL;
    cJSON *passwd_item = NULL;
    cJSON *authkey_item = NULL;

    do
    {
        if (0 == _stricmp(region,  "cn"))
        {
            is_global_site = 0;
        }
        else
        {
            is_global_site = 1;
        }

        _snprintf(header_auth, sizeof(header_auth), 
            "Authorization: Bearer %s",
            token_str);
        _snprintf(header_userid, sizeof(header_userid), 
            "user-id: %s",
            user_id);
        _snprintf(body, sizeof(body), 
            "{\"dev_id\": \"%s\"}", dev_id);

        curl_global_init(CURL_GLOBAL_DEFAULT);

        curl = curl_easy_init();
        if (curl == NULL)
        {
            ret = -1;
            fprintf(stderr, "curl_easy_init failed\n");
            break;
        }

        // struct data config;
        // config.trace_ascii = 1; /* enable ascii tracing */
        // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);
        // curl_easy_setopt(curl, CURLOPT_DEBUGDATA, &config);

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        curl_easy_setopt(curl, CURLOPT_URL, is_global_site ? site_global : site_cn);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        header_list = curl_slist_append(header_list, 
            "Content-Type: application/json");
        header_list = curl_slist_append(header_list, "accept: application/json");
        header_list = curl_slist_append(header_list, header_userid);
        header_list = curl_slist_append(header_list, header_auth);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, &body[0]);

        // struct bambu_curl_memory_data body_data = {0};
        // body_data.data = &body[0];
        // body_data.total_size = strlen(body) + 1;
        // body_data.pos = 0;
        //curl_easy_setopt(curl, CURLOPT_READFUNCTION, body_read_callback);
        //curl_easy_setopt(curl, CURLOPT_READDATA, (void *)&body_data);

        struct bambu_curl_memory_data rsp_data = {0};
        rsp_data.data = &rsp_json[0];
        rsp_data.total_size = sizeof(rsp_json) - sizeof('\0');
        rsp_data.pos = 0;
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, rsp_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&rsp_data);

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            ret = -1;
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
            break;
        }

        iot_json = cJSON_Parse(rsp_json);
        if (iot_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "Parse json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        message_item = cJSON_GetObjectItem(iot_json, "message");
        if (message_item == NULL 
            || !cJSON_IsString(message_item)
            || 0 != strcmp(cJSON_GetStringValue(message_item), "success"))
        {
            ret = -1;
            fprintf(stderr, "iot req failed\n %s\n", rsp_json);
            break;
        }

        ttcode_item = cJSON_GetObjectItem(iot_json, "ttcode");
        if (ttcode_item == NULL
            || !cJSON_IsString(ttcode_item))
        {
            ret = -1;
            fprintf(stderr, "failed get ttcode\n %s\n", rsp_json);
            break;
        }

        passwd_item = cJSON_GetObjectItem(iot_json, "passwd");
        if (passwd_item == NULL
            || !cJSON_IsString(passwd_item))
        {
            ret = -1;
            fprintf(stderr, "failed get passwd\n %s\n", rsp_json);
            break;
        }

        authkey_item = cJSON_GetObjectItem(iot_json, "authkey");
        if (authkey_item == NULL
            || !cJSON_IsString(authkey_item))
        {
            ret = -1;
            fprintf(stderr, "failed get authkey\n %s\n", rsp_json);
            break;
        }

        _snprintf(camera_url, 256, "bambu:///%s?authkey=%s&passwd=%s&region=%s",
            cJSON_GetStringValue(ttcode_item),
            cJSON_GetStringValue(authkey_item),
            cJSON_GetStringValue(passwd_item),
            region
            );

        ret = 0;
    } while (false);

    if (iot_json != NULL)
    {
        cJSON_Delete(iot_json);
        iot_json = NULL;
    }

    if (header_list != NULL)
    {
        curl_slist_free_all(header_list);
        header_list = NULL;
    }

    if (curl != NULL)
    {
        curl_easy_cleanup(curl);
        curl = NULL;
    }
    
    curl_global_cleanup();

    return ret;
}
#endif

static int get_camera_url_local(char camera_url[256], const char *ip_addr, const char *access_code)
{
    _snprintf(camera_url, 256, "bambu:///local/%s.?port=6000&user=bblp&passwd=%s",
        ip_addr, access_code);
    return 0;
}

static int get_first_dev_id_enum_func(void *ctx, cJSON *device_info_item)
{
    char **dev_id = (char **)ctx;
    cJSON *dev_id_item = NULL;
    dev_id_item = cJSON_GetObjectItem(device_info_item, "dev_id");
    if (dev_id_item == NULL 
        || !cJSON_IsString(dev_id_item))
    {
        fprintf(stderr, "failed get dev_id\n");
        return 0;
    }

    if (dev_id != NULL)
    {
        size_t len = strlen(cJSON_GetStringValue(dev_id_item)) + sizeof('\0');
        char *str = (char *)malloc(len);
        strcpy(str, cJSON_GetStringValue(dev_id_item));
        *dev_id = str;
    }

    return 0;
}

static int get_first_dev_id(const char *token, const char *region, char **dev_id)
{
    return enum_dev_lst(token, region, get_first_dev_id_enum_func, dev_id);
}

static int list_machine_info_enum_func(void *ctx, cJSON *device_info_item)
{
    char **dev_id = (char **)ctx;
    cJSON *dev_id_item = NULL;
    cJSON *dev_name_item = NULL;
    cJSON *dev_product_name_item = NULL;
    cJSON *dev_access_code_item = NULL;
    dev_id_item = cJSON_GetObjectItem(device_info_item, "dev_id");
    if (dev_id_item == NULL 
        || !cJSON_IsString(dev_id_item))
    {
        fprintf(stderr, "failed get dev_id\n");
        return 1;
    }

    fprintf(stderr, "---------------------\n");
    fprintf(stderr, "  dev_id: %s\n", cJSON_GetStringValue(dev_id_item));

    dev_name_item = cJSON_GetObjectItem(device_info_item, "name");
    if (dev_name_item != NULL)
    {
        fprintf(stderr, "  name: %s\n", cJSON_GetStringValue(dev_name_item));
    }

    dev_product_name_item = cJSON_GetObjectItem(device_info_item, "dev_product_name");
    if (dev_product_name_item != NULL)
    {
        fprintf(stderr, "  product: %s\n", cJSON_GetStringValue(dev_product_name_item));
    }

    dev_access_code_item = cJSON_GetObjectItem(device_info_item, "dev_access_code");
    if (dev_access_code_item != NULL)
    {
        fprintf(stderr, "  access_code: %s\n", cJSON_GetStringValue(dev_access_code_item));
    }

    return 1;
}

static int list_machine_info(const char *token, const char *region)
{
    return enum_dev_lst(token, region, list_machine_info_enum_func, NULL);
}

int gen_simple_bambu_cfg_file(const char *user_id, const char *region, const char *token,  const char *dev_id)
{
    int ret = 0;
    char bambu_cfg_path[256 + 1] = "BambuNetworkEngine.conf";
    FILE *cfg_file = NULL;
    char *cfg_data = NULL;
    cJSON *cfg_json = NULL;
    cJSON *last_monitor_machine_item = NULL;
    cJSON *user_item = NULL;
    cJSON *user_id_item = NULL;
    cJSON *token_item = NULL;
    cJSON *country_code_item = NULL;
    char *json_str = NULL;

    do
    {
        cfg_file = fopen(bambu_cfg_path, "w");
        if (cfg_file == NULL)
        {
            fprintf(stderr, "fopen failed\n");
            break;
        }
        cfg_json = cJSON_CreateObject();
        if (cfg_json == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "create cfg json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        if (cJSON_AddStringToObject(cfg_json, "country_code", region) == NULL) 
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "add country_code to json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        if (cJSON_AddStringToObject(cfg_json, "last_monitor_machine", dev_id) == NULL) 
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "add last_monitor_machine to json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        cJSON *user_item = cJSON_CreateObject();
        if (user_item == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "create user json object failed %s\n", error_ptr ? error_ptr : "");
            break;
        }
        if (cJSON_AddStringToObject(user_item, "user_id", user_id) == NULL) 
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "add user_id to json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }
        if (cJSON_AddStringToObject(user_item, "token", token) == NULL) 
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "add token to json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        if (!cJSON_AddItemToObject(cfg_json, "user", user_item))
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "add user object to json failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        json_str = cJSON_Print(cfg_json);
        if (json_str == NULL)
        {
            const char *error_ptr = cJSON_GetErrorPtr();
            ret = -1;
            fprintf(stderr, "cfg json print failed %s\n", error_ptr ? error_ptr : "");
            break;
        }

        size_t cfg_file_size = strlen(json_str) + sizeof('\0');
        fwrite(json_str, 1, cfg_file_size, cfg_file);
        fflush(cfg_file);

        ret = 0;
    } while(false);

    if (json_str != NULL)
    {
        free(json_str);
        json_str = NULL;
    }

    if (cfg_json != NULL)
    {
        cJSON_Delete(cfg_json);
        cfg_json = NULL;
    }

    if (cfg_data != NULL)
    {
        free(cfg_data);
        cfg_data = NULL;
    }

    if (cfg_file != NULL)
    {
        fclose(cfg_file);
        cfg_file = NULL;
    }

    return ret;
}

struct BambuLib lib = {0};
#if defined(_MSC_VER) || defined(_WIN32)
static HMODULE module = NULL;
#else
static void* module = NULL;
#endif

static void* get_function(const char* name)
{
    void* function = NULL;

    if (!module)
    {
        return function;
    }

#if defined(_MSC_VER) || defined(_WIN32)
    function = (void *)GetProcAddress(module, name);
#else
    function = (void *)dlsym(module, name);
#endif

    if (!function) 
    {
        fprintf(stderr, ", can not find function %s", name);
        exit(-1);
    }
    return function;
}

#define GET_FUNC(x) *((void **)&lib.x) = get_function(#x)

void bambu_log(void *ctx, int level, tchar const * msg)
{
    if (level <= 1)
    {
#if defined(_MSC_VER) || defined(_WIN32)
      fwprintf(stderr, L"[%d] %s\n", level, msg);
#else
      fprintf(stderr, "[%d] %s\n", level, msg);
#endif
      lib.Bambu_FreeLogMsg(msg);
    }
}

int start_bambu_stream(char *camera_url)
{
    Bambu_Tunnel tunnel = NULL;
    int is_bambu_open = 0;
    int ret = 0;

    do {
        fprintf(stderr, "Starting Session\n");

        ret = lib.Bambu_Create(&tunnel, camera_url);
        if (ret != 0)
        {
            fprintf(stderr, "Bambu_Create failed 0x%x\n", ret);
            break;
        }

        lib.Bambu_SetLogger(tunnel, bambu_log, tunnel);

        ret = lib.Bambu_Open(tunnel);
        if (ret != 0)
        {
            fprintf(stderr, "Bambu_Open failed: 0x%x\n", ret);
            break;
        }
        is_bambu_open++;

        size_t i;
        for (i = 0; i < BAMBUE_START_STREAM_RETRY_COUNT; i++)
        {
            ret = lib.Bambu_StartStream(tunnel, true);
            //fprintf(stderr, "Bambu_StartStream ret: 0x%x\n", ret);

            if (ret == 0)
            {
                break;
            }

#if defined(_MSC_VER) || defined(_WIN32)
            Sleep(1000);
#else
            usleep(1000 * 1000);
#endif
        }

        if (ret != 0)
        {
            fprintf(stderr, "Bambu_StartStream failed 0x%x\n", ret);
            break;
        }

        int result = 0;
        while (true) 
        {
            Bambu_Sample sample;
            result = lib.Bambu_ReadSample(tunnel, &sample);

            if (result == Bambu_success) 
            {
                fwrite(sample.buffer, 1, sample.size, stdout);
                fflush(stdout);
                continue;
            } 
            else if (result == Bambu_would_block) 
            {
#if defined(_MSC_VER) || defined(_WIN32)
                Sleep(100);
#else
                usleep(100 * 1000);
#endif
                continue;
            } 
            else if (result == Bambu_stream_end) 
            {
                fprintf(stderr, "Bambu_stream_end\n");
                result = 0;
            } 
            else 
            {
                result = -1;
                fprintf(stderr, "ERROR_PIPE\n");
                ret = -1;
            }
            break;
        }
    } while (false);

    if (is_bambu_open)
    {
        lib.Bambu_Close(tunnel);
    }

    if (tunnel != NULL)
    {
        lib.Bambu_Destroy(tunnel);
        tunnel = NULL;
    }

    return ret;
}

void print_usage(const char *app)
{
    fprintf(stderr, "%s usage:\n", app);
    fprintf(stderr, "   %s start_stream [options]\n", app);
    fprintf(stderr, "      Start a camera stream and write the raw stream to stdout\n");
    fprintf(stderr, "   %s start_stream_local -s <ip_addr_of_p1p> -a <access_code_of_p1p>\n", app);
    fprintf(stderr, "      Start a local camera stream for p1p and write the raw stream to stdout\n");
    fprintf(stderr, "   %s gen_cfg -u <account_name> -p <password> -r <region: us cn> [-d <dev_id>] [other_options]\n", app);
    fprintf(stderr, "      Generate a simple BambuNetworkEngine.conf file\n");
    fprintf(stderr, "   %s list_dev [options]\n", app);
    fprintf(stderr, "      list machines info and access code\n");
    fprintf(stderr, "\n");
    fprintf(stderr, " options:\n");
    fprintf(stderr, "   -u <account_name>\n");
    fprintf(stderr, "   -p <password>\n");
    fprintf(stderr, "   -t <token>\n");
    fprintf(stderr, "   -r <region>\n");
    fprintf(stderr, "      us cn\n");
    fprintf(stderr, "   -i <user_id>\n");
    fprintf(stderr, "   -d <dev_id>\n");
    fprintf(stderr, "   -s <ip_addr_of_p1p>\n");
    fprintf(stderr, "   -a <access_code_of_p1p>\n");

}

enum bbl_app_mode {
    BBL_START_STREAM = 1,
    BBL_GEN_CFG,
    BBL_LIST_DEV,
    BBL_START_STREAM_LOCAL,
};

#if defined(_MSC_VER) || defined(_WIN32)
int __cdecl main(int argc, char * argv[])
#else
int main(int argc, char * argv[])
#endif
{
    static char region_cn[] = "cn";
    static char region_us[] = "us";
    int ret = 0;
    int arg_offset = 0;
    enum bbl_app_mode app_mode = BBL_START_STREAM;
    int is_bambu_init = 0;
    char *user_name = NULL;
    char *passwd = NULL;
    char *user_id = NULL;
    char *token = NULL;
    char *dev_id = NULL;
    char *p1p_ip_addr = NULL;
    char *p1p_access_code = NULL;
    int is_region_specified = 0;
    char *region = region_us;
    char camera_url[256] = {0};

    fprintf(stderr, "by hisptoot 2022.12.26\n");


    if (argc > 1
        && (0 == strcmp(argv[1], "help")
            || 0 == strcmp(argv[1], "--help")
            || 0 == strcmp(argv[1], "-h")
            ))
    {
        print_usage(argv[0]);
        return 0;
    }
    else if (argc > 1 
        && 0 == strcmp(argv[1], "gen_cfg"))
    {
        arg_offset++;
        app_mode = BBL_GEN_CFG;
    }
    else if (argc > 1 
            && 0 == strcmp(argv[1], "start_stream"))
    {
        arg_offset++;
        app_mode = BBL_START_STREAM;
    }
    else if (argc > 1
            && 0 == strcmp(argv[1], "start_stream_local"))
    {
        arg_offset++;
        app_mode = BBL_START_STREAM_LOCAL;
    }
    else if (argc > 1 
            && 0 == strcmp(argv[1], "list_dev"))
    {
        arg_offset++;
        app_mode = BBL_LIST_DEV; 
    }

    int option;
    while ((option = getopt(argc - arg_offset, &argv[arg_offset], "u:p:t:r:i:d:s:a:")) !=
           -1) 
    {
        switch (option) 
        {
        case 'u':
            user_name = optarg;
            fprintf(stderr, "user_name: %s\n", optarg);
            break;
        case 'p':
            fprintf(stderr, "passwd specified by user\n");
            passwd = optarg;
            break;
        case 't':
            fprintf(stderr, "token specified by user\n");
            token = optarg;
            break;
        case 'r':
            fprintf(stderr, "region: %s\n", optarg);
            is_region_specified++;
            region = optarg;
            break;
        case 'i':
            fprintf(stderr, "user_id: %s\n", optarg);
            user_id = optarg;
            break;
        case 'd':
            fprintf(stderr, "dev_id: %s\n", optarg);
            dev_id = optarg;
            break;
        case 's':
            fprintf(stderr, "p1p ip: %s\n", optarg);
            p1p_ip_addr = optarg;
            break;
        case 'a':
            fprintf(stderr, "access code specified by user\n");
            p1p_access_code = optarg;
            break;
        default:
            break;
        }
    }

#if defined(_MSC_VER) || defined(_WIN32)
    module = LoadLibraryA("BambuSource.dll");
    if (module == NULL)
    {
        fprintf(stderr, "Failed loading BambuSource.dll\n");
        return -1;
    }
#else

    static char lib_path[1024] = {0};
    char *p = NULL;
    int n = readlink("/proc/self/exe", lib_path, 1023);
    if (NULL != (p = strrchr(lib_path, '/')))
    {
        *p = '\0';
    }
    else
    {
        lib_path[0] = '.';
    }
    strcat(lib_path, "/libBambuSource.so");
    //fprintf(stderr, "loading %s\n", lib_path);
    module = dlopen(lib_path, RTLD_LAZY);
    if (module == NULL)
    {
        fprintf(stderr, "Failed loading libBambuSource.so\n");
        return -1;
    }
#endif

    GET_FUNC(Bambu_Init);
    GET_FUNC(Bambu_Deinit);
    GET_FUNC(Bambu_Create);
    GET_FUNC(Bambu_Destroy);
    GET_FUNC(Bambu_Open);
    GET_FUNC(Bambu_StartStream);
    GET_FUNC(Bambu_SendMessage);
    GET_FUNC(Bambu_ReadSample);
    GET_FUNC(Bambu_Close);
    GET_FUNC(Bambu_SetLogger);
    GET_FUNC(Bambu_FreeLogMsg);
    GET_FUNC(Bambu_GetLastErrorMsg);
    GET_FUNC(Bambu_GetStreamCount);
    GET_FUNC(Bambu_GetDuration);
    GET_FUNC(Bambu_GetStreamInfo);

#if defined(_MSC_VER) || defined(_WIN32)
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    do
    {
        // ret = lib.Bambu_Init();
        // if (0 != ret)
        // {
        //     ret = -1;
        //     fprintf(stderr, "Bambu_Init failed 0x%x\n", ret);
        //     break;
        // }
        is_bambu_init++;

        if (token == NULL) 
        {
            if (user_name != NULL && passwd != NULL) 
            {
                fprintf(stderr, "getting token by login\n");
                if (0 != get_user_token(user_name, passwd, region, &token))
                {
                    fprintf(stderr, "get_user_token by login request failed\n");
                }
            } 

            if (token == NULL)
            {
                char *temp_user_id = NULL;
                char *temp_dev_id = NULL;
                char *temp_region = NULL;

                if (app_mode == BBL_GEN_CFG)
                {
                    fprintf(stderr, "token is unknown, check username, password and region\n");
                    break;
                }

                fprintf(stderr, "getting user info by BambuNetworkEngine.conf\n");
                if (0 != get_bambu_studio_user_info(&temp_user_id, &temp_dev_id, &token, &temp_region)) 
                {
                    ret = -1;
                    fprintf(stderr, "get_bambu_studio_user_info failed\n");
                    break;
                }

                if (user_id == NULL) 
                {
                    user_id = temp_user_id;
                } 
                else 
                {
                    free(temp_user_id);
                    temp_user_id = NULL;
                }

                if (dev_id == NULL) 
                {
                    dev_id = temp_dev_id;
                } 
                else 
                {
                    free(temp_dev_id);
                    temp_dev_id = NULL;
                }

                if (!is_region_specified) 
                {
                    region = temp_region;
                } 
                else 
                {
                    free(temp_region);
                    temp_region = NULL;
                }
            }
        }

        if (user_id == NULL) 
        {
            fprintf(stderr, "getting user_id by token\n");
            get_user_id(token, region, &user_id);
            if (user_id == NULL) 
            {
                fprintf(stderr, "failed get_user_id\n");
                break;
            }
            fprintf(stderr, "user_id: %s\n", user_id);
        }

        if (dev_id == NULL) 
        {
            if (app_mode != BBL_LIST_DEV)
            {
                fprintf(stderr, "getting dev_id by token\n");
                get_first_dev_id(token, region, &dev_id);
                if (dev_id == NULL) 
                {
                    fprintf(stderr, "failed get_dev_id\n");
                    break;
                }
                fprintf(stderr, "dev_id: %s\n", dev_id);
            }
        }

        fprintf(stderr, "region: %s user_id: %s dev_id: %s\n", region, user_id, dev_id);
        if (app_mode == BBL_GEN_CFG)
        {
            if (0 != gen_simple_bambu_cfg_file(user_id, region, token, dev_id))
            {
                fprintf(stderr, "gen_simple_bambu_cfg_file failed\n");
                break;
            }
        }
        else if (app_mode == BBL_START_STREAM)
        {
            if (0 != get_camera_url(camera_url, user_id, dev_id, token, region))
            {
                fprintf(stderr, "get_camera_url failed\n");
                break;
            }
            //fprintf(stderr, "camera_url: %s\n", camera_url);
            ret = start_bambu_stream(camera_url);
            if (!ret)
            {
                fprintf(stderr, "start_bambu_stream failed %d\n", ret);
            }
        }
        else if (app_mode == BBL_START_STREAM_LOCAL)
        {
            if (p1p_ip_addr == NULL || p1p_access_code == NULL)
            {
                fprintf(stderr, "unknow p1p ip addr or access code\n");
                break;
            }
            if (0 != get_camera_url_local(camera_url, p1p_ip_addr, p1p_access_code))
            {
                fprintf(stderr, "get_camera_url_local failed\n");
                break;
            }
            //fprintf(stderr, "camera_url: %s\n", camera_url);
            ret = start_bambu_stream(camera_url);
            if (!ret)
            {
                fprintf(stderr, "start_bambu_stream failed %d\n", ret);
            }
        }
        else if (app_mode == BBL_LIST_DEV)
        {
            if (0 != list_machine_info(token, region))
            {
                fprintf(stderr, "list_machine_info failed %d\n", ret);
            }
        }
    } while (false);

    if (is_bambu_init)
    {
        // lib.Bambu_Deinit();
        is_bambu_init = 0;
    }

    if (user_id != NULL)
    {
        free(user_id);
        user_id = NULL;
    }

    if (dev_id != NULL)
    {
        free(dev_id);
        dev_id = NULL;
    }

    if (token != NULL)
    {
        free(token);
        token = NULL;
    }

    return 0;
}
