/**
 * MelodyEditor.h - Text based melody editor for LMMS
 *                - Can parse Western notes
 *                - Can parse SARGAM notes
 *                - Can parse some virual keyboard entries
 *                - Processes text size less than 20kb only
 * 
 * (c) 2025 Bimal Poudel <@anytizer>
 * 
 * Discussion Thread:
 * @see https://github.com/LMMS/lmms/discussions/7950
*/

#include <QApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QTextStream>
#include <QKeySequence>
#include <QShortcut>

#include "DataFile.h"
#include "MidiClip.h"

#ifndef LMMS_GUI_MELODYEDITOR_H
#define LMMS_GUI_MELODYEDITOR_H

namespace lmms::gui
{


class Cell
{
	public:
		int position = 0;
		QString notation = "";

		/**
		 * Beat time length controller in percentage? Based on number of commas in the notes.
		 * eg. 1.00  eg.: S
		 * eg. 0.50  eg.: S,R
		 * eg. 0.33  eg.: S,R,G // NOT recommended. Eg. in flutes.
		 * eg. 0.25  eg.: S,R,G,M
		 * 
		 * Even shorter is possible, but NOT recommended.
		 */
		float length = 0.00f;
};


struct NotationsSystem
{
	QString parser; // @see parseas_xx() functions
	QString name;
	QString help; // @todo Implement help text
	QRadioButton * radio;

	public:
		NotationsSystem(): parser(""), name(""), help(""), radio(new QRadioButton("")) {}
		NotationsSystem(QString parser_name, QString display_name) : parser(parser_name), name(display_name)
		{
			this->help = "";
			this->radio = new QRadioButton(this->name);
		}
};



class FindAndReplace
{
	public:
		QString find = "";
		QString replaceWith = "";
		FindAndReplace(QString f, QString r);
};


FindAndReplace::FindAndReplace(QString f, QString r)
{
	this->find = f;
	this->replaceWith = r;
}


class XPTBuilder
{
	private:
		QList<FindAndReplace *> *replaces = new QList<FindAndReplace *>();
		QList<QString> notes = {}; // pure western names only with sharps | no flats.
	
		
		bool chord_processing = false;
		int chord_start_position = 0;
		QList<Cell> chords = {};

		void cells_to_xml(QList<Cell>& cells, QString &xml);
		int getPianoKey(QString notation);
		void replace_oddities_in_notes(QString &notes);

		/**
		 * Notations handlers
		 * These functions update the "cells" variable.
		 */
		int process_block(const QString block, QList<Cell>& cells, int &position);
		int process_line(const QString line, QList<Cell>& cells, int &position);
		int process_beatnotes(const QString column, QList<Cell>& cells, int &position); // old name: process_note
		int process_tone(const QString tone, QList<Cell>& cells, float semilength, int &position); // return error

	public:
		XPTBuilder();
		~XPTBuilder();

