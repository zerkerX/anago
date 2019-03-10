famicom ROM cartridge utility - anago
by 鰻開発協同組合
公式サイト http://unagi.sourceforge.jp/
掲示板     http://unagi.sourceforge.jp/t/bbs.html

--はじめに--
anago は unagi のソースから実験的な機能を盛り込んだものです。ハード
ウェアは kazzo のみの対応です。d mode は unagi と同じ処理速度で、
f mode は並行処理ができる場合はちょっとだけスピードアップします。

--コマンドライン引数--
anago.exe [mode] [script file] [target file] ...
== d D ROM DUMP MODE ==
ROM イメージを作成します。
anago.exe [d??] [script file] [dump file] [mapper]
d?? or D?? - 
  スクリプトの読み込み量を指定します。省略時は 1 倍。?? の1文字目が 
  CPU area, 2文字目が PPU area。
  2 2倍。unrom.ad, mmc3.ad, mmc5.ad で有効。
  4 4倍。mmc5.ad で有効。
D?? - 読みだし状況表示を unagi.exe と同じにする。
script file - 対応するマッパの .ad ファイルを指定します。
dump file - 出力する ROM イメージファイルを指定します。
以下は、通常は必要ありません。
mapper - スクリプトの設定を無視してマッパ番号を設定します。

例1.1: UNROM イメージを取得する
> anago.exe d unrom.ad unrom.nes
anago は VRAM mirroring をカートリッジから取得し、nes ファイルに反映し
ます。

例1.2: UOROM イメージを取得する
> anago.exe d2 unrom.ad unrom.nes
UOROM は UNROM のデータ量が2倍になっていますが、ソフトの数が少ないので
個別指定をしてください。

例2: メタルスレイダーグローリーのイメージを取得する
> anago.exe d22 mmc5.ad hal_4j.nes
MMC5 ソフトの多くは 2M+2M で事足りますが、管理できる容量は 8M+8M まで
なので、容量に応じて個別指定をしてください。

例3: イース III のイメージを取得する
> anago.exe d mmc3.ad vfr_q8_12.nes 118
MMC3 variant のマッパ番号を変更する場合は最後に数字を入れてください。
MMC3 variant は namcot 109, tengen RAMBO-1, MMC6 など種類が豊富です。

== f F FLASH MEMORY PROGRAMMING MODE ==
カートリッジ上の ROM を flashmemory に交換した上で、ROM イメージデータ
を代替デバイスに転送するモードです。
anago.exe [f??] [script file] [programming file] [cpu device] [ppu device]

f?? - ROM イメージ転送の仕方を指定します。省略時は f. ?? の1文字目が 
  CPU area, 2文字目が PPU area。
  f 管理メモリ全てにデータを埋める。管理容量の少ない ROM イメージと互
    換性が完全だが転送時間がかかる。
  t 前詰めで転送する。多くの Program イメージで大丈夫。Charcter イメー
    ジはこっちで99%大丈夫。
  b 後ろ詰めで転送する。
  e 転送しない。dummy と同じ。
F?? - 転送後に内容比較を行う。転送に失敗した場合は中断する。
script file - 対応するマッパの .af ファイルを指定します。
programming file - 転送する ROM イメージファイルを指定します。
cpu device, ppu device -
    カートリッジに接続した flash memory のデバイスを指定します。対応
    デバイスは flashdevice.nut を参照。dummy は例外扱いで転送しません。

例1.1: mmc3 TLROM に 1M+1M のイメージを転送する
> anago.exe f mmc3.af tlrom_1M_1M.nes AM29F040B AM29F040B
mmc3 は 4M+2M まで管理できますので、4M の中に 1M のデータを4回繰り返し、
2M の中に 1M のデータを2回繰り返します。転送したカートリッジを読み出し
直すと同じ結果が得られるのはこのモードだけです。

例1.2: mmc3 TLROM に 1M+1M のイメージを転送する
> anago.exe ftt mmc3.af tlrom_1M_1M.nes AM29F040B AM29F040B
4M の中に 1M のデータを前に詰め、2M の中に 1M のデータを前に詰めます。
多くのイメージはこれで動き、転送時間を節約できます。

例1.3: mmc3 TLROM に 1M+1M のイメージを転送する
> anago.exe fbt mmc3.af tlrom_1M_1M.nes AM29F040B AM29F040B
4M の中に 1M のデータを後ろに詰め、2M の中に 1M のデータを前に詰めます。
傾向として Konami と Namcot の Program イメージで動く場合が多いです。

余談ですが、namcot 109 のイメージを mmc3 へ転送しても仕様が違う
(mirroring が半田付け)ので互換性はありません。109 のイメージは 109 に
転送してください。

さらに余談ですが、音源がついてることで有名な 106 は実在しないので 109 
と間違えないでください。

例2: UOROM に UNROM(1M) のイメージを転送する
> anago.exe ft uorom.af unrom.nes AM29F040B
Charcter Memory が RAM の場合は指定を略せます。
また UNROM ソフトを UOROM で動かす場合は t で大丈夫な様です。

例3: mmc1 SKROM にキャラクタROMイメージだけ転送する
> anago.exe ftt mmc1_slrom.af skrom.nes dummy AM29F040B
> anago.exe fet mmc1_slrom.af skrom.nes AM29F040B AM29F040B
どちらも動きは同じです。好きな方を使ってください。

--その他--
- 前詰め、後ろ詰めの概念の説明は難しいのでよくわからないなら全詰めをしてください。
- RAM read/write mode はありませんので、unagi を使ってください。

--使用ライブラリ--
[LibUSB-Win32]
Copyright (c) 2002-2004 Stephan Meyer, <ste_meyer@web.de>
Copyright (c) 2000-2004 Johannes Erdfelt, <johannes@erdfelt.com>
Copyright (c) Thomas Sailer, <sailer@ife.ee.ethz.ch>
[SQUIRREL 2.1.2 stable] Copyright (c) 2003-2007 Alberto Demichelis
[opendevice.c] (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
