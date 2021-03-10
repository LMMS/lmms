//
// Created by seledreams on 2021-03-10.
//

#ifndef NOTEPARAMETER_H
#define NOTEPARAMETER_H
#include <iostream>
#include <lmms_math.h>

class NoteParameter{
public:
	NoteParameter(const std::string &key, const std::string &name, double value = 0, double maxValue = 1,double minValue = 0);
	virtual ~NoteParameter() = default;
	void setKey(const std::string &newKey)
	{
		m_key = newKey;
	}
	inline const std::string &key()
	{
		return m_key;
	}
	void setName(const std::string &newName)
	{
		m_name = newName;
	}
	inline const std::string &name()
	{
		return m_name;
	}
	void setValue(double newValue)
	{
		m_value = fmin(fmax(newValue,m_minValue),m_maxValue);
	}
	inline double value()
	{
		return m_value;
	}
	void setMaxValue(double newMaxValue)
	{
		if (newMaxValue <= 0)
		{
			std::cout << "Warning : the max value of this property is negative or 0, setting max value to 1" << std::endl;
		}
		m_maxValue = fmin(1,newMaxValue);
	}
	inline double maxValue()
	{
		return m_maxValue;
	}
	void setMinValue(double newMinValue)
	{
		if (newMinValue > m_maxValue)
		{
			std::cout << "Warning : the min value of this property is superior to max value, setting min value to 0" << std::endl;
			m_minValue = 0;
		}
		else
		{
			m_minValue = newMinValue;
		}
	}
private:
	std::string m_key;
	std::string m_name;
	double m_minValue;
	double m_maxValue;
	double m_value;
};
#endif //NOTEPARAMETER_H
