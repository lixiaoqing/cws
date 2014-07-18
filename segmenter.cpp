#include "segmenter.h"

Segmenter::Segmenter(Resources &resources, string &input_sen)
{
	NGRAM = 3;
	kenlm = resources.kenlm;
	kenflm = resources.kenflm;
	maxent_model = resources.maxent_model;
	dict = resources.dict;
	char_type = resources.char_type;
	TrimLine(input_sen);
	char_vec = {"B_1","B_0"};
	Str_to_char_vec(char_vec,input_sen,"gbk");
	char_vec.push_back("E_0");
	char_vec.push_back("E_1");
	const Vocabulary &flm_vocab = kenflm->GetVocabulary();
	for (auto const c : char_vec)
	{
		meta_char_vec.push_back(char_type->char2meta(c));
		index_vec.push_back(flm_vocab.Index(meta_char_vec.back()));
	}
	reverse(index_vec.begin(),index_vec.end());
	sen_len = char_vec.size();

	tag2sub['B'] = 0;
	tag2sub['M'] = 1;
	tag2sub['E'] = 2;
	tag2sub['S'] = 3;
	matched_words_vec.resize(sen_len);
	len_vec.resize(sen_len,1);
	dict_tag_vec.resize(sen_len,'S');
	included_ambiguity_vec.resize(sen_len,false);
	crossed_ambiguity_vec.resize(sen_len,false);
	ambiguity_status_vec.resize(sen_len,'N');
	matching_indicator_vec.resize(sen_len,{'N','N','N','N'});
	fill_dict_info();

	maxent_scores_vec.resize(sen_len);
	for (cur_pos=2;cur_pos<sen_len-2;cur_pos++)
	{
		vector<string> features = get_features();
		maxent_model->eval_all(maxent_scores_vec.at(cur_pos),features);
	}

	State state(kenlm->BeginSentenceState());
	Cand init_cand = {"SS",state,0.0};
	candlist_old.push_back(init_cand);

	validtagtable['B'] = {'M','E'};
	validtagtable['M'] = {'M','E'};
	validtagtable['E'] = {'S','B'};
	validtagtable['S'] = {'S','B'};
}

void Segmenter::fill_dict_info()
{
	for (cur_pos=2;cur_pos<sen_len-2;cur_pos++)
	{
		vector<pair<int,int> > matched_words = dict->find_matched_dict_words(char_vec,cur_pos);
		for (const auto &w : matched_words)
		{
			for(size_t i=w.first;i<=w.second;i++)
			{
				matched_words_vec.at(i).push_back(w);
			}
		}
	}
	for (cur_pos=2;cur_pos<sen_len-2;cur_pos++)
		get_raw_ambiguity_status();
	for (cur_pos=2;cur_pos<sen_len-2;cur_pos++)
		get_final_ambiguity_status();
	for (cur_pos=2;cur_pos<sen_len-2;cur_pos++)
		get_matching_indicator_and_lt0();
}

void Segmenter::get_raw_ambiguity_status()
{
	auto &words = matched_words_vec.at(cur_pos);
	if (words.size() <= 1)
		return;
	vector<pair<int,int> > bwords,ewords;
	for (const auto w : words)
	{
		if (w.first == cur_pos)
			bwords.push_back(w);
		if (w.second == cur_pos)
			ewords.push_back(w);
	}
	if (bwords.size()>0)
	{
		sort(bwords.begin(),bwords.end(),[](pair<int,int> &lhs,pair<int,int> &rhs){return lhs.second<rhs.second;});
		for (const auto w : words)
		{
			if (w.first < cur_pos && w.second >= bwords.at(0).second)
			{
				included_ambiguity_vec.at(cur_pos-1) = true;
				included_ambiguity_vec.at(cur_pos) = true;
			}
			if (w.first < cur_pos && w.second < bwords.at(bwords.size()-1).second)
			{
				crossed_ambiguity_vec.at(cur_pos-1) = true;
				crossed_ambiguity_vec.at(cur_pos) = true;
			}
		}
	}
	if (ewords.size()>0)
	{
		sort(ewords.begin(),ewords.end(),[](pair<int,int> &lhs,pair<int,int> &rhs){return lhs.first<rhs.first;});
		for (const auto w : words)
		{
			if (w.first <= ewords.at(ewords.size()-1).first && w.second > cur_pos)
			{
				included_ambiguity_vec.at(cur_pos) = true;
				included_ambiguity_vec.at(cur_pos+1) = true;
			}
			if (w.first > ewords.at(0).first && w.second > cur_pos)
			{
				crossed_ambiguity_vec.at(cur_pos) = true;
				crossed_ambiguity_vec.at(cur_pos+1) = true;
			}
		}
	}
}

