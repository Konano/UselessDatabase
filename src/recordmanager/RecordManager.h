#include "../bufmanager/BufPageManager.h"
#include "../fileio/FileManager.h"
#include <vector>
#include <string>
using namespace std;

class Data
{
public:
	virtual string toString();
};

class IntData : public Data {
	int Data;
	string toString(){
		return to_string(Data);
	}
};

class CharData : public Data {
	char Data;
	string toString(){
		return string(1,Data);
	}
};

class StringData : public Data {
	string Data;
	string toString(){
		return Data;
	}
};

struct Record
{
	int RID;
	vector<Data> data;
	string toString(){
		string ans;
		ans.clear();
		for(auto a : data){
			ans = ans + a.toString();
		}
		return ans;
	}
};

struct TableHeader
{
	int recordLenth;
	int tableName;
	vector<string> headLines;
	int RIDcounter;
};

struct TableFrame
{
	int label;
	vector<Data> data;
	string toString(){
		string ans = to_string(label);
		for(auto a: data){
			ans = ans + a.toString();
		}
		return ans;
	}
};

class RecordManager
{
public:
	BufPageManager *bufmanager;
	FileManager *filemanager;
	RecordManager(BufPageManager *bm,FileManager *fm){
		bufmanager = bm;
		filemanager = fm;
	}
	~RecordManager(){
		BufPageManager->close();
	}
	bool createFile(const char* name){
		return filemanager->createFile(name);
	}
	bool openFile(const char* name, int& fileID){
		return filemanager->openFile(name,fileID);
	}
	bool closeFile(int fileID){
		return filemanager->closeFile(fileID);
	}
	bool addRecord(Record x){

	}
	bool removeRecord(int RID){

	}
	bool renewRecord(int RID,Record x){

	}
	vector<Record> findRecord(){

	}
};