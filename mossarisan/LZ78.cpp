// っていうか、もっさりしすぎ。
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

		// 同じパターンがないか検索
		for (int i = size_dic - 1; i >= 0; i--)
		{
			// サイズが限界なら無視
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
		// 最大値が２５５以下なら１バイトで表記する
		if (size_dic <= 0xFF)
			flushsize = 1;
		else
			flushsize = 2;

		// バッファにコードをｿｼｰﾝ
		if (running != target)	// 一番初めの奴は抜く
		{
			memcpy(running_buffer, &code, flushsize);
			running_buffer += flushsize;
			*size_buffer += flushsize;
		}
#ifdef _DEBUG
		else
			assert(code == 0);
#endif

		// 辞書にあったところは飛ばす
		if (code != 0)
		{
			running += size_code[code - 1];
			if (running >= target + size_target)
				return;
		}

		// バッファにデータをｿｼｰﾝ
		memcpy(running_buffer, running, 1);
		running_buffer++;
		(*size_buffer)++;

		// 辞書を追加
		const USHORT latest = size_dic;

		// 最大値に到達→処理がめんどいのでキャンセル
		if (latest >= 65536 - 1)
			continue;

		size_code[latest] = 0;
		if (code != 0)
		{
			// どこかのパターンに合致していればまずそれをつっこむ
			memcpy(codes[latest] + size_code[latest], codes[code - 1], size_code[code - 1]);
			size_code[latest] += size_code[code - 1];
		}
		// で、今の場所つっこむ
		memcpy(codes[latest] + size_code[latest], running, 1);
		size_code[latest]++;

		// 辞書一個追加しましたー
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

		// 最大値が２５５以下なら１バイト
		if (size_dic <= 0xFF)
			loadsize = 1;
		else
			loadsize = 2;

		// コード読み込み
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

		// データ読み込み
		memcpy(running_buffer, running, 1);
		running_buffer++;
		(*size_buffer)++;

		// 辞書を追加
		const USHORT latest = size_dic;

		// 最大値に到達→処理がめんどいのでキャンセル
		if (latest >= 65536 - 1)
			continue;

		size_code[latest] = 0;
		if (code != 0)
		{
			if (code > size_dic) return FALSE;
			// どこかのパターンに合致していればまずそれをつっこむ
			memcpy(codes[latest] + size_code[latest], codes[code - 1], size_code[code - 1]);
			size_code[latest] += size_code[code - 1];
		}
		// で、今の場所つっこむ
		memcpy(codes[latest] + size_code[latest], running, 1);
		size_code[latest]++;

		// 辞書一個追加しましたー
		size_dic++;
	}

	return TRUE;
}
