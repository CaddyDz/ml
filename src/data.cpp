#include "data.hpp"

CData::CData(const std::map<std::string,double>& data, double&& result)
{
	m_data = data;
	m_result = result;
}

CData::CData(std::map<std::string,double>&& data, double&& result)
{
	m_data = data;
	m_result = result;
}
