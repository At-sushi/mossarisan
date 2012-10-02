// LZW符号化
// っていうか、もっさりしすぎ。
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

DWORD LZWCompare(const LPBYTE mem1, const LPBYTE mem2, DWORD length);

void LZWEncode(BYTE* Buffer, DWORD* size_buffer, BYTE* target, DWORD size_target)
{
	USHORT code_prev = 0;
	BYTE length_prev = 0;
	int running_block = 0;

	*size_buffer = 0;
	running_buffer = Buffer;
	size_dic = 0x100;
	for (short i = 0; i < 0x100; i++)
	{
		codes[i][0] = (BYTE)i;
		size_code[i] = 1;
	}

	running = target; 
	while (running < target + size_target)
	{
		int code = -1;
		BYTE size_now = 0;
		int target_nokori = size_target - (running - target);

		// 同じパターンがないか検索
		for (int i = 0; i < size_dic; i++)
		{
			// サイズが限界の場合は切りつめる。
			// 辞書サイズが崩れるが、どうせ切った以降は使わないので（ﾟεﾟ）ｷﾆｼﾅｲ!!
			if (size_code[i] > target_nokori)
				size_code[i] = target_nokori;

			if ( size_code[i] > size_now )
			{
				BYTE matchlen = (BYTE)LZWCompare(codes[i], running, size_code[i]);

				if (matchlen > size_now)
				{
					code = i;
					size_now = matchlen;
				}
			}
		}

		if (code == -1)
		{
			// 見つからなかったら、0-255
			code = *running;
			size_now = 1;
		}

#ifdef _DEBUG
		assert( (code <= 0xFFFF) && (code >= 0) );
#endif
		// バッファにコードをｿｼｰﾝ
		BufBlock[running_block].code = (USHORT)code;
		BufBlock[running_block].length = size_now;
		running_block++;

		// 辞書を追加
		if (running != target)	// 最初はデータがないのでパス
		{
			BYTE writting_size = size_now;	// 追加するデータ量

			if (length_prev == size_code[code_prev])
			{
				// 辞書データ数限界、切りつめる
				if (size_code[code_prev] + size_now > 256)
					writting_size -= 256 - (size_code[code_prev] + size_now);

				// 古いコードに追加書き込み
				memcpy(codes[code_prev] + size_code[code_prev], codes[code], writting_size);
				size_code[code_prev] += writting_size;
			}
			else
			{
				const USHORT latest = size_dic;

				// 最大値に到達→処理がめんどいのでキャンセル
				if (latest >= 65536)
					continue;

				size_code[latest] = 0;

				// 前のコードをまず入れる
				memcpy(codes[latest] + size_code[latest], codes[code_prev], length_prev);
				size_code[latest] += length_prev;

				// 辞書データ数限界、切りつめる
				if (size_code[latest] + size_now > 256)
					writting_size -= 256 - (size_code[code_prev] + size_now);

				// で、今の場所つっこむ
				memcpy(codes[latest] + size_code[latest], codes[code], writting_size);
				size_code[latest] += writting_size;

				{
					BOOL found = FALSE;

					// 同じ物がないかチェック
					for (int j = 0; j < size_dic; j++)
						if ( memcmp(codes[j], codes[latest],
							 (size_code[j] < size_code[latest] ? size_code[j] : size_code[latest])) == 0 )
					{
						if (size_code[j] < size_code[latest])
						{
							memcpy(codes[j] + size_code[j], codes[latest] + size_code[j], size_code[latest] - size_code[j]);
							size_code[j] = size_code[latest];
						}
						found = TRUE;
						break;
					}

					if (!found)
					{
						// 辞書一個追加しましたー
						size_dic++;
					}
				}
			}
		}

		// 辞書にあったところは飛ばす
		running += size_now;

		// 前のコードを保持
		code_prev = code;
		length_prev = size_now;
	}

	// 以下、テスト用部分なので最後には消すつもり。
/*	// 最小符号化長検査、オプションで。
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

	// テスト用、最終的にはなくなる予定。
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

// データを比較して一致長を吐きます
DWORD LZWCompare(const LPBYTE mem1, const LPBYTE mem2, DWORD length)
{
	DWORD length_d = length;

	// 慣れないインラインアセンブラ
	_asm
	{
		; 初期設定
		cld
		mov esi, mem1
		mov edi, mem2
		mov ecx, length_d

		; 比較はじめ
		jecxz CMPFINISH	; ecxが0だとﾔｳﾞｧｲ事になるらしいので回避
CMPLOOP:
		cmpsb			; ハズレにぶち当たるまで繰り返し
		jne CMPFINISH
		loop CMPLOOP

CMPFINISH:
		sub length_d, ecx	; 最初のカウンタ値－カウンタの残り＝一致長
	}

	return length_d;
}
