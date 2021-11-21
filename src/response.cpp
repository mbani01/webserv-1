#include "../includes/response.hpp"
#include "../includes/location.hpp"


Response::Response(void): _response(""), _loc()
, _root(""), _uri(""), _error_pages(""){}

Response::Response(std::string const& root, Location const& loc
, std::string const& uri, std::string const& error_pages): _loc(loc), _root(root)
, _uri(uri), _error_pages(error_pages)
{
	_type.insert(std::make_pair("json", "application"));
	_type.insert(std::make_pair("html", "text"));
}

Response::Response(Response const& x) { *this = x;	}
Response::~Response(void) { _file.close(); }
Response& Response::operator=(Response const& x)
{
	_response = x._response;
	return *this;
}

void Response::Get_request(void)
{
	std::vector<std::string> allowed = _loc.getAllowedMethods();
	// lets first check for alowed methods in this location
	if (find(allowed.begin(), allowed.end(), "GET") == allowed.end())
	{
		_fill_response(_error_pages + '/' + "403.html", 403, "Forbiden");
		return;
	}
	if (_is_dir())
	{
		if (_loc.getAutoIndex() == "on")
			return;
		_root += '/' + _uri;
		_uri.clear();
	}
	/*
	* if the location is the @default location then we should check the type of the uri, is it a directory or a file
	* and if it's a direcotry then we should check if it containes one of the names in the index directive, then we should return
	* it's content, otherwise we should check if the autoindex it set to on means that we should list all the files in that directory
	* if non of the prev condition is true then we should return an error
	*/
	if (_loc.getPath() == "/")
	{
		if (!_default_location()) // if we had an error while trying to open the file location we should return back
			return;
	}
	if (_file_path.empty()) // if we get an empty path than we didn't found the file we are searching for in the index directive or we are not in the default location
	{
		if (!_file_is_good(true))
			return;
	}
	_fill_response(_file_path, 200, "OK"); // if all good than we should fill the response with 200 status code
}

bool Response::_default_location(void)
{
	std::vector<std::string> const	index = _loc.getIndex();
	bool							found(false);

	// first lets search if the location contains the uri as one of its index names
	if (_uri.empty())
	{
		for (size_t i = 0; i < index.size(); ++i)
		{
			_file_path = _root + '/' + index[i];
			if (!(found = _file_is_good(false)) && errno != 2) // if the file exists but we don't have the permissions to read from it
			{
				_fill_response(_error_pages + '/' + "403.html", 403, "Forbiden");
				return false;
			}
			else if (found)
				break;
		}
		if (!found)
		{
			_fill_response(_error_pages + '/' + "404.html", 404, "Not found");
			return false;
		}
	}
	return true;
}

bool Response::_is_dir(void) const
{
	struct stat s;

	if (!lstat(_uri.c_str(), &s))
	{
		if (S_ISDIR(s.st_mode))
			return true;
		else
			return false;
	}
	return false;
}

void Response::_set_headers(size_t status_code, std::string const& message, size_t content_length, std::string const& path)
{
	time_t rawtime;

	time (&rawtime);
	_response += "HTTP/1.1 " +  std::to_string(status_code) + " " + message + '\n';
	_response += "Date: " + std::string(ctime(&rawtime));
	_response += "Server: webserver\n";
	_response += "Content-Length: " + std::to_string(content_length) + '\n';
	_response += "Content-Type: " + _type[path.substr(path.find_last_of(".") + 1)] + '/'
	+ path.substr(path.find_last_of(".") + 1) + '\n';
	_response += "Connection: close\n";
	_response += '\n';
}

void Response::_fill_response(std::string const& path, size_t status_code, std::string const& message)
{
	std::string 	line;
	std::string		tmp_resp;
	size_t			content_counter(0);
	
	_file.open(path);
	while (!_file.eof())
	{
		std::getline(_file, line);
		content_counter += line.size();
		if (!_file.eof())
		{
			line += '\n';
			content_counter++;
		}
		tmp_resp += line;
	}
	// set all the needed response header
	_set_headers(status_code, message, content_counter, path);
	_response += tmp_resp;
}

bool Response::_file_is_good(bool fill_resp)
{
	if (_file_path.empty())
		_file_path = _root + '/' + _uri;
	if (open(_file_path.c_str(), O_RDONLY) < 0)
	{
		if (errno == 2 && fill_resp)
			_fill_response(_error_pages + '/' + "404.html", 404, "Not Found");
		else if (fill_resp)
			_fill_response(_error_pages + '/' + "403.html", 403, "Forbidden");
		return false;
	}
	return true;
}
std::string const& 	Response::get_response(void) const	{ return _response; }