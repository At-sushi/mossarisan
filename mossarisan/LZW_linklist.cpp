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

int linklist[MAXSIZE_CODES];
struct
{
	int start;
	int end;
} linkpos[65536];

DWORD LZWCompare(const LPBYTE mem1, const LPBYTE mem2, DWORD length);
//WORD CreateHash(const LPBYTE pSource, int length);
//BOOL CompHash(DWORD val1, DWORD val2, int len1, int len2);
// ハッシュもどき作成マクロ、参考書からの丸写し。
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

		// 同じパターンがないか検索
		int val = HASHMODOKI(running, 2);
		int nowlink = linkpos[val].start;

		while (nowlink != -1)
		{
			int i = nowlink;

			// サイズが限界の場合は切りつめる。
			// 辞書サイズが崩れるが、どうせ切った以降は使わないので（ﾟεﾟ）ｷﾆｼﾅｲ!!
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
				BOOL Update = (size_code[code_prev] < 2) ? TRUE : FALSE;

				// 辞書データ数限界、切りつめる
				if (size_code[code_prev] + size_now > 256)
					writting_size -= 256 - (size_code[code_prev] + size_now);

				// 古いコードに追加書き込み
				memcpy(codes[code_prev] + size_code[code_prev], codes[code], writting_size);
				size_code[code_prev] += writting_size;

				if (Update)	// ハッシュ更新
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

				// ハッシュ作成
				int val = HASHMODOKI(codes[latest], size_code[latest]);

				{
					BOOL found = FALSE;
					int nowlink = linkpos[val].start;

					// 同じ物がないかチェック
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

								if (Update)	// ハッシュ更新
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
						// リンクリスト初期化
						linklist[latest] = -1;

						// リンクリストにデータ追加
						if (linkpos[val].end == -1)
							linkpos[val].start = latest;
						else
							linklist[ linkpos[val].end ] = latest;

						linkpos[val].end = latest;

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
		jecxz CMPFINISH		; ecxが0だとﾔｳﾞｧｲ事になるらしいので回避
CMPLOOP:
		cmpsb				; ハズレにぶち当たるまで繰り返し
		jne CMPFINISH
		loop CMPLOOP

CMPFINISH:
		sub length_d, ecx	; 最初のカウンタ値－カウンタの残り＝一致長
	}

	return length_d;
}

// ハッシュもどき作成関数、参考書からの丸写し。
// マクロ付けたので使ってません。
//WORD CreateHash(const LPBYTE pSource, int length)
//{
//	WORD val1 = *pSource;
//	WORD val2 = (1 < length) ? *(pSource + 1) : 0;
//	DWORD val3 = (2 < length) ? *(pSource + 2) : 0;
//	DWORD val4 = (3 < length) ? *(pSource + 3) : 0;

//	return val1 | (val2 << 8)/* | (val3 << 16) | (val4  << 24)*/;
//}

// ハッシュもどき比較関数。処理が重いので使用してません。
// 型キャスト等行えば、大小違うファイルでも比較できます。
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