		/**
		 * Parsers only.
		 * Returns number of key errors
		 * Out parameter: xml
		 */
		int parseas_default(const QString &userNotations, QString &xml); // when unable to determine route to one of the known parser
		int parseas_sargam(const QString &userNotations, QString &xml); // Classical romanized sargams
		int parseas_western(const QString &userNotations, QString &xml); // Octaved notes eg: C4, D4, ..
		int parseas_vk(const QString& userNotations, QString &xml); // virtual keyboard | many possibilities
};


XPTBuilder::XPTBuilder()
{
	/**
	 * Adapted from: https://pastebin.com/6p7e1vYw
	 *
	 * Treats [SPACE]  as a short pause. [PIPE] as longer pause.
	 * special characters: x, , -, |, \n
	 */
	this->replaces->append(new FindAndReplace("1", "C2"));
	this->replaces->append(new FindAndReplace("2", "D2"));
	this->replaces->append(new FindAndReplace("3", "E2"));
	this->replaces->append(new FindAndReplace("4", "F2"));
	this->replaces->append(new FindAndReplace("5", "G2"));
	this->replaces->append(new FindAndReplace("6", "A2"));
	this->replaces->append(new FindAndReplace("7", "B2"));
	this->replaces->append(new FindAndReplace("8", "C3"));
	this->replaces->append(new FindAndReplace("9", "D3"));
	this->replaces->append(new FindAndReplace("0", "E3"));
	this->replaces->append(new FindAndReplace("q", "F3"));
	this->replaces->append(new FindAndReplace("w", "G3"));
	this->replaces->append(new FindAndReplace("e", "A3"));
	this->replaces->append(new FindAndReplace("r", "B3"));
	this->replaces->append(new FindAndReplace("t", "C4"));
	this->replaces->append(new FindAndReplace("y", "D4"));
	this->replaces->append(new FindAndReplace("u", "E4"));
	this->replaces->append(new FindAndReplace("i", "F4"));
	this->replaces->append(new FindAndReplace("o", "G4"));
	this->replaces->append(new FindAndReplace("p", "A4"));
	this->replaces->append(new FindAndReplace("a", "B4"));
	this->replaces->append(new FindAndReplace("s", "C5"));
	this->replaces->append(new FindAndReplace("d", "D5"));
	this->replaces->append(new FindAndReplace("f", "E5"));
	this->replaces->append(new FindAndReplace("g", "F5"));
	this->replaces->append(new FindAndReplace("h", "G5"));
	this->replaces->append(new FindAndReplace("j", "A5"));
	this->replaces->append(new FindAndReplace("k", "B5"));
	this->replaces->append(new FindAndReplace("l", "C6"));
	this->replaces->append(new FindAndReplace("z", "D6"));
	this->replaces->append(new FindAndReplace("x", "E6"));
	this->replaces->append(new FindAndReplace("c", "F6"));
	this->replaces->append(new FindAndReplace("v", "G6"));
	this->replaces->append(new FindAndReplace("b", "A6"));
	this->replaces->append(new FindAndReplace("n", "B6"));
	this->replaces->append(new FindAndReplace("m", "C7"));

	this->replaces->append(new FindAndReplace("!", "C#2"));
	this->replaces->append(new FindAndReplace("@", "D#2"));
	this->replaces->append(new FindAndReplace("#", "F2"));
	this->replaces->append(new FindAndReplace("$", "F#2"));
	this->replaces->append(new FindAndReplace("%", "G#2"));
	this->replaces->append(new FindAndReplace("^", "A#2"));
	this->replaces->append(new FindAndReplace("&", "C3"));
	this->replaces->append(new FindAndReplace("*", "C#3"));
	this->replaces->append(new FindAndReplace("(", "D#3"));
	this->replaces->append(new FindAndReplace(")", "F3"));
	this->replaces->append(new FindAndReplace("Q", "F#3"));
	this->replaces->append(new FindAndReplace("W", "G#3"));
	this->replaces->append(new FindAndReplace("E", "A#3"));
	this->replaces->append(new FindAndReplace("R", "C4"));
	this->replaces->append(new FindAndReplace("T", "C#4"));
	this->replaces->append(new FindAndReplace("Y", "D#4"));
	this->replaces->append(new FindAndReplace("U", "F4"));
	this->replaces->append(new FindAndReplace("I", "F#4"));
	this->replaces->append(new FindAndReplace("O", "G#4"));
	this->replaces->append(new FindAndReplace("P", "A#4"));
	this->replaces->append(new FindAndReplace("A", "C5"));
	this->replaces->append(new FindAndReplace("S", "C#5"));
	this->replaces->append(new FindAndReplace("D", "D#5"));
	this->replaces->append(new FindAndReplace("F", "F5"));
	this->replaces->append(new FindAndReplace("G", "F#5"));
	this->replaces->append(new FindAndReplace("H", "G#5"));
	this->replaces->append(new FindAndReplace("J", "A#5"));
	this->replaces->append(new FindAndReplace("K", "C6"));
	this->replaces->append(new FindAndReplace("L", "C#6"));
	this->replaces->append(new FindAndReplace("Z", "D#6"));
	this->replaces->append(new FindAndReplace("X", "F6"));
	this->replaces->append(new FindAndReplace("C", "F#6"));
	this->replaces->append(new FindAndReplace("V", "G#6"));
	this->replaces->append(new FindAndReplace("B", "A#6"));
	this->replaces->append(new FindAndReplace("N", "C7"));
	this->replaces->append(new FindAndReplace("M", "C#7"));

	this->notes.append("C");  // 0
	this->notes.append("C#"); // 1
	this->notes.append("D");  // 2
	this->notes.append("D#"); // 3
	this->notes.append("E");  // 4
	this->notes.append("F");  // 5
	this->notes.append("F#"); // 6
	this->notes.append("G");  // 7
	this->notes.append("G#"); // 8
	this->notes.append("A");  // 9
	this->notes.append("A#"); // 10
	this->notes.append("B");  // 11
}


XPTBuilder::~XPTBuilder()
{
	this->replaces = {};
}


/**
 * When the application did not understand which parser to use.
 * Ideally, control should never reach here. But if we implement
 * dynically selected parser, chances are...
 * 
 * @return Number of errors.
 * @output: XML Variable.
 */
int XPTBuilder::parseas_default(const QString &userNotations, QString &xml)
{
	return this->parseas_sargam(userNotations, xml);
}


/**
 * Expects that user types notations in Classical SARGAMS based on Roman letters.
 * 
 * Allowed letters: S, r, R, g, G, m, M, P, d, D, n, N
 * Allowed symbols: x, -, |, *, . (period)
 */
int XPTBuilder::parseas_sargam(const QString &userNotations, QString &xml)
{
	int errors = 0;

	QString un = userNotations;
	this->replace_oddities_in_notes(un);
    QStringList blocks = un.split("#//");

    int position = 0;

	QList<Cell> cells = {};
    for (const QString& block : blocks)
	{
        errors += this->process_block(block, cells, position);
    }

	this->cells_to_xml(cells, xml);
	return errors;
}


/**
 * Expects that user types notations in Western Notes using plain key names.
 * 
 * Western notes already apppear as parser friendly / ready.
 * 
 * Allowed letters:
 * eg. C* C** C*** C**** C***** (with higher octaves mark)
 * eg. D. D.. D... D.... (with lower octaves mark)
 * eg. C D E F G A B C# D# F# G# A# (4th octave without number)
 * 
 * And with octave numbers as well:
 * eg. C8 D8 E8 F8 G8 A8 B8 C#8 D#8 F#8 G#8 A#8
 * eg. C7 D7 E7 F7 G7 A7 B7 C#7 D#7 F#7 G#7 A#7
 * eg. C6 D6 E6 F6 G6 A6 B6 C#6 D#6 F#6 G#6 A#6
 * eg. C5 D5 E5 F5 G5 A5 B5 C#5 D#5 F#5 G#5 A#5
 * eg. C4 D4 E4 F4 G4 A4 B4 C#4 D#4 F#4 G#4 A#4
 * eg. C3 D3 E3 F3 G3 A3 B3 C#3 D#3 F#3 G#3 A#3
 * eg. C2 D2 E2 F2 G2 A2 B2 C#2 D#2 F#2 G#2 A#2
 * eg. C1 D1 E1 F1 G1 A1 B1 C#1 D#1 F#1 G#1 A#1
 * 
 * And the regular symbols: x, -, . (period), COMMA (,)
 * 
 * @todo Flats are not considered / parsed. They need shifing notes by one semitone down.
*/
int XPTBuilder::parseas_western(const QString &userNotations, QString &xml)
{
	QString un = userNotations.toUpper();
	un.replace("0", "....");
	un.replace("1", "...");
	un.replace("2", "..");
	un.replace("3", ".");
	un.replace("4", ""); // marker is optional for 4th octave
	un.replace("5", "*"); // @todo Chords do not understand this key...
	un.replace("6", "**");
	un.replace("7", "***");
	un.replace("8", "****");
	un.replace("9", "*****");

	int position = 0;

	QList<Cell> cells = {};
	QStringList blocks = un.split("#//");
	int errors = 0;
    for (const QString& block : blocks)
	{
        errors += this->process_block(block, cells, position);
    }

	this->cells_to_xml(cells, xml);
	return errors;
}


/**
 * Build XPT xml data
 * @todo Use LMMS's internal xml writer if available.
 */
void XPTBuilder::cells_to_xml(QList<Cell> &cells, QString &xml)
{
	int steps = 16;

	/**
	 * Fake information to simulate to the latest LMMS version data
	 */
	
	// must be: integer, @see DataFile.cpp#2166
 	QString version = "31";
 	
	// This application name
	QString creator = "MelodyEditor";
	
	// @todo Determine it
	QString creator_version = "1.3.0-alpha.1.851+a505f57";
	
	// @todo Use block number as well
	// "MIDI Clip: %1";
	QString block_name = "Melody";// " #%1").arg();
	block_name.replace("'", ""); // trim the characters that can break quoted data in xml | from user input
	
	// @todo Use color rotator for each block
 	QString color = "#A507F0"; // always prepended # sign

	/**
	 * @todo Find out an alternative way to build this xml
	 */
 	xml = QString(
//"<?xml version='1.0'?>\r\n"
//"<!DOCTYPE lmms-project>\r\n"
"<midiclip type='midiclip' version='%1' creator='%2' creatorversion='%3' steps='%4' muted='0' name='%5' color='%6' type='1' pos='0'>\r\n"
	).arg(
		version,
		creator,
		creator_version,
		QString("%1").arg(steps),
		block_name,
		color
	);

 	for(Cell cell: cells) // cells | notes
 	{
		int pianoKey = this->getPianoKey(cell.notation);
		if(pianoKey == -1) continue; // skip invalid key

 		xml += QString("  <note metadata='0' pan='0' vol='100' key='%1' len='%2' pos='%3' name='%4' />\r\n").arg(
			QString("%1").arg(pianoKey),
			QString("%1").arg(cell.length),
			QString("%1").arg(cell.position),
			cell.notation
		);
 	}
 	xml += QString("</midiclip>");
    xml.replace("'", "\"");
}



/**
 * There are many virtual keyboards possible.
 * 
 * @todo Pick one virtual piano keyboard.
 * eg.: LMMS internal virual keyboard?
 * eg.: https://freepiano.tiwb.com/en/ | https://freepiano.tiwb.com/en/docs/keymap/
 * eg.: https://www.musicca.com/piano
 * eg.: https://virtualpiano.net/ | https://virtualpiano.net/how-to-play/
 * eg.: https://vmpk.sourceforge.io/
 * eg.: https://pianoletternotes.blogspot.com/
 * eg.: https://musiclab.chromeexperiments.com/ - shared piano, song maker, ...
 * 
 * 
 * eg. From pianoletternotes:
 * The numbers in front of each line are the octave, each octave has an unique color so you can easily follow them.
 * Lowercase (a b c d e f g) letters are natural notes (white keys, a.k.a A B C D E F G ).
 * Uppercase (A C D F G) letters are the sharp notes (black keys a.k.a. A# C# D# F# G#)
 * The lines / dashes (-) between letters indicates timing to play the notes.
 * (usually 5-6 dashes is about 1 second)
 * RH / LH means Right Hand / Left Hand and it's mostly for people who play the piano, it tells them with what hand to play the lines.
 * Also, if you want to play a easy version of the song, playing only the RH lines does exactly that,
 * because on most songs RH notes are for melody and LH notes are for bass.
 * 
 * eg. from VirtualPiano.net:
 * ORDER OF LETTERS | HOW THEY’RE PLAYED
 * -----------------|-------------------------------------------------
 * [asdf]           | Play notes together simultaneously
 * [a s d f]        | Play the sequence at fastest possible speed
 * asdf             | Play notes one after the other quickly
 * a s d f          | Play each note after a short pause
 * [as] [df]        | Play “as” together, short pause, then “df” together
 * as|df            | Pause for “|”
 * as| df           | Long pause for “|” with one extra space
 * as | df          | Longer pause for “|” with 2 extra spaces
 * as| |df          | Longest pause for 2 “|” with an extra space
 * Paragraph Break  | Extended pause
 * 
 * Inferences
 *   - space is a pause denotation
 *   - pipe is a long pause
 *   - braces are like chords
 *
 * @todo vk() needs to know the long pressed keys | done for notes but the chords.
 * @todo Writing comments for vk() notations is currently impossible, due to # in use.
 * @todo Move *fr variable outside of the loop | reuse from parent.
 */
int XPTBuilder::parseas_vk(const QString &userNotations, QString &xml)
{
	QString un = userNotations;
	un.replace("\r", "\n");
	un.replace("\n\n", "\n");
	QStringList input = un.split(""); // , Qt::KeepEmptyParts);

	QStringList western = {};
    for (const QString& column : input)
	{
		if(column.contains("["))
		{
			western.append("[");
		}
		else if(column.contains("]"))
		{
			// @todo Attached symbols won't load. eg. --]
			western.append("]");
		}
		else if(column == " " || column == "\n")
		{
			western.append("x");
		}
		else if (column == "|")
		{
			// longer silence
			western.append("x");
			western.append("x");
		}
		else if(column == "-")
		{
			// to elongate chords or notes
			western.append("-");
		}
		else
		{
			for(int i=0; i<this->replaces->size(); ++i)
			{
				if(this->replaces->at(i)->find == column)
				{
					western.append(this->replaces->at(i)->replaceWith);
					break;
				}
			}
		}
	}

	return this->parseas_western(western.join(" "), xml);
}


/**
 * This replacement list is manually ordered. DO NOT CHANGE.
 * It converts a basic SARGAM into western notes.
 * Discards other characters used to format the SARGAMs,
 */
void XPTBuilder::replace_oddities_in_notes(QString &notes)
{
	notes.replace("º", "*");
	notes.replace("™", "#");
	// notes.replace("‘", replace = SpecialKeys.LOWER_OCTAVE_NOTATION }); // appears in front

	// common symbols
	notes.replace("°", "*");
	//notes.replace("*", replace = sk.HIGHER_OCTAVE_NOTATION }); // not necessary | same destination

	// super oddities, rather used in octave identification
	notes.replace("`", "#"); // sharp/flat
	notes.replace("'", "#"); // sharp/flat
	notes.replace("’", "#"); // sharp/flat

	// continuation mark
	notes.replace("_", "-");
	notes.replace("-", "-");
	notes.replace("—", "-");
	notes.replace("~", "-");

	// division / bar separator
	notes.replace("/", "|");
	notes.replace("।", "|");
	notes.replace("|", "|");
	notes.replace("\\", "|");

	// to sargam - single letter
	notes.replace("h", ""); // Dha => h part
	notes.replace("a", "");
	notes.replace("A", "");
	notes.replace("e", ""); // NOT a key name
	notes.replace("E", ""); // not a key name
	notes.replace("i", "");
	notes.replace("I", "");
	notes.replace("ha", ""); // dha
	notes.replace("HA", ""); // DHA
	notes.replace("h", ""); // dha
	notes.replace("H", ""); // dha

	// to sargam standards - multi letters
	notes.replace("S", "SA");
	notes.replace("s", "SA");
	notes.replace("सा", "SA");

	notes.replace("रे", "R");
	notes.replace("r", "re");
	notes.replace("R", "RE");

	notes.replace("g", "ga");
	notes.replace("G", "GA");
	notes.replace("ग", "GA");
	notes.replace("गा", "GA");

	notes.replace("m", "ma");
	notes.replace("M", "MA'");
	notes.replace("म", "MA'");

	notes.replace("p", "P");
	notes.replace("P", "PA");
	notes.replace("प", "PA");

	notes.replace("d", "dha"); // colliding with D?
	notes.replace("D", "DHA"); // colliding D?
	notes.replace("ध", "DHA"); // colliding D?

	notes.replace("n", "ni");
	notes.replace("N", "NI");
	notes.replace("नि", "NI");
	notes.replace("नी", "NI");
	//notes.replace("ऩ", "NI" });  // @todo: NI, nee
	//notes.replace("ऩ", "NI" });

	// convert to western notes
	notes.replace("SA", "C");
	notes.replace("re", "C#");
	notes.replace("RE", "D");
	notes.replace("ga", "D#");
	notes.replace("GA", "E");
	notes.replace("ma", "F");
	notes.replace("MA'", "F#");
	notes.replace("PA", "G");
	notes.replace("dha", "G#");
	notes.replace("DHA", "A");
	notes.replace("ni", "A#");
	notes.replace("NI", "B");
	notes.replace("##", "#"); // MA' => F## => F#
}


/**
 * @todo Reuse data from PianoRoll
 * Usage purpose:
 *  - To validate a user input key.
 *  - To convert a key into MIDI key number
 */
int XPTBuilder::getPianoKey(QString note)
{
	int octave = 4 + 1; // C4 is the standard root key
	octave += note.count("*");
	octave -= note.count(".");

	QString purenote = note;
	purenote.replace("*", "");
	purenote.replace(".", "");

	int pianokey = -1;
	for(int nn = 0; nn<this->notes.count(); ++nn)
	{
		if(purenote == this->notes[nn])
		{
			pianokey = 12 * octave + nn; // notenumber
			break;
		}
	}

	return pianokey; // -1 is an erraneous entry
}



/**
 * @todo Save individual XMLs too, in notations-xx.xpt.
 * For vk(), treat new line as spacer | Done in the source.
 */
int XPTBuilder::process_block(const QString block, QList<Cell> &cells, int &position)
{
	int errors = 0;

    QString _block = block;
    _block.replace("\r", "\n");

    QStringList lines = _block.split("\n");
    for(const QString& line: lines)
    {
		QString ln = line.trimmed();

        if(ln == "")
            continue;

		// comments be fitered out
        if(!ln.startsWith("#"))
        {
            errors += this->process_line(ln, cells, position);
        }
    }

	return errors;
}


int XPTBuilder::process_line(const QString line, QList<Cell> &cells, int &position)
{
	int errors = 0;
    QString oneline = line;
    oneline.replace("/", "|");
    oneline.replace("|", " ");
	
    QStringList columns = oneline.split(" ");
    for(const QString& column: columns)
    {
		QString col = column.trimmed();
        if(col!="")
        {
            errors += this->process_beatnotes(col, cells, position);
        }
    }

	return errors;
}


/**
 * One beat of a measure!
 * 
 * Multiple notes may share the time using comma.
 * eg. "C,D" - 50% time shared
 * eg. "G,R,S" - 33% time shared
 * eg. "R,S,D.,P." - 25% time shared
 */
int XPTBuilder::process_beatnotes(const QString column, QList<Cell> &cells, int &position)
{
	int errors = 0;
	int width = 48; // 16 x 4? nominator x denominator?

	if(column.contains(","))
	{
		float semilength = width / (column.count(",") + 1);
		for(const QString& tone: column.split(","))
		{
			errors += this->process_tone(tone, cells, semilength, position);
		}
	}
	else
	{
		errors += this->process_tone(column, cells, width, position);
	}

	return errors;
}


/**
 * At this point, you receive a particularly pitched tone.
 */
int XPTBuilder::process_tone(const QString tone, QList<Cell> &cells, float length, int &position)
{
	int errors = 0;
	
	if(tone == "x" || tone == "X")
	{
		// Silence marker. Just jump to another placement.
		position += length;
	}	
	else if(tone == "-")
	{
		if(this->chord_processing)
		{
			// elongate each note of the chords
			for(Cell &cell: this->chords)
			{
				cell.length += length;
			}
		}
		else if(cells.count() > 0)
		{
			cells.back().length += length;
		}
		// else
		// {
		// 	// cannot start with a continuation mark
		// }
		
		position += length;
	}
	else if(tone == "[")
	{
		// chords start
		this->chord_processing = true;
		this->chord_start_position = position; // [ q w e r t y ]
	}
	else if(tone == "]")
	{
		this->chord_processing = false;
		for(const Cell& chord: this->chords)
		{
			cells.append(chord);
		}
		
		this->chords = {}; // for next round
		position += length;
	}
	else if(this->getPianoKey(tone)!=-1)// @todo if tone_exists(tone)?
	// @todo eg. Filter E#, W#, ... kind of wrong notes.
	// @todo Currently they simply occupy place but do not appear in C4.
	// @todo Steps still counted.
	{
		// @todo Validate tone. Otherwise, they appear in C4 position.
		Cell cell = Cell();

		if(this->chord_processing)
		{
			// @todo Make it posible to control length of a chord.
			cell.position = this->chord_start_position;
			cell.length = 48; // length; // @todo Fix hard coded value | or, share from above
			cell.notation = tone;
			this->chords.append(cell);
		}
		else
		{
			cell.position = position;
			cell.length = length;
			cell.notation = tone;
			cells.append(cell);
		
			position += length;
		}
	}
	else
	{
		errors = 1;
	}

	return errors;
}


class MelodyEditor
{
	public:
		QList<NotationsSystem> NotationsSystems;

