#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include <iostream>
#include <algorithm>

using namespace std;

enum Mode{csv_to_wiki, wiki_to_csv};

void output_wiki(vector<vector<string>>& table, string filename = "table.txt");
void output_csv(vector<vector<string>>& table, string filename = "table.txt");

string csv_to_wiki_transform(string text);
string wiki_to_csv_transform(string text);

vector<vector<string>> parse_csv_to_wiki(const string& input);
vector<vector<string>> parse_wiki_to_csv(const string& input);

/*
 * argv[1] is the output mode, either "wiki" or "csv"
 * argv[2] is input file
 * argv[3] is output file (defaults to table.txt)
 */
int main(int argc, char* argv[])
{
	switch(argc)
	{
		case 3:
			cout << "Outputing to table.txt" << endl;
			break;
		case 4:
			break;
		default:
			cout << "Wrong number of arguments" << endl;
			return -1;
	}
	string mode_string = argv[1];
	Mode mode;
	if(mode_string == "wiki")
		mode = Mode::csv_to_wiki;
	else if(mode_string == "csv")
		mode = Mode::wiki_to_csv;
	else
	{
		cout << argv[1] << " is not a valid mode" << endl;
		return -1;
	}
	ifstream file(argv[2]);
	if(not file.is_open())
	{
		cout << "Could not find file";
		return -1;
	}

	string input;
	getline(file, input, '\0');
	vector<vector<string>> table;
	try
	{
		if(mode == Mode::csv_to_wiki)
			table = parse_csv_to_wiki(input);
		else if(mode == Mode::wiki_to_csv)
			table = parse_wiki_to_csv(input);
	}
	catch(string booboo)
	{
		cout << booboo;
	}
	switch(mode)
	{
		case Mode::csv_to_wiki:
			if(argc == 4)
				output_wiki(table, argv[3]);
			else
				output_wiki(table);
			break;
		case Mode::wiki_to_csv:
			if(argc == 4)
				output_csv(table, argv[3]);
			else
				output_csv(table);
	}
	
	return 0;
}

/*
 * Parsing:
 * First line must be empty, to indicate number of columns - this line is discarded
 * Each cell starts and ends with a "
 * Replace @TEXT@INT with [[File:TEXT.png|INTpx]]
 * When newline is encountered within a cell, replace that with <br>
 * when the last cell of the row is concluded start a new row
 */
vector<vector<string>> parse_csv_to_wiki(const string& input)
{
	int columns = 0;
	int rows = 0;
	vector<vector<string>> table;
	string table_string = "";
	string row_string = "";
	string cell_string = "";

	regex row_regex;//This is going to get ugly
	string regex_string = "";

	if(input.find('\n') == string::npos)
		throw("Only one row/no row found");

	//count the number of columns
	while(input.at(columns++) != '\n')

	try
	{
		table_string = input.substr(input.find('"'), string::npos);
	}
	catch(out_of_range e)
	{
		throw("No \" found. You need to change your csv settings to \
			  enclose all cells with quotation marks");
	}

	//rows is the number of cells (each have 2 ") divided by columns
	rows = count(input.begin(), input.end(), '\"')/(2*(columns));

	//for two columns row_regex becomes ".*",?".*",? 
	//but uglier because C++ needs " to be a special character
	for(int i = 0; i < columns; i++)
	{
		regex_string += "\"[^]*?\",?";
	}
	row_regex = regex_string + "\n?";

	//for each row
	while(table_string.length() > 1)
	{
		smatch row_match;
		int start = 0;
		int end = -1;

		table.push_back({});

		regex_search(table_string,row_match, row_regex);
		row_string = row_match.str(0);

		//Iterate over one row
		for(int i = 0; i < columns; i++)
		{
			//more safe than just one operation i guess??
			start = row_string.find('"', end+1);
			end = row_string.find('"', start+1);

			cell_string = row_string.substr(start+1, end-1-start);

			cell_string = csv_to_wiki_transform(cell_string);
			table.back().push_back(cell_string);
		}

		regex_search(table_string, row_match, row_regex);
		table_string.replace(0, row_match.position(0) + row_match.length(0), "");
	}
	return table;
}
vector<vector<string>> parse_wiki_to_csv(const string& input)
{
	int columns = 0;
	int rows = 0;
	int row_start = 0;

	vector<vector<string>> table;
	string table_string = "";
	string row_string = "";
	string cell_string = "";

	regex row_regex;
	string row_regex_string;

	

	if(input.find('\n') == string::npos)
		throw("Only one row/no row found");
	table_string = input.substr(input.find('!'), input.find_last_not_of("}|\n "));
	//remove all occurences of "\n|-" and "^| "
	table_string = regex_replace(table_string, regex("\n\\|-|^\\| "), "");

	columns = (count(table_string.begin(),
					table_string.begin() + table_string.find('\n'),
					'!')
			   + 1) / 2;
	rows = count(table_string.begin(), table_string.end(), '\n');

	for(int i = 0; i < columns; i++)
	{
		if(i == 0)
			row_regex_string = "(?:\\| |! )";
		else
			row_regex_string += "(?: \\|\\| | !! )";
		row_regex_string += "(.*)";
	}

	row_regex = row_regex_string;

	for(int i = 0; i < rows; i++)
	{
		smatch m;

		row_string = table_string.substr(row_start, table_string.find('\n', row_start) - row_start);
		row_start = table_string.find('\n', row_start + 1) + 1;

		fstream error("error.txt");
		if(not regex_search(row_string, m, row_regex))
			error << row_regex_string << "\nfailed to match on\n" << row_string << endl;
		
		//New row
		table.push_back({});
		for(int j = 1; j <= columns; j++)
		{
			//TODO:This will output the same string over and over again
			//combine a couple of the regex string to make a regex and use that to loop should do it
			string cell_string = wiki_to_csv_transform(m.str(j));
			table.back().push_back(cell_string);
		}
	}
	return table;
}

