#!/usr/bin/env python
"""
sync_translations.py - sync strings and translations with Transifex

Copyright (c) 2025 allejok96 (gmail)

This file is part of LMMS - https://lmms.io

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public
License along with this program (see COPYING); if not, write to the
Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301 USA.
"""

import argparse
import contextlib
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Iterator, NamedTuple, Optional

try:
	# Using lxml instead of ElementTree because it preserves formatting
	from lxml import etree
except ImportError:
	print(f"ERROR: Could not find package `lxml`. Run `pip install lxml` to install it.", file=sys.stderr)
	exit(1)

#
# CONSTANTS
#

LUPDATE_EXECUTABLES = 'lupdate', 'lupdate-qt5'
LRELEASE_EXECUTABLES = 'lrelease', 'lrelease-qt5'
TRANSIFEX_EXECUTABLES = 'tx', 'tx-cli' # (arch linux)

# Files to look for strings in (copied from lupdate --help)
LUPDATE_EXTENSIONS = 'java,jui,ui,c,c++,cc,cpp,cxx,ch,h,h++,hh,hpp,hxx,js,qs,qml,qrc'.split(',')

# Path where the .ts files are stored
TRANSLATION_DIR = Path('./data/locale')

# Names for temporary .ts file backups
BACKUP_PREFIX = time.strftime('%y%m%d_%H%M%S_')
BACKUP_SUFFIX = '.bak'

#
# ARGUMENTS
#

LANGUAGES: list[str] = []


#
# HELPER FUNCTIONS
#

def msg(label: str, *lines: str, **kwargs):
	label = f'[{label}] '
	indentation = '\n' + ' ' * len(label)
	print(f'{label}{indentation.join(lines)}', **kwargs)


def info(*lines: str):
	msg('INFO', *lines)


def error(*lines: str):
	msg('ERROR', *lines, file=sys.stderr)
	exit(1)


def choice(prompt: str, choices: str):
	"""User input prompt like Proceed? [y/n]

	 Returns upper-case letter
	 """
	while True:
		reply = input(f'[?] {prompt} [{"/".join(choices)}]\n>>> ')
		if len(reply) == 1 and reply.upper() in choices.upper():
			return reply.upper()


def run(command: list[str], exit_on_error=True, **kwargs) -> bool:
	"""Run an external command"""

	print(f'[DEBUG] Running {shlex.join(command)}')
	proc = subprocess.run(command, **kwargs)
	if proc.returncode != 0 and exit_on_error:
		error(f'Failed with code {proc.returncode}')
	print('----------------------------------------------')
	return proc.returncode == 0


def find_executable(executables: tuple[str, ...]) -> str:
	"""Return first existing executable"""

	for ex in executables:
		if shutil.which(ex) is not None:
			return ex
	raise OSError(f'None of these commands were found: {executables}')


# TODO Python 3.12: use tempfile.NamedTempFile(delete=True, delete_on_close=False) instead

@contextlib.contextmanager
def NamedTempFile(suffix: str) -> str:
	"""Context manager that returns a file name to a temp file

	The file is deleted when the context ends (not when the file is closed).

	with NamedTempFile(ext) as file:
	    do_stuff(file)
	"""
	file_descriptor, file_name = tempfile.mkstemp(suffix=suffix, text=True)
	os.close(file_descriptor)
	try:
		yield file_name
	finally:
		os.remove(file_name)


def iter_ts_files(include_english: bool) -> Iterator[Path]:
	"""Iterate through *.ts files for given languages"""

	for file in TRANSLATION_DIR.glob('*.ts'):
		if not LANGUAGES or file.stem in LANGUAGES:
			yield file
		elif include_english and file.name == 'en.ts':
			yield file


#
# TRANSLATION FILE PARSING
#

SourceString = str
TranslationDict = dict[SourceString, str]


class UntranslatedMessage(NamedTuple):
	message: etree.Element
	source: SourceString
	translation: Optional[etree.Element]


