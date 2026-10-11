#ifndef DATA_HPP
#define DATA_HPP

#include<map>
#include<string>
#include<utility>

/// One reference sample: the values of the variables and the expected result.
class CData
{
private:
	std::map<std::string,double> m_data;
	double m_result;

public:
	CData()=delete;
	CData(const std::map<std::string,double>& data, double result);
	CData(std::map<std::string,double>&& data, double result);
	CData(const CData&) = delete;
	~CData(){}

	double get_result() const{return m_result;}
	const std::map<std::string,double>& get_data() const{return m_data;}
	double get_data(std::string) const;   ///< value of one variable, NaN if the sample has none

	void s_data(std::map<std::string,double>&& data){m_data = std::move(data);}
	void s_result(double result){m_result = result;}
};

#endif // DATA_HPP
