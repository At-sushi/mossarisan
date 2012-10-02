// LZW������
// ���Ă������A�������肵�����B
#include "stdafx.h"

#define MAXSIZE_CODES 65536
BYTE codes[MAXSIZE_CODES][256];
USHORT size_dic;
BYTE size_code[MAXSIZE_CODES];
BYTE* running;
BYTE* running_buffer;
struct
{
	USHORT code;
	BYTE length;
} BufBlock[10000000];

int linklist[MAXSIZE_CODES];
struct
{
	int start;
	int end;
} linkpos[65536];

DWORD LZWCompare(const LPBYTE mem1, const LPBYTE mem2, DWORD length);
//WORD CreateHash(const LPBYTE pSource, int length);
//BOOL CompHash(DWORD val1, DWORD val2, int len1, int len2);
// �n�b�V�����ǂ��쐬�}�N���A�Q�l������̊ێʂ��B
#define HASHMODOKI(pSource, length) ( *pSource | ((1 < length) ? ((WORD)*(pSource + 1) << 8) : 0) )

void LZWEncode(BYTE* Buffer, DWORD* size_buffer, BYTE* target, DWORD size_target)
{
	USHORT code_prev = 0;
	BYTE length_prev = 0;
	int running_block = 0;

	*size_buffer = 0;
	running_buffer = Buffer;
	size_dic = 0x100;
	memset(linkpos, -1, sizeof(linkpos));

	for (short i = 0; i < 0x100; i++)
	{
		codes[i][0] = (BYTE)i;
		size_code[i] = 1;
		linklist[i] = -1;

		int val = HASHMODOKI(codes[i], size_code[i]);

		linkpos[val].start = linkpos[val].end = i;
	}

	running = target; 
	while (running < target + size_target)
	{
		int code = -1;
		BYTE size_now = 0;
		int target_nokori = size_target - (running - target);

		// �����p�^�[�����Ȃ�������
		int val = HASHMODOKI(running, 2);
		int nowlink = linkpos[val].start;

		while (nowlink != -1)
		{
			int i = nowlink;

			// �T�C�Y�����E�̏ꍇ�͐؂�߂�B
			// �����T�C�Y������邪�A�ǂ����؂����ȍ~�͎g��Ȃ��̂Łi߃�߁j�ƼŲ!!
			if (size_code[i] > target_nokori)
				size_code[i] = target_nokori;

			if (size_code[i] > size_now)
			{
				BYTE matchlen = (BYTE)LZWCompare(codes[i], running, size_code[i]);

				if (matchlen > size_now)
				{
					code = i;
					size_now = matchlen;
				}
			}

			nowlink = linklist[nowlink];
		}

		if (code == -1)
		{
			// ������Ȃ�������A0-255
			code = *running;
			size_now = 1;
		}

#ifdef _DEBUG
		assert( (code <= 0xFFFF) && (code >= 0) );
#endif
		// �o�b�t�@�ɃR�[�h�𿼰�
		BufBlock[running_block].code = (USHORT)code;
		BufBlock[running_block].length = size_now;
		running_block++;

		// ������ǉ�
		if (running != target)	// �ŏ��̓f�[�^���Ȃ��̂Ńp�X
		{
			BYTE writting_size = size_now;	// �ǉ�����f�[�^��

			if (length_prev == size_code[code_prev])
			{
				BOOL Update = (size_code[code_prev] < 2) ? TRUE : FALSE;

				// �����f�[�^�����E�A�؂�߂�
				if (size_code[code_prev] + size_now > 256)
					writting_size -= 256 - (size_code[code_prev] + size_now);

				// �Â��R�[�h�ɒǉ���������
				memcpy(codes[code_prev] + size_code[code_prev], codes[code], writting_size);
				size_code[code_prev] += writting_size;

				if (Update)	// �n�b�V���X�V
				{
					int val = HASHMODOKI(codes[code_prev], size_code[code_prev]);

					if (val != codes[code_prev][0])
					{
						if (linkpos[val].end == -1)
							linkpos[val].start = code_prev;
						else
							linklist[linkpos[val].end] = code_prev;

						linkpos[val].end = linkpos[ codes[code_prev][0] ].end;
					}
				}
			}
			else
			{
				const USHORT latest = size_dic;

				// �ő�l�ɓ��B���������߂�ǂ��̂ŃL�����Z��
				if (latest >= 65536)
					continue;

				size_code[latest] = 0;

				// �O�̃R�[�h���܂������
				memcpy(codes[latest] + size_code[latest], codes[code_prev], length_prev);
				size_code[latest] += length_prev;

				// �����f�[�^�����E�A�؂�߂�
				if (size_code[latest] + size_now > 256)
					writting_size -= 256 - (size_code[code_prev] + size_now);

				// �ŁA���̏ꏊ������
				memcpy(codes[latest] + size_code[latest], codes[code], writting_size);
				size_code[latest] += writting_size;

				// �n�b�V���쐬
				int val = HASHMODOKI(codes[latest], size_code[latest]);

				{
					BOOL found = FALSE;
					int nowlink = linkpos[val].start;

					// ���������Ȃ����`�F�b�N
					while (nowlink != -1)
					{
						int j = nowlink;

						if ( memcmp(codes[j], codes[latest],
							 (size_code[j] < size_code[latest] ? size_code[j] : size_code[latest])) == 0 )
						{
							if (size_code[j] < size_code[latest])
							{
								BOOL Update = (size_code[j] < 2) ? TRUE : FALSE;

								memcpy(codes[j] + size_code[j], codes[latest] + size_code[j], size_code[latest] - size_code[j]);
								size_code[j] = size_code[latest];

								if (Update)	// �n�b�V���X�V
								{
									int val2 = HASHMODOKI(codes[j], size_code[j]);

									if (val2 != codes[j][0])
									{
										if (linkpos[val2].end == -1)
											linkpos[val2].start = j;
										else
											linklist[ linkpos[val2].end ] = j;

										linkpos[val2].end = linkpos[ codes[j][0] ].end;
									}
								}
							}
							found = TRUE;
							break;
						}

						nowlink = linklist[nowlink];
					}

					if (!found)
					{
						// �����N���X�g������
						linklist[latest] = -1;

						// �����N���X�g�Ƀf�[�^�ǉ�
						if (linkpos[val].end == -1)
							linkpos[val].start = latest;
						else
							linklist[ linkpos[val].end ] = latest;

						linkpos[val].end = latest;

						// ������ǉ����܂����[
						size_dic++;
					}
				}
			}
		}

		// �����ɂ������Ƃ���͔�΂�
		running += size_now;

		// �O�̃R�[�h��ێ�
		code_prev = code;
		length_prev = size_now;
	}

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
	FILE* file2 = fopen("block.bin", "wb");
	fwrite(BufBlock, sizeof(BufBlock) / 10000000, running_block, file2);
	fclose(file2);
}

