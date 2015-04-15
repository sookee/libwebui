#ifndef INCLUDE_WEBUI_WEBUI_H_
#define INCLUDE_WEBUI_WEBUI_H_
/*
 * webui.h
 *
 *  Created on: Feb 8, 2015
 *      Author: SooKee
 */

#include <sookee/types/basic.h>
#include <sookee/types/stream.h>
#include <sookee/types/typedefs_vec.h>

#include <sookee/cal.h>
#include <sookee/cfg.h>
#include <sookee/bug.h>
#include <sookee/log.h>
#include <sookee/mem.h>
#include <sookee/stl.h>
#include <sookee/str.h>
#include <sookee/ios.h>

#include <sookee/stencil.h>

#include <sys/socket.h>

namespace webui {

//using namespace sookee::arg;
using namespace sookee::bug;
using namespace sookee::cal;
using namespace sookee::mem;
using namespace sookee::log;
using namespace sookee::stl;
using namespace sookee::ios;
using namespace sookee::props;
using namespace sookee::types;
using namespace sookee::utils;

using namespace std::string_literals;

struct request
{
	str error;
	str protocol;
	str method;
	str path;
	str_map headers;
	str_map params;

	request& set_error(const str& msg) { error = msg; return *this; }
	bool has_error() const { return !error.empty(); }
};

request parse_request(sis& is);
request parse_request(const char* buf, ssize_t len);

class WebUIPage
{
public:
	virtual ~WebUIPage() {}

	virtual bool initialize() { return true; }

	/**
	 * Return HTML page from given request
	 * @param rq
	 * @return
	 */
	virtual str process(const request& rq) = 0;
};

class WebUIStencilPage
: public WebUIPage
{
	str filename;
	map_stencil st;
	map_stencil::dict d;

public:
	WebUIStencilPage(const str& filename): filename(filename) {}

	void set_dict(const map_stencil::dict& d) { this->d = d; }

	// WebUIPage Api

	bool initialize() override
	{
		if(!st.compile_file(filename))
			return false;
		return true;
	}

	/**
	 * Create map_stencil::dict from form paramerters.
	 * @param rq
	 * @return
	 */
	str process(const request& rq) override
	{
		set_dict(rq.params);
		return st.create(d);
	}
};

using WebUIPageUPtr = std::unique_ptr<WebUIPage>;
using WebUIPageUPtrMap = std::map<str, WebUIPageUPtr>; // path -> WebUIPageUPtr

class WebUIServer
{
	bool done = false;
	str port = "8008";

	WebUIPageUPtrMap pages;

	// get sockaddr, IPv4 or IPv6:
	void* get_in_addr(sockaddr* sa) const;

	str get_addr(sockaddr_storage& ss) const;

public:
	WebUIServer() {}
	WebUIServer(WebUIServer&& wuis): pages(std::move(wuis.pages)) {}

	/** Takes ownership of the WebUIPage
	 *
	 * @param path
	 * @param page
	 */
	void add(const str& path, WebUIPage* page) { pages[path].reset(page); }

	bool start();
};

} // ::webui

#endif // INCLUDE_WEBUI_WEBUI_H_
