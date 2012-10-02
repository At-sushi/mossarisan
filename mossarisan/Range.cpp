// ��r�񓪋�
#include "stdafx.h"

extern struct
{
	USHORT code;
	short length;
} BufBlock[1];
#define MAXSIZE_CODES 65536
extern UINT frequency_table_code[MAXSIZE_CODES];
extern UINT frequency_table_length[0x101];
extern UINT frequency_sum_code[MAXSIZE_CODES + 1];
extern UINT frequency_sum_length[0x101 + 1];
extern int size_dic;
extern int maxlength;

#define MAX_RANGE 0x80000000
#define MIN_RANGE 0x00800000
#define MASK 0x7FFFFFFF

UINT low = 0;
UINT Range = MAX_RANGE;

/* �f�[�^���������O�o�b�t�@�i�o���p�x�\�̍X�V�Ɏg�p�j */
#define RING_MAX	16384
USHORT ring_code[RING_MAX];
short ring_length[RING_MAX];
int startpoint = 0, ring_filled = 0;
int startpoint2 = 0, ring_filled2 = 0;

// �p�x�\��������
void UpdateFreq_code(int c)
{
   int heri;

	frequency_table_code[c]++;

	heri = size_dic;
	if (ring_filled >= RING_MAX)
	{
		/* RING_MAX�o�C�g�O�ɕ����������f�[�^�̏o���p�x�����炷 */
		heri = ring_code[startpoint % RING_MAX];
		frequency_table_code[heri]--;

		/* �������X���C�h���A������ǉ�����B */
		startpoint++;
		if (startpoint >= INT_MAX) startpoint %= RING_MAX;
		ring_code[ (startpoint + (RING_MAX - 1)) % RING_MAX ] = c;
	}
	else
	{
		/* �����O�𖄂߂� */
		ring_code[ (startpoint + ring_filled) % RING_MAX ] = c;
		ring_filled++;
	}

	if (c < heri)
		while( ++c <= heri ) frequency_sum_code[c]++;
	else if (c > heri)
		while( ++heri <= c ) frequency_sum_code[heri]--;

	if (frequency_sum_code[size_dic] >= MIN_RANGE)
	{
		for (int i = 0; i < size_dic; i++)
		{
			frequency_table_code[i] >>= 1;
			frequency_table_code[i] |= 1;

			frequency_sum_code[i + 1] = frequency_sum_code[i] + frequency_table_code[i];
		}
	}
}

void UpdateFreq_len(int c)
{
   int heri;

	frequency_table_length[c]++;

	heri = maxlength + 1;
	if (ring_filled2 >= RING_MAX)
	{
		/* RING_MAX�o�C�g�O�ɕ����������f�[�^�̏o���p�x�����炷 */
		heri = ring_length[startpoint2 % RING_MAX];
		frequency_table_length[heri]--;

		/* �������X���C�h���A������ǉ�����B */
		startpoint2++;
		if (startpoint2 >= INT_MAX) startpoint2 %= RING_MAX;
		ring_length[ (startpoint2 + (RING_MAX - 1)) % RING_MAX ] = c;
	}
	else
	{
		/* �����O�𖄂߂� */
		ring_length[ (startpoint2 + ring_filled2) % RING_MAX ] = c;
		ring_filled2++;
	}

	if (c < heri)
		while( ++c <= heri ) frequency_sum_length[c]++;
	else if (c > heri)
		while( ++heri <= c ) frequency_sum_length[heri]--;

	while (frequency_sum_length[maxlength + 1] >= MIN_RANGE)
	{
		for (int i = 0; i <= maxlength; i++)
		{
			frequency_table_length[i] >>= 1;
			frequency_table_length[i] |= 1;

			frequency_sum_length[i + 1] = frequency_sum_length[i] + frequency_table_length[i];
		}
	}
}

