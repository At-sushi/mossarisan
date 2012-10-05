// LZW符号化
// っていうか、もっさりしすぎ。
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
			printf("ERROR:圧縮後のサイズが16Mバイトを超過しました");
			exit(-1);
		}

		if (target_nokori < 256)
		{
			for (int i = 0; i < size_dic; i++)
			{
				// サイズが限界の場合は切りつめる。
				// 辞書サイズが崩れるが、どうせ切った以降は使わないので（ﾟεﾟ）ｷﾆｼﾅｲ!!
				if (size_code[i] > target_nokori)
				{
					size_code[i] = target_nokori;
					if (size_code[i] == 1)
					{
						hash_code_list[CreateHash(codes[i], 2)].erase(i);	// 古いのは消す
						hash_code_list[CreateHash(codes[i], size_code[i])].insert(i);
					}
				}
			}
		}

		// 同じパターンがないか検索
		WORD hash_running = CreateHash(running, 2);
		if (!hash_code_list[hash_running].empty())
			for (std::set<USHORT>::iterator i = hash_code_list[hash_running].begin();
				 i != hash_code_list[hash_running].end();
				 ++i)
		{
			if ( size_code[*i] > size_now )
			{
				// 最初の2文字は一致確定しているので比較対象から外す
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
			// 見つからなかったら、0-255
			code = *running;
			size_now = 1;
		}

		assert( (code <= 0xFFFF) && (code >= 0) );
		// バッファにコードをｿｼｰﾝ
		BufBlock[0].code = (USHORT)code;
		BufBlock[0].length = size_now;
		// レンジコーダ発進
		RangeEncode(Buffer, size_buffer, 0);
		//running_block++;

		// 辞書を追加
		LZWAddDic(running, target, code, code_prev, size_now, length_prev);

		// 辞書にあったところは飛ばす
		running += size_now;

		// 前のコードを保持
		code_prev = code;
		length_prev = size_now;
	}

	RangeEncFinish(Buffer, size_buffer);
	//*size_blocks = running_block;

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
	/*FILE* file2 = fopen("block.bin", "wb");
	fwrite(BufBlock, sizeof(BufBlock) / 1, 1, file2);
	fclose(file2);*/
}

BOOL LZWDecode(BYTE* Buffer, DWORD* size_buffer, BYTE* data, DWORD size_data, DWORD size_original)
{
	// TODO:初期化を統一する
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

		// 符号化に合わせ、サイズが限界の場合は切りつめる。
		if (target_nokori < 256)
		{
			for (int i = 0; i < size_dic; i++)
			{
				if (size_code[i] > target_nokori)
				{
					size_code[i] = target_nokori;
					if (size_code[i] == 1)
					{
						hash_code_list[CreateHash(codes[i], 2)].erase(i);	// 古いのは消す
						hash_code_list[CreateHash(codes[i], size_code[i])].insert(i);
					}
				}
			}
		}

		// たった１分レンジでチン
		RangeDecode(data, size_data);
		code = BufBlock[0].code;
		size_now = BufBlock[0].length;

		// データが㌧㌦
		if ( (code > 0xFFFF) || (code < 0) || (size_now > size_code[code]) )
			return FALSE;

		// 辞書を追加
		LZWAddDic(running_buffer, Buffer, code, code_prev, size_now, length_prev);

		memcpy(running_buffer, codes[code], size_now);
		*size_buffer += size_now;
		running_buffer += size_now;

		// 前のコードを保持
		code_prev = code;
		length_prev = size_now;
	}

	return TRUE;
}

// データを比較して一致長を吐きます
DWORD LZWCompare(const LPBYTE mem1, const LPBYTE mem2, DWORD length)
{
	// TODO:完全なアセンブリ形式に置き換え
	// というか、アセンブラで書く必要あったんだろうか、これ。
	register DWORD length_d = length;

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
		repe cmpsb		; ハズレにぶち当たるまで繰り返し
		je CMPFINISH
		inc ecx			; 最後の1文字だけ違った場合

CMPFINISH:
		sub length_d, ecx	; 最初のカウンタ値－カウンタの残り＝一致長
	}

	return length_d;
}

