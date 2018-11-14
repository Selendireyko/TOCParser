#include "TOCParser.h"

#include <iostream>
#include <fstream>
#include <sstream>

bool iequals(const string& a, const string& b)
{
	if (a.size() != b.size())
		return false;

	for (unsigned int i = 0; i < a.size(); i++)
		if (tolower(a[i]) != tolower(b[i]))
			return false;
	return true;
}

string trimleft(string str, const string strTrim = " ")
{
	str.erase(0, str.find_first_not_of(strTrim));
	return str;
}

string trimright(string str, const string strTrim = " ")
{
	str.erase(str.find_last_not_of(strTrim) + 1);
	return str;
}

string trim(string str, const string strTrim = " ")
{
	str = trimleft(str, strTrim);
	str = trimright(str, strTrim);
	return str;
}

CTOCParser::CTOCParser()
	: m_nStartContentLine(-1)
	, m_nEndContentLine(-1)
	, m_nEndContentPos(-1)
{
}

CTOCParser::CTOCParser(const string& strFilePath)
	: m_nStartContentLine(-1)
	, m_nEndContentLine(-1)
	, m_nEndContentPos(-1)
{
	LoadFile(strFilePath);
	Parse();
}

CTOCParser::~CTOCParser()
{
}

void CTOCParser::LoadFile(const string& strFilePath)
{
	m_strText.clear();

	cout << "Открываю файл: " << strFilePath << endl;

	ifstream ifs;
	
	ifs.open(strFilePath, ios_base::in | ios_base::binary);

	if (!ifs.is_open())
		throw runtime_error("Ошибка открытия файла: " + strFilePath);

	ifs.seekg(0, ios::end);
	size_t nFileSize = (size_t)ifs.tellg();
	ifs.seekg(0, ios::beg);

	m_strText.reserve(nFileSize);
	m_strText.assign((istreambuf_iterator<char>(ifs)), istreambuf_iterator<char>());

	ifs.close();

	cout << "Файл прочитан успешно" << endl;
	cout << "Размер файла " << nFileSize << " байт" << endl;
}

void CTOCParser::Parse()
{
	m_nStartContentLine = -1;
	m_nEndContentLine = -1;
	m_nEndContentPos = -1;
	m_vContents.clear();

	if (m_strText.empty())
		throw runtime_error("Ошибка - Пустой текст");

	cout << "Анализирую файл... "<< endl;

	const int nMaxBrokenLines = 1;
	int nBrokenLines = 0;
	int nTotalBrokenLines = 0;
	string strBrokenLine;

	int nLine = 0;
	string strLine;

	unsigned int nPos = 0;
	unsigned int nBrokenLinePos = 0;

	stringstream strstr(m_strText);

	while (getline(strstr, strLine))
	{
		unsigned int nLength = (unsigned int)strLine.length();
		nPos += nLength + 1;

		if (nLength > 0 && strLine[nLength - 1] == '\r')
			strLine.erase(strLine.end() - 1);

		nLine++;

		strLine = trim(strLine);

		if (strLine.empty())
			continue;

		if (m_nStartContentLine == -1)
		{
			if (iequals(strLine, "Содержание"))
			{
				m_nStartContentLine = nLine;
				continue;
			}
		}
		else
		if (m_nEndContentLine == -1)
		{
			
			basic_string<char>::size_type index = strLine.find_last_not_of("0123456789");
			if (index == string::npos || index == strLine.size() - 1)
			{
				if (nBrokenLines == nMaxBrokenLines)
				{
					nTotalBrokenLines -= nMaxBrokenLines;
					m_nEndContentLine = nLine - nMaxBrokenLines;
					m_nEndContentPos = nBrokenLinePos;
					continue;
				}

				if (nBrokenLines == 0)
					nBrokenLinePos = nPos - nLength - 1;

				nBrokenLines++;
				nTotalBrokenLines++;
				strBrokenLine += strLine;
			}
			else
			{
				if (nBrokenLines > 0)
				{
					strBrokenLine += ' ';
					strBrokenLine += strLine;
					strLine = strBrokenLine;
					nBrokenLines = 0;
					strBrokenLine.clear();
				}
			}

			if (nBrokenLines == 0)
			{
				SContent Content;
				ParseLine(strLine, Content);
				m_vContents.push_back(Content);
			}
		}
	}

	if (m_vContents.empty())
		throw runtime_error("Ошибка - Содержание не найдено");

	if (m_nEndContentPos == -1)
		throw runtime_error("Ошибка - позиция конца Оглавления не найдено");

	BuildRelationships();
	BuildLinks();

	cout << "Найдено " << nLine << " строк и " << m_vContents.size() << " оглавлений" << endl;

	if (nTotalBrokenLines > 0)
		cout << "Востановленно " << nTotalBrokenLines << " сломанных строк" << endl;

	cout << "Номер строки начала Оглавления " << m_nStartContentLine << endl;
	cout << "Номер строки конца Оглавления " << m_nEndContentLine << endl;
	cout << "Позиция конца Оглавления " << m_nEndContentPos << endl;
}

