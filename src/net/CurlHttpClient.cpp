/*
 * Copyright (c) 2018 Egor Pugin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifdef HAVE_CURL

#include "tgbot/net/CurlHttpClient.h"

#include <boost/asio/ssl.hpp>

namespace TgBot {

CurlHttpClient::CurlHttpClient() : _httpParser() {
    curlSettings = curl_easy_init();
}

CurlHttpClient::~CurlHttpClient() {
    curl_easy_cleanup(curlSettings);
}

static size_t curlWriteString(char* ptr, size_t size, size_t nmemb, void* userdata) {
    static_cast<std::string *>(userdata)->append(ptr, size * nmemb);
    return size * nmemb;
};

std::string CurlHttpClient::makeRequest(const Url& url, const std::vector<HttpReqArg>& args) const {
    // Copy settings for each call because we change CURLOPT_URL and other stuff.
    // This also protects multithreaded case.
    auto curl = curl_easy_duphandle(curlSettings);

    std::string u = url.protocol + "://" + url.host + url.path;
    curl_easy_setopt(curl, CURLOPT_URL, u.c_str());

    // disable keep-alive
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Connection: close");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_mime *mime;
    curl_mimepart *part;
    mime = curl_mime_init(curl);
    if (!args.empty()) {
        for (const HttpReqArg& a : args) {
            part = curl_mime_addpart(mime);

            curl_mime_data(part, a.value.c_str(), a.value.size());
            curl_mime_type(part, a.mimeType.c_str());
            curl_mime_name(part, a.name.c_str());
            if (a.isFile) {
                curl_mime_filename(part, a.fileName.c_str());
            }
        }
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteString);

    auto res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_mime_free(mime);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    }

    return _httpParser.extractBody(response);
}

}

#endif