		MelodyEditor();
		~MelodyEditor();
		QWidget * startEditing(MidiClip * m_midiClip); // client will .show() the widget

	private:
		QPlainTextEdit * editor = new QPlainTextEdit(); // user input area

	private slots:
		int replaceMIDIClip(MidiClip * m_midiClip);
};


MelodyEditor::MelodyEditor()
{
	/**
	 * Parser function prefixed as:
	 * - parseas_sargam(); ...
	 * - parseas_western(); ...
	 * - parseas_vk(); ...
	 * 
	 * Has a possibility to add other systems as well.
	 * eg. German
	 * 
	 * @see XPTBuilder::build();
	*/
	this->NotationsSystems = {
		NotationsSystem("western", "Western"),
		NotationsSystem("sargam",  "SARGAM"),
		NotationsSystem("vk",      "Virtual Keyboard"), // virtual piano
	};
}


MelodyEditor::~MelodyEditor()
{
}



int MelodyEditor::replaceMIDIClip(MidiClip * m_midiClip)
{
	/**
	 * Determine which notation system was selected
	 * in the radio buttons
	 */
	int selected = 0;
	for(int i=0; i<NotationsSystems.count(); ++i)
	{
		if(NotationsSystems[i].radio->isChecked() == true)
		{
			selected = i;
			break;
		}
	}

	// Do not process too long melodies. Saves CPU.
	// Prevents accidental freezing, or data loss.
	QString userNotations = this->editor->toPlainText();
	if(userNotations.count() > 20000)
	{
		// 20 kb size max
		// 20k/4 ==> 2000 to 5000 measures - which is nearly 1 hour long play time, @160 bpm.
		userNotations = userNotations.left(20000);
		QMessageBox::information(nullptr, "Input Error", "Input was too large to process. Trimmed.");
		// return; // if...
	}

	XPTBuilder * xb = new XPTBuilder();
	NotationsSystem inputType = NotationsSystems[selected];

	// @todo Select parser DYNAMICALLY, using reflection or invoking object method from string value.
	int errors = 0;
	QString xpt_content = "";
	if(inputType.parser == "sargam") { errors = xb->parseas_sargam(userNotations, xpt_content); }
	else if(inputType.parser == "western") { errors = xb->parseas_western(userNotations, xpt_content); }
	else if(inputType.parser == "vk") { errors = xb->parseas_vk(userNotations, xpt_content); }
	else { errors = xb->parseas_default(userNotations, xpt_content); };

	if(xpt_content.length()>0) // Do not empty the melody in case of total silence.
	{
		QDomDocument *root = new QDomDocument();
		root->setContent(xpt_content);
		QDomElement xml = root->documentElement(); // <midiclip/>
		// @todo | optional: Precheck the length of child nodes, and process only if multiple nodes found.
		// Do NOT replace with emtpy notes.
		if(xml.hasChildNodes()) // <note/>[]
		{
			TimePos pos = m_midiClip->startPosition(); // Backup position in timeline.
			m_midiClip->loadSettings(xml);
			m_midiClip->movePosition(pos);
		}
		else
		{
			// warn: notes not found
		}
	}

	return errors; // @todo numer of keys that were NOT parsed
}


/**
 * Bring up the MelodyEditor GUI Window.
 */
QWidget * MelodyEditor::startEditing(MidiClip * m_midiClip)
{
	// QString notations_filename = "notations.txt";	
	QString notations_plain_text = ""; // file get contents from temporary file
	if(notations_plain_text.length() == 0)
	{
		/**
		 * Set default notaions or documentation hints.
		 */
		notations_plain_text = "# @see documenation for details.\r\n"
			"#\r\n"
			"# Line starting with a hash ( # ) is a comment.\r\n"
			"# Allowed Western notes: C D E F G A B C# D# F# G# A#.\r\n"
			"# Allowed SARGAM notes: S r R g G m M P d D n N.\r\n"
			"# Allowed Virtual Piano Keys: alphabets, numerals.\r\n"
			"#   - eg. [qwe]rty|uiop"
			"#   - [SPACE] character for silece in VK.\r\n"
			"#\r\n"
			"# Allowed symbols:\r\n"
			"#     x : Silence marker in the beat.\r\n"
			"#     - : Continuation of the last note.\r\n"
			"#     , : To split notes within a beat.\r\n"
			"# [ & ] : Chorded keys within braces.\r\n"
			"#       : Elongate chord with hyphens.\r\n" // @todo Half time for chords are not possible.
			"#     * : Higher octave mark.\r\n"
			"#     . : Lower octave mark."
		;
	}
	
	QPlainTextEdit *editor = this->editor;
	editor->setPlainText(notations_plain_text);

	QPushButton *um = new QPushButton();
	um->setText("Replace MIDI Clip");
	QObject::connect(um, &QPushButton::clicked, [this, m_midiClip]()
		{
			int errors = this->replaceMIDIClip(m_midiClip);
			if(errors > 0)
			{
				QMessageBox::information(nullptr, "Invalid note detected", QString("Input had %1 errors!").arg(errors));
			}
		}
	);

	QHBoxLayout * inputs = new QHBoxLayout();
	for(int i=0; i<NotationsSystems.count(); ++i)
	{
		NotationsSystems[i].radio = new QRadioButton(NotationsSystems[i].name);
		inputs->addWidget(NotationsSystems[i].radio);
	}
	
	NotationsSystems[0].radio->setChecked(true); // defaults to western | sargam; as in the order of entry

	QVBoxLayout * layout = new QVBoxLayout();
	layout->addWidget(this->editor);
	layout->addLayout(inputs);
	layout->addWidget(um);

	QWidget * w = new QWidget(nullptr);
	w->setWindowTitle("Text Based Melody Editor"); // @todo Apply title of new clip name
	w->setFixedSize(450, 300);
	w->setWindowFlags(w->windowFlags() | Qt::WindowStaysOnTopHint); // always on top
	w->setWindowFlags(w->windowFlags() & ~Qt::WindowMinimizeButtonHint); // hide minimize button
	// | Qt::X11BypassWindowManagerHint
	w->setWindowModality(Qt::WindowModal);
	w->move(200, 200); // @todo Handle more screens!
	w->setLayout(layout);
	// connect(...) // called by client
	// w->show(); // called by client

	return w;
}


} // namespace lmms::gui

#endif // LMMS_GUI_MELODYEDITOR_H