#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <stdio.h>
#include <ctype.h>
#include "XMLFix.h"

// #include "../common/ustring.h"

using std::cout;
using std::string;
using namespace std;

int  comCount = 0; // Track number of open/closed comments.
bool inUIButton = false; // Used to fix </UIButton> tags that are never opened.


ifstream getInputStream(std::string &fileName) {
	// Then open the actual files. (converted to CStrings so fstream won't complain).
	ifstream readFile(fileName.c_str(), ios::in);
	if (!readFile.is_open()) { // We check twice so that we don't create files if garbage is passed in.
		system("pause");
		cout << "Error opening files.\n";
		throw exception("Error opening files.");
	}
	return readFile;
}

ofstream getOutputStream(std::string &fileName) {
	ofstream writeFile(fileName.c_str(), ios::out | ios::trunc);
	if (!writeFile.is_open()) {
		system("pause");
		cout << "Error opening files.\n";
		throw exception("Error opening files.");
	}
	return writeFile;
}

string getOutputFileName(std::string inputFileName) {
	std::string newFileName = inputFileName;
	size_t      perLoc = inputFileName.find("."); // Location of period in the file name.
												  // There is a period
	if (perLoc != std::string::npos) // Add Fixed before the extension
		newFileName.insert(perLoc, "Fixed");
	else // Add Fixed at the end of the file
		newFileName = newFileName + "Fixed";
	return newFileName;
}

void checkInputFilename(int argc) {
	if (argc != 2) {
		system("pause");
		cout << "Please specify an xml file to parse.\n";
		throw exception("Please specify an xml file to parse.\n"
			"a file name <fileName>Fixed will be created.\n"); // TODO: update this when we no longer create new files. Double-check, but I believe we want to modify the original file, not create a new one. But double check before we change THAT particular part of the implementation.
	}
}

void loadAndRepairXMLTag(ifstream &readFile, ofstream &writeFile) {
	std::string line;
	if (getline(readFile, line)) {
		while (line.empty())
			getline(readFile, line);
		if (trim(line).find("<?xml") == 0) // If we start with an XML formatter
			writeFile << fixXMLTag(line) << "\n";
		else {
			system("pause");
			cout << "Improper XML file.\n";
			throw exception("Improper XML file.\n");
		}
	}
	else {
		system("pause");
		cout << "Error reading file\n";
		throw exception("Error reading file\n");
	}
}

std::string fixFontFamilyNoSpace(const std::string &line) {
	size_t pos = line.find("fontfamily");
	if (pos == string::npos)
		return line;

	string result = line.substr(0, pos);
	result += ' ';
	result += line.substr(pos, line.size() - pos);
	return result;
}



int main(int argc, char *argv[]) {

	checkInputFilename(argc);

	std::string oldFileName = argv[1];
	std::string newFileName = getOutputFileName(oldFileName);

	ifstream readFile = getInputStream(oldFileName);
	ofstream writeFile = getOutputStream(newFileName);

	loadAndRepairXMLTag(readFile, writeFile);

	// Then insert the root element.
	writeFile << "<Root>\n";

	std::string line; // Each line of XML we parse
	while (getline(readFile, line)) {
		if (line.empty())
			continue;

		checkBeginComment(line);
		writeFile << parseLine(line) << "\n";
		checkEndComment(line);
	}
	writeFile << "</Root>\n";
	readFile.close();
	writeFile.close();
	// system("pause");
	return 0;
}

bool isSpace(char sign) {
	// return isspace(sign);
	std::string whitespace = " \t\n\v\f\r";
	return whitespace.find(sign) != std::string::npos;
}

std::string parseComment(std::string line) {
	line = fixCopyright(line); // Does this need to run EVERY line? //Worst case improvement, we could have a global variable for whether or not we've found it, and just run it once per file, on the assumption that it will only appear once per file.
	line = doubleDashFix(line);
	return line;
}

