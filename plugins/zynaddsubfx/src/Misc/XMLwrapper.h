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

#include <mxml.h>
#include <string>
#ifndef REALTYPE
#define REALTYPE float
#endif

#ifndef XML_WRAPPER_H
#define XML_WRAPPER_H

#define TMPSTR_SIZE 50

//the maxim tree depth
#define STACKSIZE 100

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
    int saveXMLfile(const std::string &filename);

    /**
     * Return XML tree as a string.
     * Note: The string must be freed with free() to deallocate
     * @returns a newly allocated NULL terminated string of the XML data.
     */
    char *getXMLdata();

    /**
     * Add simple parameter.
     * @param name The name of the mXML node.
     * @param val  The string value of the mXml node
     */
    void addpar(const std::string &name,int val);

    /**
     * Adds a realtype parameter.
     * @param name The name of the mXML node.
     * @param val  The REALTYPE value of the node.
     */
    void addparreal(const std::string &name,REALTYPE val);

    /**
     * Add boolean parameter.
     * \todo Fix this reverse boolean logic.
     * @param name The name of the mXML node.
     * @param val The boolean value of the node (0->"yes";else->"no").
     */
    void addparbool(const std::string &name,int val);

    /**
     * Add string parameter.
     * @param name The name of the mXML node.
     * @param val  The string value of the node.
     */
    void addparstr(const std::string &name,const std::string &val);

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
    int getbranchid(int min, int max);

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
    int getpar(const std::string &name,int defaultpar,int min,int max);

    /**
     * Returns the integer value stored in the node with range [0,127].
     * @param name The parameter name.
     * @param defaultpar The default value if the real value is not found.
     */
    int getpar127(const std::string &name,int defaultpar);

    /**
     * Returns the boolean value stored in the node.
     * @param name The parameter name.
     * @param defaultpar The default value if the real value is not found.
     */
    int getparbool(const std::string &name,int defaultpar);

    /**
     * Get the string value stored in the node.
     * @param name The parameter name.
     * @param par  Pointer to destination string
     * @param maxstrlen Max string length for destination
     */
    void getparstr(const std::string &name,char *par,int maxstrlen);

    /**
     * Returns the real value stored in the node.
     * @param name The parameter name.
     * @param defaultpar The default value if the real value is not found.
     */
    REALTYPE getparreal(const char *name,REALTYPE defaultpar);

    /**
     * Returns the real value stored in the node.
     * @param name The parameter name.
     * @param defaultpar The default value if the real value is not found.
     * @param min The minimum value
     * @param max The maximum value
     */
    REALTYPE getparreal(const char *name,REALTYPE defaultpar,REALTYPE min,REALTYPE max);

    bool minimal;/**<false if all parameters will be stored (used only for clipboard)*/

    struct {
        bool PADsynth_used;/**<if PADsynth is used*/
    }information;/**<Defines if PADsynth is used*/

    /**
     * Opens a file and parses the "information" section data on it.
     * @returns "true" if all went ok or "false" on errors.
     */
    bool checkfileinformation(const char *filename);

private:

    /**
     * Save the file.
     * @param filename File to save to
     * @param compression Level of gzip compression
     * @param xmldata String to be saved
     */
    int dosavefile(const char *filename,int compression,const char *xmldata);

    /**
     * Loads specified file and returns data.
     * 
     * Will load a gziped file or an uncompressed file.
     * @param filename the file
     * @return The decompressed data
     */
    char *doloadfile(const std::string &filename);


    mxml_node_t *tree;/**<all xml data*/
    mxml_node_t *root;/**<xml data used by zynaddsubfx*/
    mxml_node_t *node;/**<current node*/
    mxml_node_t *info;/**<Node used to store the information about the data*/

    /**
     * Adds params like this:
     * <name>.
     * @returns The node
     */
    mxml_node_t *addparams0(const char *name);

    /**
     * Adds params like this:
     * <name par1="val1">.
     * @returns The node
     */
    mxml_node_t *addparams1(const char *name,const char *par1,const char *val1);

    /**
     * Adds params like this:
     * <name par1="val1" par2="val2">.
     * @returns the node
     */
    mxml_node_t *addparams2(const char *name,const char *par1,const char *val1,const char *par2, const char *val2);

    /**
     * Convert integer to string
     * @param x integer input
     * @returns string output
     */
    char *int2str(int x);

    /**
     * Convert integer to string
     * @param x integer input
     * @returns string output
     */
    char *real2str(REALTYPE x);

    /**
     * Convert string to int
     * @param str string input
     * @returns integer output
     */
    int str2int(const char *str);

    /**
     * Convert string to realtype
     * @param x integer input
     * @returns string output
     */
    REALTYPE str2real(const char *str);

    /**Temporary string for various uses*/
    char tmpstr[TMPSTR_SIZE];


    /**this is used to store the parents.
     * @todo Use the stack class provided by C++*/
    mxml_node_t *parentstack[STACKSIZE];
    int stackpos;/**<position in stack*/


    void push(mxml_node_t *node);

    /**Pops top node off of parent stack*/
    mxml_node_t *pop();
    /**Returns top node off of parent stack*/
    mxml_node_t *peek();

    struct {
        struct {
            int major;/**<major version number.*/
            int minor;/**<minor version number.*/
        }xml_version;/**<Stores ZynAddSubFX versioning information*/
    }values;/**< Stores ZynAddSubFX versioning information*/

};

#endif
