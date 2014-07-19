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
		int find_longest_match(vector<string> &char_vec,size_t pos);
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

class AVfeature
{
	public:
		AVfeature(){load_av();};
		string get_lav(const string &ngram);
		string get_rav(const string &ngram);
	private:
		void load_av();
		unordered_map<string,string> lav;
		unordered_map<string,string> rav;
};