void Segmenter::get_final_ambiguity_status()
{
	if (included_ambiguity_vec.at(cur_pos) == true && crossed_ambiguity_vec.at(cur_pos) == true)
		ambiguity_status_vec.at(cur_pos) = 'M';
	else if (included_ambiguity_vec.at(cur_pos) == true && crossed_ambiguity_vec.at(cur_pos) == false)
		ambiguity_status_vec.at(cur_pos) = 'I';
	else if (included_ambiguity_vec.at(cur_pos) == false && crossed_ambiguity_vec.at(cur_pos) == true)
		ambiguity_status_vec.at(cur_pos) = 'C';
	else
		ambiguity_status_vec.at(cur_pos) = 'N';
}

void Segmenter::get_matching_indicator_and_lt0()
{
	int max_len = 1;
	char dict_tag = 'S';
	auto &words = matched_words_vec.at(cur_pos);
	if (words.size() == 0)
		return;
	for (const auto w : words)
	{
		if (w.second - w.first + 1 > max_len)
		{
			max_len = w.second - w.first + 1;
			if (w.first == cur_pos)
				dict_tag = 'B';
			else if (w.second == cur_pos)
				dict_tag = 'E';
			else
				dict_tag = 'M';
		}
	}
	len_vec.at(cur_pos) = min(max_len,4);
	dict_tag_vec.at(cur_pos) = dict_tag;
	for (const auto w : words)
	{
		if (w.second - w.first + 1 == max_len)
		{
			if (w.first == cur_pos)
				matching_indicator_vec.at(cur_pos).at(tag2sub['B']) = 'L';
			else if (w.second == cur_pos)
				matching_indicator_vec.at(cur_pos).at(tag2sub['E']) = 'L';
			else
				matching_indicator_vec.at(cur_pos).at(tag2sub['M']) = 'L';
		}
		else
		{
			if (w.first == cur_pos && matching_indicator_vec.at(cur_pos).at(tag2sub['B']) != 'L')
				matching_indicator_vec.at(cur_pos).at(tag2sub['B']) = 'S';
			else if (w.second == cur_pos && matching_indicator_vec.at(cur_pos).at(tag2sub['E']) != 'L')
				matching_indicator_vec.at(cur_pos).at(tag2sub['E']) = 'S';
			else if (matching_indicator_vec.at(cur_pos).at(tag2sub['M']) != 'L')
				matching_indicator_vec.at(cur_pos).at(tag2sub['M']) = 'S';
		}
	}
}

string Segmenter::get_output(const string &tags)
{
	string output_sen;
	for (cur_pos=2;cur_pos<sen_len-2;cur_pos++)
	{
		if (tags.at(cur_pos) == 'B' || tags.at(cur_pos) == 'M')
			output_sen += char_vec.at(cur_pos);
		else
			output_sen += char_vec.at(cur_pos)+" ";
	}
	TrimLine(output_sen);
	return output_sen;
}

string Segmenter::decode()
{
	for (cur_pos=2;cur_pos<sen_len-2;cur_pos++)
	{
		for (const auto &e_cand : candlist_old)
		{
			vector<Cand> candvec = expand(e_cand,maxent_scores_vec.at(cur_pos));
			add_to_new(candvec);
		}
		candlist_old.swap(candlist_new);
		candlist_new.resize(0);
	}
	for (auto &cand : candlist_old)
	{
		State out_state;
		cand.score += kenlm->Score(cand.lm_state, kenlm->GetVocabulary().EndSentence(), out_state);
	}
	auto it = max_element(candlist_old.begin(),candlist_old.end());
	return get_output(it->tags);
}

