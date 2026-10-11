#include "defs.hpp"

#include <locale>
#include <sstream>

namespace
{
	// Reads the whole string as a double. A stream with the classic locale always
	// expects a '.', whereas std::stod follows the C locale, which QApplication sets
	// from the system (with a French locale std::stod reads "2.5" as 2).
	// Fails for a value a double cannot hold, such as 1e999.
	bool read_number(const std::string& t, double& value)
	{
		std::istringstream is(t);
		is.imbue(std::locale::classic());
		is >> value;

		return !is.fail() && is.eof();
	}
}

bool is_number(const std::string& t)
{
	double value;

	return regex_match( t, std::regex{ R"z((\d+(\.\d*)?|\.\d+)([Ee][\+\-]?\d+)?)z" } )
		&& read_number(t, value);
}

double to_number(const std::string& t)
{
	double value = 0.0;
	read_number(t, value);

	return value;
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