// ハッシュもどき作成関数、参考書からの丸写し。
WORD CreateHash(const LPBYTE pSource, int length)
{
	WORD val1 = *pSource;
	WORD val2 = (1 < length) ? *(pSource + 1) : 0;
//	DWORD val3 = (2 < length) ? *(pSource + 2) : 0;
//	DWORD val4 = (3 < length) ? *(pSource + 3) : 0;

	return val1 | (val2 << 8)/* | (val3 << 16) | (val4  << 24)*/;
}

/// ハッシュもどき比較関数。
// 型キャスト等行えば、大小違うファイルでも比較できます。
// （未使用、重いため）
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

// 辞書を追加
void LZWAddDic(BYTE* running, BYTE* startpos, USHORT code, USHORT code_prev, short size_now, short length_prev)
{
	if (running != startpos)	// 最初はデータがないのでパス
	{
		short writting_size = size_now;	// 追加するデータ量

		if (length_prev == size_code[code_prev])
		{
			BOOL Update = (size_code[code_prev] < 4) ? TRUE : FALSE;

			// 辞書データ数限界、切りつめる
			if (size_code[code_prev] + size_now > 256)
				writting_size -= (size_code[code_prev] + size_now) - 256;

			// 古いコードに追加書き込み
			memcpy(codes[code_prev] + size_code[code_prev], codes[code], writting_size);
			size_code[code_prev] += writting_size;

			if (Update)	// ハッシュ更新
				hash_code_list[CreateHash(codes[code_prev], size_code[code_prev])].insert(code_prev);

			// 最大一致長上げる
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

			// 最大値に到達→処理がめんどいのでキャンセル
			if (latest >= 65536)
				return;

			size_code[latest] = 0;

			// 前のコードをまず入れる
			memcpy(codes[latest] + size_code[latest], codes[code_prev], length_prev);
			size_code[latest] += length_prev;

			// 辞書データ数限界、切りつめる
			if (size_code[latest] + size_now > 256)
				writting_size -= (size_code[latest] + size_now) - 256;

			// で、今の場所つっこむ
			memcpy(codes[latest] + size_code[latest], codes[code], writting_size);
			size_code[latest] += writting_size;

			// ハッシュ作成
			WORD hash_latest = CreateHash(codes[latest], size_code[latest]);

			{
				BOOL found = FALSE;

				// 同じ物がないかチェック
				if (!hash_code_list[hash_latest].empty())
					for (std::set<USHORT>::iterator j = hash_code_list[hash_latest].begin();
						 j != hash_code_list[hash_latest].end();
						 ++j)
						if ( size_code[latest] <= 2 ||
							 size_code[*j] <= 2 ||
							 memcmp(codes[*j] + 2, codes[latest] + 2,
							 (size_code[*j] < size_code[latest] ? size_code[*j] : size_code[latest]) - 2) == 0 )	// 最初の2文字は一致確定してるので飛ばします
				{
					if (size_code[*j] < size_code[latest])
					{
						// TODO:上の処理とかぶってるので統一したい
						BOOL Update = (size_code[*j] < 4) ? TRUE : FALSE;

						memcpy_s(codes[*j] + size_code[*j], 256 - size_code[*j], codes[latest] + size_code[*j], size_code[latest] - size_code[*j]);
						size_code[*j] = size_code[latest];

						// 最大一致長上げる
						if (size_code[*j] > maxlength)
						{
							for (int i = maxlength + 1; i <= size_code[*j]; i++)
							{
								frequency_table_length[i] = 1;
								frequency_sum_length[i + 1] = frequency_sum_length[i] + 1;
							}

							maxlength = size_code[*j];
						}

						if (Update)	// ハッシュ更新
						{
							hash_code_list[CreateHash(codes[*j], size_code[*j])].insert(*j);
						}
					}
					found = TRUE;
					break;
				}

				if (!found)
				{
					// 辞書一個追加しましたー
					size_dic++;

					// 頻度表も更新
					frequency_table_code[latest] = 1;
					frequency_sum_code[latest + 1] = frequency_sum_code[latest] + 1;

					// 最大一致長上げる
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