def collect_messages(tree: etree.ElementTree, skip_untranslated=False) \
	-> tuple[TranslationDict, list[UntranslatedMessage]]:
	"""Collect translations from a TS file

	:param tree: XML tree of a TS file
	:param skip_untranslated: don't collect untranslated elements (faster)
	:return: dict of {source: translation} plus a list of untranslated elements
	"""

	translated: TranslationDict = {}
	untranslated: list[UntranslatedMessage] = []
	ambiguous: set[SourceString] = set()

	for ctx_element in tree.getroot().iterfind('context'):
		for msg_element in ctx_element.iterfind('message'):

			# Only collect message if it has a source string
			src_element = msg_element.find('source')
			if src_element is None:
				continue
			src_string = src_element.text
			if not src_string:
				continue

			tr_element = msg_element.find('translation')

			# TODO support <numerusform>
			# https://help.transifex.com/en/articles/6223301-qt-linguist

			# Message has no translation - collect it maybe
			if tr_element is None or tr_element.get('type') == 'unfinished' or not tr_element.text:
				if not skip_untranslated:
					untranslated.append(UntranslatedMessage(msg_element, src_string, tr_element))

			# Translation doesn't match previous translation - mark it ambiguous
			elif src_string in translated and translated[src_string] != tr_element.text:
				del translated[src_string]
				ambiguous.add(src_string)

			# Message has unambiguous translation - collect it
			elif src_string not in ambiguous:
				translated[src_string] = tr_element.text

	return translated, untranslated


def merge_translations(local_translations: TranslationDict,
					   online_translations: TranslationDict,
					   file: Path) -> TranslationDict:
	"""Merge local and online translations, detecting merge conflicts

	If a translator has been working offline, the local and online translations will differ. This is not a problem,
	except when the translator has CHANGED translations locally. Normally the online translations will overwrite
	the local ones, but in this case we don't want that...

	To detect this and prevent the translator's hard work from getting lost, we check if the local file has MORE
	translations than the online resource. If that's the case we ask the user which translations to use.

	IMPORTANT NOTE:
		If the translator has NOT ADDED, but ONLY CHANGED translations locally, their changes will be overwritten.
		To prevent this the translator must upload the local TS file to Transifex, or send it to someone who does.

	TODO:
		To solve the above quirk we could ask any time there's a merge conflict, but that would mean asking every time
		a translated string has changed online. The translation maintainer who runs this script (for ALL languages)
		would get overwhelmed by these questions and either quit or blindly answer YES to all.
		We could also check if the local file has been edited more recently than the online resource,
	    but this is dangerous since the online resource might change at any time.
	"""

	prefer_online = True

	# Check if the local file has strings that are not present at Transifex
	# (this should not happen unless the local file was edited manually)
	if any(source_string not in online_translations for source_string in local_translations):

		# Find strings that are identical between the files but translated differently
		conflicts = [(source, local, online_translations[source])
					 for source, local in local_translations.items()
					 if source in online_translations
					 if online_translations[source] != local]

		if len(conflicts) > 0:
			msg('WARN',
				f"{file.name} have been edited outside of Transifex!",
				f"Local translations: {len(local_translations)}",
				f"Online translations: {len(online_translations)}",
				f"Conflicting translations:",
				*(f'  {source!r} | local: {old!r} | online: {new!r}' for source, old, new in conflicts))

			if choice(f"Use the local or online translation in these {len(conflicts)} conflicts?", 'LO') == 'L':
				prefer_online = False
			info('Using ' + ('online' if prefer_online else 'local') + ' translations')

	if prefer_online:
		return local_translations | online_translations
	else:
		return online_translations | local_translations


#
# MAIN FUNCTIONS
#

def check_requirements():
	"""Make sure we have all the required executables"""

	if not TRANSLATION_DIR.exists():
		error(f"Can't find the {TRANSLATION_DIR} directory",
			  "You should run this script from the root of the git repo")

	if shutil.which('git') is None:
		error("You don't seem to have `git` installed")

	# In case something goes horribly wrong, we want to be able to git restore
	ts_files = [str(path) for path in iter_ts_files(include_english=True)]
	if ts_files and subprocess.run(['git', 'diff', '--quiet', '--'] + ts_files).returncode != 0:
		error("The translations have uncommited changes",
			  "Please git commit or git restore them")

	if all(shutil.which(cmd) is None for cmd in TRANSIFEX_EXECUTABLES):
		error(f"You don't seem to have the Transifex client `{TRANSIFEX_EXECUTABLES[0]}` installed",
			  "Get the latest release here: https://github.com/transifex/cli"
			  "or install some package like `transifex-cli`")

	for commands in (LUPDATE_EXECUTABLES, LRELEASE_EXECUTABLES):
		if all(shutil.which(cmd) is None for cmd in commands):
			error(f"You don't seem to have `{commands[0]}` installed",
				  "Usually this comes with your Qt installation, or you need to install some",
				  "package like `qt5-tools`, `qttools5-dev-tools`, `qt6-l10n-tools` or similar")


