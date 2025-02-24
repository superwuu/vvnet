#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>

#include <sys/stat.h>
#include <string.h>

std::unordered_map<int,std::string> _statusInfo={
    {100, "Continue"},
    {101, "Switching Protocol"},
    {102, "Processing"},
    {103, "Early Hints"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {207, "Multi-Status"},
    {208, "Already Reported"},
    {226, "IM Used"},
    {300, "Multiple Choice"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "unused"},
    {307, "Temporary Redirect"},
    {308, "Permanent Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Payload Too Large"},
    {414, "URI Too Long"},
    {415, "Unsupported Media Type"},
    {416, "Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {418, "I'm a teapot"},
    {421, "Misdirected Request"},
    {422, "Unprocessable Entity"},
    {423, "Locked"},
    {424, "Failed Dependency"},
    {425, "Too Early"},
    {426, "Upgrade Required"},
    {428, "Precondition Required"},
    {429, "Too Many Requests"},
    {431, "Request Header Fields Too Large"},
    {451, "Unavailable For Legal Reasons"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {506, "Variant Also Negotiates"},
    {507, "Insufficient Storage"},
    {508, "Loop Detected"},
    {510, "Not Extended"},
    {511, "Network Authentication Required"}
};

std::unordered_map<std::string,std::string> _mimeType={
    {".aac", "audio/aac"},
    {".abw", "application/x-abiword"},
    {".arc", "application/x-freearc"},
    {".avi", "video/x-msvideo"},
    {".azw", "application/vnd.amazon.ebook"},
    {".bin", "application/octet-stream"},
    {".bmp", "image/bmp"},
    {".bz", "application/x-bzip"},
    {".bz2", "application/x-bzip2"},
    {".csh", "application/x-csh"},
    {".css", "text/css"},
    {".csv", "text/csv"},
    {".doc", "application/msword"},
    {".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
    {".eot", "application/vnd.ms-fontobject"},
    {".epub", "application/epub+zip"},
    {".gif", "image/gif"},
    {".htm", "text/html"},
    {".html", "text/html"},
    {".ico", "image/vnd.microsoft.icon"},
    {".ics", "text/calendar"},
    {".jar", "application/java-archive"},
    {".jpeg", "image/jpeg"},
    {".jpg", "image/jpeg"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".jsonld", "application/ld+json"},
    {".mid", "audio/midi"},
    {".midi", "audio/x-midi"},
    {".mjs", "text/javascript"},
    {".mp3", "audio/mpeg"},
    {".mpeg", "video/mpeg"},
    {".mpkg", "application/vnd.apple.installer+xml"},
    {".odp", "application/vnd.oasis.opendocument.presentation"},
    {".ods", "application/vnd.oasis.opendocument.spreadsheet"},
    {".odt", "application/vnd.oasis.opendocument.text"},
    {".oga", "audio/ogg"},
    {".ogv", "video/ogg"},
    {".ogx", "application/ogg"},
    {".otf", "font/otf"},
    {".png", "image/png"},
    {".pdf", "application/pdf"},
    {".ppt", "application/vnd.ms-powerpoint"},
    {".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
    {".rar", "application/x-rar-compressed"},
    {".rtf", "application/rtf"},
    {".sh", "application/x-sh"},
    {".svg", "image/svg+xml"},
    {".swf", "application/x-shockwave-flash"},
    {".tar", "application/x-tar"},
    {".tif", "image/tiff"},
    {".tiff", "image/tiff"},
    {".ttf", "font/ttf"},
    {".txt", "text/plain"},
    {".vsd", "application/vnd.visio"},
    {".wav", "audio/wav"},
    {".weba", "audio/webm"},
    {".webm", "video/webm"},
    {".webp", "image/webp"},
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".xhtml", "application/xhtml+xml"},
    {".xls", "application/vnd.ms-excel"},
    {".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
    {".xml", "application/xml"},
    {".xul", "application/vnd.mozilla.xul+xml"},
    {".zip", "application/zip"},
    {".3gp", "video/3gpp"},
    {".3g2", "video/3gpp2"},
    {".7z", "application/x-7z-compressed"}
};

class Util{
public:
    // URL编码，避免URL中资源路径与查询字符串中的特殊字符与HTTP请求中特殊字符产生歧义
    // 编码格式：将特殊字符的ascii值，转换为两个16进制字符，前缀%   C++ -> C%2B%2B
    //   不编码的特殊字符： RFC3986文档规定 . - _ ~ 字母，数字属于绝对不编码字符
    // RFC3986文档规定，编码格式 %HH
    // W3C标准中规定，查询字符串中的空格，需要编码为+， 解码则是+转空格
    static std::string urlEncode(const std::string url, bool convert_space_to_plus)
    {
        std::string res;
        for (auto &c : url)
        {
            // 如果是大小写字母或数字也不编码
            if (c == '.' || c == '-' || c == '_' || c == '~' || isalnum(c))
            {
                res += c;
                continue;
            }
            if (c == ' ' && convert_space_to_plus)
            {
                res += '+';
                continue;
            }
            // 剩下的字符都是需要编码成%HH格式
            char tmp[4] = {0};
            snprintf(tmp, 4, "%%%02X", c);
            res += tmp;
        }
        return res;
    }
    static char HEXTOI(char c)
    {
        if (c >= '0' && c <= '9')
        {
            return c - '0';
        }
        else if (c >= 'a' && c <= 'z')
        {
            return c - 'a' + 10;
        }
        else if (c > 'A' && c <= 'Z')
        {
            return c - 'A' + 10;
        }
        return -1;
    }
    // URL解码
    static std::string urlDecode(const std::string url, bool convert_plus_to_space)
    {
        // 遇到了%，则将紧随其后的2个字符，转换为数字，第一个数字左移4位，然后加上第二个数字  + -> 2b  %2b->2 << 4 + 11
        std::string res;

        for (int i = 0; i < url.size(); i++)
        {
            if (url[i] == '+' && convert_plus_to_space == true)
            {
                res += ' ';
                continue;
            }
            if (url[i] == '%' && (i + 2) < url.size())
            {
                char v1 = HEXTOI(url[i + 1]);
                char v2 = HEXTOI(url[i + 2]);
                char v = v1 * 16 + v2;
                res += v;
                i += 2;
                continue;
            }
            res += url[i];
        }
        return res;
    }

    // 字符串分割函数,将src字符串按照sep字符进行分割，得到的各个字串放到arry中，最终返回字串的数量
    static size_t usplit(const std::string &src, const std::string &sep, std::vector<std::string> *arry)
    {
        size_t offset = 0;
        // 有10个字符，offset是查找的起始位置，范围应该是0~9，offset==10就代表已经越界了
        while (offset < src.size())
        {
            size_t pos = src.find(sep, offset); // 在src字符串偏移量offset处，开始向后查找sep字符/字串，返回查找到的位置
            if (pos == std::string::npos)
            { // 没有找到特定的字符
                // 将剩余的部分当作一个字串，放入arry中
                if (pos == src.size())
                    break;
                arry->push_back(src.substr(offset));
                return arry->size();
            }
            if (pos == offset)
            {
                offset = pos + sep.size();
                continue; // 当前字串是一个空的，没有内容
            }
            arry->push_back(src.substr(offset, pos - offset));
            offset = pos + sep.size();
        }
        return arry->size();
    }

    // 读取文件的所有内容，将读取的内容放到一个Buffer中
    static bool readFile(const std::string &filename, std::string *buf)
    {
        std::ifstream ifs(filename, std::ios::binary);
        if (ifs.is_open() == false)
        {
            printf("OPEN %s FILE FAILED!!", filename.c_str());
            return false;
        }
        size_t fsize = 0;
        ifs.seekg(0, ifs.end); // 跳转读写位置到末尾
        fsize = ifs.tellg();   // 获取当前读写位置相对于起始位置的偏移量，从末尾偏移刚好就是文件大小
        ifs.seekg(0, ifs.beg); // 跳转到起始位置
        buf->resize(fsize);    // 开辟文件大小的空间
        ifs.read(&(*buf)[0], fsize);
        if (ifs.good() == false)
        {
            printf("READ %s FILE FAILED!!", filename.c_str());
            ifs.close();
            return false;
        }
        ifs.close();
        return true;
    }

    // 向文件写入数据
    static bool writeFile(const std::string &filename, const std::string &buf)
    {
        std::ofstream ofs(filename, std::ios::binary | std::ios::trunc);
        if (ofs.is_open() == false)
        {
            return false;
        }
        ofs.write(buf.c_str(), buf.size());
        if (ofs.good() == false)
        {
            ofs.close();
            return false;
        }
        ofs.close();
        return true;
    }

    static std::string getStatusInfo(int status){
        auto it = _statusInfo.find(status);
        if (it != _statusInfo.end())
        {
            return it->second;
        }
        return "Unknown";
    }

    static std::string getExtMime(const std::string& filename){
        size_t pos = filename.find_last_of('.');
        if (pos == std::string::npos)
        {
            return "application/octet-stream";
        }
        // 根据扩展名，获取mime
        std::string ext = filename.substr(pos);
        auto it = _mimeType.find(ext);
        if (it == _mimeType.end())
        {
            return "application/octet-stream";
        }
        return it->second;
    }

    // 判断一个文件是否是目录
    static bool isDir(const std::string& filename){
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISDIR(st.st_mode);
    }

    // 判断一个文件是否是普通文件
    static bool isRegular(const std::string& filename){
        struct stat st;
        int ret = stat(filename.c_str(), &st);
        if (ret < 0)
        {
            return false;
        }
        return S_ISREG(st.st_mode);
    }

    // http请求的资源路径有效性判断
    //  /index.html  --- 前边的/叫做相对根目录  映射的是某个服务器上的子目录
    //  想表达的意思就是，客户端只能请求相对根目录中的资源，其他地方的资源都不予理会
    //  /../login, 这个路径中的..会让路径的查找跑到相对根目录之外，这是不合理的，不安全的
    static bool validPath(const std::string &path)
    {
        // 思想：按照/进行路径分割，根据有多少子目录，计算目录深度，有多少层，深度不能小于0
        std::vector<std::string> subdir;
        usplit(path, "/", &subdir);
        int level = 0;
        for (auto &dir : subdir)
        {
            if (dir == "..")
            {
                level--; // 任意一层走出相对根目录，就认为有问题
                if (level < 0)
                    return false;
                continue;
            }
            level++;
        }
        return true;
    }
};

class SecureHashAlgorithm {
public:
    SecureHashAlgorithm() { Init(); }

    static const int kDigestSizeBytes = 20;
    void Init(){
        A = 0;
        B = 0;
        C = 0;
        D = 0;
        E = 0;
        cursor = 0;
        l = 0;
        H[0] = 0x67452301;
        H[1] = 0xefcdab89;
        H[2] = 0x98badcfe;
        H[3] = 0x10325476;
        H[4] = 0xc3d2e1f0;
    }

    void Update(const void* data, size_t nbytes){
        const uint8_t* d = reinterpret_cast<const uint8_t*>(data);
        while (nbytes--) {
            M[cursor++] = *d++;
            if (cursor >= 64)
            Process();
            l += 8;
        }
    }
    void Final(){
        Pad();
        Process();

        for (int t = 0; t < 5; ++t){
            swapends(&H[t]);
        }
    }

    // 20 bytes of message digest.
    const unsigned char* Digest() const {
        return reinterpret_cast<const unsigned char*>(H);
    }

private:
    void Pad(){
        M[cursor++] = 0x80;
        if (cursor > 64-8) {
            // pad out to next block
            while (cursor < 64)
            M[cursor++] = 0;

            Process();
        }

        while (cursor < 64-4)
            M[cursor++] = 0;

        M[64-4] = (l & 0xff000000) >> 24;
        M[64-3] = (l & 0xff0000) >> 16;
        M[64-2] = (l & 0xff00) >> 8;
        M[64-1] = (l & 0xff);
    }

    void Process(){
        uint32_t t;
        for (t = 0; t < 16; ++t)
            swapends(&W[t]);

        // b.
        for (t = 16; t < 80; ++t)
            W[t] = S(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);

        // c.
        A = H[0];
        B = H[1];
        C = H[2];
        D = H[3];
        E = H[4];

        // d.
        for (t = 0; t < 80; ++t) {
            uint32_t TEMP = S(5, A) + f(t, B, C, D) + E + W[t] + K(t);
            E = D;
            D = C;
            C = S(30, B);
            B = A;
            A = TEMP;
        }

        // e.
        H[0] += A;
        H[1] += B;
        H[2] += C;
        H[3] += D;
        H[4] += E;

        cursor = 0;
    }

    uint32_t A, B, C, D, E;

    uint32_t H[5];

    union {
    uint32_t W[80];
    uint8_t M[64];
    };

    uint32_t cursor;
    uint32_t l;

    static inline uint32_t f(uint32_t t, uint32_t B, uint32_t C, uint32_t D) {
        if (t < 20) {
            return (B & C) | ((~B) & D);
        } else if (t < 40) {
            return B ^ C ^ D;
        } else if (t < 60) {
            return (B & C) | (B & D) | (C & D);
        } else {
            return B ^ C ^ D;
        }
    }

    static inline uint32_t S(uint32_t n, uint32_t X) {
        return (X << n) | (X >> (32-n));
    }

    static inline uint32_t K(uint32_t t) {
        if (t < 20) {
            return 0x5a827999;
        } else if (t < 40) {
            return 0x6ed9eba1;
        } else if (t < 60) {
            return 0x8f1bbcdc;
        } else {
            return 0xca62c1d6;
        }
    }

    static inline void swapends(uint32_t* t) {
        *t = ((*t & 0xff000000) >> 24) |
            ((*t & 0xff0000) >> 8) |
            ((*t & 0xff00) << 8) |
            ((*t & 0xff) << 24);
    }

};


class SHA1{
public:
    static const size_t kSHA1Length = 20;
    static std::string SHA1HashString(const std::string& str){
        char hash[SecureHashAlgorithm::kDigestSizeBytes];
        SHA1HashBytes(reinterpret_cast<const unsigned char*>(str.c_str()),
                    str.length(), reinterpret_cast<unsigned char*>(hash));
        return std::string(hash, SecureHashAlgorithm::kDigestSizeBytes);
    }

    static void SHA1HashBytes(const unsigned char* data, size_t len,unsigned char* hash){
        SecureHashAlgorithm sha;
        sha.Update(data, len);
        sha.Final();

        memcpy(hash, sha.Digest(), SecureHashAlgorithm::kDigestSizeBytes);
    }
};

static const char basis_64[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static const unsigned char pr2six[256] =
    {
        /* ASCII table */
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };

class BASE64{
public:
    static int Base64encode_len(int len){return ((len + 2) / 3 * 4) + 1;}

    static int Base64encode(char * encoded, const char *string,int len){
        int i;
        char *p;

        p = encoded;
        for (i = 0; i < len - 2; i += 3) {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        *p++ = basis_64[((string[i] & 0x3) << 4) |
                        ((int) (string[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((string[i + 1] & 0xF) << 2) |
                        ((int) (string[i + 2] & 0xC0) >> 6)];
        *p++ = basis_64[string[i + 2] & 0x3F];
        }
        if (i < len) {
        *p++ = basis_64[(string[i] >> 2) & 0x3F];
        if (i == (len - 1)) {
            *p++ = basis_64[((string[i] & 0x3) << 4)];
            *p++ = '=';
        }
        else {
            *p++ = basis_64[((string[i] & 0x3) << 4) |
                            ((int) (string[i + 1] & 0xF0) >> 4)];
            *p++ = basis_64[((string[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
        }

        *p++ = '\0';
        return p - encoded;
    }

    static int Base64decode_len(const char * bufcoded){
        int nbytesdecoded;
        const unsigned char *bufin;
        int nprbytes;

        bufin = (const unsigned char *) bufcoded;
        while (pr2six[*(bufin++)] <= 63);

        nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
        nbytesdecoded = ((nprbytes + 3) / 4) * 3;

        return nbytesdecoded + 1;
    }
    static int Base64decode(char * bufplain, const char *bufcoded){
        int nbytesdecoded;
        const unsigned char *bufin;
        unsigned char *bufout;
        int nprbytes;

        bufin = (const unsigned char *) bufcoded;
        while (pr2six[*(bufin++)] <= 63);
        nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
        nbytesdecoded = ((nprbytes + 3) / 4) * 3;

        bufout = (unsigned char *) bufplain;
        bufin = (const unsigned char *) bufcoded;

        while (nprbytes > 4) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
        }

        /* Note: (nprbytes == 1) would be an error, so just ingore that case */
        if (nprbytes > 1) {
        *(bufout++) =
            (unsigned char) (pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        }
        if (nprbytes > 2) {
        *(bufout++) =
            (unsigned char) (pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        }
        if (nprbytes > 3) {
        *(bufout++) =
            (unsigned char) (pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        }

        *(bufout++) = '\0';
        nbytesdecoded -= (4 - nprbytes) & 3;
        return nbytesdecoded;
    }
};