/* 
 * Output:
 * Separate rows with "<CR>|-<CR>"
 * Replace <CR> within strings with "<br>"
 * Start each line with a vertical line and space: "| "
 * Separate cells with " || "
 */
void output_wiki(vector<vector<string>>& table, string filename)
{
	ofstream out_file(filename);
	string out_text = "{| class=\"wikitable\"\n";
	//header is a special case
	for(int i = 0; i < table[0].size(); i++)
	{
		if(i == 0)
			out_text += "! " + table[0][i];
		else
			out_text += " !! " + table[0][i];
	}
	for(int row = 1; row < table.size(); row++)
	{
		out_text += "\n|-\n";
		for(int cell = 0; cell < table[row].size(); cell++)
		{
			if(cell == 0)
				out_text += "| " + table[row][cell];
			else
				out_text += " || " + table[row][cell];
		}
	}
	out_text += "\n|}";
	out_file << out_text;
}

void output_csv(vector<vector<string>>& table, string filename)
{
	ofstream out_file(filename);
	string out_text = "";
	for(int i = 1; i < table[0].size(); i++)
		out_text += ",";

	for(auto row : table)
	{
		out_text += '\n';
		for(int j = 0; j < row.size(); j++)
		{
			if(j != 0)
				out_text += ',';
			out_text += "\"" + row[j] + "\"";
		}
	}
	out_file << out_text;
}
string csv_to_wiki_transform( string text)
{
	//While there are @ left in text
	for(int pos = text.find('@'); pos != string::npos; pos = text.find('@'))
	{
		smatch cell_match;
		regex_search(text,
					cell_match,
					regex("@(.+?)@(\\d+)"));

		string replacement_string = "[[File:" + cell_match[1].str() +
									".png|" + cell_match[2].str() + "px]]";
		text.replace(pos,
					 cell_match.position(2) + cell_match.length(2),
					 replacement_string);
	}
	//While there are newlines left in text
	for(int pos = text.find('\n'); pos != string::npos; pos = text.find('\n'))
	{
		text.replace(pos, 1, "<br>");
	}
	return text;
}

string wiki_to_csv_transform(string text)
{
	for(int pos = text.find('['); pos != string::npos; pos = text.find('['))
	{
		smatch m;
		regex_search(text,
					 m,
					 regex("\\[\\[File:(.+?)\\.png\\|(\\d+)"));
		if(m.empty())
			throw("Formating error");
		string replacement_string = "@" + m.str(1) + "@" + m.str(2);
		text.replace(pos,
					 m.position(2) + m.length(2) + 4,
					 replacement_string);
	}
	for(int pos = text.find("<br>"); pos != string::npos; pos = text.find("<br>"))
	{
		text.replace(pos, 4, "\n");
	}
	return text;
}
/*
 * Take in data
 * Parsing:
 * First line is empty, to indicate number of columns - this line is discarded
 * Each cell starts and ends with a "
 * When @ is encountered interpret as file name until next @ followed by pixel size
 * When , is encountered new cell
 * when newline is encountered outside a string new row
 *
 * Output:
 * Replace @TEXT@INT with [[File:TEXT.png|INTpx]]
 * Separate rows with "<CR>|-<CR>"
 * Replace <CR> within strings with "<br>"
 * Start each line with a vertical line and space: "| "
 * Separate cells with " || "
 */
