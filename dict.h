#include "stdafx.h"
#include "myutils.h"

struct TrieNode
{
	bool flag;
	map<string,TrieNode*> char_to_children_map;
};

class Dict
{
	public:
		Dict()
		{
			root = new TrieNode;
			load_dict();
		};
		vector<pair<int,int> > find_matched_dict_words(vector<string> &char_vec,size_t pos);
	private:
		void load_dict();
		void add_word(vector<string> &char_vec);

	private:
		TrieNode *root;
};

class CharType
{
	public:
		CharType(){load_chartype();};
		string char2meta(const string &mychar);
	private:
		void load_chartype();
		set<string> fnchar;
		set<string> cnchar;
};
