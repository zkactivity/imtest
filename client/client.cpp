/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2015, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/ 
/* <DESC>
 * simple HTTP POST using the easy interface
 * </DESC>
 */ 
#include <stdio.h>
#include <curl/curl.h>
 
int main(void)
{
  CURL *curl;
  CURLcode res;
 
  /* In windows, this will init the winsock stuff */ 
  curl_global_init(CURL_GLOBAL_ALL);
 
  /* get a curl handle */ 
  curl = curl_easy_init();
  if(curl) {
    /* First set the URL that is about to receive our POST. This URL can
       just as well be a https:// URL if that is what should receive the
       data. */ 
    curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.199.216:9877");
	curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    struct curl_slist *chunk = NULL;
    chunk = curl_slist_append(chunk, "Upgrade: websocket");
    chunk = curl_slist_append(chunk, "Connection: Upgrade");
    chunk = curl_slist_append(chunk, "Sec-WebSocket-Accept: fFBooB7FAkLlXgRSz0BT3v4hq5s=");
    chunk = curl_slist_append(chunk, "Sec-WebSocket-Location: ws://localhost/");
	
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
 	
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout); //将返回的http头输出到fp指向的文件
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, stdout); //将返回的html主体数据输出到fp指向的文件

    /* Perform the request, res will get the return code */ 
    res = curl_easy_perform(curl);
    /* Check for errors */ 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    /* always cleanup */ 
    curl_easy_cleanup(curl);
  }
  curl_global_cleanup();
  return 0;
}
