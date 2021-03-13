//
// Created by seledreams on 2021-03-10.
//

#include "NoteParameter.h"

NoteParameter::NoteParameter(const std::string &key,
							 const std::string &name,
							 int value,
							 int maxValue,
							 int minValue)
{
	setKey(key);
	setName(name);
	setMaxValue(maxValue);
	setMinValue(minValue);
	setValue(value);
	setDefaultValue(value);
}
NoteParameter::NoteParameter()
{

}
