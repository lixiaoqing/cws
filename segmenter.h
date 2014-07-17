#include "stdafx.h"
#include "maxent.h"
#include "lm/model.hh"
using namespace lm::ngram;

struct Cand
{
	string tags;
	State lm_state;
	double score;
	bool operator< (const Cand &cand_rhs) const
	{
		return (score < cand_rhs.score);
	}
};

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

class Segmenter
{
	public:
		Segmenter(Model *ikenlm, MaxentModel *imaxent_model, Dict *idict, string &input_sen);
		string decode();
	private:
		vector<Cand> expand(const Cand &cand,vector<double> &maxent_scores);
		void add_to_new(const vector<Cand> &candlist);
		bool check_is_history_same(const Cand &cand0, const Cand &cand1);

		void load_chartype();
		string char2meta(const string &mychar);
		string get_output(const string &tags);
		vector<string> get_features();
		void fill_lt0_vec();

	private:
		Model *kenlm;
		MaxentModel *maxent_model;
		Dict *dict;
		size_t NGRAM;
		set<string> fnchar;
		set<string> cnchar;
		map<char,vector<char> > validtagtable;
		vector<string> char_vec;
		vector<string> meta_char_vec;
		vector<pair<int,char> > lt0_vec;
		vector<vector<double> > maxent_scores_vec;
		vector<Cand> candlist_old;
		vector<Cand> candlist_new;
		size_t cur_pos;
};
