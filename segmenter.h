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

class Segmenter
{
	public:
		Segmenter(Model *ikenlm, MaxentModel *imaxent_model, double ialpha, string &input_sen);
		string decode();
	private:
		vector<Cand> expand(const Cand &cand,vector<double> &maxent_scores);
		void add_to_new(const vector<Cand> &candlist);
		bool check_is_history_same(const Cand &cand0, const Cand &cand1);

		void Str_to_char_vec(vector<string> &cv, string &s, const string &encoding);
		void load_chartype();
		string char2meta(const string &mychar);
		string get_output(const string &tags);
		vector<string> get_features();

	private:
		Model *kenlm;
		MaxentModel *maxent_model;
		size_t NGRAM;
		double alpha;
		set<string> fnchar;
		set<string> cnchar;
		map<char,vector<char> > validtagtable;
		vector<string> char_vec;
		vector<string> meta_char_vec;
		vector<Cand> candlist_old;
		vector<Cand> candlist_new;
		size_t cur_pos;
};