void RangeEncode(BYTE* Buffer, DWORD* size_buffer, UINT number)
{
	UINT temp;
	BYTE* sousa_buffer = Buffer + *size_buffer;

	// �Ƃ肠�����R�[�h����ˁB

	temp = Range / frequency_sum_code[size_dic];

	low += temp * frequency_sum_code[ BufBlock[number].code ];
	Range = temp * frequency_table_code[ BufBlock[number].code ];

	// �p�x�\��������
	UpdateFreq_code(BufBlock[number].code);

 	while (low & ~MASK)
	{
		// �P�^�A�K�[��
		BYTE* i;

		for (i = sousa_buffer - 1; i >= Buffer; i--)
		{
			int code = *i + 1;
			*i = (BYTE)code;
			if (code < 0x100) break;
		}

		low &= MASK;
	}

	while (Range < MIN_RANGE)
	{
		*(sousa_buffer++) = (BYTE)(low >> 23);
		(*size_buffer)++;

		low <<= 8;
		low &= MASK;
		Range <<= 8;
	}


	// ���x�͈�v���i�߂�ǂ��̂ōė��p�����ɂ��̂܂܃R�s�y�j

	temp = Range / frequency_sum_length[maxlength + 1];

	low += temp * frequency_sum_length[ BufBlock[number].length ];
	Range = temp * frequency_table_length[ BufBlock[number].length ];

	// �p�x�\��������
	UpdateFreq_len(BufBlock[number].length);

	if (low & ~MASK)
	{
		// �P�^�A�K�[��
		BYTE* i;

		for (i = sousa_buffer - 1; i >= Buffer; i--)
		{
			int code = *i + 1;
			*i = (BYTE)code;
			if (code < 0x100) break;
		}

		low &= MASK;
	}

	while (Range < MIN_RANGE)
	{
		*(sousa_buffer++) = (BYTE)(low >> 23);
		(*size_buffer)++;

		low <<= 8;
		low &= MASK;
		Range <<= 8;
	}
}

void RangeEncFinish(BYTE* Buffer, DWORD* size_buffer)
{
	BYTE* sousa_buffer = Buffer + *size_buffer;

	*(sousa_buffer++) = (BYTE)(low >> 23);
	*(sousa_buffer++) = (BYTE)(low >> 15);
	*(sousa_buffer++) = (BYTE)(low >> 7);
	*(sousa_buffer++) = (BYTE)(low << 1);

	*size_buffer += 4;
}

// �э��ڐA
DWORD running_rc;
UINT code;

void RangeDecInit(BYTE* Buffer, DWORD size_buffer)
{
	// ���������
	if (size_buffer < 4)
		return;

	Range = MAX_RANGE;

	// code���������B
	code = ((UINT)(Buffer[0]) << 23) | ((UINT)(Buffer[1]) << 15) |
			((UINT)(Buffer[2]) << 7) | ((UINT)(Buffer[3]) >> 1);

	running_rc = 4;
}

void RangeDecode(BYTE* Buffer, DWORD size_buffer)
{
	int i, j;
	UINT tmp, tmp1;

	// �R�[�h����B

	tmp = Range / frequency_sum_code[size_dic];
	tmp1 = code / tmp;

	// �񕪒T��������
	i = 0; j = size_dic - 1;

	// �ꏊ��T��
	while (i < j)
	{
		// �^�񒆂͂ǂ���
		int cent = i + ( (j - i) / 2 );

		if (frequency_sum_code[cent + 1] <= tmp1)
			i = cent + 1; // ��������
		else if (frequency_sum_code[cent] > tmp1)
			j = cent - 1; // �ł�����
		else
		{
			// �K�b�e�B��
			i = j = cent;
			break;
		}
	}

	// ���ʂ�f���Ƃ��܂��ˁB
	BufBlock[0].code = i;

	// �R�[�h��i�߂�
	code -= tmp * frequency_sum_code[i];
	Range = tmp * frequency_table_code[i];

	// �p�x�X�V
	UpdateFreq_code(i);

	while (Range < MIN_RANGE)
	{
		code = ((code << 8) & MASK);
		Range <<= 8;

		if (running_rc < size_buffer)
		{
			code |= (BYTE)(Buffer[running_rc - 1] << 7);
			code |= (Buffer[running_rc++] >> 1);
		}
	}


	// ��v���ˁi���ς�炸�R�s�y�j

	tmp = Range / frequency_sum_length[maxlength + 1];
	tmp1 = code / tmp;

	// �񕪒T��������
	i = 0; j = maxlength;

	// �ꏊ��T��
	while (i < j)
	{
		// �^�񒆂͂ǂ���
		int cent = i + ( (j - i) / 2 );

		if (frequency_sum_length[cent + 1] <= tmp1)
			i = cent + 1; // ��������
		else if (frequency_sum_length[cent] > tmp1)
			j = cent - 1; // �ł�����
		else
		{
			// �K�b�e�B��
			i = j = cent;
			break;
		}
	}

	// ���ʂ�f���Ƃ��܂��ˁB
	BufBlock[0].length = i;

	// �R�[�h��i�߂�
	code -= tmp * frequency_sum_length[i];
	Range = tmp * frequency_table_length[i];

	// �p�x�X�V
	UpdateFreq_len(i);

	while (Range < MIN_RANGE)
	{
		code = ((code << 8) & MASK);
		Range <<= 8;

		if (running_rc < size_buffer)
		{
			code |= (BYTE)(Buffer[running_rc - 1] << 7);
			code |= (Buffer[running_rc++] >> 1);
		}
	}
}
