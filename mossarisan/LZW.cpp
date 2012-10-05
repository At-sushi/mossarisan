// LZW������
// ���Ă������A�������肵�����B
#include "stdafx.h"

#define MAXSIZE_CODES 65536
BYTE codes[MAXSIZE_CODES][256];
int size_dic;
short size_code[MAXSIZE_CODES];
BYTE* running;
BYTE* running_buffer;
struct
{
	USHORT code;
	short length;
} BufBlock[1];
UINT NumBlock;
std::set<USHORT> hash_code_list[0x10000];
int maxlength;
const int BufferSize = 16777216;

DWORD LZWCompare(const LPBYTE mem1, const LPBYTE mem2, DWORD length);
WORD CreateHash(const LPBYTE pSource, int length);
BOOL CompHash(DWORD val1, DWORD val2, int len1, int len2);
void LZWAddDic(BYTE* running, BYTE* startpos, USHORT code, USHORT code_prev, short size_now, short length_prev);

void RangeEncode(BYTE* Buffer, DWORD* size_buffer, UINT number);
void RangeEncFinish(BYTE* Buffer, DWORD* size_buffer);
void RangeDecInit(BYTE* Buffer, DWORD size_buffer);
void RangeDecode(BYTE* Buffer, DWORD size_buffer);

UINT frequency_table_code[MAXSIZE_CODES];
UINT frequency_table_length[0x101];
UINT frequency_sum_code[MAXSIZE_CODES + 1];
UINT frequency_sum_length[0x101 + 1];

void LZWEncode(BYTE* Buffer, DWORD* size_buffer, BYTE* target, DWORD size_target)
{
	USHORT code_prev = 0;
	short length_prev = 0;
	//int running_block = 0;

	*size_buffer = 0;
	running_buffer = Buffer;
	size_dic = 0x100;
	frequency_sum_code[0] = 0;
	frequency_sum_length[0] = 0;

	maxlength = 1;
	frequency_table_length[0] = 0;
	frequency_table_length[1] = 1;
	frequency_sum_length[0] = 0;
	frequency_sum_length[1] = 0;
	frequency_sum_length[2] = 1;
	for (short i = 0; i < 0x100; i++)
	{
		codes[i][0] = (BYTE)i;
		size_code[i] = 1;
		hash_code_list[CreateHash(codes[i], size_code[i])].insert(i);
		frequency_table_code[i] = 1;
		frequency_sum_code[i + 1] = i + 1;
	}

	running = target; 
	while (running < target + size_target)
	{
		int code = -1;
		short size_now = 0;
		int target_nokori = size_target - (running - target);

		if (*size_buffer >= BufferSize)
		{
			printf("ERROR:���k��̃T�C�Y��16M�o�C�g�𒴉߂��܂���");
			exit(-1);
		}

		if (target_nokori < 256)
		{
			for (int i = 0; i < size_dic; i++)
			{
				// �T�C�Y�����E�̏ꍇ�͐؂�߂�B
				// �����T�C�Y������邪�A�ǂ����؂����ȍ~�͎g��Ȃ��̂Łi߃�߁j�ƼŲ!!
				if (size_code[i] > target_nokori)
				{
					size_code[i] = target_nokori;
					if (size_code[i] == 1)
					{
						hash_code_list[CreateHash(codes[i], 2)].erase(i);	// �Â��̂͏���
						hash_code_list[CreateHash(codes[i], size_code[i])].insert(i);
					}
				}
			}
		}

		// �����p�^�[�����Ȃ�������
		WORD hash_running = CreateHash(running, 2);
		if (!hash_code_list[hash_running].empty())
			for (std::set<USHORT>::iterator i = hash_code_list[hash_running].begin();
				 i != hash_code_list[hash_running].end();
				 ++i)
		{
			if ( size_code[*i] > size_now )
			{
				// �ŏ���2�����͈�v�m�肵�Ă���̂Ŕ�r�Ώۂ���O��
				short matchlen = (size_code[*i] > 2) ? 2 + (short)LZWCompare(codes[*i] + 2, running + 2, size_code[*i] - 2) : size_code[*i];

				if (matchlen > size_now)
				{
					code = *i;
					size_now = matchlen;
				}
			}
		}

		if (code == -1)
		{
			// ������Ȃ�������A0-255
			code = *running;
			size_now = 1;
		}

		assert( (code <= 0xFFFF) && (code >= 0) );
		// �o�b�t�@�ɃR�[�h�𿼰�
		BufBlock[0].code = (USHORT)code;
		BufBlock[0].length = size_now;
		// �����W�R�[�_���i
		RangeEncode(Buffer, size_buffer, 0);
		//running_block++;

		// ������ǉ�
		LZWAddDic(running, target, code, code_prev, size_now, length_prev);

		// �����ɂ������Ƃ���͔�΂�
		running += size_now;

		// �O�̃R�[�h��ێ�
		code_prev = code;
		length_prev = size_now;
	}

	RangeEncFinish(Buffer, size_buffer);
	//*size_blocks = running_block;

	// �ȉ��A�e�X�g�p�����Ȃ̂ōŌ�ɂ͏�������B
/*	// �ŏ��������������A�I�v�V�����ŁB
	static double soge[0x1000000];
	double zentai = 0.0; double yopparai = 0.0;
	for (int i = 0; i < 0x1000000; i++)
		soge[i] = 0.0;
	for (int i = 0; i < running_block; i++)
	{
		soge[ BufBlock[i].code | ((USHORT)BufBlock[i].length << 16) ] += 1.0;
		zentai += 1.0;
	}
	for (int i = 0; i < 0x1000000; i++) if (soge[i] > 0.0)
	{
		soge[i] /= zentai;
		yopparai += log(soge[i]);
	}*/

	// �e�X�g�p�A�ŏI�I�ɂ͂Ȃ��Ȃ�\��B
	/*FILE* file2 = fopen("block.bin", "wb");
	fwrite(BufBlock, sizeof(BufBlock) / 1, 1, file2);
	fclose(file2);*/
}

