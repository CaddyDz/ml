#include "data.hpp"

#include <limits>
#include <utility>

CData::CData(const std::map<std::string,double>& data, double result)
{
	m_data = data;
	m_result = result;
}

CData::CData(std::map<std::string,double>&& data, double result)
{
	m_data = std::move(data);
	m_result = result;
}

double CData::get_data(std::string name) const
{
	auto it = m_data.find(name);

	return (it != m_data.end()) ? it->second : std::numeric_limits<double>::quiet_NaN();
}
