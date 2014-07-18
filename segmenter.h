#include "stdafx.h"
#include "dict.h"
#include "maxent.h"
#include "lm/model.hh"
using namespace lm::ngram;

struct Resources
{
	Model *kenlm;
	Model *kenflm;
	MaxentModel *maxent_model;
	Dict *dict;
	CharType *char_type;
};

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

class Segmenter
{
	public:
		Segmenter(Resources &resources, string &input_sen);
		string decode();
	private:
		vector<Cand> expand(const Cand &cand,vector<double> &maxent_scores);
		void add_to_new(const vector<Cand> &candlist);
		bool check_is_history_same(const Cand &cand0, const Cand &cand1);

		string get_output(const string &tags);
		vector<string> get_features();
		void fill_dict_info();
		void get_raw_ambiguity_status();
		void get_final_ambiguity_status();
		void get_matching_indicator_and_lt0();

	private:
		Model *kenlm;
		MaxentModel *maxent_model;
		Model *kenflm;
		Dict *dict;
		CharType *char_type;
		size_t NGRAM;
		map<char,vector<char> > validtagtable;
		map<char,int> tag2sub;
		vector<string> char_vec;
		vector<string> meta_char_vec;
		vector<lm::WordIndex> index_vec;

		vector<vector<pair<int,int> > > matched_words_vec;
		vector<int> len_vec;
		vector<char> dict_tag_vec;
		vector<bool> included_ambiguity_vec;
		vector<bool> crossed_ambiguity_vec;
		vector<char> ambiguity_status_vec;
		vector<vector<char> > matching_indicator_vec;

		vector<vector<double> > maxent_scores_vec;

		vector<Cand> candlist_old;
		vector<Cand> candlist_new;
		size_t cur_pos;
		size_t sen_len;
};
