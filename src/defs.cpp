#include "defs.hpp"

bool is_number(const std::string& t)
{
	return regex_match( t, std::regex{ R"z((\d+(\.\d*)?|\.\d+)([Ee][\+\-]?\d+)?)z" } );
}


bool is_operator(const std::string& t )
{
	if (t.size() == 1)
	{
		auto it = str2op.find(t);
		if (it != str2op.end())
			return true;
	}

	return false;
}

bool is_func( const std::string& t)
{
	auto it = str2fn.find(t);
	if (it != str2fn.end())
		return true;

	return false;
}

bool is_variable(const std::string& t)
{
	if (!is_func(t))
		return regex_match( t, std::regex{"[a-zA-Z]+"} );

	return false;
}