BOOL LZWDecode(BYTE* Buffer, DWORD* size_buffer, BYTE* data, DWORD size_data)
{
	*size_buffer = 0;
	running_buffer = Buffer;
	size_dic = 0;

	for (running = data; running < data + size_data; running++)
	{
		USHORT code = 0;
		int loadsize;

		// �ő�l���Q�T�T�ȉ��Ȃ�P�o�C�g
		if (size_dic <= 0xFF)
			loadsize = 1;
		else
			loadsize = 2;

		// �R�[�h�ǂݍ���
		if (running == data)
			code = 0;
		else
			memcpy(&code, running, loadsize);

		if (code != 0)
		{
			if (code <= size_dic)
			{
				memcpy(running_buffer, codes[code - 1], size_code[code - 1]);
				running_buffer += size_code[code - 1];
				*size_buffer += size_code[code - 1];
			}
			else
				return FALSE;
		}

		if (running != data)
		{
			running += loadsize;
			if (running >= data + size_data)
				return TRUE;
		}

		// �f�[�^�ǂݍ���
		memcpy(running_buffer, running, 1);
		running_buffer++;
		(*size_buffer)++;

		// ������ǉ�
		const USHORT latest = size_dic;

		// �ő�l�ɓ��B���������߂�ǂ��̂ŃL�����Z��
		if (latest >= 65536 - 1)
			continue;

		size_code[latest] = 0;
		if (code != 0)
		{
			if (code > size_dic) return FALSE;
			// �ǂ����̃p�^�[���ɍ��v���Ă���΂܂������������
			memcpy(codes[latest] + size_code[latest], codes[code - 1], size_code[code - 1]);
			size_code[latest] += size_code[code - 1];
		}
		// �ŁA���̏ꏊ������
		memcpy(codes[latest] + size_code[latest], running, 1);
		size_code[latest]++;

		// ������ǉ����܂����[
		size_dic++;
	}

	return TRUE;
}

// �f�[�^���r���Ĉ�v����f���܂�
DWORD LZWCompare(const LPBYTE mem1, const LPBYTE mem2, DWORD length)
{
	DWORD length_d = length;

	// ����Ȃ��C�����C���A�Z���u��
	_asm
	{
		; �����ݒ�
		cld
		mov esi, mem1
		mov edi, mem2
		mov ecx, length_d

		; ��r�͂���
		jecxz CMPFINISH		; ecx��0����Գާ����ɂȂ�炵���̂ŉ��
CMPLOOP:
		cmpsb				; �n�Y���ɂԂ�������܂ŌJ��Ԃ�
		jne CMPFINISH
		loop CMPLOOP

CMPFINISH:
		sub length_d, ecx	; �ŏ��̃J�E���^�l�|�J�E���^�̎c�聁��v��
	}

	return length_d;
}

// �n�b�V�����ǂ��쐬�֐��A�Q�l������̊ێʂ��B
// �}�N���t�����̂Ŏg���Ă܂���B
//WORD CreateHash(const LPBYTE pSource, int length)
//{
//	WORD val1 = *pSource;
//	WORD val2 = (1 < length) ? *(pSource + 1) : 0;
//	DWORD val3 = (2 < length) ? *(pSource + 2) : 0;
//	DWORD val4 = (3 < length) ? *(pSource + 3) : 0;

//	return val1 | (val2 << 8)/* | (val3 << 16) | (val4  << 24)*/;
//}

// �n�b�V�����ǂ���r�֐��B�������d���̂Ŏg�p���Ă܂���B
// �^�L���X�g���s���΁A�召�Ⴄ�t�@�C���ł���r�ł��܂��B
//BOOL CompHash(DWORD val1, DWORD val2, int len1, int len2)
//{
//	if (len1 == 1)
//		val2 = (BYTE)val2;
//	if (len2 == 1)
//		val1 = (BYTE)val1;
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

//	return val1 == val2;
//}