def backup_translations():
	info("Creating temporary backup files")

	for file in iter_ts_files(include_english=True):
		backup = file.parent / (BACKUP_PREFIX + file.name + BACKUP_SUFFIX)
		shutil.copy2(file, backup)


def pull_translations_from_tx():
	info('Pulling down translations from Transifex')

	command = [
		find_executable(TRANSIFEX_EXECUTABLES),
		'pull',  # this will overwrite files in data/locale [1]
		'--mode', 'translator',  # both translated and untranslated strings [2]
		'--force',  # even if the local file is newer than the online resource
	]

	if LANGUAGES:
		command += ['--languages', ','.join(LANGUAGES)]
	else:
		command += ['--all']
	# command += ['--minimum-perc', '50']

	run(command)

	# [1] https://github.com/transifex/cli?tab=readme-ov-file#pulling-files-from-transifex
	# [2] https://help.transifex.com/en/articles/6223301-qt-linguist
	...


def find_new_strings_in_codebase():
	"""Extract translatable strings from source code and merge them into existing TS files

	This is done using `lupdate` and only for files that are added to git.
	"""

	info("Adding new strings from the codebase")

	# Find all files that may contain strings and write filenames to a text file
	with NamedTempFile('.txt') as list_file:
		with open(list_file, 'w') as fd:
			# Find file in git with the supported file extensions, case-insensitive [1]
			# git ls-files ":(icase)*.cpp" ... > list_file
			run(['git', 'ls-files'] + [f':(icase)*.{ext}' for ext in LUPDATE_EXTENSIONS], stdout=fd)

		ts_files = [str(path) for path in iter_ts_files(include_english=True)]

		# If data/locales is empty, generate a file of English strings
		if not ts_files:
			ts_files.append('en.ts')

		# Usage: lupdate [options] @lst-file -ts ts-files ... [2]
		run([
			find_executable(LUPDATE_EXECUTABLES),
			'-no-obsolete',  # drop obsolete and vanished strings
			'-I', 'include/',  # location to look for include files
			f'@{list_file}',  # read file names (one per line) from this file
			'-ts', *ts_files
		])

	# [1] https://git-scm.com/docs/gitglossary#Documentation/gitglossary.txt-pathspec
	# [2] https://doc.qt.io/qt-6/linguist-lupdate.html
	...


