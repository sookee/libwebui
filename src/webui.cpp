

#include <webui.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/select.h>

#include <sookee/network.h>

namespace webui {

using namespace sookee::net;

//static constexpr const size_t min_get = sizeof("GET / HTTP/1.1") - 1;
//static constexpr const size_t min_post = sizeof("POST / HTTP/1.1") - 1;

const str NL = "\r\n"s;

request parse_request(sis& is)
{
	bug_func();
	request r;

	str line;
	if(!sgl(is, line))
		return r.set_error("reading request");

	bug_var(line);

	str query;

	siss iss(line);

	if(!(iss >> r.method >> query >> r.protocol))
		return r.set_error("parsing request");

	bug_var(r.method);
	bug_var(query);
	bug_var(r.protocol);

	// read in HTTP headers

	str key, val;

	while(sgl(is, line))
		if(sgl(sgl(siss(line), key, ':'), val))
			r.headers[lower(trim(key))] = trim(val);

	// reset the siss with the query string

	iss.clear();
	iss.str(urldecode(query));

	if(!sgl(iss, r.path, '?')) // remove the URL part
		return r.set_error("parsing request path");

	bug_var(r.path);

	if(upper(r.method) == "GET")
	{
		str item;
		while(sgl(iss, item, '&'))
			if(sgl(sgl(siss(item), key, '='), val))
				r.params[key] = val;
	}
	else if(r.method == "POST")
	{
		if(r.headers["content-type"] == "application/x-www-form-urlencoded")
		{
			// TODO: implement POST requests
			siz len = 0;
			if(!(siss(r.headers["content-length"]) >> len))
				return r.set_error("parsing POST bad content-length: " + r.headers["content-length"]);

			query.resize(len);
			if(!is.read(&query[0], len))
				return r.set_error("reading POST: ");

			iss.clear();
			iss.str(urldecode(query));

			str item;
			while(sgl(iss, item, '&'))
				if(sgl(sgl(siss(item), key, '='), val))
					r.params[key] = val;
		}
		else if(r.headers["content-type"] == "multipart/form-data")
		{
//			Content-Type: multipart/form-data; boundary=AaB03x
//dfdsf
		}
	}


	return r;
}

request parse_request(const char* buf, ssize_t len)
{
	siss iss(str(buf, len));
	return parse_request(iss);
}

void* WebUIServer::get_in_addr(sockaddr* sa) const
{
	if(sa->sa_family == AF_INET)
		return &(((sockaddr_in*) sa)->sin_addr);
	return &(((sockaddr_in6*) sa)->sin6_addr);
}

str WebUIServer::get_addr(sockaddr_storage& ss) const
{
	char ip[INET6_ADDRSTRLEN];
	return inet_ntop(ss.ss_family, get_in_addr((sockaddr*) &ss), ip, INET6_ADDRSTRLEN);
}

bool WebUIServer::start()
{
	addrinfo hints;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int error;
	addrinfo* ai;
	if((error = getaddrinfo(0, port.c_str(), &hints, &ai)))
	{
		log("ERROR: " << gai_strerror(error));
		return false;
	}

	int listener;

	addrinfo* p;
	int yes = 1;

	for(p = ai; p; p = p->ai_next)
	{
		if((listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) != -1)
		{
			if(!setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
				if(!bind(listener, p->ai_addr, p->ai_addrlen))
					break;
			close(listener);
		}
	}

	freeaddrinfo(ai);

	if(!p)
	{
		log("ERROR: failed to bind: " << std::strerror(errno));
		return false;
	}

	if(listen(listener, 10) == -1)
	{
		log("ERROR: " << std::strerror(errno));
		close(listener);
		return false;
	}

	fd_set master;
	fd_set read_fds;

	FD_SET(listener, &master);

	// keep track of the biggest file descriptor
	int fdmax = listener; // so far, it's this one

	while(!done)
	{
		read_fds = master; // copy it
		if(select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1)
		{
			log("ERROR: " << std::strerror(errno));
			return false;
		}

		// run through the existing connections looking for data to read
		for(int fd = 0; fd <= fdmax; fd++)
		{
			if(!FD_ISSET(fd, &read_fds))
				continue;

			sockaddr_storage store; // client address
			socklen_t addrlen = sizeof(store);

			if(fd == listener)
			{
				// handle new connections
				int newfd = accept(listener, (struct sockaddr *) &store, &addrlen);

				if(newfd == -1)
					log("ERROR: " << std::strerror(errno));
				else
				{
					FD_SET(newfd, &master); // add to master set
					fdmax = std::max(fdmax, newfd);

					log("INFO: connect: [" << newfd << "] " << get_addr(store));
				}
			}
			else // handle data from a client
			{
				log("INFO: incoming data: [" << fd << "]");

				const ssize_t max_len = 4 * 1024;

				ssize_t len;
				ssize_t total_len = 0;
				char buf[1024];
				sss ss;

				bug_var(sizeof(buf));

				while((len = recvfrom(fd, buf, sizeof(buf)
					, 0, (sockaddr*)&store, &addrlen)) == sizeof(buf))
				{
					bug_var(str(buf, 4));
					ss << str(buf, len);
					total_len += len;
					if(total_len > max_len)
					{
						log("ERROR: too much data [" << total_len << "b] from: " << get_addr(store));
						close (fd);
						FD_CLR(fd, &master); // remove from master set
						continue;
					}
				}

				ss << str(buf, len);

				total_len += len;

				bug_var(len);
				bug_var(total_len);

				if(len < 0)
				{
					log("ERROR: " << std::strerror(errno));
					close (fd);
					FD_CLR(fd, &master); // remove from master set
					continue;
				}

				request rq = parse_request(ss);

				if(rq.has_error())
				{
					log("ERROR: " << rq.error);
					close (fd);
					FD_CLR(fd, &master); // remove from master set
					continue;
				}

				log("\n== RESPONSE ==\n");

				str msg;
				str mimetype = "text/html";

				auto page = pages.find(rq.path);

				if(page != pages.end())
					msg = pages[rq.path]->process(rq);
				else
				{
					bug_var(rq.path);
					sifs ifs("ui/" + rq.path);
					if(!ifs)
						log("ERROR: opening file: " << "ui/" + rq.path);
					else
					{
						str ext;
						auto pos = rq.path.rfind('.');
						if(pos != str::npos)
							ext = rq.path.substr(pos + 1);

						bug_var(ext);

						static const str_map mimetypes
						{
							{"html", "text/html"}
							, {"css", "text/css"}
							, {"png", "image/png"}
							, {"jpg", "image/jpeg"}
							, {"xml", "application/xml"}
							, {"js", "application/javascript"}
						};

						auto found = mimetypes.find(ext);

						if(found != mimetypes.end())
							mimetype = found->second;

						msg.assign(sisb_iter(ifs), sisb_iter());
					}
				}

				if(msg.size() < 256)
					bug_var(msg);

				std::time_t t = std::time(0);
				tm* timeinfo = localtime(&t);
				char buffer[80];
				std::size_t size = strftime(buffer, 80, "%a, %d %b %Y %T %Z", timeinfo);
				str date(buffer, size);

				soss oss;
				oss << "HTTP/1.1 200 OK" << NL;
				oss << "Server: webui/0.0.1" << NL;
				oss << "Date: " << date << NL;
				oss << "Content-Type: " << mimetype << NL;
				oss << "Content-Length: " << msg.size() << NL;
				oss << "Connection: close" << NL;
				oss << "Cache-Control: no-cache" << NL;
				oss << NL;
				oss << msg << std::flush;

				str data = oss.str();

				if((len = send(fd, data.data(), data.size(), 0)) != (int)data.size())
					log("ERROR: " << std::strerror(errno));

				bug_var(len);
				//log("DBUG: data: " << data);
				bug_var(data.size());

				if(close(fd) == -1)
					log("ERROR: close: " << std::strerror(errno));

				FD_CLR(fd, &master); // remove from master set
			}
		}
	}
}

} // ::webui
