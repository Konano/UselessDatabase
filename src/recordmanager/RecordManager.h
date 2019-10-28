#include "../bufmanager/BufPageManager.h"
#include "../fileio/FileManager.h"
#include "../utils/pagedef.h"
#include <vector>
#include <string>
using namespace std;

class Data
{
public:
	virtual string toString() = 0;
	virtual BufType toBufType() = 0; 
};

//type 0
class IntData : public Data {
	int Data;
public:
	IntData(int data){
		Data = data;
	}
	string toString(){
		return to_string(Data);
	}
	BufType toBufType(){
		BufType a = new unsigned int[1];
		a[0] = (unsigned int)Data;
		return a;
	}
};

//type1
class CharData : public Data {
	char Data;
public:
	CharData(char a){
		Data = a;
	}
	string toString(){
		return string(1,Data);
	}
	BufType toBufType(){
		BufType a = new unsigned int[1];
		a[0] = (unsigned int)Data;
		return a;
	}
};

//type2
class StringData : public Data {
	string Data;
public:
	StringData(string a){
		Data = a;
	}
	string toString(){
		return Data;
	}
	BufType toBufType(){
		BufType a = new unsigned int[Data.length()];
		for(int i = 0;i < Data.length();i ++){
			a[i] = (unsigned int)Data[i];
		}
		return a;
	}
};

struct Record
{
	int RID;
	vector<Data*> data;
	string toString(){
		string ans = to_string(RID);
		//ans.clear();
		for(auto a : data){
			ans = ans + " " + a->toString();
		}
		return ans;
	}
};

struct TableHeader
{
	int recordLength;
	int maxPerPage;
	string tableName;
	vector<string> headLines;
	vector<int> dataLength;
	vector<int> type;
	int RIDcounter;
	int PageCounter;
};

/*
struct TableFrame
{
	unsigned int label;
	vector<Record> data;
	string toString(){
		string ans = to_string(label);
		for(auto a: data){
			ans = ans + a.toString();
		}
		return ans;
	}
};
*/

struct RecordManagerData
{
	int fileID;
	TableHeader head;
	RecordManagerData(int fileID,TableHeader head){
		this->fileID = fileID;
		this->head = head;
	}
};