std::string parseSource(std::string line) {
	line = fixFontFamilyNoSpace(line);
	line = fixUnclosedNodes(line);
	// line = escapeSpacedStrings(line, false);//Fix problematic strings (with spaces or special characters)
	line = fixOpenQuotes(line); // It's imperative that this run before
	line = fixMismatchedParen(line);
	// the copyright line, or not on it at all. Could update to ignore
	// Comments.
	line = escapeInnerQuotes(line);
	// line = quotedCloseFix(line);
	line = tripleQuoteFix(line);
	// line = escapeSpacedStrings(line, true);//Restore the problematic strings to their proper values.
	return line;
}

/**Read and fix any line of XML that is passed in,
* Returns that fixed line.
*/
std::string parseLine(std::string line) {

	if (comCount > 0) {
		if ((line.find("-->") != std::string::npos) || (line.find("<!--") != std::string::npos))
			line = extractComment(line);
		else
			line = parseComment(line);
	}
	else
		line = parseSource(line);


	return line;
}

/**
* Removes copyright sign, as it is invalid
* Unicode that xmllint doesn't like.
* This probably doesn't need to run on every
* Line.
*/
std::string fixCopyright(std::string line) {
	// If this is the copyright line, remove the unicode.
	if (line.find("Copyright") != std::string::npos) {
		if (!comCount)
			return "<!-- Copyright 2006 Obsidian Entertainment, Inc. -->";
		else // If we're in a comment, don't add a new one.
			return "Copyright 2006 Obsidian Entertainment, Inc.";
		// This may not be a perfect match (in the else case), but it gets the point across.
	}
	return line;
}

/**Corrects improper opening XML tags.
* An improper XML tag has <xml instead
* Of <?xml.
* Also changes references to NWN2UI to
* XML so xml-lint reads it properly.
* Returns the unmodified line with the
* Proper opening XML tag.
*/
std::string fixXMLTag(std::string line) {
	// Let's ensure we close this properly.
	if (line.find("<?xml") != string::npos) {
		line = trim(line);
		if (line.at(line.length() - 2) != '?')
			line.insert(line.length() - 1, "?");
		// NWN2UI is not a supported format. changing it to xml appears to work.
		if (line.find("encoding=\"NWN2UI\"") != std::string::npos)
			return "<?xml version=\"1.0\" encoding=\"utf-8\"?>";
	}
	return line;
}

/**
* If there is a close node without an open node
* This will delete it. Right now it only works
* If there is a closed UIButton without an open
* UIButton.
*/
std::string fixUnclosedNodes(std::string line) {
	size_t pos = line.find("<UIButton");
	// Open node
	if (pos != string::npos)
		inUIButton = true;
	pos = line.find("</UIButton>");
	// Close node
	if (pos != string::npos) {
		// If we aren't in a node, delete the close node.
		if (!inUIButton)
			line.replace(pos, 11, "");
		inUIButton = false;
	}
	return line;
}


/**
* Finds and escapes quotes in an element,
* Returns a fixed line.
* The only time we're seeing faulty quotes is
* In the context open("FooBar"), so that's the only
* Case we look for right now.
*/

inline std::string::iterator next(std::string::iterator &it) {
	std::string::iterator res = it;
	return ++res;
}

string charToString(char c) {
	string res;
	res += c;
	return res;
}

bool contain(std::string set, int value) {
	for (char sign : set)
		if (sign == value)
			return true;

	return false;
}

bool isInnerQuote(string &output, string::iterator it, std::string &line) {
	return contain("(,", output.back()) || (next(it) != line.end() && contain("),", *next(it)));
}

std::string escapeInnerQuotes(std::string line) {
	string output;
	for (string::iterator it = line.begin(); it != line.end(); ++it) {
		if ((*it == '"') && isInnerQuote(output, it, line))
			output += "&quot;";
		else
			output.push_back(*it);
	}
	return output;
}

/**
* Counts the number of times a character, find,
* Appears in a string, line, and returns that
* Number. //TODO: can we replace this with std::count?
*/
int countOccurances(std::string line, char find) {
	int count = 0;
	for (size_t i = 0; i < line.length(); i++) {
		if (line[i] == find)
			count++;
	}
	return count;
}

