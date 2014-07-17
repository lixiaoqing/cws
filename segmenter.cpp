#include "segmenter.h"

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

Segmenter::Segmenter(Model *ikenlm, MaxentModel *imaxent_model, Dict *idict, string &input_sen)
{
	NGRAM = 3;
	kenlm = ikenlm;
	maxent_model = imaxent_model;
	dict = idict;
	load_chartype();
	TrimLine(input_sen);
	char_vec = {"B_1","B_0"};
	Str_to_char_vec(char_vec,input_sen,"gbk");
	char_vec.push_back("E_0");
	char_vec.push_back("E_1");
	for (auto const c : char_vec)
	{
		meta_char_vec.push_back(char2meta(c));
	}

	lt0_vec.resize(char_vec.size(),make_pair(1,'S'));
	fill_lt0_vec();
	maxent_scores_vec.resize(char_vec.size());
	for (cur_pos=2;cur_pos<char_vec.size()-2;cur_pos++)
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

void Segmenter::fill_lt0_vec()
{
	for (cur_pos=2;cur_pos<meta_char_vec.size()-2;cur_pos++)
	{
		int max_len = dict->find_longest_match(meta_char_vec,cur_pos);
		if (max_len >1)
		{
			if (lt0_vec.at(cur_pos).first < max_len)
			{
				lt0_vec.at(cur_pos).first = max_len;
				lt0_vec.at(cur_pos).second = 'B';
			}
			if (lt0_vec.at(cur_pos+max_len-1).first < max_len)
			{
				lt0_vec.at(cur_pos+max_len-1).first = max_len;
				lt0_vec.at(cur_pos+max_len-1).second = 'E';
			}
			for (size_t i=cur_pos+1;i<cur_pos+max_len-1;i++)
			{
				if (lt0_vec.at(i).first < max_len)
				{
					lt0_vec.at(i).first = max_len;
					lt0_vec.at(i).second = 'M';
				}
			}
		}
	}
}

void Segmenter::load_chartype()
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

string Segmenter::char2meta(const string &mychar)
{
	if (fnchar.count(mychar))
		return "f";
	if (cnchar.count(mychar))
		return "c";
	return mychar;
}

string Segmenter::get_output(const string &tags)
{
	string output_sen;
	for (cur_pos=2;cur_pos<char_vec.size()-2;cur_pos++)
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
	for (cur_pos=2;cur_pos<char_vec.size()-2;cur_pos++)
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
	feature_vec.push_back("10/"+to_string(lt0_vec.at(cur_pos).first)+lt0_vec.at(cur_pos).second);
	feature_vec.push_back("11/"+meta_char_vec.at(cur_pos-1)+lt0_vec.at(cur_pos).second);
	feature_vec.push_back("12/"+meta_char_vec.at(cur_pos)+lt0_vec.at(cur_pos).second);
	feature_vec.push_back("13/"+meta_char_vec.at(cur_pos+1)+lt0_vec.at(cur_pos).second);
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
		State out_state;
		const Vocabulary &vocab = kenlm->GetVocabulary();
		string ct = meta_char_vec.at(cur_pos) + "/" + e_tag;
		double lm_score = kenlm->Score(cand.lm_state, vocab.Index(ct), out_state);
		cand_new.lm_state = out_state;
		cand_new.score = cand.score + 0.4*lm_score + 0.6*maxent_score;
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

