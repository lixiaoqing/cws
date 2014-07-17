#include "stdafx.h"
#include "segmenter.h"

void seg_file(Model *kenlm, MaxentModel *maxent_model,Dict *dict, double alpha, string &inputfile, string &outputfile)
{
	ifstream fin;
	fin.open(inputfile.c_str());
	if (!fin.is_open())
	{
		cerr<<"Fail to open input file!"<<endl;
		return;
	}

	vector<string> input_sen;
	vector<string> output_sen;
	string line;
	while(getline(fin,line))
	{
		TrimLine(line);
		input_sen.push_back(line);
	}
	fin.close();

	output_sen.resize(input_sen.size());
//#pragma omp parallel for num_threads(4)
	for (size_t i=0;i<input_sen.size();i++)
	{
		Segmenter segmenter(kenlm,maxent_model,dict,alpha,input_sen.at(i));
		output_sen.at(i) = segmenter.decode();
	}

	ofstream fout;
	fout.open(outputfile.c_str());
	for (const auto &sen : output_sen)
	{
		fout<<sen<<endl;
	}

	fout.close();
}

int main(int argc, char* argv[])
{
	clock_t begin,end;
	begin = clock();

	const char* test_help="\
	format:\n\
	Usage : seg [options]\n\
	option  meaning\n\
	-i      input file name.\n\
	-o      output file name.\n\
	-a      the weight of generative model.\n\
	-c      corpus\n\
	-h      print this message\n\
	";
	if (argc == 1)
	{
		cerr << test_help << endl;
		return 1;
	}	
	string inputfile;
	string outputfile;
	string s_alpha;
	string corpus_name;
	for (int i=1;i<argc;++i)
	{
		string si(argv[i]);
		
		if (si=="-i") inputfile = argv[++i];
		else if (si=="-o") outputfile = argv[++i];
		else if (si=="-a") s_alpha = argv[++i];
		else if (si=="-c") corpus_name = argv[++i];
		else if (si=="-h")
		{
			cerr << test_help << endl;
			return 1;
		}
	}
		
	double my_alpha = stod(s_alpha);
	string lm_file = "model/lm_" + corpus_name;
	string maxent_file = "model/memodel_chartype_dict_" + corpus_name;
	Model kenlm(lm_file.c_str());
	MaxentModel maxent_model(maxent_file);
	Dict dict;

	end = clock();
	cout<<"It takes "<<(double)(end-begin)/CLOCKS_PER_SEC<<" seconds to load model.\n";
	begin = clock();

	seg_file(&kenlm,&maxent_model,&dict,my_alpha,inputfile,outputfile);

	end = clock();
	cout<<"It takes "<<(double)(end-begin)/CLOCKS_PER_SEC<<" seconds to segment the file.\n";

	return 0;
}