def fill_in_known_translations():
	"""Imitate Transifex's translation memory fill-up (paid feature) [1]

	This indexes all translations, and if an untranslated source string matches an already translated source
	string by 100% it copies the translation. If a source string is translated in two different ways, it will be marked
	ambiguous and the translation won't be copied to any other string.

	This technique may lead to some bad translations because a word can have multiple meanings (e.g. please "note"
	and whole "note"). If that were to happen it can be corrected later by the Transifex translators.

	In the end this saves a lot of time since every time a string is copied/moved to a different class, or the class
	is renamed, the string needs to be translated again at Transifex. Even if it only takes a single click thanks
	to Transifex's Translation Memory system, it still requires manual intervention.

	The paid Transifex fill-up feature keeps translations in its Translation Memory indefinitely. This script can only
	compare the old and new TS file. But at least it solves the problem of translations disappearing due to moving or
	renaming classes.

	[1] https://help.transifex.com/en/articles/6224817-setting-up-translation-memory-fill-up
	"""

	info('Filling up missing or moved translations')

	for new_ts in iter_ts_files(include_english=False):

		# Collect translations from the old local file
		old_ts = new_ts.parent / (BACKUP_PREFIX + new_ts.name + BACKUP_SUFFIX)
		if old_ts.exists():
			old_tree = etree.parse(old_ts)
			old_translations, _ = collect_messages(old_tree, skip_untranslated=True)
		else:
			old_translations = {}

		# Collect translations from the new Transifex file
		new_tree = etree.parse(new_ts)
		new_translations, untranslated_messages = collect_messages(new_tree)

		# Merge the old and new translations (detect merge conflicts)
		translations = merge_translations(old_translations, new_translations, new_ts)

		# Fill-up untranslated messages in the XML DOM with known translations
		for untranslated in untranslated_messages:
			if untranslated.source not in translations:
				continue

			# Add <translation> element if it doesn't exist
			# (does this ever happen though?)
			if untranslated.translation is None:
				untranslated.translation = etree.Element('translation')
				untranslated.message.append(untranslated.translation)

			untranslated.translation.text = translations[untranslated.source]

			# Remove type="unfinished"
			if 'type' in untranslated.translation.attrib:
				del untranslated.translation.attrib['type']

		# Write the updated XML to disk
		# For some reason pushing doesn't support utf8, even if the files returned by pull are utf8
		# It results in: `Your file doesn't seem to contain valid xml: not well-formed (invalid token)`
		new_tree.write(new_ts, encoding="ascii", xml_declaration=True)


def validate_translation_files():
	"""Try compiling the translations files

	This is done using `lrelease` which converts text based TS files to binary QM files.
	It will at least detect if the file is severely broken or empty...
	"""

	info("Trying to compile translation files into binary format")
	found_errors = False

	for file in iter_ts_files(include_english=True):
		with NamedTempFile('.qm') as temp_qm:
			if not run([find_executable(LRELEASE_EXECUTABLES), str(file), '-qm', temp_qm]):
				found_errors = True
	if found_errors:
		exit(1)

	info('All translation files compiled successfully!')


def push_translations_to_tx():
	info("Pushing translations to Transifex")

	# Side note (you don't need to know this)
	#
	# You may have seen `--replace-edited-strings` and `--keep-translations` in the docs and wondered what they do.
	#
	# If I understand this PR [1] correctly
	# --replace-edited-strings is used to overwrite source strings that have been edited online [2].
	# --keep-translations is used when a source string has changed but its string key [3] remains the same.
	#
	# I don't think we want the first, and I don't think the latter can happen, since in the case of TS files
	# the string key and the source string are the same [4].

	tx = find_executable(TRANSIFEX_EXECUTABLES)

	# Push the updated English strings
	run([
		tx,
		'push',
		'--source',
		'--force',
	])

	# Push the updated translations
	command = [
		tx,
		'push',
		'--translation',
		'--force',  # push even if the online resource was edited while this script was running [5]
	]
	if LANGUAGES:
		command += ['--languages', ','.join(LANGUAGES)]
	run(command)

	info('Translations uploaded!')

	# [1] https://github.com/transifex/cli/pull/195
	# [2] https://help.transifex.com/en/articles/6236820-edit-source-strings-online
	# [3] https://help.transifex.com/en/articles/6649327-how-string-hashes-are-calculated
	# [4] https://help.transifex.com/en/articles/6223301-qt-linguist
	# [5] https://github.com/transifex/cli?tab=readme-ov-file#pushing-files-to-transifex
	...


def delete_backups():
	info('Deleting temporary backup files')
	for file in TRANSLATION_DIR.glob(BACKUP_PREFIX + '*.ts' + BACKUP_SUFFIX):
		file.unlink()


def main():
	global LANGUAGES

	parser = argparse.ArgumentParser(description='Pull translations from and push strings to Transifex')
	parser.add_argument('language', nargs='*', help='Language code (default all languages)')
	args = parser.parse_args()
	LANGUAGES = args.language

	check_requirements()
	backup_translations()
	try:
		pull_translations_from_tx()
		find_new_strings_in_codebase()
		fill_in_known_translations()
		validate_translation_files()
	finally:
		delete_backups()

	if choice("Push translations to Transifex?", 'YN') == 'Y':
		push_translations_to_tx()

	msg('BYE', 'See you later, translator!')


if __name__ == '__main__':
	main()