// Adds a closing paren if a line is missing such a thing.
std::string fixMismatchedParen(std::string line) {
	std::string result;

	int  openParenNo = 0;
	bool openQuote = false;
	for (auto &it : line) {
		if (it == '"')
			openQuote = !openQuote;

		if (!openQuote)
			switch (it) {
			case ('('):
				++openParenNo;
				break;

			case (')'):
				--openParenNo;
				break;

			case ('>'):
				result += std::string(openParenNo, ')');
				break;
			}

		result += it;
	}

	return result;
}

int getNextNospaceIndex(const std::string &line, size_t index) {
	index++; // finding should be start from next index
	while (index < line.size() && isSpace(line.at(index)))
		index++;
	return index;
}

bool containAt(std::string line, size_t index, std::string patern) {
	if (index + patern.length() > line.length())
		return false;
	size_t patInd = 0;
	while (patInd < patern.length() && line.at(index) == patern.at(patInd)) {
		patInd++;
		index++;
	}
	if (patInd == patern.length())
		return true;
	return false;
}

bool isEndOfVariableAsLessThan(std::string &line, size_t index) {
	return line.at(index) == '>' && ((line.size() == index + 1) || isSpace(line[index + 1]) || (line[index + 1] == '<'));
}

bool isEndOfVariable(std::string &line, size_t index) {
	if (isSpace(line.at(index)) || containAt(line, index, "/>") || isEndOfVariableAsLessThan(line, index)) {
		size_t nextVarIdx = line.find('=', index + 1);
		if (nextVarIdx == string::npos) {
			nextVarIdx = line.find('>', index + 1);
			if (nextVarIdx == string::npos)
				return true;
		}

		int  numberOfVars = 0;
		bool inVar = false;
		while (--nextVarIdx >= index) {
			if (inVar && isSpace(line[nextVarIdx])) {
				inVar = false;
				numberOfVars++;
			}
			else if (!inVar && !isSpace(line[nextVarIdx]))
				inVar = true;
		}
		if (numberOfVars > 1)
			return false;
		else
			return true;
	}
	else
		return false;
}

std::string extractCommentFromBegin(std::string &line) {
	size_t      commentEnd = line.find("-->");
	size_t      commentBegin = line.find("<!--");
	std::string comment, result;
	if ((commentEnd != std::string::npos) && !((commentBegin != std::string::npos) && (commentEnd > commentBegin))) {
		comment = line.substr(0, commentEnd + 3);
		line = line.substr(commentEnd + 3, line.size() - comment.size());
	}
	if (!comment.empty())
		comment = parseComment(comment);

	return comment;
}

std::string extractComment(std::string line) {

	std::string result;
	if (line.find("-->") != std::string::npos)
		result = extractCommentFromBegin(line);

	size_t      commentEnd = line.find("-->");
	size_t      commentBegin = line.find("<!--");
	std::string comment, source;

	commentBegin = line.find("<!--");
	if (commentBegin != std::string::npos) {
		commentEnd = line.find("-->");
		if (commentEnd != std::string::npos) {
			comment = line.substr(commentBegin, commentEnd + 3 - commentBegin);
			source = line.substr(0, commentBegin);
			if (!trim(source).empty())
				result += parseSource(source);
			if (!comment.empty())
				result += parseComment(comment);
			line = line.substr(commentEnd + 3, line.size() - 2 - commentEnd);
			if (!trim(line).empty()) {
				result += extractComment(line);
				line = "";
			}
		}
		else {
			comment = line.substr(commentBegin, line.size() - commentBegin);
			source = line.substr(0, commentBegin);
			if (!trim(source).empty())
				result += parseSource(source);
			if (!comment.empty())
				result += parseComment(comment);
			line = "";
		}
	}
	if (!trim(line).empty())
		result += parseSource(line);
	return result;
}

