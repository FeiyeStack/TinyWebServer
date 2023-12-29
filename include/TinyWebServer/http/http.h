#pragma once
#include <map>
#include <string>
#include <ostream>
#include <memory>
#include <optional>
#include "../util.h"
namespace WebSrv::http
{
	/* Status Codes */
#define HTTP_STATUS_MAP(XX)                                                   \
	XX(100, CONTINUE, Continue)                                               \
	XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                         \
	XX(102, PROCESSING, Processing)                                           \
	XX(200, OK, OK)                                                           \
	XX(201, CREATED, Created)                                                 \
	XX(202, ACCEPTED, Accepted)                                               \
	XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)   \
	XX(204, NO_CONTENT, No Content)                                           \
	XX(205, RESET_CONTENT, Reset Content)                                     \
	XX(206, PARTIAL_CONTENT, Partial Content)                                 \
	XX(207, MULTI_STATUS, Multi - Status)                                     \
	XX(208, ALREADY_REPORTED, Already Reported)                               \
	XX(226, IM_USED, IM Used)                                                 \
	XX(300, MULTIPLE_CHOICES, Multiple Choices)                               \
	XX(301, MOVED_PERMANENTLY, Moved Permanently)                             \
	XX(302, FOUND, Found)                                                     \
	XX(303, SEE_OTHER, See Other)                                             \
	XX(304, NOT_MODIFIED, Not Modified)                                       \
	XX(305, USE_PROXY, Use Proxy)                                             \
	XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                           \
	XX(308, PERMANENT_REDIRECT, Permanent Redirect)                           \
	XX(400, BAD_REQUEST, Bad Request)                                         \
	XX(401, UNAUTHORIZED, Unauthorized)                                       \
	XX(402, PAYMENT_REQUIRED, Payment Required)                               \
	XX(403, FORBIDDEN, Forbidden)                                             \
	XX(404, NOT_FOUND, Not Found)                                             \
	XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                           \
	XX(406, NOT_ACCEPTABLE, Not Acceptable)                                   \
	XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)     \
	XX(408, REQUEST_TIMEOUT, Request Timeout)                                 \
	XX(409, CONFLICT, Conflict)                                               \
	XX(410, GONE, Gone)                                                       \
	XX(411, LENGTH_REQUIRED, Length Required)                                 \
	XX(412, PRECONDITION_FAILED, Precondition Failed)                         \
	XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                             \
	XX(414, URI_TOO_LONG, URI Too Long)                                       \
	XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                   \
	XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                     \
	XX(417, EXPECTATION_FAILED, Expectation Failed)                           \
	XX(421, MISDIRECTED_REQUEST, Misdirected Request)                         \
	XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                       \
	XX(423, LOCKED, Locked)                                                   \
	XX(424, FAILED_DEPENDENCY, Failed Dependency)                             \
	XX(426, UPGRADE_REQUIRED, Upgrade Required)                               \
	XX(428, PRECONDITION_REQUIRED, Precondition Required)                     \
	XX(429, TOO_MANY_REQUESTS, Too Many Requests)                             \
	XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large) \
	XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)     \
	XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                     \
	XX(501, NOT_IMPLEMENTED, Not Implemented)                                 \
	XX(502, BAD_GATEWAY, Bad Gateway)                                         \
	XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                         \
	XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                 \
	XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)           \
	XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                 \
	XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                       \
	XX(508, LOOP_DETECTED, Loop Detected)                                     \
	XX(510, NOT_EXTENDED, Not Extended)                                       \
	XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

