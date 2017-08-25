/*
  ZynAddSubFX - a software synthesizer

  Control.h - Defines a variable that can be controled from a frontend

  Copyright (C) 2009 Harald Hvaal
  Author: Harald Hvaal

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2 or later) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
*/

#ifndef _CONTROL_H_
#define _CONTROL_H_

#include <string>

class Control
{
    public:
        /**
         * The parent is the logical owner of this control. Parent should only
         * be null for the root node.
         * The id is a string uniquely identifying this control within the
         * context of the parent control. No spaces or dots are allowed in this
         * id.
         * Children id's are denoted by <parent-id>.<children-id>, so that one
         * can refer to any control in the hierarchy by separating them with
         * dots. Example: Main.AddSynth.FrequencyLFO.Amplitude
         */
        Control(Control *parent, string id);

        /**
         * Will recursively get the XML representation for all the subcontrols.
         * Used for saving to file and copy-pasting settings
         */
        string getXMLRepresentation();

        /**
         * Set the value of this (and possibly subcomponents as well) based on
         * a xml description.
         */
        void restoreFromXML(string xml);

        /**
         * Register a controluser. This will cause this user to be notified
         * whenever the contents of the control changes.
         */
        void registerControlUser(ControlUser *user);

        /**
         * This should return a string representation of the controls internal
         * value
         */
        virtual string getStringRepresentation() = 0;
};

class FloatControl:public Control
{
    public:
        /**
         * Set the value of this control. If the ControlUser variable is set,
         * then this user will not be updated with the new value. This is to
         * avoid setting a value being set back to the source that set it
         * (which would be redundant, or possibly causing infinite setValue
         * loops).
         * NOTE: this function is thread-safe (using a mutex internally)
         */
        void setValue(float value, ControlUser *user = NULL);

        /**
         * Reimplemented from Control
         */
        virtual string getStringRepresentation();

        float value();
};

class ControlUser
{
    public:
        /**
         * Pure virtual method, to notify the controluser that the value has
         * been changed internally, and needs to be read again.
         */
        virtual void controlUpdated(Control *control) = 0;
};

#endif /* _CONTROL_H_ */
