#pragma once

#include <string>
#include <vector>

using namespace std;

class CTOCParser
{
public:
	
	CTOCParser();

	CTOCParser(const string& strFilePath);

	~CTOCParser();

	void LoadFile(const string& strFilePath);

	void Parse();

	void PrintContents() const;

	void PrintRelationshipsAndLinks() const;

private:
	typedef vector<string> vecText;
	typedef vector<int> vecTitleNumber;
	typedef vector<int> vecChildren;

	struct SContent
	{
		SContent()
			: nPageNumber(-1)
			, nParent(-1)
			, nStartPos(-1)
			, nEndPos(-1)
		{}
		vecTitleNumber vTitleNumber;
		string strTitleName;
		int nPageNumber;
		int nParent;
		vecChildren vChildren;
		int nStartPos;
		int nEndPos;
	};
	typedef vector<SContent> vecContents;

	void ParseLine(const string& strLine, SContent& rContent) const;

	void ParseTitleNumber(const string& strTitleNumber, vecTitleNumber& vTitleNumber) const;

	void BuildRelationships();

	void BuildLinks();

	bool IsChild(const SContent& rParent, const SContent& rChild) const;

	bool IsParent(const SContent& rParent, const SContent& rChild) const;

	string m_strText;
	int m_nStartContentLine;
	int m_nEndContentLine;
	int m_nEndContentPos;
	vecContents m_vContents;
};
