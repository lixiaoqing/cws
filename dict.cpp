#include "dict.h"

void Dict::load_dict()
{
	fstream fin;
	fin.open("model/dict");
	if (!fin.is_open())
	{
		cerr<<"Fail to open dict file!\n";
		return;
	}
	string w;
	while(fin>>w)
	{
		vector<string> char_vec;
		Str_to_char_vec(char_vec,w,"gbk");
		add_word(char_vec);
	}
	fin.close();
	cout<<"load dict over\n";
}

void Dict::add_word(vector<string> &char_vec)
{
	TrieNode* current = root;
	for (const auto &c : char_vec)
	{
		auto it = current->char_to_children_map.find(c);
		if ( it != current->char_to_children_map.end() )
		{
			current = it->second;
		}
		else
		{
			TrieNode* tmp = new TrieNode();
			current->char_to_children_map.insert(make_pair(c,tmp));
			current = tmp;
		}
	}
	current->flag = true;
}

int Dict::find_longest_match(vector<string> &char_vec,size_t pos)
{
	TrieNode* current = root;
	int len = 1;
	for (size_t i=pos;i<char_vec.size();i++)
	{
		auto it = current->char_to_children_map.find(char_vec.at(i));
		if (it != current->char_to_children_map.end())
		{
			current = it->second;
			if (current->flag == true)
			{
				len = i - pos + 1;
			}
		}
		else
			break;
	}
	return len;
}

void CharType::load_chartype()
{
	ifstream fin;
	fin.open("model/chartype");
	if (!fin.is_open())
	{
		cerr<<"Fail to open chartype file!\n";
		return;
	}
	vector <string> charvec;
	string line;
	getline(fin,line);
	Split(charvec,line);
	for (size_t i = 0; i < charvec.size(); i++)
		fnchar.insert(charvec.at(i));
	getline(fin,line);
	Split(charvec,line);
	for (size_t i = 0; i < charvec.size(); i++)
		cnchar.insert(charvec.at(i));
}

string CharType::char2meta(const string &mychar)
{
	if (fnchar.count(mychar))
		return "f";
	if (cnchar.count(mychar))
		return "c";
	return mychar;
}