class RecordManager
{
private:
	BufPageManager *bufmanager;
	FileManager *filemanager;
	vector<RecordManagerData> tables;
	unsigned int getMask(unsigned int a){
		int b = 1;
		while(-- a)b = (b << 1) + 1;
		return b;
	}
	int findEmptyPage(int tableID){
		int i;
		for(i = 1;;i ++){
			int index;
			BufType b = bufmanager->getPage(tables[tableID].fileID,i,index);
			if(b[0] ^ getMask(tables[tableID].head.maxPerPage)) break;
		}
		return i;
	}
	int findTableID(const char* name){
		//cout << name << endl;
		for(int i = 0;i < tables.size();i ++){
			//cout << tables[i].head.tableName << " " << name << endl;
			if(tables[i].head.tableName == string(name))return i;
		}
		return -1;
	}
public:
	RecordManager(BufPageManager *bm,FileManager *fm){
		bufmanager = bm;
		filemanager = fm;
	}
	void close(){
		for(int j = 0; j < tables.size();j ++){
			int index;
			BufType b = bufmanager->getPage(tables[j].fileID,0,index);
			b[0] = tables[j].head.RIDcounter;
			b[1] = tables[j].head.PageCounter;
			b[2] = tables[j].head.recordLength;
			b[3] = tables[j].head.maxPerPage;
			b[4] = tables[j].head.dataLength.size();
			for(int i = 0;i < tables[j].head.dataLength.size();i ++)b[5 + i] = tables[j].head.dataLength[i];
			for(int i = tables[j].head.dataLength.size();i < 2 * tables[j].head.dataLength.size();i ++)b[5 + i] = tables[j].head.type[i - tables[j].head.dataLength.size()];
			bufmanager->markDirty(index);
		}
		bufmanager->close();
	}
	~RecordManager(){
		bufmanager->close();
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
	bool openTable(const char* tableName){
		int fileID;
		int index;
		string TableName = tableName;
		filemanager->openFile(("dataset_" + TableName).c_str(),fileID);
		BufType b = bufmanager->getPage(fileID,0,index);
		TableHeader header;
		header.tableName = tableName;
		header.RIDcounter = b[0];
		header.PageCounter = b[1];
		header.recordLength = b[2];
		header.maxPerPage = b[3];
		for(int i = 0;i < b[4];i ++){
			header.dataLength.push_back(b[5 + i]);
			//cout << b[5 + i] << endl;
		}
		for(int i = b[4];i < 2 * b[4];i ++){
			header.type.push_back(b[5 + i]);
			//cout << b[5 + i] << endl;
		}
		tables.push_back(RecordManagerData(fileID,header));
		return 1;
	}
	bool createTable(const char* tableName,vector<string> headLines,vector<int> dataLength,vector<int> type){
		TableHeader header;
		header.headLines = headLines;
		header.dataLength = dataLength;
		header.type = type;
		header.RIDcounter = 0;
		header.tableName = tableName;
		header.recordLength = 1;
		header.PageCounter = 0;
		for(auto a: dataLength)header.recordLength += a;
		header.maxPerPage = 2047 / header.recordLength > 32 ? 32 : 2047 / header.recordLength;
		filemanager->createFile(("dataset_" + header.tableName).c_str());
		int fileID;
		filemanager->openFile(("dataset_" + header.tableName).c_str(),fileID);
		tables.push_back(RecordManagerData(fileID,header));
		//cout << "check" << tables.size() << endl;
		return 1;
	}
	bool addRecord(const char* tableName,Record x){
		int tableID = findTableID(tableName);
		//cout << tableID << endl;
		int pageID = findEmptyPage(tableID);
		//cout << tableID << pageID << endl;
		if(pageID > tables[tableID].head.PageCounter)tables[tableID].head.PageCounter = pageID;
		int index;
		BufType b = bufmanager->getPage(tables[tableID].fileID,pageID,index);
		int cnt = 0;
		//cout << tableID << pageID << index << endl;
		//cout << b[0] << endl;
		for(unsigned int loc = 1;cnt < 32; cnt ++,loc <<= 1){
			//cout << loc << endl;
			//cout << (loc & b[0]) << endl;
			if((loc & b[0]) == 0){b[0] ^= loc;break;}
		}
		int j = 1 + cnt * tables[tableID].head.recordLength;
		b[j] = ++tables[tableID].head.RIDcounter;
		int k = 1;
		for(int l = 0;l < tables[tableID].head.dataLength.size();l ++){
			BufType y = x.data[l]->toBufType();
			for(int cnt = 0;cnt < tables[tableID].head.dataLength[l];cnt ++){
				b[j + k] = y[cnt];
				//cout << y[cnt] << endl;
				k ++;
			}
		}
		bufmanager->markDirty(index);
		//cout << tableID << " " << pageID << " " << index << endl;
		return 1;
	}
	bool removeRecord(const char* tableName,int RID){
		int tableID = findTableID(tableName);
		for(int i = 1;i <= tables[tableID].head.PageCounter;i ++){
			int index;
			BufType b = bufmanager->getPage(tables[tableID].fileID,i,index);
			for(int j = 1,cnt = 0;cnt < 32;j += tables[tableID].head.recordLength,cnt ++){
				if( ((1 << (cnt - 1)) & b[0]) == 0)continue;
				//cout << j << endl;
				if(b[j] == RID){
					b[0] ^= 1 << (j - 1);
					//cout << RID << endl;
					bufmanager->markDirty(index);
					return 1;
				}
			}
		}
		return 0;
	}
	bool renewRecord(const char* tableName,int RID,Record x){
		int tableID = findTableID(tableName);
		for(int i = 1;i < tables[tableID].head.PageCounter;i ++){
			int index;
			BufType b = bufmanager->getPage(tables[tableID].fileID,i,index);
			for(int j = 1,cnt = 0;cnt < 32;j += tables[tableID].head.recordLength,cnt ++){
				if( ((1 << (j - 1)) & b[0]) == 0)continue;
				if(b[j] == RID){
					int k = 1;
					for(int l = 0;l < tables[tableID].head.dataLength.size();l ++){
						BufType y = x.data[l]->toBufType();
						for(int cnt = 0;cnt < tables[tableID].head.dataLength[l];cnt ++){
							b[j + k] = y[cnt];
							k ++;
						}
					}
					bufmanager->markDirty(index);
					return 1;
				}
			}
		}
		return 0;
	}
	vector<Record> findRecord(const char* tableName ,int loc,Data* x){
		vector<Record> v;
		int tableID = findTableID(tableName);
		int dataOffset = 1;
		int dataLength = tables[tableID].head.dataLength[loc];
		BufType y = x->toBufType();
		for(int i = 0;i < loc;i ++){
			dataOffset += tables[tableID].head.dataLength[i];
		}
		for(int i = 1;i <= tables[tableID].head.PageCounter;i ++){
			int index;
			BufType b = bufmanager->getPage(tables[tableID].fileID,i,index);
			for(int j = 1,cnt = 0;cnt < 32;j += tables[tableID].head.recordLength,cnt ++){
				if( ((1 << (j - 1)) & b[0]) == 0)continue;
				bool flag = 1;
				//cout << j << " " << dataOffset << " " << dataLength << endl;
				for(int k = j + dataOffset,cnt = 0;k < j + dataOffset + dataLength;k ++,cnt ++){
					//cout << b[k] << y[cnt] << endl;
					if(b[k] != y[cnt]){
						flag = 0;
						break;
					}
				}
				if(flag){
					int k = j;
					Record ans;
					ans.RID = b[k ++];
					for(int l = 0;l < tables[tableID].head.dataLength.size();l ++){
						//BufType y = x[l]->toBufType();
						if(tables[tableID].head.type[l] == 0){
							for(int cnt = 0;cnt < tables[tableID].head.dataLength[l];cnt ++){
								Data* a = new IntData((int)b[k]);
								ans.data.push_back(a);
								k ++;
							}
						}
						else if(tables[tableID].head.type[l] == 1){
							for(int cnt = 0;cnt < tables[tableID].head.dataLength[l];cnt ++){
								Data* a = new CharData((int)b[k]);
								ans.data.push_back(a);
								k ++;
							}
						}
						else{
							string str;
							for(int cnt = 0;cnt < tables[tableID].head.dataLength[l];cnt ++){
								str = str + (char) b[k];
								k ++;
							}
							Data* a = new StringData(str);
							ans.data.push_back(a);
						}
					}
					v.push_back(ans);
				}
			}
		}
		return v;
	}
};