std::string fixOpenQuotes(std::string line) {
	bool isInQuote = false;
	// if (comCount > 0 && (line.find("<!--") != std::string::npos || line.find("-->") != std::string::npos))

	for (size_t index = 0; index < line.length(); index++) {
		if (line.at(index) == '=') {
			index = getNextNospaceIndex(line, index);
			if (line.at(index) != '"')
				line.insert(index, "\"");
			isInQuote = true;
		}
		else if (isInQuote && isEndOfVariable(line, index)) {
			if (line.at(index - 1) != '"')
				line.insert(index, "\"");
			isInQuote = false;
		}
	}
	if (isInQuote && (*line.rbegin() != '"'))
		line.push_back('"');
	return line;
}

/**
* If a close brace exists (not a comment),
* there isn't a close quote, AND we have an
* odd number of quotes.
*/
std::string fixUnevenQuotes(std::string line) {
	size_t closeBrace = line.find("/>");
	// We don't have a close quote before our close brace
	// Sometimes there is a space after a quote
	if ((closeBrace != string::npos) && (closeBrace > 0) &&
		((line.at(closeBrace - 1) != '\"') || (line.at(closeBrace - 2) != '\"')) &&
		countOccurances(line, '"') % 2)
		line.insert(closeBrace, "\"");
	return line;
}

/**
* After all of this, if we can iterate through a string
* And find a quote followed by a whitespace character, insert a quote.
* Preconditions are such that this should never occur naturally at this
* Point in the code, and if we do end up adding too many, it will be
* Removed in a later function (such as fixTripleQuote())
*/
std::string fixUnclosedQuote(std::string line) {
	line += '\n';
	bool   inQuote = false; // Tracks if we are inside a quote
	size_t end = line.length();
	for (size_t i = 0; i < end; i++) {
		if (!inQuote) {
			if (line[i] == '"')
				inQuote = true;
		}
		else {
			if (line[i] == '"') { // Inquote is true, we're in a quoted part.
				inQuote = false; // This is a close quote
								 // A close quote should be followed by a space.
				if ((i + 1 != end) && (line.at(i + 1) != ' ') && (line.at(i + 1) != '/') && (line.at(i + 1) != '"')) {
					line.insert(i + 1, " ");
					i++;
					end++;
				}
			}
			else if (isSpace(line[i])) { // We can't check for just a space,
										 // because files sometimes also contain newlines.
				line.insert(i, "\"");
				i++;
				end++;
				inQuote = false;
			}
		}
	}
	line.pop_back();
	return line;
}

/**
* Another close brace fix. If we're in a quote and we don't
* have a close quote and we see a />, we add a close quote.
*/
std::string fixCloseBraceQuote(std::string line) {
	bool   inQuote = false;
	size_t end = line.length();
	for (size_t i = 0; i < end; i++) {
		if (!inQuote) {
			if (line[i] == '"')
				inQuote = true;
		}
		else {
			size_t pos = line.find("/>");
			if (line[i] == '"') // Inquote is true, we're in a quoted part.
				inQuote = false;
			else if (pos != std::string::npos) {
				if (line.at(pos - 1) != '"') {
					line.insert(pos, "\"");
					break;
				}
			}
		}
	}
	return line;
}

/**
* If there are any -- inside of a comment,
* This will remove them and replace it with
* A single dash. Otherwise this breaks
* Compatibility.
*/

bool isPrefix(string::iterator textIt, const string::iterator &textEndIt, const string &supposedPrefix) {
	for (const auto &it : supposedPrefix) {
		if ((textIt == textEndIt) || (*textIt != it))
			return false;
		++textIt;
	}

	return true;
}