BOOL LZWDecode(BYTE* Buffer, DWORD* size_buffer, BYTE* data, DWORD size_data, DWORD size_original)
{
	// TODO:�������𓝈ꂷ��
	USHORT code_prev = 0;
	short length_prev = 0;

	*size_buffer = 0;
	running_buffer = Buffer;
	size_dic = 0x100;
	frequency_sum_code[0] = 0;
	frequency_sum_length[0] = 0;

	maxlength = 1;
	frequency_table_length[0] = 0;
	frequency_table_length[1] = 1;
	frequency_sum_length[0] = 0;
	frequency_sum_length[1] = 0;
	frequency_sum_length[2] = 1;
	for (short i = 0; i < 0x100; i++)
	{
		codes[i][0] = (BYTE)i;
		size_code[i] = 1;
		hash_code_list[CreateHash(codes[i], size_code[i])].insert(i);
		frequency_table_code[i] = 1;
		frequency_sum_code[i + 1] = i + 1;
	}
	RangeDecInit(data, size_data);

	while (running_buffer < Buffer + size_original)
	{
		int code;
		short size_now;
		int target_nokori = size_original - (running_buffer - Buffer);

		// �������ɍ��킹�A�T�C�Y�����E�̏ꍇ�͐؂�߂�B
		if (target_nokori < 256)
		{
			for (int i = 0; i < size_dic; i++)
			{
				if (size_code[i] > target_nokori)
				{
					size_code[i] = target_nokori;
					if (size_code[i] == 1)
					{
						hash_code_list[CreateHash(codes[i], 2)].erase(i);	// �Â��̂͏���
						hash_code_list[CreateHash(codes[i], size_code[i])].insert(i);
					}
				}
			}
		}

		// �������P�������W�Ń`��
		RangeDecode(data, size_data);
		code = BufBlock[0].code;
		size_now = BufBlock[0].length;

		// �f�[�^���d�j
		if ( (code > 0xFFFF) || (code < 0) || (size_now > size_code[code]) )
			return FALSE;

		// ������ǉ�
		LZWAddDic(running_buffer, Buffer, code, code_prev, size_now, length_prev);

		memcpy(running_buffer, codes[code], size_now);
		*size_buffer += size_now;
		running_buffer += size_now;

		// �O�̃R�[�h��ێ�
		code_prev = code;
		length_prev = size_now;
	}

	return TRUE;
}

// �f�[�^���r���Ĉ�v����f���܂�
DWORD LZWCompare(const LPBYTE mem1, const LPBYTE mem2, DWORD length)
{
	// TODO:���S�ȃA�Z���u���`���ɒu������
	// �Ƃ������A�A�Z���u���ŏ����K�v�������񂾂낤���A����B
	register DWORD length_d = length;

	// ����Ȃ��C�����C���A�Z���u��
	_asm
	{
		; �����ݒ�
		cld
		mov esi, mem1
		mov edi, mem2
		mov ecx, length_d

		; ��r�͂���
		jecxz CMPFINISH	; ecx��0����Գާ����ɂȂ�炵���̂ŉ��
		repe cmpsb		; �n�Y���ɂԂ�������܂ŌJ��Ԃ�
		je CMPFINISH
		inc ecx			; �Ō��1��������������ꍇ

CMPFINISH:
		sub length_d, ecx	; �ŏ��̃J�E���^�l�|�J�E���^�̎c�聁��v��
	}

	return length_d;
}

// �n�b�V�����ǂ��쐬�֐��A�Q�l������̊ێʂ��B
WORD CreateHash(const LPBYTE pSource, int length)
{
	WORD val1 = *pSource;
	WORD val2 = (1 < length) ? *(pSource + 1) : 0;
//	DWORD val3 = (2 < length) ? *(pSource + 2) : 0;
//	DWORD val4 = (3 < length) ? *(pSource + 3) : 0;

	return val1 | (val2 << 8)/* | (val3 << 16) | (val4  << 24)*/;
}

