// ���Ă������A�������肵�����B
#include "stdafx.h"
#include "define_gcd.h"
#include "GCDComp.h"

BYTE codes[65536 - 1][256];
USHORT size_dic;
BYTE size_code[65536 - 1];
BYTE* running;
BYTE* running_buffer;

void LZ78Encode(BYTE* Buffer, BYTE* target, DWORD* size_buffer, DWORD size_target)
{
	*size_buffer = 0;
	running_buffer = Buffer;
	size_dic = 0;

	for (running = target; running < target + size_target; running++)
	{
		USHORT code = 0;
		BYTE size_now = 0;
		int target_nokori = size_target - (running - target);

		// �����p�^�[�����Ȃ�������
		for (int i = size_dic - 1; i >= 0; i--)
		{
			// �T�C�Y�����E�Ȃ疳��
			if (size_code[i] > target_nokori)
				break;

			if ( size_code[i] > size_now &&
				 memcmp(codes[i], running, size_code[i]) == 0 )
			{
				code = i + 1;
				size_now = size_code[i];
			}
		}

		int flushsize;
#ifdef _DEBUG
		assert( !(size_dic <= 0xFF && code > 0xFF) );
#endif
		// �ő�l���Q�T�T�ȉ��Ȃ�P�o�C�g�ŕ\�L����
		if (size_dic <= 0xFF)
			flushsize = 1;
		else
			flushsize = 2;

		// �o�b�t�@�ɃR�[�h�𿼰�
		if (running != target)	// ��ԏ��߂̓z�͔���
		{
			memcpy(running_buffer, &code, flushsize);
			running_buffer += flushsize;
			*size_buffer += flushsize;
		}
#ifdef _DEBUG
		else
			assert(code == 0);
#endif

		// �����ɂ������Ƃ���͔�΂�
		if (code != 0)
		{
			running += size_code[code - 1];
			if (running >= target + size_target)
				return;
		}

		// �o�b�t�@�Ƀf�[�^�𿼰�
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
}

BOOL LZ78Decode(BYTE* Buffer, BYTE* data, DWORD* size_buffer, DWORD size_data)
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
