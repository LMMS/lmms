//
// Created by seledreams on 2021-03-10.
//

#include "NoteParameter.h"

NoteParameter::NoteParameter(const std::string &key,
							 const std::string &name,
							 double value,
							 double maxValue,
							 double minValue)
{
	setKey(key);
	setName(name);
	setMaxValue(maxValue);
	setMinValue(minValue);
	setValue(value);
}