/**
* If there are three consecutive quotes,
* Replace with one quote.
* Let's be honest, this can only happen as a
* Result of other methods, and this is a kludgy fix.
* Note that this will only find one per line.
* This will also remove double quotes that are not
* Intended to be in the XML. name="" is the only
* Legitimate appearance of "" in the NWN xml code.
* Returns the modified line, without double or
* Triple quotes.
*/
std::string tripleQuoteFix(std::string line) {
	size_t pos = line.find("\"\"\"");
	if (pos != std::string::npos)
		line.erase(pos, 2); // Remove two quotes.

							// Might as well escape "" as well, while we're at it.
	if (line.find("name=\"\"") == std::string::npos) { // If the line doesn't contain name=""
		pos = line.find("\"\"");
		if (pos != std::string::npos)
			line.erase(pos, 2); // Remove one quote.
	}
	return line;
}

/**
* Some lines contain problematic phrases. (phrases that include
* String literals with spaces or the > or / characters).
* If left untouched, other functions will destroy these strings
* Instead of fixing them.
* Returns a safe string (devoid of problematic phrases) if undo is false, or
* The original string, with problematic phrases restored if undo is true.
*/
std::string escapeSpacedStrings(std::string line, bool undo) {
	// Just used as containers.
	// Might be an easier/cleaner way to do this, perhaps with some sort of map instead of two arrays.
	string switchWordsFrom[] = {
		"portrait frame",           "0 / 0 MB",     "->",  ">>",
		"capturemouseevents=false", "Speaker Name", " = ", "Player Chat"
	};
	string switchWordsTo[] = {
		"portrait_frame", "0_/_0_MB",                  "ReplaceMe1",
		"ReplaceMe2",     "capturemouseevents=false ", "Speaker_Name","=", "Player_Chat"
	};

	// The arrays we actually reference
	string *fromTemp = switchWordsFrom;
	string *toTemp = switchWordsTo;
	// Swap
	// No need to switch the first time, but The second time we
	// Call this, we want to switch from safe to original strings
	if (undo) {
		// Native array swap wasn't introduced until c++ 2011
		// So we do this with pointers.
		string *swapTemp = fromTemp;
		fromTemp = toTemp;
		toTemp = swapTemp;
	}
	// Number of elements in the array.
	int length = sizeof(switchWordsFrom) / sizeof(switchWordsFrom[0]);
	// Do the actual replacement inline.
	for (int i = 0; i < length; i++) {
		size_t pos = line.find(*(fromTemp + i));
		if (pos != std::string::npos)
			line.replace(pos, (fromTemp + i)->length(), *(toTemp + i));
	}
	return line;
}


bool isCommentBegin(std::string line, basic_string<char, char_traits<char>, allocator<char> >::size_type occurence) {
	return occurence > 0 && line[occurence - 1] == '!';
}

bool isCommentEnd(std::string line, basic_string<char, char_traits<char>, allocator<char> >::size_type occurence) {
	const size_t doubleDashLength = 2;
	return occurence + doubleDashLength < line.size() && line[occurence + doubleDashLength] == '>';
}

std::string doubleDashFix(std::string line) {
	if (comCount > 0) {
		for (auto occurence = line.find("--");
			occurence < line.size();
			occurence = line.find("--", occurence + 1)) {
			if (!isCommentBegin(line, occurence) && !isCommentEnd(line, occurence)) {
				string::iterator occurenceIt = std::next(line.begin(), occurence);
				line.erase(occurence, 2);
			}
		}
	}
	return line;
}

/**
* Track number of open and closed comments
* Used for tracking copyright.
*/
void checkBeginComment(std::string line) {
	if (line.find("<!--") != std::string::npos)
		comCount++;
}

void checkEndComment(std::string line) {
	if (line.find("-->") != std::string::npos)
		comCount--;
}

/**
* Remove leading and trailing whitespace.
* Returns line without leading and trailing
* Spaces.
*/
std::string trim(std::string line) {
	string whitespace = " \t\n\v\f\r";
	size_t lineBegin = line.find_first_not_of(whitespace);
	if (lineBegin == std::string::npos)
		return ""; // empty string
	int lineEnd = line.find_last_not_of(whitespace);
	int lineRange = lineEnd - lineBegin + 1;
	return line.substr(lineBegin, lineRange);
}