/* Request Methods */
#define HTTP_METHOD_MAP(XX)          \
	XX(0, DELETE, DELETE)            \
	XX(1, GET, GET)                  \
	XX(2, HEAD, HEAD)                \
	XX(3, POST, POST)                \
	XX(4, PUT, PUT)                  \
	/* pathological */               \
	XX(5, CONNECT, CONNECT)          \
	XX(6, OPTIONS, OPTIONS)          \
	XX(7, TRACE, TRACE)              \
	/* WebDAV */                     \
	XX(8, COPY, COPY)                \
	XX(9, LOCK, LOCK)                \
	XX(10, MKCOL, MKCOL)             \
	XX(11, MOVE, MOVE)               \
	XX(12, PROPFIND, PROPFIND)       \
	XX(13, PROPPATCH, PROPPATCH)     \
	XX(14, SEARCH, SEARCH)           \
	XX(15, UNLOCK, UNLOCK)           \
	XX(16, BIND, BIND)               \
	XX(17, REBIND, REBIND)           \
	XX(18, UNBIND, UNBIND)           \
	XX(19, ACL, ACL)                 \
	/* subversion */                 \
	XX(20, REPORT, REPORT)           \
	XX(21, MKACTIVITY, MKACTIVITY)   \
	XX(22, CHECKOUT, CHECKOUT)       \
	XX(23, MERGE, MERGE)             \
	/* upnp */                       \
	XX(24, MSEARCH, M - SEARCH)      \
	XX(25, NOTIFY, NOTIFY)           \
	XX(26, SUBSCRIBE, SUBSCRIBE)     \
	XX(27, UNSUBSCRIBE, UNSUBSCRIBE) \
	/* RFC-5789 */                   \
	XX(28, PATCH, PATCH)             \
	XX(29, PURGE, PURGE)             \
	/* CalDAV */                     \
	XX(30, MKCALENDAR, MKCALENDAR)   \
	/* RFC-2068, section 19.6.1.2 */ \
	XX(31, LINK, LINK)               \
	XX(32, UNLINK, UNLINK)           \
	/* icecast */                    \
	XX(33, SOURCE, SOURCE)

	/**
	 * @brief Http状态
	 *
	 */
	enum HttpStatus
	{
#define XX(num, name, string) HTTP_STATUS_##name = num,
		HTTP_STATUS_MAP(XX)
#undef XX
	};
	/**
	 * @brief Http 方法
	 *
	 */
	enum HttpMethod
	{
#define XX(num, name, string) HTTP_##name = num,
		HTTP_METHOD_MAP(XX)
#undef XX
			HTTP_INVALID_METHOD
	};

	/**
	 * @brief 将std::string 字符串类型转换成HttpMethod
	 *
	 * @param method
	 * @return HttpMethod
	 */
	HttpMethod HttpMethodFromString(const std::string &method);
	/**
	 * @brief 将const char* 风格字符串类型转换成HttpMethod
	 *
	 * @param method
	 * @return HttpMethod
	 */
	HttpMethod HttpMethodFromChars(const char *method);

	/**
	 * @brief 将HttpMethod 类型转成字符串
	 *
	 * @param method
	 * @return const char*
	 */
	const char *HttpMethodToString(const HttpMethod &method);

	/**
	 * @brief 将HttpState转成字符串
	 *
	 * @param status
	 * @return const char*
	 */
	const char *HttpStateToString(const HttpStatus &status);

	/**
	 * @brief 不区分大小写比较仿函数
	 *
	 */
	struct CaseInsensitiveLess
	{
		bool operator()(const std::string &lhs, const std::string &rhs) const;
	};

	/**
	 * @brief 获取以字符串为key,map的key值,并转成对应类型
	 *
	 * @tparam MapType
	 * @tparam T
	 * @param map std::map
	 * @param key 键
	 * @param def 默认值
	 * @return T 存在且转换成功返回对应值，否则为默认值
	 */
	template <typename MapType, typename T>
	T getAs(const MapType &map, const std::string &key, const T &def = T())
	{
		auto it = map.find(key);
		if (it == map.end())
		{
			return def;
		}
		// 尝试转换
		try
		{
			return lexicalCast<T>(it->second);
		}
		catch (...)
		{
			return def;
		}
	}

	/**
	 * @brief 获取m以字符串为key,map的key值,并转成对应类型
	 *
	 * @tparam MapType
	 * @tparam T
	 * @param map std::map
	 * @param key 键
	 * @param result 返回值
	 * @param def 默认值
	 * @return true 存在且转换成功返回对应值
	 * @return false 返回默认值
	 */
	template <typename MapType, typename T>
	bool checkGetAs(const MapType &map, const std::string &key, T &result, const T &def = T())
	{
		auto it = map.find(key);
		if (it == map.end())
		{
			result = def;
			return false;
		}
		// 尝试转换
		try
		{
			result = lexicalCast<T>(it->second);
			return true;
		}
		catch (...)
		{
			result = def;
			return false;
		}
	}

	class HttpResponse;

	/**
	 * @brief http请求报文结构体
	 *
	 */
	class HttpRequest
	{
	public:
		// 比较时不区分大小写
		using Map = std::map<std::string, std::string, CaseInsensitiveLess>;
		using ptr = std::shared_ptr<HttpRequest>;

		HttpRequest(uint8_t version = 0x11);

		// 获取http 方法
		HttpMethod getMethod() const { return _method; }
		// 获取http 版本
		uint8_t getVersion() const { return _version; }
		// 获取请求路径
		const std::string &getPath() const { return _path; }
		// 获取请求fragment
		const std::string &getFragment() const { return _fragment; }
		// 获取请求消息体
		const std::string &getBody() const { return _body; }
		// 获取请求头部Map
		const Map &getHeaders() const { return _headers; }
		// 获取请求参数Map
		const Map &getParams() const { return _params; }
		// 获取请求cookie Map
		const Map &getCookies() const { return _cookies; }
		// 设置http 方法
		void setHttpMethod(HttpMethod method) { _method = method; }
		// 设置http 版本
		void setVersion(uint8_t version) { _version = version; }
		// 设置请求路径
		void setPath(const std::string &path) { _path = path; }
		// 设置请求fragment
		void setFragment(const std::string &fragment) { _fragment = fragment; }
		// 设置请求头部Map
		void setHeaders(const Map &headers) { _headers = headers; }
		// 设置请求参数Map
		void setParams(const Map &params) { _params = params; }
		// 设置请求cookie Map
		void setCookies(const Map &cookies) { _cookies = cookies; }
		// 设置请求消息体
		void setBody(const std::string &body) { _body = body; }
		/**
		 * @brief 获取HTTP请求的头部参数(不包含cookie)
		 *
		 * @param key 键
		 * @param def 默认值
		 * @return std::string 存在返回对应值，否则为默认值
		 */
		std::string getHeader(const std::string &key, const std::string &def = "") const;

		/**
		 * @brief 获取HTTP请求的请求参数
		 *
		 * @param key 键
		 * @param def 默认值
		 * @return std::string 存在返回对应值，否则为默认值
		 */
		std::string getParam(const std::string &key, const std::string &def = "") const;

		/**
		 * @brief 获取HTTP请求的cookie参数
		 *
		 * @param key 键
		 * @param def 默认值
		 * @return std::string 存在返回对应值，否则为默认值
		 */
		std::string getCookie(const std::string &key, const std::string &def = "") const;

		/**
		 * @brief 判断Http请求的头部参数(不包含cookie)
		 *
		 * @param key 键
		 * @param result 返回值
		 * @return true 存在result非空返回为对应值
		 * @return false
		 */
		bool hasHeader(const std::string &key, std::string *result) const;

		/**
		 * @brief 判断Http请求的请求参数
		 *
		 * @param key 键
		 * @param result 返回值
		 * @return true 存在result非空返回为对应值
		 * @return false
		 */
		bool hasParam(const std::string &key, std::string *result) const;
		/**
		 * @brief 判断Http请求的cookie参数
		 *
		 * @param key 键
		 * @param result 返回值
		 * @return true 存在result非空返回为对应值
		 * @return false
		 */
		bool hasCookie(const std::string &key, std::string *result) const;
		/**
		 * @brief 设置HTTP请求的头部参数(不包含cookie)
		 *
		 * @param key 键
		 * @param value 值
		 */
		void setHeader(const std::string &key, const std::string &value);

		/**
		 * @brief 设置HTTP请求的请求参数
		 *
		 * @param key 键
		 * @param value 值
		 */
		void setParam(const std::string &key, const std::string &value);
		/**
		 * @brief 设置HTTP请求的cookie参数
		 *
		 * @param key 键
		 * @param value 值
		 */
		void setCookie(const std::string &key, const std::string &value);
		/**
		 * @brief 删除对应键值的HTTP请求的头部参数(不包含cookie)
		 *
		 * @param key 键
		 */
		void delHeader(const std::string &key);

		/**
		 * @brief 删除对应键值的HTTP请求的param参数
		 *
		 * @param key 键
		 */
		void delParam(const std::string &key);

		/**
		 * @brief 删除对应键值的HTTP请求的cookie参数
		 *
		 * @param key 键
		 */
		void delCookie(const std::string &key);

		/**
		 * @brief 获取http请求的头部参数 （通常不用只有少部分需要类型转换）
		 *
		 * @tparam T 参数类型
		 * @param key 键
		 * @param def 默认值
		 * @return T 存在且转换成功返回对应值，否则为默认值
		 */
		template <typename T>
		T getHeaderAs(const std::string &key, const T &def = T())
		{
			return getAs(_headers, key, def);
		}

		/**
		 * @brief 检查并获取Http请求的头部参数（通常不用只有少部分需要类型转换）
		 *
		 * @tparam T 参数类型
		 * @param key 键
		 * @param result 返回值
		 * @param def 默认值
		 * @return true 存在且转换成功result返回为对应值
		 * @return false result返回默认值
		 */
		template <typename T>
		bool checkGetHeaderAs(const std::string &key, T &result, const T &def = T())
		{
			return checkGetAs(_headers, key, result, def);
		}

		/**
		 * @brief 获取http请求的请求参数
		 *
		 * @tparam T 参数类型
		 * @param key 键
		 * @param def 默认值
		 * @return T 存在且转换成功返回对应值，否则为默认值
		 */
		template <typename T>
		T getParamAs(const std::string &key, const T &def = T())
		{
			return getAs(_params, key, def);
		}

		/**
		 * @brief 检查并获取Http请求的请求参数
		 *
		 * @tparam T 参数类型
		 * @param key 键
		 * @param result 返回值
		 * @param def 默认值
		 * @return true 存在且转换成功result返回为对应值
		 * @return false result返回默认值
		 */
		template <typename T>
		bool checkGetParamAs(const std::string &key, T &result, const T &def = T())
		{
			return checkGetAs(_params, key, result, def);
		}

		/**
		 * @brief 获取http请求的cookie参数
		 *
		 * @tparam T 参数类型
		 * @param key 键
		 * @param def 默认值
		 * @return T 存在且转换成功返回对应值，否则为默认值
		 */
		template <typename T>
		T getCookieAs(const std::string &key, const T &def = T())
		{
			return getAs(_cookies, key, def);
		}

		/**
		 * @brief 检查并获取Http请求的Cookie参数
		 *
		 * @tparam T 参数类型
		 * @param key 键
		 * @param result 返回值
		 * @param def 默认值
		 * @return true 存在且转换成功result返回为对应值
		 * @return false result返回默认值
		 */
		template <typename T>
		bool checkGetCookieAs(const std::string &key, T &result, const T &def = T())
		{
			return checkGetAs(_cookies, key, result, def);
		}
		/**
		 * @brief 序列化输出到流中
		 *
		 * @param os
		 * @return std::ostream&
		 */
		std::ostream &dump(std::ostream &os) const;
		/**
		 * @brief 转成字符串类型
		 *
		 * @return std::string
		 */
		std::string toString() const;

	private:
		void addLine(std::ostream &os) const;
		void addHeaders(std::ostream &os) const;
		void addBody(std::ostream &os) const;

	private:
		// http 方法
		HttpMethod _method;
		// http 版本
		uint8_t _version;
		// 请求路径
		std::string _path;
		// 请求fragment(锚点用于客户端本地服务不会上传服务器)
		std::string _fragment;
		// 请求消息体
		std::string _body;
		// 请求头部Map
		Map _headers;
		// 请求参数Map
		Map _params;
		// 请求cookie Map
		Map _cookies;
		// 初始化请求参数，成功后添加标志位，不在初始化
		uint8_t _parserParamStatus;
	};

	/**
	 * @brief http响应报文结构体
	 *
	 */
	class HttpResponse
	{
	public:
		using ptr = std::shared_ptr<HttpResponse>;
		using Map = std::map<std::string, std::string, CaseInsensitiveLess>;
		/**
		 * @brief cookie关键参数
		 *
		 */
		struct CookieOptions
		{
			CookieOptions()
				: expiration(0),
				  secure(false), httpOnly(false),
				  partitioned(false)
			{
			}
			enum SameSite
			{
				Strict,
				Lax,
				None,
			};
			/// @brief 指定 cookie 可以送达的主机。
			std::string domain;
			/// @brief 以 HTTP 日期时间戳形式指定的 cookie 的最长有效时间,0 时为会话期
			int64_t expiration;
			/// @brief 阻止 JavaScript 通过 Document.cookie 属性访问 cookie
			bool httpOnly;
			/// @brief 在 cookie 过期之前需要经过的秒数。
			std::optional<int64_t> maxAge;
			/// @brief 表示应使用分区存储来存储 cookie。
			bool partitioned;
			/// @brief 表示要发送该 Cookie 标头时，请求的 URL 中所必须存在的路径。
			std::string path;
			/// @brief 控制 cookie 是否随跨站请求一起发送
			std::optional<SameSite> sameSite;
			/// @brief 表示仅当请求通过 https: 协议（localhost 不受此限制）发送时才会将该 cookie 发送到服务器
			bool secure;
		};

		using CookieMap=std::map<std::string,std::pair<std::string,HttpResponse::CookieOptions>,CaseInsensitiveLess>;

		HttpResponse(uint8_t version=0x11);

		/// @brief 获取响应状态
		HttpStatus getStatus() const { return _status; }
		/// @brief 获取http 版本
		uint8_t getVersion() const { return _version; }
		/// @brief 获取响应消息体
		const std::string &getBody() const { return _body; }
		/// @brief 获取响应原因
		const std::string &getReason() const { return _reason; }
		/// @brief 获取响应头部map
		const Map &getHeaders() const { return _headers; }
		/// @brief 响应状态
		void setStatus(HttpStatus status) { _status = status; }
		/// @brief http 版本
		void setVersion(uint8_t version) { _version = version; }
		/// @brief 响应消息体
		void setBody(const std::string &body) { _body = body; }
		/// @brief 响应原因
		void setReason(const std::string &reason) { _reason = reason; }
		/// @brief 响应头部map
		void setHeaders(const Map &headers) { _headers = headers; }

		/**
		 * @brief 获取HTTP请求的头部参数(不包含cookie)
		 *
		 * @param key 键
		 * @param def 默认值
		 * @return std::string 存在返回对应值，否则为默认值
		 */
		std::string getHeader(const std::string &key, const std::string &def = "") const;
		/**
		 * @brief 判断Http请求的头部参数(不包含cookie)
		 *
		 * @param key 键
		 * @param result 返回值
		 * @return true 存在result非空返回为对应值
		 * @return false
		 */
		bool hasHeader(const std::string &key, std::string *result) const;
		/**
		 * @brief 设置HTTP请求的头部参数(不包含cookie)
		 *
		 * @param key 键
		 * @param value 值
		 */
		void setHeader(const std::string &key, const std::string &value);
		/**
		 * @brief 删除对应键值的HTTP请求的头部参数(不包含cookie)
		 *
		 * @param key 键
		 */
		void delHeader(const std::string &key);

		/**
		 * @brief 设置cookie 参数
		 *
		 * @param key
		 * @param value
		 * @param options
		 */
		void setCookie(const std::string &key, const std::string &value, const CookieOptions &options = CookieOptions());

		/**
		 * @brief 获取cookie 参数，不存在时第一个参数为""
		 * 
		 * @param key 
		 * @return std::pair<std::string,HttpResponse::CookieOptions> 
		 */
		std::pair<std::string,HttpResponse::CookieOptions> getCookie(const std::string &key);
		/**
		 * @brief 获取cookieMap
		 * 
		 * @return const CookieMap& 
		 */
		const CookieMap& getCookies() const {return _cookies;}
		/**
		 * @brief 获取http响应的头部参数 （通常不用只有少部分需要类型转换）
		 *
		 * @tparam T 参数类型
		 * @param key 键
		 * @param def 默认值
		 * @return T 存在且转换成功返回对应值，否则为默认值
		 */
		template <typename T>
		T getHeaderAs(const std::string &key, const T &def = T())
		{
			return getAs(_headers, key, def);
		}

		/**
		 * @brief 检查并获取Http响应的头部参数（通常不用只有少部分需要类型转换）
		 *
		 * @tparam T 参数类型
		 * @param key 键
		 * @param result 返回值
		 * @param def 默认值
		 * @return true 存在且转换成功result返回为对应值
		 * @return false result返回默认值
		 */
		template <typename T>
		bool checkGetHeaderAs(const std::string &key, T &result, const T &def = T())
		{
			return checkGetAs(_headers, key, result, def);
		}
		/**
		 * @brief 序列化输出到流中
		 *
		 * @param os
		 * @return std::ostream&
		 */
		std::ostream &dump(std::ostream &os) const;
		/**
		 * @brief 转成字符串类型
		 *
		 * @return std::string
		 */
		std::string toString() const;
	private:
		void addCookie(std::ostream& os) const;
	private:
		/// @brief 响应状态
		HttpStatus _status;
		/// @brief http 版本
		uint8_t _version;
		/// @brief 响应消息体
		std::string _body;
		/// @brief 响应原因（自定义响应）
		std::string _reason;
		/// @brief 响应头部map
		Map _headers;
		/// @brief 响应cookie
		CookieMap _cookies;
	};

	std::ostream &operator<<(std::ostream &os, const HttpRequest &other);

	std::ostream &operator<<(std::ostream &os, const HttpResponse &other);

} // namespace WebSrv::http
