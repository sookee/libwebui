#include <webui.h>

using namespace webui;

#ifdef DEBUG
#define throw_runtime_error(msg) \
	do {\
		soss oss; \
		oss << (msg) << " in file:" << __FILE__; \
		oss << " at line:" << __LINE__; \
		throw std::runtime_error(oss.str()); \
	}while(0)
#else
#define throw_runtime_error(msg) \
	throw std::runtime_error(msg)
#endif

class Widget
{
	using WidgetUPtr = std::unique_ptr<Widget>;
	using WidgetUPtrVec = std::vector<WidgetUPtr>;

	WidgetUPtrVec ws;

protected:
	str id;
	str_map atts;

public:

	void set_att(const str& att, const str& val) { atts[att] = val; }

	Widget* getById(const str& id)
	{
		if(this->id == id)
			return this;

		for(auto&& w: ws)
			if(w->getById(id))
				return w.get();

		return nullptr;
	}

	Widget(const str& id): id(id) {}
	Widget(Widget&& w)
	: ws(std::move(w.ws)), id(std::move(w.id)), atts(std::move(w.atts)) {}
	Widget(const Widget& w) = delete;

	bool operator==(const Widget& w) const { return id == w.id; }
	bool operator!=(const Widget& w) const { return id != w.id; }
	bool operator<(const Widget& w) const { return id < w.id; }

	virtual ~Widget() {}
	virtual str realize()
	{
		str s;
		for(auto&& w: ws)
			s += w->realize();
		return s;
	}
	void add(Widget* w)
	{
		if(!w)
			return;
		if(!w->id.empty())
			if(std::find_if(ws.begin(), ws.end(), [w](const WidgetUPtr& wp){return !wp->id.empty() || *wp.get() == *w;}) != ws.end())
				throw_runtime_error("duplicate id: " + w->id);
		ws.emplace_back(w);
	}
};

using WidgetUPtr = std::unique_ptr<Widget>;
using WidgetUPtrMap = std::map<str, WidgetUPtr>;
using WidgetUPtrVec = std::vector<WidgetUPtr>;

class DivWidget
: public Widget
{
public:

	str realize() override
	{
		str tag = "<div";
		if(!id.empty())
			tag += " id='" + id + "'";
		for(auto&& att: atts)
			tag += " " + att.first + "='" + att.second + "'";
		tag += ">\n" + Widget::realize() + "\n</div>";
		return tag;
	}
};

class WidgitPage
: public WebUIPage
{
//	str_set ids; // ensure uniqueness
	Widget widget;

public:

	void add(Widget* widget) { this->widget.add(widget); }

	str process(const request& rq) override
	{
		return {};
	}
};

int main()
{
//	int i = 'DIVA';
	WebUIServer server;

//	server.add("", new WebUIStencilPage);

	server.start();
}
