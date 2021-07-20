#ifndef CNSWRESOURCE_H
#define CNSWRESOURCE_H
#include <Wt/WResource.h>
#include <iostream>
#include <Wt/Http/Response.h>
#include <curl/curl.h>
#include <Wt/Http/Client.h>
#include <Wt/Http/Message.h>
#include "string.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <Wt/WRasterImage.h>
#include <Wt/WRectF.h>
#include <Wt/WImage.h>
#include <Wt/WPainter.h>

using namespace Wt;
using namespace Wt::Http;

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

// Wrapper du service ArcGis du SPW pour la CNSW; on contourne le bridage qui empêche de visualiser la CNSW à des échelles trop fines.
class cnswresource : public WResource {
public:
    cnswresource(std::string aTmpDir);
    virtual ~cnswresource();
   protected:
    virtual void handleRequest(const Request &request, Response &response);
private:
    std::string mTmpDir;
};

#endif // CNSWRESOURCE_H
