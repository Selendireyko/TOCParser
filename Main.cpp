#include "TOCParser.h"

#include <iostream>

int main(int argc, char* argv[])
{
	int nExitCode = 0;

	setlocale(LC_ALL, "ru-RU");

	cout << "�������������� ���������� ����������" << endl;

	if (argc != 2)
	{
		cerr << "�������������: " << argv[0] << " ���������_����" << endl;
		nExitCode = 1;
	}
	else
	{
		try
		{
			CTOCParser Parser;

			Parser.LoadFile(argv[1]);
			Parser.Parse();
			Parser.PrintContents();
			Parser.PrintRelationshipsAndLinks();
		}
		catch (const exception& ex)
		{
			cerr << ex.what() << endl;
			nExitCode = 1;
		}
	}

	cout << "������� ����� ������ � ������� ������� ����" << endl;
	char ch;
	cin >> ch;

	return nExitCode;
}
