#include "stdafx.h"
#include "mimetypesmap.h"
#include <ams/stringutils.h>
namespace {
// mime type map. This map will give an html content-type when supplied with an extension:
std::map<std::string, std::string> mimeTypeMap = {
    { "au", "audio/basic" },
    { "avi", "video/msvideo" },
    { "bmp", "image/bmp" },
    { "bz2", "application/x-bzip2" },
    { "css", "text/css" },
    { "dtd", "application/xml-dtd" },
    { "doc", "application/msword" },
    { "docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
    { "dotx", "application/vnd.openxmlformats-officedocument.wordprocessingml.template" },
    { "es", "application/ecmascript" },
    { "exe", "application/octet-stream" },
    { "gif", "image/gif" },
    { "gz", "application/x-gzip" },
    { "hqx", "application/mac-binhex40" },
    { "htm", "text/html" },
    { "html", "text/html" },
    { "jar", "application/java-archive" },
    { "jpg", "image/jpeg" },
    { "js", "application/x-javascript" },
    { "json", "application/x-json" },
    { "midi", "audio/x-midi" },
    { "mp3", "audio/mpeg" },
    { "mpeg", "video/mpeg" },
    { "ogg", "audio/vorbis" },
    { "pdf", "application/pdf" },
    { "pl", "application/x-perl" },
    { "png", "image/png" },
    { "potx", "application/vnd.openxmlformats-officedocument.presentationml.template" },
    { "ppsx", "application/vnd.openxmlformats-officedocument.presentationml.slideshow" },
    { "ppt", "application/vnd.ms-powerpointtd>" },
    { "pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation" },
    { "ps", "application/postscript" },
    { "qt", "video/quicktime" },
    { "rtf", "application/rtf" },
    { "sgml", "text/sgml" },
    { "sit", "application/x-stuffit" },
    { "sldx", "application/vnd.openxmlformats-officedocument.presentationml.slide" },
    { "svg", "image/svg+xml" },
    { "swf", "application/x-shockwave-flash" },
    { "tar.gz", "application/x-tar" },
    { "tgz", "application/x-tar" },
    { "tiff", "image/tiff" },
    { "tsv", "text/tab-separated-values" },
    { "txt", "text/plain" },
    { "wav", "audio/wav" },
    { "xlam", "application/vnd.ms-excel.addin.macroEnabled.12" },
    { "xls", "application/vnd.ms-excel" },
    { "xlsb", "application/vnd.ms-excel.sheet.binary.macroEnabled.12" },
    { "xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
    { "xltx", "application/vnd.openxmlformats-officedocument.spreadsheetml.template" },
    { "xml", "application/xml" },
    { "zip", "application/zip" }
};
}

std::string GetMimeType(std::string s)
{
    std::string result;
    ams::MakeLower(s);
    auto p = mimeTypeMap.find(s);
    if (p != mimeTypeMap.end())
    {
        result = p->second;
    }
    else
    {
        result = "";
    }
    return result;
}
