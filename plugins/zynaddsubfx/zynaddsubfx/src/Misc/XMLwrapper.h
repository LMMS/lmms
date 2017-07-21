/*
  ZynAddSubFX - a software synthesizer

  XMLwrapper.h - XML wrapper
  Copyright (C) 2003-2005 Nasca Octavian Paul
  Copyright (C) 2009-2009 Mark McCurry
  Author: Nasca Octavian Paul
          Mark McCurry

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

#if 1
#include "QtXmlWrapper.h"
#else
#include <mxml.h>
#include <string>
#ifndef float
#define float float
#endif

#ifndef XML_WRAPPER_H
#define XML_WRAPPER_H

/**Mxml wrapper*/
class XMLwrapper
{
    public:
        /**
         * Constructor.
         * Will Construct the object and fill in top level branch
         * */
        XMLwrapper();

        /**Destructor*/
        ~XMLwrapper();

        /**
         * Saves the XML to a file.
         * @param filename the name of the destination file.
         * @returns 0 if ok or -1 if the file cannot be saved.
         */
        int saveXMLfile(const std::string &filename) const;

        /**
         * Return XML tree as a string.
         * Note: The string must be freed with free() to deallocate
         * @returns a newly allocated NULL terminated string of the XML data.
         */
        char *getXMLdata() const;

        /**
         * Add simple parameter.
         * @param name The name of the mXML node.
         * @param val  The string value of the mXml node
         */
        void addpar(const std::string &name, int val);

        /**
         * Adds a realtype parameter.
         * @param name The name of the mXML node.
         * @param val  The float value of the node.
         */
        void addparreal(const std::string &name, float val);

        /**
         * Add boolean parameter.
         * \todo Fix this reverse boolean logic.
         * @param name The name of the mXML node.
         * @param val The boolean value of the node (0->"yes";else->"no").
         */
        void addparbool(const std::string &name, int val);

        /**
         * Add string parameter.
         * @param name The name of the mXML node.
         * @param val  The string value of the node.
         */
        void addparstr(const std::string &name, const std::string &val);

        /**
         * Create a new branch.
         * @param name Name of new branch
         * @see void endbranch()
         */
        void beginbranch(const std::string &name);
        /**
         * Create a new branch.
         * @param name Name of new branch
         * @param id "id" value of branch
         * @see void endbranch()
         */
        void beginbranch(const std::string &name, int id);

        /**Closes new branches.
         * This must be called to exit each branch created by beginbranch( ).
         * @see void beginbranch(const std::string &name)
         * @see void beginbranch(const std::string &name, int id)
         */
        void endbranch();

        /**
         * Loads file into XMLwrapper.
         * @param filename file to be loaded
         * @returns 0 if ok or -1 if the file cannot be loaded
         */
        int loadXMLfile(const std::string &filename);

        /**
         * Loads string into XMLwrapper.
         * @param xmldata NULL terminated string of XML data.
         * @returns true if successful.
         */
        bool putXMLdata(const char *xmldata);

        /**
         * Enters the branch.
         * @param name Name of branch.
         * @returns 1 if is ok, or 0 otherwise.
         */
        int enterbranch(const std::string &name);

        /**
         * Enter into the branch \c name with id \c id.
         * @param name Name of branch.
         * @param id Value of branch's "id".
         * @returns 1 if is ok, or 0 otherwise.
         */
        int enterbranch(const std::string &name, int id);

        /**Exits from a branch*/
        void exitbranch();

        /**Get the the branch_id and limits it between the min and max.
         * if min==max==0, it will not limit it
         * if there isn't any id, will return min
         * this must be called only imediately after enterbranch()
         */
        int getbranchid(int min, int max) const;

        /**
         * Returns the integer value stored in node name.
         * It returns the integer value between the limits min and max.
         * If min==max==0, then the value will not be limited.
         * If there is no location named name, then defaultpar will be returned.
         * @param name The parameter name.
         * @param defaultpar The default value if the real value is not found.
         * @param min The minimum return value.
         * @param max The maximum return value.
         */
        int getpar(const std::string &name, int defaultpar, int min,
                   int max) const;

        /**
         * Returns the integer value stored in the node with range [0,127].
         * @param name The parameter name.
         * @param defaultpar The default value if the real value is not found.
         */
        int getpar127(const std::string &name, int defaultpar) const;

        /**
         * Returns the boolean value stored in the node.
         * @param name The parameter name.
         * @param defaultpar The default value if the real value is not found.
         */
        int getparbool(const std::string &name, int defaultpar) const;

        /**
         * Get the string value stored in the node.
         * @param name The parameter name.
         * @param par  Pointer to destination string
         * @param maxstrlen Max string length for destination
         */
        void getparstr(const std::string &name, char *par, int maxstrlen) const;

        /**
         * Get the string value stored in the node.
         * @param name The parameter name.
         * @param defaultpar The default value if the real value is not found.
         */
        std::string getparstr(const std::string &name,
                              const std::string &defaultpar) const;

        /**
         * Returns the real value stored in the node.
         * @param name The parameter name.
         * @param defaultpar The default value if the real value is not found.
         */
        float getparreal(const char *name, float defaultpar) const;

        /**
         * Returns the real value stored in the node.
         * @param name The parameter name.
         * @param defaultpar The default value if the real value is not found.
         * @param min The minimum value
         * @param max The maximum value
         */
        float getparreal(const char *name,
                         float defaultpar,
                         float min,
                         float max) const;

        bool minimal; /**<false if all parameters will be stored (used only for clipboard)*/

        /**
         * Sets the current tree's PAD Synth usage
         */
        void setPadSynth(bool enabled);
        /**
         * Checks the current tree for PADsynth usage
         */
        bool hasPadSynth() const;

    private:

        /**
         * Save the file.
         * @param filename File to save to
         * @param compression Level of gzip compression
         * @param xmldata String to be saved
         */
        int dosavefile(const char *filename,
                       int compression,
                       const char *xmldata) const;

        /**
         * Loads specified file and returns data.
         *
         * Will load a gziped file or an uncompressed file.
         * @param filename the file
         * @return The decompressed data
         */
        char *doloadfile(const std::string &filename) const;

        mxml_node_t *tree; /**<all xml data*/
        mxml_node_t *root; /**<xml data used by zynaddsubfx*/
        mxml_node_t *node; /**<current subtree in parsing or writing */
        mxml_node_t *info; /**<Node used to store the information about the data*/

        /**
         * Create mxml_node_t with specified name and parameters
         *
         * Results should look like:
         * <name optionalParam1="value1" optionalParam2="value2" ...>
         *
         * @param name The name of the xml node
         * @param params The number of the attributes
         * @param ... const char * pairs that are in the format attribute_name,
         * attribute_value
         */
        mxml_node_t *addparams(const char *name, unsigned int params,
                               ...) const;

        /**@todo keep these numbers up to date*/
        struct {
            int Major; /**<major version number.*/
            int Minor; /**<minor version number.*/
            int Revision; /**<version revision number.*/
        } version;
};

#endif
#endif