void CTOCParser::ParseLine(const string& strLine, SContent& rContent) const
{
	basic_string<char>::size_type index = strLine.find_first_not_of("0123456789.");
	if (index > 0 && index < strLine.size())
	{
		const string strTitleNumber = strLine.substr(0, index);

		ParseTitleNumber(strTitleNumber, rContent.vTitleNumber);

		rContent.strTitleName = strLine.substr(index);
		rContent.strTitleName = trimleft(rContent.strTitleName, " \t");
	}
	else
		rContent.strTitleName = strLine;

	index = rContent.strTitleName.find_last_not_of("0123456789");
	if (index > 0 && index < rContent.strTitleName.size() - 1)
	{
		const string strPageNumber = rContent.strTitleName.substr(index + 1);
		rContent.nPageNumber = stoi(strPageNumber);

		rContent.strTitleName = rContent.strTitleName.substr(0, index);
		rContent.strTitleName = trimright(rContent.strTitleName, " \t.");
	}
}

void CTOCParser::ParseTitleNumber(const string& strTitleNumber, vecTitleNumber& vTitleNumber) const
{
	vTitleNumber.clear();

	if (strTitleNumber.empty())
		return;

	basic_string<char>::size_type index1 = 0;
	basic_string<char>::size_type index2 = string::npos;

	while (true)
	{
		index2 = strTitleNumber.find('.', index1);
		if (index2 == string::npos)
			break;

		const string strSubTitleNumber = strTitleNumber.substr(index1, index2 - index1);

		index1 = index2 + 1;

		int nSubTitleNumber = stoi(strSubTitleNumber);
		vTitleNumber.push_back(nSubTitleNumber);
	}
}

bool CTOCParser::IsChild(const SContent& rParent, const SContent& rChild) const
{
	if (rParent.vTitleNumber.empty())
		return false;
	if (rChild.vTitleNumber.empty())
		return false;
	if (rChild.vTitleNumber.size() < rParent.vTitleNumber.size())
		return false;

	for (unsigned int i = 0; i < rParent.vTitleNumber.size() && i < rChild.vTitleNumber.size(); i++)
	{
		int nParent = rParent.vTitleNumber[i];
		int nChild = rChild.vTitleNumber[i];

		if (nParent != nChild)
			return false;
	}

	return rParent.vTitleNumber.size() != rChild.vTitleNumber.size();
}

bool CTOCParser::IsParent(const SContent& rParent, const SContent& rChild) const
{
	if (rParent.vTitleNumber.empty())
		return false;
	if (rChild.vTitleNumber.empty())
		return false;
	if (rChild.vTitleNumber.size() > rParent.vTitleNumber.size())
		return false;

	for (unsigned int i = 0; i < rParent.vTitleNumber.size() && i < rChild.vTitleNumber.size(); i++)
	{
		int nParent = rParent.vTitleNumber[i];
		int nChild = rChild.vTitleNumber[i];

		if (nParent != nChild)
			return false;
	}

	return rParent.vTitleNumber.size() != rChild.vTitleNumber.size();
}

void CTOCParser::BuildRelationships()
{
	for (unsigned int i = 0; i < m_vContents.size(); i++)
	{
		SContent& rParent = m_vContents[i];

		for (unsigned int j = 0; j < m_vContents.size(); j++)
		{
			if (i == j)
				continue;

			const SContent& rChild = m_vContents[j];

			if (IsParent(rParent, rChild))
				rParent.nParent = j;

			if (IsChild(rParent, rChild))
				rParent.vChildren.push_back(j);
		}
	}
}

