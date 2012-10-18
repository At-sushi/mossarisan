// mossarisan.cpp : �R���\�[�� �A�v���P�[�V�����̃G���g�� �|�C���g���`���܂��B
//

#include "stdafx.h"

void LZWEncode(BYTE* Buffer, DWORD* size_buffer, BYTE* target, DWORD size_target);
BOOL LZWDecode(BYTE* Buffer, DWORD* size_buffer, BYTE* data, DWORD size_data, DWORD size_original);
const int BufferSize = 16777216;
BYTE Buffer[BufferSize];
BYTE Buffer2[BufferSize];
const char signature[4 + 1] = "BKP2";

int _tmain(int argc, char* argv[])
{
	char filename[FILENAME_MAX] = "stdafx.obj";
	FILE *file = NULL, *file2 = NULL;
	DWORD size;
	DWORD orig = 0;
	BOOL Encoded = FALSE;

	if (argc >= 2)
		strcpy_s(filename, argv[1]);
	/*else
	{
		printf("�t�@�C���������܂ւ�ŁB\n");
		return 0;
	}*/

	fopen_s(&file, filename, "rb");
	if (!file)
	{
		printf("�t�@�C���ǂݍ���Ŷ��B\n");
		return 0;
	}

	if ( strcmp(filename + (strlen(filename) - 4), ".bkp") == 0)
	{
		char Buffer[4];
		fread(Buffer, 1, 4, file);
		if ( memcmp(Buffer, signature, 4) != 0 )
		{
			printf("�ςȃt�@�C�������荞�܂�܂����B\n");
			fclose(file);
			return 0;
		}
		fread(&orig, sizeof(DWORD), 1, file);
	}
	// �܂����̎��O���[�h
	for (size = 0; size < BufferSize; size++)
	{
		int hoge = fgetc(file);

		if (hoge == EOF)
			break; // �X�`�[���{�[�� Da! Da! Da!
		*(Buffer + size) = (BYTE)hoge;
	}

	fclose(file);
	if (size >= BufferSize)
	{
		printf("ERROR:16M�o�C�g�ȏ�̃t�@�C���ɂ͖��Ή��ł��B");
		return -1;
	}

	if ( strcmp(filename + (strlen(filename) - 4), ".bkp") != 0)
	{
		orig = size;
		LZWEncode(Buffer2, &size, Buffer, orig);
		strcat_s(filename, ".bkp");
		Encoded = TRUE;
	}
	else
	{
		if (!LZWDecode(Buffer2, &size, Buffer, size, orig))
		{
			printf("���s���܂���\n");
			return 0;
		}
		filename[strlen(filename) - 4] = '\0';
	}

/*	int soge[0x10000];
	ZeroMemory(soge, sizeof(soge));
	for (DWORD i = 0; i < size; i++)
		soge[ Buffer2[i] ]++;*/

	fopen_s(&file2, filename, "wb");
	if (!file2)
	{
		printf("�t�@�C����������Ŷ��B\n");
		return 0;
	}
	if (Encoded)
	{
		fwrite(signature, 1, 4, file2);
		fwrite(&orig, sizeof(DWORD), 1, file2);
	}
	fwrite(Buffer2, sizeof(BYTE), size, file2);
	fclose(file2);

/*	strcat(filename, "h");
	file2 = fopen(filename, "wb");
	fwrite(soge, sizeof(int), sizeof(soge) / sizeof(int), file2);
	fclose(file2);*/
	printf("�I������ہB\n");

	return 0;
}