vector<string> Segmenter::get_features()
{
	vector<string> feature_vec;
	feature_vec.push_back("0/"+meta_char_vec.at(cur_pos-2));
	feature_vec.push_back("1/"+meta_char_vec.at(cur_pos-1));
	feature_vec.push_back("2/"+meta_char_vec.at(cur_pos));
	feature_vec.push_back("3/"+meta_char_vec.at(cur_pos+1));
	feature_vec.push_back("4/"+meta_char_vec.at(cur_pos+2));
	feature_vec.push_back("5/"+meta_char_vec.at(cur_pos-2)+meta_char_vec.at(cur_pos-1));
	feature_vec.push_back("6/"+meta_char_vec.at(cur_pos-1)+meta_char_vec.at(cur_pos));
	feature_vec.push_back("7/"+meta_char_vec.at(cur_pos)+meta_char_vec.at(cur_pos+1));
	feature_vec.push_back("8/"+meta_char_vec.at(cur_pos+1)+meta_char_vec.at(cur_pos+2));
	feature_vec.push_back("9/"+meta_char_vec.at(cur_pos-1)+meta_char_vec.at(cur_pos+1));
	feature_vec.push_back("10/"+to_string(len_vec.at(cur_pos))+dict_tag_vec.at(cur_pos));
	feature_vec.push_back("11/"+meta_char_vec.at(cur_pos-1)+dict_tag_vec.at(cur_pos));
	feature_vec.push_back("12/"+meta_char_vec.at(cur_pos)+dict_tag_vec.at(cur_pos));
	feature_vec.push_back("13/"+meta_char_vec.at(cur_pos+1)+dict_tag_vec.at(cur_pos));
	return feature_vec;
}

vector<Cand> Segmenter::expand(const Cand &cand, vector<double> &maxent_scores)
{
	vector<Cand> candvec;
	char last_tag = cand.tags.at(cand.tags.size()-1);
	vector<char> validtagset = validtagtable[last_tag];

	for (const auto &e_tag : validtagset)
	{
		Cand cand_new;
		cand_new.tags = cand.tags;
		cand_new.tags += e_tag;
		string mytag(1,e_tag);
		double maxent_score = maxent_scores.at(maxent_model->get_tagid(mytag));

		double lm_score = 0.0, flm_score = 0.0;
		State out_state;
		const Vocabulary &vocab = kenlm->GetVocabulary();
		string ct = meta_char_vec.at(cur_pos) + "/" + e_tag;
		lm_score = kenlm->Score(cand.lm_state, vocab.Index(ct), out_state);
		cand_new.lm_state = out_state;

		string matching_indicator(1,matching_indicator_vec.at(cur_pos).at(tag2sub[e_tag]));
		State tmp_state;
		const Vocabulary &flm_vocab = kenflm->GetVocabulary();
		flm_score = kenflm->FullScoreForgotState(&index_vec.at(sen_len-1-cur_pos),&index_vec.at(sen_len+1-cur_pos),flm_vocab.Index(matching_indicator),tmp_state).prob;

		cand_new.score = cand.score + 0.5*lm_score + 0.5*flm_score + 0.5*maxent_score;
		candvec.push_back(cand_new);
	}
	return candvec;
}

bool Segmenter::check_is_history_same(const Cand &cand0, const Cand &cand1)
{
	for (size_t k=0;k<NGRAM;k++)
	{
		if (cand0.tags.at(cur_pos-k) != cand1.tags.at(cur_pos-k))
		{
			return false;
		}
	}
	return true;
}

void Segmenter::add_to_new(const vector<Cand> &candvec)
{
	for (const auto &e_cand : candvec)
	{
		bool is_history_same = false;
		for (auto &e_ori_cand : candlist_new)
		{
			is_history_same = check_is_history_same(e_cand,e_ori_cand);
			if (is_history_same == true)
			{
				if (e_cand.score > e_ori_cand.score)
				{
					e_ori_cand = e_cand;
				}
				break;
			}
		}
		if (is_history_same == false)
		{
			candlist_new.push_back(e_cand);
		}
	}
}