void CTOCParser::BuildLinks()
{
	SContent* pPrevContent = NULL;

	basic_string<char>::size_type index1 = m_nEndContentPos;
	basic_string<char>::size_type index2 = string::npos;

	for (unsigned int i = 0; i < m_vContents.size(); i++)
	{
		SContent& rContent = m_vContents[i];

		rContent.nStartPos = -1;
		rContent.nEndPos = -1;

		string strTitle;
		string strTitleNumber;

		for (unsigned int j = 0; j < rContent.vTitleNumber.size(); j++)
		{
			int nSubTitleNumber = rContent.vTitleNumber[j];

			strTitleNumber += to_string(nSubTitleNumber);
			strTitleNumber += '.';
		}

		if (!strTitleNumber.empty())
			strTitle = strTitleNumber;
		else
			strTitle = rContent.strTitleName;

		if (strTitle.empty())
			continue;

		index2 = m_strText.find(strTitle, index1);
		if (index2 != string::npos)
		{
			rContent.nStartPos = (int)index2;

			if (pPrevContent)
				pPrevContent->nEndPos = (int)(index2 - 1);

			pPrevContent = &rContent;

			index1 = index2;
		}
	}

	if (pPrevContent)
		pPrevContent->nEndPos = (int)(m_strText.length() - 1);
}

void CTOCParser::PrintContents() const
{
	cout << endl;
	cout << "******************** Содержание ********************" << endl;
	cout << endl;

	for (unsigned int i = 0; i < m_vContents.size(); i++)
	{
		const SContent& rContent = m_vContents[i];

		string strContent;

		string strTitleNumber;

		for (unsigned int j = 0; j < rContent.vTitleNumber.size(); j++)
		{
			int nSubTitleNumber = rContent.vTitleNumber[j];

			strTitleNumber += to_string(nSubTitleNumber);
			strTitleNumber += '.';
		}

		strContent += strTitleNumber;

		if (!strContent.empty())
			strContent += '\t';

		strContent += rContent.strTitleName;

		if (rContent.nPageNumber != -1)
		{
			string strPageNumber = to_string(rContent.nPageNumber);
			if (!strPageNumber.empty())
			{
				strContent += " - ";
				strContent += strPageNumber;
			}
		}

		cout << strContent << endl;
	}
	cout << endl;
}

void CTOCParser::PrintRelationshipsAndLinks() const
{
	cout << endl;
	cout << "******************** Отношения и ссылки ********************" << endl;
	cout << endl;

	cout << "Номер строки\tНомер оглавления\tРодитель\tДети\tНачало раздела\tКонец раздела" << endl;

	const string strNo = "-";

	for (unsigned int i = 0; i < m_vContents.size(); i++)
	{
		const SContent& rContent = m_vContents[i];

		string strContent;

		strContent = to_string(i);

		strContent += '\t';

		if (!rContent.vTitleNumber.empty())
		{
			string strTitleNumber;

			for (unsigned int j = 0; j < rContent.vTitleNumber.size(); j++)
			{
				int nSubTitleNumber = rContent.vTitleNumber[j];

				strTitleNumber += to_string(nSubTitleNumber);
				strTitleNumber += '.';
			}

			strContent += strTitleNumber;
		}
		else
			strContent += strNo;

		strContent += "\t";

		if (rContent.nParent != -1)
		{
			string strParent = to_string(rContent.nParent);
			if (!strParent.empty())
				strContent += strParent;
		}
		else
			strContent += strNo;

		strContent += "\t";

		if (!rContent.vChildren.empty())
		{
			string strChildren;

			for (unsigned int j = 0; j < rContent.vChildren.size(); j++)
			{
				int nChild = rContent.vChildren[j];

				string strChild = to_string(nChild);
				if (!strChild.empty())
				{
					strChildren += strChild;
					strChildren += ' ';
				}
			}
			strChildren = trimright(strChildren);

			strContent += strChildren;
		}
		else
			strContent += strNo;

		strContent += "\t";
		if (rContent.nStartPos != -1)
			strContent += to_string(rContent.nStartPos);
		else
			strContent += strNo;

		strContent += "\t";
		if (rContent.nEndPos != -1)
			strContent += to_string(rContent.nEndPos);
		else
			strContent += strNo;

		cout << strContent << endl;
	}
	cout << endl;
}
