#ifndef DATA_HPP
#define DATA_HPP

#include<map>
#include<string>

class CData
{
private:
	std::map<std::string,double> m_data;
	double m_result;

public:
	CData()=delete;
	CData(const std::map<std::string,double>& data, double&& result);
	CData(std::map<std::string,double>&& data, double&& result);
	CData(const CData&) = delete;
	~CData(){}

	double get_result() const{return m_result;}
	const std::map<std::string,double>& get_data() const{return m_data;}
	double get_data(std::string) const;

	void s_data(std::map<std::string,double>&& data){m_data = data;}
	void s_result();
};

#endif // DATA_HPP
