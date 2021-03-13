//
// Created by seledreams on 2021-03-10.
//

#ifndef NOTEPARAMETER_H
#define NOTEPARAMETER_H
#include <iostream>
#include <lmms_math.h>
#include <qdebug.h>
#include <shared_object.h>

class NoteParameter
{
public:
	NoteParameter(const std::string &key, const std::string &name, int value = 0, int maxValue = 1,int minValue = 0);
	NoteParameter();
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
	void setValue(int newValue)
	{
		m_value = newValue;
		qDebug() << m_value;
	}
	inline int value() const
	{
		return m_value;
	}
	void setMaxValue(int newMaxValue)
	{
		m_maxValue = newMaxValue;
	}
	inline int maxValue() const
	{
		return m_maxValue;
	}
	void setMinValue(int newMinValue)
	{
		m_minValue = newMinValue;
	}
	inline int minValue() const
	{
		return m_minValue;
	}
	inline int defaultValue() const
	{
		return m_defaultValue;
	}
	void setDefaultValue(int newDefaultValue)
	{
		m_defaultValue = newDefaultValue;
	}

private:
	std::string m_key;
	std::string m_name;
	int m_minValue;
	int m_maxValue;
	int m_value;
	int m_defaultValue;
};
#endif //NOTEPARAMETER_H
