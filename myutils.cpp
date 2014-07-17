#include "myutils.h"

void Split(vector <string> &vs, string &s)
{
	vs.clear();
	stringstream ss;
	string e;
	ss << s;
	while(ss >> e)
		vs.push_back(e);
}

void Split(vector <string> &vs, string &s, string &sep)
{
	int cur = 0,next;
	next = s.find(sep);
	while(next != string::npos)
	{
		if(s.substr(cur,next-cur) !="")
			vs.push_back(s.substr(cur,next-cur));
		cur = next+sep.size();
		next = s.find(sep,cur);
	}
	vs.push_back(s.substr(cur));
}

void TrimLine(string &line)
{
	line.erase(0,line.find_first_not_of(" \t\r\n"));
	line.erase(line.find_last_not_of(" \t\r\n")+1);
}

void Str_to_char_vec(vector<string> &cv, string &s, const string &encoding)
{
	if (encoding == "gbk")
	{
		for (size_t i = 0; i < s.size(); i++)
		{
			if (s[i] >= 0)
				cv.push_back(s.substr(i,1));
			else
				cv.push_back(s.substr(i++,2));
		}
	}
	else if(encoding == "utf8")
	{
		for (size_t i = 0; i < s.size(); i++)
		{
			unsigned char x = (unsigned char)s[i];
			if (x < 128)
				cv.push_back(s.substr(i,1));
			else if (x < 224)
				cv.push_back(s.substr(i,2));
			else if (x < 240)
			{
				cv.push_back(s.substr(i,3));
				i += 2;
			}
			else if (x < 248)
			{
				cv.push_back(s.substr(i,4));
				i += 3;
			}
			else
			{
				cout<<"bad char!\n";
				return;
			}
		}
	}
}

