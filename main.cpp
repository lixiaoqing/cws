#include "stdafx.h"
#include "segmenter.h"

void seg_file(Model *kenlm, MaxentModel *maxent_model, double alpha, string &inputfile, string &outputfile)
{
	ifstream fin;
	fin.open(inputfile.c_str());
	if (!fin.is_open())
	{
		cerr<<"Fail to open input file!"<<endl;
		return;
	}

	ofstream fout;
	fout.open(outputfile.c_str());
	string input_sen;
	while(getline(fin,input_sen))
	{
		Segmenter segmenter(kenlm,maxent_model,alpha,input_sen);
		string output_sen = segmenter.decode();
		fout<<output_sen<<endl;
	}
	fout.close();
	fin.close();
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
	string maxent_file = "model/memodel_chartype_" + corpus_name;
	Model kenlm(lm_file.c_str());
	MaxentModel maxent_model(maxent_file);

	end = clock();
	cout<<"It takes "<<(double)(end-begin)/CLOCKS_PER_SEC<<" seconds to load model.\n";
	begin = clock();

	seg_file(&kenlm,&maxent_model,my_alpha,inputfile,outputfile);

	end = clock();
	cout<<"It takes "<<(double)(end-begin)/CLOCKS_PER_SEC<<" seconds to segment the file.\n";

	return 0;
}
