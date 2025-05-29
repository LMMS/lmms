#
# setupeffect.py - Script for quickly generating template effect source code
#
# Copyright (c) 2025 regulus79
#
# This file is part of LMMS - https://lmms.io
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public
# License along with this program (see COPYING); if not, write to the
# Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA.
#


import os
import shutil


#
# Load template files
#

templateFolderPath = os.path.join(os.path.dirname(__file__), "TemplateEffect")
templateName = "TemplateEffect"
templateNameLowercase = "templateeffect"
templateNameUppercase = "TEMPLATE_EFFECT"
templateHumanName = "TemplateHumanName"
templateDescription = "TemplateDescription"
templateAuthor = "TemplateAuthor"
templateNumKnobs = "TEMPLATE_NUM_KNOBS"

with open(os.path.join(templateFolderPath, "TemplateEffect.cpp"), "r") as file:
	templateMainCpp = file.read()
with open(os.path.join(templateFolderPath, "TemplateEffect.h"), "r") as file:
	templateMainHeader = file.read()
with open(os.path.join(templateFolderPath, "TemplateEffectControls.cpp"), "r") as file:
	templateControlsCpp = file.read()
with open(os.path.join(templateFolderPath, "TemplateEffectControls.h"), "r") as file:
	templateControlsHeader = file.read()
with open(os.path.join(templateFolderPath, "TemplateEffectControlDialog.cpp"), "r") as file:
	templateControlDialogCpp = file.read()
with open(os.path.join(templateFolderPath, "TemplateEffectControlDialog.h"), "r") as file:
	templateControlsDialogHeader = file.read()
with open(os.path.join(templateFolderPath, "CMakeLists.txt"), "r") as file:
	templateCMakeLists = file.read()


#
# Get user input
#

effectName = input("Effect Name (UpperCamelCase): ")

assert not " " in effectName, "Effect name cannot contain spaces."
assert not effectName[0].isdigit(), "Effect name cannot start with a number."
assert all(c.isalnum() for c in effectName), "Effect name must be alphanumeric."
assert any(c.islower() for c in effectName), "Effect name must be in UpperCamelCase (have at least one lowercase letter.)"
assert any(c.isupper() for c in effectName), "Effect name must be in UpperCamelCase (have at least one uppercase letter.)"

effectNameLowercase = effectName.lower()
# The uppercase name should have _ between words, so RandomEffect becomes RANDOM_EFFECT
effectNameUppercase = ""
for i in range(len(effectName)):
	if i < len(effectName) - 1 and effectName[i + 1].isupper() and effectName[i].islower():
		effectNameUppercase += effectName[i].upper() + "_"
	else:
		effectNameUppercase += effectName[i].upper()

print(f"Lowercase version: {effectNameLowercase}, Uppercase version: {effectNameUppercase}")

effectHumanName = input("Effect Name (Human readable, spaces allowed): ")
effectDescription = input("Effect Description: ")
effectAuthor = input("Effect Author: ")
effectKnobs = int(input("Number of knobs: "))


#
# Create directory and write files
#

effectFolderPath = os.path.join(os.path.dirname(__file__), effectName)
assert not os.path.exists(effectFolderPath), f"Directory {effectFolderPath} already exists."

print(f"Creating directory {effectFolderPath}")
os.makedirs(effectFolderPath)


def replaceTemplateKeywords(text):
	# Replace keywords
	text = text\
		.replace(templateName, effectName)\
		.replace(templateNameLowercase, effectNameLowercase)\
		.replace(templateNameUppercase, templateNameUppercase)\
		.replace(templateHumanName, effectHumanName)\
		.replace(templateDescription, effectDescription)\
		.replace(templateAuthor, effectAuthor)\
		.replace(templateNumKnobs, str(effectKnobs))
	# Replace/repeat knob creation code
	textLines = text.split("\n")
	newTextLines = []
	tmp = []
	inLoop = False
	for i, line in enumerate(textLines):
		if "TEMPLATE_KNOB_LOOP_START" in line:
			inLoop = True
		elif "TEMPLATE_KNOB_LOOP_END" in line:
			inLoop = False
			for knob in range(effectKnobs):
				newTextLines.append("\n".join(tmp).replace("TEMPLATE_KNOB_NUMBER", str(knob)))
			tmp = []
		elif inLoop:
			tmp.append(line)
		else:
			newTextLines.append(line)
	return "\n".join(newTextLines)


with open(os.path.join(effectFolderPath, f"{effectName}.cpp"), "w") as file:
	file.write(replaceTemplateKeywords(templateMainCpp))
with open(os.path.join(effectFolderPath, f"{effectName}.h"), "w") as file:
	file.write(replaceTemplateKeywords(templateMainHeader))
with open(os.path.join(effectFolderPath, f"{effectName}Controls.cpp"), "w") as file:
	file.write(replaceTemplateKeywords(templateControlsCpp))
with open(os.path.join(effectFolderPath, f"{effectName}Controls.h"), "w") as file:
	file.write(replaceTemplateKeywords(templateControlsHeader))
with open(os.path.join(effectFolderPath, f"{effectName}ControlDialog.cpp"), "w") as file:
	file.write(replaceTemplateKeywords(templateControlDialogCpp))
with open(os.path.join(effectFolderPath, f"{effectName}ControlDialog.h"), "w") as file:
	file.write(replaceTemplateKeywords(templateControlsDialogHeader))
with open(os.path.join(effectFolderPath, f"CMakeLists.txt"), "w") as file:
	file.write(replaceTemplateKeywords(templateCMakeLists))
shutil.copy(os.path.join(templateFolderPath, "artwork.svg"), os.path.join(effectFolderPath, "artwork.svg"))
shutil.copy(os.path.join(templateFolderPath, "logo.svg"), os.path.join(effectFolderPath, "logo.svg"))

print(f"{effectName} template files successfully written to {effectFolderPath}")


#
# Update ../cmake/modules/PluginList.cmake
#

with open(os.path.join(os.path.dirname(__file__), f"../cmake/modules/PluginList.cmake"), "r+") as file:
	contents = file.readlines()
	file.seek(0)
	foundList = False
	insertedEffectName = False
	for i, line in enumerate(contents):
		# Find where the list starts
		if line == "SET(LMMS_PLUGIN_LIST\n":
			foundList = True
		# Then go through the list until the effect with a name after in alphabetical order is found
		if foundList:
			if line.lstrip() > effectName:
				contents.insert(i, f"	{effectName}\n")
				insertedEffectName = True
				break
	if insertedEffectName:
		file.write("".join(contents))
		print("Updated ../cmake/modules/PluginList.cmake")
	else:
		print("ERROR Failed to insert effect in ../cmake/modules/PluginList.cmake")