/// �n�b�V�����ǂ���r�֐��B
// �^�L���X�g���s���΁A�召�Ⴄ�t�@�C���ł���r�ł��܂��B
// �i���g�p�A�d�����߁j
BOOL CompHash(DWORD val1, DWORD val2, int len1, int len2)
{
	if (len1 == 1)
		val2 = (BYTE)val2;
	if (len2 == 1)
		val1 = (BYTE)val1;
/*	else if (len1 == 2 || len2 == 2)
	{
		val1 = (WORD)val1;
		val2 = (WORD)val2;
	}
	else if (len1 == 3 || len2 == 3)
	{
		val1 &= 0x00FFFFFF;
		val2 &= 0x00FFFFFF;
	}*/

	return val1 == val2;
}

// ������ǉ�
void LZWAddDic(BYTE* running, BYTE* startpos, USHORT code, USHORT code_prev, short size_now, short length_prev)
{
	if (running != startpos)	// �ŏ��̓f�[�^���Ȃ��̂Ńp�X
	{
		short writting_size = size_now;	// �ǉ�����f�[�^��

		if (length_prev == size_code[code_prev])
		{
			BOOL Update = (size_code[code_prev] < 4) ? TRUE : FALSE;

			// �����f�[�^�����E�A�؂�߂�
			if (size_code[code_prev] + size_now > 256)
				writting_size -= (size_code[code_prev] + size_now) - 256;

			// �Â��R�[�h�ɒǉ���������
			memcpy(codes[code_prev] + size_code[code_prev], codes[code], writting_size);
			size_code[code_prev] += writting_size;

			if (Update)	// �n�b�V���X�V
				hash_code_list[CreateHash(codes[code_prev], size_code[code_prev])].insert(code_prev);

			// �ő��v���グ��
			if (size_code[code_prev] > maxlength)
			{
				for (int i = maxlength + 1; i <= size_code[code_prev]; i++)
				{
					frequency_table_length[i] = 1;
					frequency_sum_length[i + 1] = frequency_sum_length[i] + 1;
				}

				maxlength = size_code[code_prev];
			}
		}
		else
		{
			const int latest = size_dic;

			// �ő�l�ɓ��B���������߂�ǂ��̂ŃL�����Z��
			if (latest >= 65536)
				return;

			size_code[latest] = 0;

			// �O�̃R�[�h���܂������
			memcpy(codes[latest] + size_code[latest], codes[code_prev], length_prev);
			size_code[latest] += length_prev;

			// �����f�[�^�����E�A�؂�߂�
			if (size_code[latest] + size_now > 256)
				writting_size -= (size_code[latest] + size_now) - 256;

			// �ŁA���̏ꏊ������
			memcpy(codes[latest] + size_code[latest], codes[code], writting_size);
			size_code[latest] += writting_size;

			// �n�b�V���쐬
			WORD hash_latest = CreateHash(codes[latest], size_code[latest]);

			{
				BOOL found = FALSE;

				// ���������Ȃ����`�F�b�N
				if (!hash_code_list[hash_latest].empty())
					for (std::set<USHORT>::iterator j = hash_code_list[hash_latest].begin();
						 j != hash_code_list[hash_latest].end();
						 ++j)
						if ( size_code[latest] <= 2 ||
							 size_code[*j] <= 2 ||
							 memcmp(codes[*j] + 2, codes[latest] + 2,
							 (size_code[*j] < size_code[latest] ? size_code[*j] : size_code[latest]) - 2) == 0 )	// �ŏ���2�����͈�v�m�肵�Ă�̂Ŕ�΂��܂�
				{
					if (size_code[*j] < size_code[latest])
					{
						// TODO:��̏����Ƃ��Ԃ��Ă�̂œ��ꂵ����
						BOOL Update = (size_code[*j] < 4) ? TRUE : FALSE;

						memcpy_s(codes[*j] + size_code[*j], 256 - size_code[*j], codes[latest] + size_code[*j], size_code[latest] - size_code[*j]);
						size_code[*j] = size_code[latest];

						// �ő��v���グ��
						if (size_code[*j] > maxlength)
						{
							for (int i = maxlength + 1; i <= size_code[*j]; i++)
							{
								frequency_table_length[i] = 1;
								frequency_sum_length[i + 1] = frequency_sum_length[i] + 1;
							}

							maxlength = size_code[*j];
						}

						if (Update)	// �n�b�V���X�V
						{
							hash_code_list[CreateHash(codes[*j], size_code[*j])].insert(*j);
						}
					}
					found = TRUE;
					break;
				}

				if (!found)
				{
					// ������ǉ����܂����[
					size_dic++;

					// �p�x�\���X�V
					frequency_table_code[latest] = 1;
					frequency_sum_code[latest + 1] = frequency_sum_code[latest] + 1;

					// �ő��v���グ��
					if (size_code[latest] > maxlength)
					{
						for (int i = maxlength + 1; i <= size_code[latest]; i++)
						{
							frequency_table_length[i] = 1;
							frequency_sum_length[i + 1] = frequency_sum_length[i] + 1;
						}

						maxlength = size_code[latest];
					}

					hash_code_list[hash_latest].insert(latest);
				}
			}
		}
	}
}
